#include "tty.h"
#include <process.h>
#include <scheduler.h>
#include <stddef.h>
#include <stdint.h>

#define NUM_QUEUES 4
#define PRIORITY_BOOST_INTERVAL 100

typedef struct {
  process_t *head;
  process_t *tail;
  uint32_t quantum;
} queue_t;

static queue_t mlfq[NUM_QUEUES];
static uint32_t global_ticks = 0;

void mlfq_init() {
  uint32_t current_quantum = 2;
  for (int i = 0; i < NUM_QUEUES; i++) {
    mlfq[i].head = NULL;
    mlfq[i].tail = NULL;
    mlfq[i].quantum = current_quantum;
    current_quantum += 2;
  }
}

void mlfq_enqueue(process_t *proc) {
  if (!proc || proc->state == PROCESS_DEAD) {
    return;
  }
  proc->queue_next = NULL;
  proc->state = PROCESS_READY;

  queue_t *q = &mlfq[proc->priority];

  if (!q->head) {
    proc->queue_prev = NULL;
    q->head = proc;
    q->tail = proc;
  } else {
    proc->queue_prev = q->tail;
    q->tail->queue_next = proc;
    q->tail = proc;
  }
}

process_t *mlfq_pick_next(void) {
  for (int i = 0; i < NUM_QUEUES; i++) {
    if (mlfq[i].head) {
      process_t *next_proc = mlfq[i].head;
      mlfq[i].head = next_proc->queue_next;

      if (mlfq[i].head) {
        mlfq[i].head->queue_prev = NULL;
      } else {
        mlfq[i].tail = NULL;
      }

      next_proc->queue_next = NULL;
      next_proc->queue_prev = NULL;
      return next_proc;
    }
  }
  return NULL;
}

static void mlfq_boost_all(void) {
  for (int i = 1; i < NUM_QUEUES; i++) {
    queue_t *q = &mlfq[i];
    while (q->head) {
      process_t *proc = q->head;
      q->head = proc->queue_next;

      proc->priority = 0;
      proc->ticks_executed = 0;
      mlfq_enqueue(proc);
    }
    q->tail = NULL;
  }
}

void mlfq_on_tick(void) {
  global_ticks++;
  if (global_ticks >= PRIORITY_BOOST_INTERVAL) {
    global_ticks = 0;
    mlfq_boost_all();
    if (current_process && current_process != idle_process) {
      current_process->priority = 0;
      current_process->ticks_executed = 0;
    }
    schedule();
    return;
  }
  if (!current_process || current_process->state != PROCESS_RUNNING) {
    return;
  }

  current_process->ticks_executed++;

  if (current_process->ticks_executed >=
      mlfq[current_process->priority].quantum) {
    if (current_process->priority < NUM_QUEUES - 1) {
      current_process->priority++;
    }
    current_process->ticks_executed = 0;
    schedule();
  }
}
