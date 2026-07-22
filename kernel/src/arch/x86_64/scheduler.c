#include <process.h>
#include <scheduler.h>
#include <stddef.h>
#include <stdint.h>
#include <tty.h>


/**
 * @file scheduler.c
 * @brief Implements a Multi-Level Feedback Queue (MLFQ) scheduler for the x86_64 architecture.
 * @see https://en.wikipedia.org/wiki/Multilevel_feedback_queue
 */

/**
 * @brief The number of priority levels in the MLFQ scheduler.
 * Each level has its own time quantum, and processes can be promoted or demoted between levels based on their CPU usage and behavior.
 */
#define NUM_QUEUES 4

/**
 * @brief The interval (in ticks) at which all processes in the MLFQ are boosted to the highest priority level.
 * This prevents starvation of lower-priority processes and ensures that all processes get a chance to run.
 */
#define PRIORITY_BOOST_INTERVAL 100

/**
 * @brief Represents a queue in the MLFQ scheduler.
 * Each queue has a head and tail pointer to the processes in that queue, as well as
 * a time quantum that determines how long a process can run before being preempted.
 * 
 * @struct queue_t 
 * @member head Pointer to the first process in the queue.
 * @member tail Pointer to the last process in the queue.
 * @member quantum The time quantum for this queue, in ticks.
 */
typedef struct
{
    process_t *head;
    process_t *tail;
    uint32_t quantum;
} queue_t;

/**
 * @brief The array of queues representing the MLFQ scheduler.
 * Each index corresponds to a priority level, with 0 being the highest priority and NUM_QUEUES - 1 being the lowest priority.
 */
static queue_t mlfq[NUM_QUEUES];

/**
 * @brief The global tick counter used to track the number of ticks since the last priority boost.
 * This counter is incremented on each timer tick and is used to determine when to boost all
 * processes to the highest priority level in the MLFQ.
 */
static uint32_t global_ticks = 0;

/**
 * @brief Initializes the MLFQ scheduler by setting up each queue with its corresponding time quantum.
 * The time quantum for each queue increases with lower priority levels, allowing higher-priority processes to
 * run for shorter periods before being preempted, while lower-priority processes can run for longer periods.
 * This function should be called during the kernel initialization phase before any processes are scheduled.    
 */
void mlfq_init(void)
{
    uint32_t current_quantum = 2;
    for (int i = 0; i < NUM_QUEUES; i++)
    {
        mlfq[i].head = NULL;
        mlfq[i].tail = NULL;
        mlfq[i].quantum = current_quantum;
        current_quantum += 2;
    }
}

/**
 * @brief Enqueues a process into the appropriate MLFQ queue based on its priority.
 * If the process is already in the queue or is dead, it will not be enqueued. 
 * The process's state is set to PROCESS_READY, and its queue_next and queue_prev pointers are
 * updated to link it into the queue. If the queue is empty, the process becomes both the head and tail of the queue.
 * 
 * @param proc Pointer to the process to enqueue.
 */
void mlfq_enqueue(process_t *proc)
{
    if (!proc || proc->state == PROCESS_DEAD)
    {
        return;
    }
    proc->queue_next = NULL;
    proc->state = PROCESS_READY;

    queue_t *q = &mlfq[proc->priority];

    if (!q->head)
    {
        proc->queue_prev = NULL;
        q->head = proc;
        q->tail = proc;
    }
    else
    {
        proc->queue_prev = q->tail;
        q->tail->queue_next = proc;
        q->tail = proc;
    }
}

/**
 * @brief Picks the next process to run from the MLFQ scheduler.
 * The scheduler checks each queue starting from the highest priority (0) to the lowest (NUM_QUEUES - 1) and returns the first process found. If no processes are ready,
 * it returns NULL. The selected process is removed from the queue and its queue_next and queue_prev pointers are cleared.
 * @return Pointer to the next process to run, or NULL if no processes are ready.
 */
process_t *mlfq_pick_next(void)
{
    for (int i = 0; i < NUM_QUEUES; i++)
    {
        if (mlfq[i].head)
        {
            process_t *next_proc = mlfq[i].head;
            mlfq[i].head = next_proc->queue_next;

            if (mlfq[i].head)
            {
                mlfq[i].head->queue_prev = NULL;
            }
            else
            {
                mlfq[i].tail = NULL;
            }

            next_proc->queue_next = NULL;
            next_proc->queue_prev = NULL;
            return next_proc;
        }
    }
    return NULL;
}

/**
 * @brief Boosts all processes in the MLFQ to the highest priority level (0).
 * This function iterates through all queues starting from priority level 1 to NUM_QUEUES - 1, and for each process in those queues, it resets their priority to 0 and
 * ticks_executed to 0, and re-enqueues them into the highest priority queue. This prevents starvation of lower-priority processes and ensures that all processes get a chance to run.
 * It is called when the global tick counter reaches the PRIORITY_BOOST_INTERVAL, and it resets the global tick counter to 0 after boosting all processes.
 */
static void mlfq_boost_all(void)
{
    for (int i = 1; i < NUM_QUEUES; i++)
    {
        queue_t *q = &mlfq[i];
        while (q->head)
        {
            process_t *proc = q->head;
            q->head = proc->queue_next;

            proc->priority = 0;
            proc->ticks_executed = 0;
            mlfq_enqueue(proc);
        }
        q->tail = NULL;
    }
}

/**
 * @brief Called on each timer tick to update the MLFQ scheduler state.
 * Increments the global tick counter and checks if a priority boost is needed. 
 * If the current process has exceeded its time quantum, it is demoted to a lower priority queue, and the scheduler
 * is invoked to pick the next process to run. If the current process is not running or is NULL, the function returns without making any changes.
 */
void mlfq_on_tick(void)
{
    global_ticks++;
    if (global_ticks >= PRIORITY_BOOST_INTERVAL)
    {
        global_ticks = 0;
        mlfq_boost_all();
        if (current_process && current_process != idle_process)
        {
            current_process->priority = 0;
            current_process->ticks_executed = 0;
        }
        schedule();
        return;
    }
    if (!current_process || current_process->state != PROCESS_RUNNING)
    {
        return;
    }

    current_process->ticks_executed++;

    if (current_process->ticks_executed >=
        mlfq[current_process->priority].quantum)
    {
        if (current_process->priority < NUM_QUEUES - 1)
        {
            current_process->priority++;
        }
        current_process->ticks_executed = 0;
        schedule();
    }
}
