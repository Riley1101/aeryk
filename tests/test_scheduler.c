#include "unity.h"
#include <process.h>
#include <scheduler.h>
#include <stddef.h>

extern int schedule_call_count;
void reset_schedule_call_count(void);

static process_t procs[8];

static void reset_proc(process_t *p, uint64_t pid, uint8_t priority)
{
    p->pid = pid;
    p->state = PROCESS_READY;
    p->priority = priority;
    p->ticks_executed = 0;
    p->queue_next = NULL;
    p->queue_prev = NULL;
}

void setUp(void)
{
    mlfq_init();
    reset_schedule_call_count();
    current_process = NULL;
    idle_process = NULL;
    for (int i = 0; i < 8; i++)
        reset_proc(&procs[i], i + 1, 0);
}

void tearDown(void) {}

/* --- mlfq_enqueue --- */

void test_enqueue_ignores_null_and_dead(void)
{
    mlfq_enqueue(NULL);
    procs[0].state = PROCESS_DEAD;
    mlfq_enqueue(&procs[0]);
    TEST_ASSERT_NULL(mlfq_pick_next());
}

void test_enqueue_sets_ready_state(void)
{
    procs[0].state = PROCESS_BLOCKED;
    mlfq_enqueue(&procs[0]);
    TEST_ASSERT_EQUAL_INT(PROCESS_READY, procs[0].state);
}

/* --- mlfq_pick_next --- */

void test_pick_next_returns_fifo_order_within_queue(void)
{
    mlfq_enqueue(&procs[0]);
    mlfq_enqueue(&procs[1]);
    mlfq_enqueue(&procs[2]);

    TEST_ASSERT_EQUAL_PTR(&procs[0], mlfq_pick_next());
    TEST_ASSERT_EQUAL_PTR(&procs[1], mlfq_pick_next());
    TEST_ASSERT_EQUAL_PTR(&procs[2], mlfq_pick_next());
    TEST_ASSERT_NULL(mlfq_pick_next());
}

void test_pick_next_prefers_higher_priority_queue(void)
{
    reset_proc(&procs[0], 1, 2);
    reset_proc(&procs[1], 2, 0);
    mlfq_enqueue(&procs[0]); /* enqueued first but sits in the low-priority queue */
    mlfq_enqueue(&procs[1]); /* higher-priority queue must be drained first */

    TEST_ASSERT_EQUAL_PTR(&procs[1], mlfq_pick_next());
    TEST_ASSERT_EQUAL_PTR(&procs[0], mlfq_pick_next());
}

void test_pick_next_unlinks_picked_process(void)
{
    mlfq_enqueue(&procs[0]);
    process_t *picked = mlfq_pick_next();
    TEST_ASSERT_EQUAL_PTR(&procs[0], picked);
    TEST_ASSERT_NULL(picked->queue_next);
    TEST_ASSERT_NULL(picked->queue_prev);
}

/* --- mlfq_on_tick ---
 *
 * global_ticks is static module state with no reset hook, so the boost test
 * MUST run first (while it's still 0) — it primes it to exactly 99 with no
 * current process running (the only way to advance it without also tripping
 * the quantum-expiry path and polluting schedule_call_count), then ticks once
 * more to land exactly on PRIORITY_BOOST_INTERVAL. It resets global_ticks back
 * to 0 on the way out, so the tests below it stay unaffected. */

void test_on_tick_boosts_all_after_interval(void)
{
    current_process = NULL;
    for (int i = 0; i < 99; i++)
        mlfq_on_tick();
    TEST_ASSERT_EQUAL_INT(0, schedule_call_count);

    idle_process = &procs[7];
    current_process = &procs[0];
    current_process->state = PROCESS_RUNNING;
    current_process->priority = 2;
    current_process->ticks_executed = 5;

    reset_proc(&procs[1], 2, 3);
    mlfq_enqueue(&procs[1]);

    mlfq_on_tick(); /* 100th tick: crosses PRIORITY_BOOST_INTERVAL */

    TEST_ASSERT_EQUAL_UINT8(0, current_process->priority);
    TEST_ASSERT_EQUAL_UINT32(0, current_process->ticks_executed);
    TEST_ASSERT_EQUAL_UINT8(0, procs[1].priority);
    TEST_ASSERT_EQUAL_UINT32(0, procs[1].ticks_executed);
    TEST_ASSERT_EQUAL_INT(1, schedule_call_count);
}

void test_on_tick_demotes_after_quantum_expires(void)
{
    /* queue 0's quantum is 2 ticks */
    current_process = &procs[0];
    current_process->state = PROCESS_RUNNING;
    current_process->priority = 0;
    current_process->ticks_executed = 0;

    mlfq_on_tick();
    TEST_ASSERT_EQUAL_UINT32(1, current_process->ticks_executed);
    TEST_ASSERT_EQUAL_UINT8(0, current_process->priority);
    TEST_ASSERT_EQUAL_INT(0, schedule_call_count);

    mlfq_on_tick();
    TEST_ASSERT_EQUAL_UINT32(0, current_process->ticks_executed);
    TEST_ASSERT_EQUAL_UINT8(1, current_process->priority);
    TEST_ASSERT_EQUAL_INT(1, schedule_call_count);
}

void test_on_tick_caps_priority_at_lowest_queue(void)
{
    current_process = &procs[0];
    current_process->state = PROCESS_RUNNING;
    current_process->priority = 3; /* lowest queue, quantum 8 */
    current_process->ticks_executed = 7;

    mlfq_on_tick();
    TEST_ASSERT_EQUAL_UINT8(3, current_process->priority);
    TEST_ASSERT_EQUAL_UINT32(0, current_process->ticks_executed);
    TEST_ASSERT_EQUAL_INT(1, schedule_call_count);
}

void test_on_tick_ignores_non_running_current(void)
{
    current_process = &procs[0];
    current_process->state = PROCESS_BLOCKED;
    current_process->ticks_executed = 0;

    mlfq_on_tick();
    TEST_ASSERT_EQUAL_UINT32(0, current_process->ticks_executed);
    TEST_ASSERT_EQUAL_INT(0, schedule_call_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_enqueue_ignores_null_and_dead);
    RUN_TEST(test_enqueue_sets_ready_state);
    RUN_TEST(test_pick_next_returns_fifo_order_within_queue);
    RUN_TEST(test_pick_next_prefers_higher_priority_queue);
    RUN_TEST(test_pick_next_unlinks_picked_process);
    RUN_TEST(test_on_tick_boosts_all_after_interval);
    RUN_TEST(test_on_tick_demotes_after_quantum_expires);
    RUN_TEST(test_on_tick_caps_priority_at_lowest_queue);
    RUN_TEST(test_on_tick_ignores_non_running_current);
    return UNITY_END();
}
