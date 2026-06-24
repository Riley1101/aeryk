#ifndef PROCESS_H
#define PROCESS_H

#include <arch/x86_64/fs/vfs.h>
#include <stdint.h>

#define MAX_FDS 32

typedef struct file_descriptor {
  vfs_node_t *node;
  uint32_t offset;
  int flags;
} file_descriptor_t;
;

typedef enum {
  PROCESS_READY,
  PROCESS_RUNNING,
  PROCESS_BLOCKED,
  PROCESS_DEAD
} process_state_t;

typedef struct process {
  uint64_t pid;
  uint64_t rsp; // saved stack pointer to use in a context switch
  uint64_t cr3; // Page table physical address
  process_state_t state;
  void *kernel_stack;

  // --  MLFQ fields --
  uint8_t priority;
  uint32_t ticks_executed;
  struct process *queue_next;
  struct process *queue_prev;
  struct process *next;
  struct process *prev;

  void (*entrypoint)();

  // -- User process fields --
  uint64_t entry;          // ELF entry point (ring 3)
  uint64_t user_stack_top; // top of the user stack (ring 3)

  file_descriptor_t fd_table[MAX_FDS];
} process_t;

void init_scheduler(void);

process_t *create_kernel_thread(void (*entry_point)());

// Loads the ELF executable at `path` from the VFS into a fresh
// per-process pagetable and queues it to run in ring 3.
// Returns NULL if the file is missing, not a valid ELF64 executable,
// or allocation fails.
process_t *create_user_process(const char *path);

extern process_t *current_process;
extern process_t *idle_process;

void schedule(void);

void enter_usermode(uint64_t entry_point, uint64_t user_stack);

#endif // !PROCESS_H
