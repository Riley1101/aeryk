#include <pipe.h>
#include <process.h>
#include <scheduler.h>
#include <slab.h>
#include <string.h>

// Wakes every process parked on `wq` (as opposed to wait_queue_pop(), which
// wakes at most one). A close() can make progress possible for every
// blocked reader/writer at once (EOF, or broken pipe), not just the head.
static void wake_all(wait_queue_t *wq)
{
    process_t *p;
    while ((p = wait_queue_pop(wq)) != NULL)
    {
        mlfq_enqueue(p); // sets state back to PROCESS_READY
    }
}

pipe_t *pipe_create(void)
{
    pipe_t *p = (pipe_t *)kmalloc(sizeof(pipe_t));
    if (!p)
    {
        return NULL;
    }
    memset(p, 0, sizeof(pipe_t));
    p->readers = 1;
    p->writers = 1;
    return p;
}

int pipe_read(pipe_t *p, char *buf, int count)
{
    int n = 0;

    while (n < count)
    {
        asm volatile("cli");

        if (p->count > 0)
        {
            buf[n++] = (char)p->buf[p->tail];
            p->tail = (p->tail + 1) % PIPE_BUF_SIZE;
            p->count--;
            process_t *waiter = wait_queue_pop(&p->write_waiters);
            asm volatile("sti");
            if (waiter)
            {
                mlfq_enqueue(waiter);
            }
            continue;
        }

        if (p->writers == 0)
        {
            // Buffer empty and no writer left to fill it: EOF.
            asm volatile("sti");
            break;
        }

        // Nothing buffered yet: register as a waiter and yield. cli/sti around
        // the empty-check + enqueue close the race against pipe_write() firing
        // in between, same as keyboard_read().
        current_process->state = PROCESS_BLOCKED_PIPE;
        wait_queue_push(&p->read_waiters, current_process);
        asm volatile("sti");
        schedule();
    }

    return n;
}

int pipe_write(pipe_t *p, const char *buf, int count)
{
    int n = 0;

    while (n < count)
    {
        asm volatile("cli");

        if (p->readers == 0)
        {
            // No reader left to ever drain this: broken pipe. Report what we
            // already managed to write, if anything, rather than discarding it.
            asm volatile("sti");
            return n > 0 ? n : -1;
        }

        if (p->count < PIPE_BUF_SIZE)
        {
            p->buf[p->head] = (uint8_t)buf[n++];
            p->head = (p->head + 1) % PIPE_BUF_SIZE;
            p->count++;
            process_t *waiter = wait_queue_pop(&p->read_waiters);
            asm volatile("sti");
            if (waiter)
            {
                mlfq_enqueue(waiter);
            }
            continue;
        }

        current_process->state = PROCESS_BLOCKED_PIPE;
        wait_queue_push(&p->write_waiters, current_process);
        asm volatile("sti");
        schedule();
    }

    return n;
}

void pipe_close_end(pipe_t *p, int is_read_end)
{
    asm volatile("cli");
    if (is_read_end)
    {
        if (p->readers > 0)
        {
            p->readers--;
        }
    }
    else
    {
        if (p->writers > 0)
        {
            p->writers--;
        }
    }
    int should_free = (p->readers == 0 && p->writers == 0);
    asm volatile("sti");

    // Wake both sides unconditionally: a writer blocked on a full buffer
    // needs to recheck readers==0, and a reader blocked on an empty buffer
    // needs to recheck writers==0. Waking a side that still can't make
    // progress just costs it a harmless re-block.
    wake_all(&p->write_waiters);
    wake_all(&p->read_waiters);

    if (should_free)
    {
        kfree(p);
    }
}
