#include <elf.h>
#include <gdt.h>
#include <pmm.h>
#include <process.h>
#include <scheduler.h>
#include <slab.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <syscall.h>
#include <vmm.h>

/**
 * @brief Switches the CPU context from the previous process to the next process.
 * This function is implemented in assembly and performs the low-level context switching
 * between two processes. It saves the state of the previous process (registers, stack pointer, etc.) and restores the state of the next process, allowing it to resume execution.
 * @param prev Pointer to the previous process that is being switched out.
 * @param next Pointer to the next process that is being switched in.
 */
extern void switch_task(process_t *prev, process_t *next);

/**
 * @brief Sets the kernel stack pointer (RSP) for the current process.
 */
process_t *current_process = NULL;

/**
 * @brief Pointer to the idle process.
 * The idle process is a special process that runs when there are no other runnable processes.
 */
process_t *idle_process = NULL;

/**
 * @brief The head of the process queue.
 * This is a circular doubly-linked list of all processes in the system, 
 * including both runnable and non-runnable processes. 
 * The scheduler uses this queue to manage process execution.
 */
process_t *process_queue = NULL;

/**
 * @brief The next available process ID (PID).
 * This variable is incremented each time a new process is created to ensure that each process has
 * a unique identifier. The PID is used for process management and tracking.
 */
uint64_t next_pid = 1;

/**
 * @brief A scratch space for the kernel stack pointer (RSP) during context switching.
 * This variable is used to temporarily hold the stack pointer of the next process being switched to.
 * It is necessary because the kernel stack pointer must be updated before switching to the next process.
 */
static void kernel_thread_exit(void) {
  current_process->exit_code = 0;
  current_process->state = PROCESS_DEAD;
  schedule();
  for (;;)
    __asm__ volatile("hlt");
}

/**
 * @brief A stub function that serves as the entry point for kernel threads.
 * This function enables interrupts and calls the actual entry point of the kernel thread.
 * If the entry point returns, it calls `kernel_thread_exit()` to terminate the thread.
 */
static void kernel_thread_stub(void) {
  __asm__ volatile("sti");
  void (*entrypoint)() = current_process->entrypoint;
  if (entrypoint) {
    entrypoint();
  }
  kernel_thread_exit();
}

/**
 * @brief The idle thread function that runs when there are no other runnable processes.
 * This function enters an infinite loop where it enables interrupts and halts the CPU until the next
 * interrupt occurs. The idle thread is always in the PROCESS_RUNNING state and has the lowest priority.
 * It is created during scheduler initialization and is never terminated.
 */
static void idle_thread(void) {
  for (;;)
    __asm__ volatile("sti; hlt");
}

/**
 * @brief A stub function that serves as the entry point for user threads.
 * This function enables interrupts and transitions the CPU from kernel mode (ring 0) to user
 * mode (ring 3) by calling `enter_usermode()` with the entry point and user stack of the current process.
 * If `enter_usermode()` returns (which it should not), 
 * it calls `kernel_thread_exit()` to terminate the thread.
 */
static void user_thread_stub(void) {
  __asm__ volatile("sti");
  enter_usermode(current_process->entry, current_process->user_stack_top);
  // enter_usermode never returns (iretq into ring 3).
  kernel_thread_exit();
}

/**
 * @brief Sets up the kernel stack for a new process.
 * This function pushes the return stub and zeroed callee-saved registers (rbx, rbp,
 * r12-r15) that switch_task expects onto a fresh kernel stack, and returns
 * the resulting rsp. `stub` is where execution lands the first time this
 * process is switched to.
 * 
 * @param kernel_stack_base The base address of the kernel stack (the lowest address).
 * @param stub The function to execute when the process is first switched to.
 * @return The initial stack pointer (RSP) for the new process.
 */
static uint64_t setup_kernel_stack(void *kernel_stack_base,
                                    void (*stub)(void)) {
  uint64_t *stack = (uint64_t *)((uint64_t)kernel_stack_base + PAGE_SIZE);

  *(--stack) = (uint64_t)stub;
  *(--stack) = 0;
  *(--stack) = 0;
  *(--stack) = 0;
  *(--stack) = 0;
  *(--stack) = 0;
  *(--stack) = 0;

  return (uint64_t)stack;
}

/**
 * @brief Enqueues a process into the global process queue.
 * This function adds the given process to the circular doubly-linked list of all processes in the
 * system. If the queue is empty, the process becomes the head of the queue. Otherwise, it is added to the end of the queue.
 * @param proc Pointer to the process to enqueue.
 */
static void enqueue_process(process_t *proc) {
  if (!process_queue) {
    process_queue = proc;
    proc->next = proc;
    proc->prev = proc;
  } else {
    proc->prev = process_queue->prev;
    proc->next = process_queue;
    process_queue->prev->next = proc;
    process_queue->prev = proc;
  }
}

