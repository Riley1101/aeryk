#include "gdt.h"
#include "pmm.h"
#include "scheduler.h"
#include "slab.h"
#include "syscall.h"
#include "vmm.h"
#include <elf.h>
#include <process.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern void switch_task(process_t *prev, process_t *next);

process_t *current_process = NULL;
process_t *idle_process = NULL;

process_t *process_queue = NULL;
uint64_t next_pid = 1;

static void kernel_thread_exit(void) {
  current_process->state = PROCESS_DEAD;
  schedule();
  for (;;)
    __asm__ volatile("hlt");
}

static void kernel_thread_stub(void) {
  __asm__ volatile("sti");
  void (*entrypoint)() = current_process->entrypoint;
  if (entrypoint) {
    entrypoint();
  }
  kernel_thread_exit();
}

static void user_thread_stub(void) {
  __asm__ volatile("sti");
  enter_usermode(current_process->entry, current_process->user_stack_top);
  // enter_usermode never returns (iretq into ring 3).
  kernel_thread_exit();
}

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

void init_scheduler() {
  mlfq_init();

  idle_process = (process_t *)kmalloc(sizeof(process_t));

  idle_process->pid = 0;
  idle_process->state = PROCESS_RUNNING;
  idle_process->priority = 3;
  idle_process->ticks_executed = 0;

  // Make sure idle process know its pagetable
  idle_process->cr3 = (uint64_t)vmm_get_kernel_pml4() - hhdm_offset;
  idle_process->kernel_stack = NULL;

  current_process = idle_process;

  process_queue = current_process;
  current_process->next = current_process;
  current_process->prev = current_process;
}

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

  uint64_t *stack = (uint64_t *)((uint64_t)proc->kernel_stack + PAGE_SIZE);

  // when entrypoint returns, land here instead of address 0
  *(--stack) = (uint64_t)kernel_thread_stub;
  // callee-saved registers (rbx, rbp, r12-r15) zeroed for switch_task restore
  *(--stack) = 0;
  *(--stack) = 0;
  *(--stack) = 0;
  *(--stack) = 0;
  *(--stack) = 0;
  *(--stack) = 0;

  proc->rsp = (uint64_t)stack;
  proc->cr3 = (uint64_t)vmm_get_kernel_pml4() - hhdm_offset;

  enqueue_process(proc);

  proc->priority = 0;
  proc->ticks_executed = 0;
  mlfq_enqueue(proc);
  return proc;
}

#define USER_STACK_TOP 0x0000700000000000ULL

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
    return NULL;
  }

  void *user_stack_phys = pmm_alloc_page();
  if (!user_stack_phys) {
    return NULL;
  }
  vmm_map_page(pml4, USER_STACK_TOP - PAGE_SIZE, (uint64_t)user_stack_phys,
               PTE_PRESENT | PTE_WRITABLE | PTE_USER | PTE_NX);

  process_t *proc = (process_t *)kmalloc(sizeof(process_t));
  if (!proc) {
    return NULL;
  }
  memset(proc, 0, sizeof(process_t));

  void *kernel_stack_phys = pmm_alloc_page();
  if (!kernel_stack_phys) {
    kfree(proc);
    return NULL;
  }
  proc->kernel_stack = (void *)((uint64_t)kernel_stack_phys + hhdm_offset);

  proc->pid = next_pid++;
  proc->state = PROCESS_READY;
  proc->entry = entry;
  proc->user_stack_top = USER_STACK_TOP;
  proc->cr3 = (uint64_t)pml4 - hhdm_offset;

  uint64_t *stack = (uint64_t *)((uint64_t)proc->kernel_stack + PAGE_SIZE);

  // when user_thread_stub returns (it shouldn't), land here instead of 0
  *(--stack) = (uint64_t)user_thread_stub;
  // callee-saved registers (rbx, rbp, r12-r15) zeroed for switch_task restore
  *(--stack) = 0;
  *(--stack) = 0;
  *(--stack) = 0;
  *(--stack) = 0;
  *(--stack) = 0;
  *(--stack) = 0;

  proc->rsp = (uint64_t)stack;

  enqueue_process(proc);

  proc->priority = 0;
  proc->ticks_executed = 0;
  mlfq_enqueue(proc);
  return proc;
}

void schedule() {
  if (!current_process)
    return;

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
