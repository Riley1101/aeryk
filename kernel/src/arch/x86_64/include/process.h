#ifndef PROCESS_H
#define PROCESS_H

#include <arch/x86_64/fs/vfs.h>
#include <stdint.h>

#define MAX_FDS 32

/**
 * @brief Structure representing a file descriptor in a process.
 * This structure contains information about an open file, including a pointer to the corresponding VFS node,
 * the current offset within the file, and any flags associated with the file descriptor.
 */
typedef struct file_descriptor {
  vfs_node_t *node;
  uint32_t offset;
  int flags;
} file_descriptor_t;
;

/**
 * @brief Snapshot of a user process's registers at the point it called
 * fork(), used to resume a freshly forked child exactly where its parent
 * left off. Field order/offsets are load-bearing: resume_usermode.asm
 * indexes into this struct directly, so keep the two in sync.
 */
typedef struct {
  uint64_t rdi, rsi, rdx, r10, r8, r9;
  uint64_t rbx, rbp, r12, r13, r14, r15;
  uint64_t rax;
  uint64_t rip, rflags, rsp;
} trapframe_t;

/**
 * @brief Enumeration representing the possible states of a process.
 */
typedef enum {
  PROCESS_READY,
  PROCESS_RUNNING,
  PROCESS_BLOCKED,
  PROCESS_DEAD
} process_state_t;

/**
 * @brief Structure representing a process in the operating system.
 * This structure contains information about a process, including its PID, state, stack pointers,
 * page table address, priority, and other relevant fields. It also includes fields for managing
 * the process's file descriptors, parent-child relationships, and exit status.
 * 
 * Note: The `entrypoint` field is used for kernel threads, while the `entry` and `user_stack_top`
 * fields are used for user processes.
 */
typedef struct process {
  /**
   * @brief The unique process ID (PID) assigned to this process.
   */
  uint64_t pid;

  /**
   * @brief The saved stack pointer (RSP) for this process, used during context switching.
   */
  uint64_t rsp; // saved stack pointer to use in a context switch

  /**
   * @brief The physical address of the page table (CR3) for this process.
   */
  uint64_t cr3; // Page table physical address

  /**
   * @brief The current state of the process (e.g., ready, running, blocked, dead).
   */
  process_state_t state;

  /**
   * @brief Pointer to the kernel stack for this process.
   * The kernel stack is used when the process is executing in kernel mode (ring 0).
   */
  void *kernel_stack;

  // --  MLFQ fields --
  /**
   * @brief The priority level of the process in the multi-level feedback queue (MLFQ).
   */
  uint8_t priority;

  /**
   * @brief The number of ticks the process has executed in its current time slice.
   */
  uint32_t ticks_executed;

  /**
   * @brief The entry point function for kernel threads.
   * This field is used for kernel threads to specify the function to execute when the thread starts
   * running. It is not used for user processes.
   */
  struct process *queue_next;

  /**
   * @brief The entry point address for user processes (ELF entry point).
   * This field is used for user processes to specify the address of the first instruction to execute
   * when the process starts running in user mode (ring 3).
   */
  struct process *queue_prev;

  /**
   * @brief Pointer to the next process in the process queue.
   * This field is used to link processes together in a circular doubly-linked list for scheduling.
   */
  struct process *next;

  /**
   * @brief Pointer to the previous process in the process queue.
   * This field is used to link processes together in a circular doubly-linked list for scheduling.
   */
  struct process *prev;

  /**
   * @brief The entry point function for kernel threads.
   * This field is used for kernel threads to specify the function to execute when the thread starts
   * running. It is not used for user processes.
   */
  void (*entrypoint)();

  // -- User process fields --
  /**
   * @brief The entry point address for user processes (ELF entry point).
   * This field is used for user processes to specify the address of the first instruction to execute
   * when the process starts running in user mode (ring 3).
   */
  uint64_t entry;          // ELF entry point (ring 3)

  /**
   * @brief The top of the user stack for this process (ring 3).
   * This field specifies the initial stack pointer for the user process when it starts executing in user mode.
   */
  uint64_t user_stack_top; // top of the user stack (ring 3)

  file_descriptor_t fd_table[MAX_FDS];

  /**
   * @brief Saved user-mode register snapshot for a forked child's first
   * run. Only populated (by fork_process()) on a process created via
   * fork(); fork_trampoline() consumes it once, on first schedule.
   */
  trapframe_t fork_frame;

  // -- Parent/child + exit status --
  struct process *parent;

  /**
   * @brief The exit code of the process.
   * This field is used to store the exit code of the process when it terminates.
   */
  int exit_code;
} process_t;

/**
 * @brief Initializes the process scheduler.
 */
void init_scheduler(void);

