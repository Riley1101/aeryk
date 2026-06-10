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
  file_descriptor_t fd_table[MAX_FDS];
} process_t;

void init_scheduler(void);

process_t *create_kernel_thread(void (*entry_point)());

extern process_t *current_process;
extern process_t *idle_process;

void schedule(void);

void enter_usermode(uint64_t entry_point, uint64_t user_stack);

#endif // !PROCESS_H