/**
 * @brief Initializes the process scheduler.
 * This function sets up the multi-level feedback queue (MLFQ) scheduler, creates the idle process,
 * and initializes the current process to the idle process. It also allocates a kernel stack for
 * the idle process and sets up its initial stack pointer (RSP) for context switching.
 * The idle process is a special process that runs when there are no other runnable processes.
 * It is always in the PROCESS_RUNNING state and has the lowest priority. The idle process is
 * created during scheduler initialization and is never terminated.
 */
void init_scheduler() {
  mlfq_init();

  idle_process = (process_t *)kmalloc(sizeof(process_t));
  memset(idle_process, 0, sizeof(process_t));

  idle_process->pid = 0;
  idle_process->state = PROCESS_RUNNING;
  idle_process->priority = 3;
  idle_process->ticks_executed = 0;
  idle_process->entrypoint = idle_thread;

  // Make sure idle process know its pagetable
  idle_process->cr3 = (uint64_t)vmm_get_kernel_pml4() - hhdm_offset;

  void *idle_stack_phys = pmm_alloc_page();
  idle_process->kernel_stack = (void *)((uint64_t)idle_stack_phys + hhdm_offset);

  // when kernel_thread_stub returns (it shouldn't, idle_thread loops
  // forever), land here instead of address 0
  idle_process->rsp =
      setup_kernel_stack(idle_process->kernel_stack, kernel_thread_stub);

  current_process = idle_process;

  process_queue = current_process;
  current_process->next = current_process;
  current_process->prev = current_process;
}

/**
 * @brief Creates a new kernel thread.
 * This function allocates and initializes a new process structure for a kernel thread.
 * It allocates a kernel stack for the thread, sets up its initial stack pointer (RSP) for context switching,
 * and enqueues it into the process queue. The new thread will start executing at the specified entry point.
 * @param entrypoint The function to execute in the new kernel thread.
 * @return A pointer to the newly created process, or NULL on failure.
 */
process_t *create_kernel_thread(void (*entrypoint)()) {
  process_t *proc = (process_t *)kmalloc(sizeof(process_t));
  if (!proc)
    return NULL;

  void *stack_phys = pmm_alloc_page();
  if (!stack_phys) {
    kfree(proc);
    return NULL;
  }
  proc->kernel_stack = (void *)((uint64_t)stack_phys + hhdm_offset);

  proc->pid = next_pid++;
  proc->state = PROCESS_READY;
  proc->entrypoint = entrypoint;
  proc->parent = current_process;

  // when entrypoint returns, land here instead of address 0
  proc->rsp = setup_kernel_stack(proc->kernel_stack, kernel_thread_stub);
  proc->cr3 = (uint64_t)vmm_get_kernel_pml4() - hhdm_offset;

  enqueue_process(proc);

  proc->priority = 0;
  proc->ticks_executed = 0;
  mlfq_enqueue(proc);
  return proc;
}

#define USER_STACK_TOP 0x0000700000000000ULL

/**
 * @brief Creates a new user process from an ELF executable.
 * Loads the ELF executable at `path` from the VFS into a fresh
 * per-process pagetable and queues it to run in ring 3.
 * Returns NULL if the file is missing, not a valid ELF64 executable,
 * or allocation fails.
 * @param path The path to the ELF executable in the virtual file system (VFS).
 * @return A pointer to the newly created process, or NULL on failure.
 */
process_t *create_user_process(const char *path) {
  vfs_node_t *file = vfs_find_node(vfs_root, path);
  if (!file || file->type != VFS_FILE) {
    return NULL;
  }

  uint64_t *pml4 = vmm_new_user_pagetable();
  if (!pml4) {
    return NULL;
  }

  uint64_t entry;
  if (elf_load(file, pml4, &entry) != 0) {
    vmm_destroy_user_pagetable(pml4);
    return NULL;
  }

  void *user_stack_phys = pmm_alloc_page();
  if (!user_stack_phys) {
    vmm_destroy_user_pagetable(pml4);
    return NULL;
  }
  vmm_map_page(pml4, USER_STACK_TOP - PAGE_SIZE, (uint64_t)user_stack_phys,
               PTE_PRESENT | PTE_WRITABLE | PTE_USER | PTE_NX);

  process_t *proc = (process_t *)kmalloc(sizeof(process_t));
  if (!proc) {
    vmm_destroy_user_pagetable(pml4);
    return NULL;
  }
  memset(proc, 0, sizeof(process_t));

  void *kernel_stack_phys = pmm_alloc_page();
  if (!kernel_stack_phys) {
    kfree(proc);
    vmm_destroy_user_pagetable(pml4);
    return NULL;
  }
  proc->kernel_stack = (void *)((uint64_t)kernel_stack_phys + hhdm_offset);

  proc->pid = next_pid++;
  proc->state = PROCESS_READY;
  proc->entry = entry;
  proc->user_stack_top = USER_STACK_TOP;
  proc->cr3 = (uint64_t)pml4 - hhdm_offset;
  proc->parent = current_process;

  // when user_thread_stub returns (it shouldn't), land here instead of 0
  proc->rsp = setup_kernel_stack(proc->kernel_stack, user_thread_stub);

  enqueue_process(proc);

  proc->priority = 0;
  proc->ticks_executed = 0;
  mlfq_enqueue(proc);
  return proc;
}

