#include "pmm.h"
#include "scheduler.h"
#include "slab.h"
#include "vmm.h"
#include <process.h>
#include <stddef.h>
#include <stdint.h>

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

void initScheduler() {
  mlfqInit();

  idle_process = (process_t *)kmalloc(sizeof(process_t));

  idle_process->pid = 0;
  idle_process->state = PROCESS_RUNNING;
  idle_process->priority = 3;
  idle_process->ticks_executed = 0;

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

  proc->priority = 0;
  proc->ticks_executed = 0;
  mlfqEnqueue(proc);
  return proc;
}

void schedule() {
  if (!current_process)
    return;

  process_t *prev = current_process;

  if (prev->state == PROCESS_RUNNING && prev != idle_process) {
    mlfqEnqueue(prev);
  }

  process_t *next = mlfqPickNext();
  if (!next) {
    next = idle_process;
  }

  if (prev == next)
    return;

  current_process = next;
  current_process->state = PROCESS_RUNNING;

  switch_task(prev, next);
}