/**
 * @brief Creates a new kernel thread.
 * This function allocates and initializes a new process structure for a kernel thread.
 * It allocates a kernel stack for the thread, sets up its initial stack pointer (R
 * SP) for context switching, and enqueues it into the process queue. The new thread will start executing at the specified entry point.
 * @param entry_point The function to execute in the new thread.
 * @return A pointer to the newly created process, or NULL on failure.
 */
process_t *create_kernel_thread(void (*entry_point)());

/**
 * @brief Creates a new user process from an ELF executable.
 * Loads the ELF executable at `path` from the VFS into a fresh
 * per-process pagetable and queues it to run in ring 3.
 * Returns NULL if the file is missing, not a valid ELF64 executable,
 * or allocation fails.
 * @param path The path to the ELF executable in the virtual file system (VFS).
 * @param args_str Optional space-separated argument string (excluding the
 * program name), or NULL/empty for no arguments. argv[0] is derived from
 * the last path component of `path`.
 * @return A pointer to the newly created process, or NULL on failure.
 */
process_t *create_user_process(const char *path, const char *args_str);

/**
 * @brief Forks the currently running user process.
 * Copy-on-write clones `parent`'s address space (see
 * vmm_clone_user_pagetable()) and copies its file descriptor table into a
 * new process, and arranges for it to resume in user mode at the exact
 * point captured in `regs` (the parent's register state when it invoked
 * the fork syscall), except with a return value of 0.
 * @param parent The process being forked (must be a user process).
 * @param regs The parent's user-mode register snapshot at the syscall.
 * @return A pointer to the newly created child process, or NULL on failure.
 */
process_t *fork_process(process_t *parent, const trapframe_t *regs);

/**
 * @brief Replaces `proc`'s address space with a freshly loaded ELF image,
 * implementing execve(). See process.c for the full behavior (this does
 * not return on success).
 * @param proc The process to execve into (must be current_process).
 * @param path The path to the new ELF executable in the VFS.
 * @param args_str Optional space-separated argument string (excluding the
 * program name), or NULL/empty for no arguments.
 * @return -1 on failure, leaving `proc` running its old image unchanged.
 * Never returns on success.
 */
int exec_process(process_t *proc, const char *path, const char *args_str);

/**
 * @brief Reclaims resources of dead processes.
 * This function walks the process queue and frees resources of processes
 * that are in the PROCESS_DEAD state and have no parent (orphaned).
 * It does not free resources of processes that are still running or have a parent.
 */

/**
 * @brief Looks for a dead child of `parent` and reaps it if found.
 * If `pid` is <= 0, any dead child matches; otherwise only that pid.
 * On a match, copies the child's exit code to `*status_out` (if non-NULL),
 * unlinks and frees the child, and returns its pid.
 * Returns 0 if `parent` has live children but none are dead yet (caller
 * should block and retry), or -1 if `parent` has no children at all (ECHILD).
 * @param parent The waiting process.
 * @param pid The pid to wait for, or <= 0 for any child.
 * @param status_out Out-param for the reaped child's exit code.
 * @return The reaped child's pid, 0, or -1.
 */
int wait_reap_child(process_t *parent, int64_t pid, int *status_out);

extern process_t *current_process;

/**
 * @brief Circular doubly-linked ring of every live and zombie process.
 * Built by enqueue_process()/create_user_process(); reap_zombies() and
 * wait_reap_child() are the only readers outside process.c.
 */
extern process_t *process_queue;

/**
 * @brief Pointer to the idle process.
 * The idle process is a special process that runs when there are no other runnable processes.
 * It is always in the PROCESS_RUNNING state and has a low priority.
 * The idle process is created during scheduler initialization and is never terminated.
 */
extern process_t *idle_process;

/**
 * @brief Schedules the next process to run.
 * This function performs context switching between processes. It saves the state of the current process,
 * selects the next process to run from the ready queue, and restores its state. If there are no runnable processes, it switches to the idle process. The scheduler also reclaims resources of dead
 * processes and manages the multi-level feedback queue (MLFQ) for process prioritization.
 * If the current process is the same as the next process, no context switch occurs.
 */
void schedule(void);

/**
 * @brief Enters user mode and starts executing a user process.
 * This function sets up the CPU state to transition from kernel mode (ring 0) to
 * user mode (ring 3) and begins executing the user process at the specified entry point with the provided user stack.
 * It uses the `iretq` instruction to perform the transition, which restores the CPU state from the stack and switches to user mode.
 * @param entry_point The entry point of the user process (the address of the first instruction to execute).
 * @param user_stack The top of the user stack (the initial stack pointer for the user process).
 */
void enter_usermode(uint64_t entry_point, uint64_t user_stack);

#endif // !PROCESS_H