/**
 * @brief Reclaims resources of dead processes.
 * This function walks the process queue and frees resources of processes
 * that are in the PROCESS_DEAD state and have no parent (orphaned).
 * It does not free resources of processes that are still running or have a parent.
 *
 * Frees process_t structs left in PROCESS_DEAD state (kernel_thread_exit
 * marks a process dead and calls schedule(), but can't free its own kernel
 * stack while still running on it). Walks the process_queue ring built by
 * enqueue_process and reclaims anything dead other than current_process.
 * A dead process is never re-enqueued into the MLFQ (see mlfq_enqueue), so
 * process_queue is the only place a zombie can still be found.
 *
 * A dead process with a live parent is left as a zombie (state ==
 * PROCESS_DEAD, exit_code populated) so a future wait()/waitpid() can read
 * its exit_code; wait() is responsible for freeing it. Only parentless
 * (orphaned) processes are auto-reaped here.
 *
 */
static void reap_zombies(void) {
  if (!process_queue) {
    return;
  }

  uint64_t kernel_cr3 = (uint64_t)vmm_get_kernel_pml4() - hhdm_offset;

  process_t *node = process_queue;
  size_t count = 0;
  process_t *p = node;
  do {
    count++;
    p = p->next;
  } while (p != node);

  for (size_t i = 0; i < count; i++) {
    process_t *victim = node;
    node = node->next;

    if (victim->state != PROCESS_DEAD || victim == current_process ||
        victim->parent != NULL) {
      continue;
    }

    victim->prev->next = victim->next;
    victim->next->prev = victim->prev;
    if (process_queue == victim) {
      process_queue = (victim->next != victim) ? victim->next : NULL;
    }

    pmm_free_page((void *)((uint64_t)victim->kernel_stack - hhdm_offset));
    // Kernel threads share the kernel pml4 and must keep it; only a user
    // process owns a private pagetable that needs tearing down.
    if (victim->cr3 != kernel_cr3) {
      vmm_destroy_user_pagetable((uint64_t *)(victim->cr3 + hhdm_offset));
    }
    kfree(victim);

    if (!process_queue) {
      return;
    }
  }
}

/**
 * @brief Schedules the next process to run.
 * This function performs context switching between processes. It saves the state of the current process,
 * selects the next process to run from the ready queue, and restores its state. If there are no runnable processes, it switches to the idle process. The scheduler also reclaims resources of dead
 * processes and manages the multi-level feedback queue (MLFQ) for process prioritization.
 * If the current process is the same as the next process, no context switch occurs.
 */
void schedule() {
  if (!current_process)
    return;

  reap_zombies();

  process_t *prev = current_process;

  if (prev->state == PROCESS_RUNNING && prev != idle_process) {
    mlfq_enqueue(prev);
  }

  process_t *next = mlfq_pick_next();
  if (!next) {
    next = idle_process;
  }

  if (prev == next)
    return;

  current_process = next;
  current_process->state = PROCESS_RUNNING;

  if (next->kernel_stack) {
      uint64_t next_stack_top = (uint64_t) next->kernel_stack + PAGE_SIZE;
      kernel_rsp_scratch = next_stack_top;
      set_kernel_stack(next_stack_top);
  }

  switch_task(prev, next);
}

/**
 * @brief Enters user mode and starts executing a user process.
 * This function sets up the CPU state to transition from kernel mode (ring 0) to
 * user mode (ring 3) and begins executing the user process at the specified entry point with the provided user stack.
 * It uses the `iretq` instruction to perform the transition, which restores the CPU state from the stack and switches to user mode.
 * @param entry_point The entry point of the user process (the address of the first instruction to execute).
 * @param user_stack The top of the user stack (the initial stack pointer for the user process).
 */
void enter_usermode(uint64_t entry_point, uint64_t user_stack) {
  asm volatile("cli \n"
               "push $0x1B \n"  // User data selector
               "push %0 \n"     // User Stack pointer
               "push $0x202 \n" // User Rflags
               "push $0x23 \n"  // User code selector (0x20 | RPL 3)
               "push %1 \n"     // User RIP entry point
               "iretq"
               :
               : "r"(user_stack), "r"(entry_point)
               : "memory"

  );
}
