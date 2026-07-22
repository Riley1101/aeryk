#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <process.h>

/**
 * @file scheduler.c
 * @brief Implements a Multi-Level Feedback Queue (MLFQ) scheduler for the x86
 * 
 * @see read OSStep Chapter 8: Scheduling - The Multi-Level Feedback Queue
 * https://daily.dev/posts/ostep-chapter-8-scheduling-the-multi-level-feedback-queue-myxfqego
 */

/**
 * @brief The number of priority levels in the MLFQ scheduler.
 * Each level has its own time quantum, and processes can be promoted or demoted between levels
 * based on their CPU usage and behavior. 
 */
void mlfq_init();

/**
 * @brief Enqueues a process into the appropriate MLFQ queue based on its priority.
 * If the process is dead or NULL, it will not be enqueued.
 * @param proc Pointer to the process to enqueue.
 */
void mlfq_enqueue(process_t * proc);

/**
 * @brief Picks the next process to run from the MLFQ scheduler.
 * The scheduler checks each queue starting from the highest priority (0) to the lowest (NUM
 * _QUEUES - 1) and returns the first process found. If no processes are ready, it returns NULL.
 * @return Pointer to the next process to run, or NULL if no processes are ready.
 */
process_t* mlfq_pick_next(void);


/**
 * @brief Called on each timer tick to update the MLFQ scheduler state.
 * Increments the global tick counter and checks if a priority boost is needed. If the current
 * process has exceeded its time quantum, it is demoted to a lower priority queue, and the
 * scheduler is invoked to pick the next process to run.
 */
void mlfq_on_tick(void);

#endif // !SCHEDULER_H
