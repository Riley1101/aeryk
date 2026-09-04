#ifndef PIPE_H
#define PIPE_H

#include <process.h>
#include <stdint.h>

#define PIPE_BUF_SIZE 512

/**
 * @brief An unnamed pipe: a fixed-size ring buffer shared between a read
 * end and a write end, with its own wait queues so pipe_read()/pipe_write()
 * can block the same way keyboard_read() does (see process.h's
 * wait_queue_t). `readers`/`writers` count how many open file descriptors
 * (across possibly multiple processes, after fork()) reference each end;
 * the pipe is freed once both drop to zero.
 */
typedef struct pipe
{
    uint8_t buf[PIPE_BUF_SIZE];
    uint32_t head;  // next slot to write
    uint32_t tail;  // next slot to read
    uint32_t count; // bytes currently buffered
    int readers;
    int writers;
    wait_queue_t read_waiters;  // blocked in pipe_read() on an empty buffer
    wait_queue_t write_waiters; // blocked in pipe_write() on a full buffer
} pipe_t;

/**
 * @brief Allocates a new pipe with one reader and one writer (the two ends
 * SYS_pipe hands back). Returns NULL on allocation failure.
 */
pipe_t *pipe_create(void);

/**
 * @brief Reads up to `count` bytes into `buf`, blocking while the buffer is
 * empty and at least one writer is still open. Returns the number of bytes
 * read, or 0 for EOF (buffer empty and no writers left).
 */
int pipe_read(pipe_t *p, char *buf, int count);

/**
 * @brief Writes up to `count` bytes from `buf`, blocking while the buffer
 * is full and at least one reader is still open. Returns the number of
 * bytes written, or -1 if no reader remains (broken pipe) and nothing was
 * written yet.
 */
int pipe_write(pipe_t *p, const char *buf, int count);

/**
 * @brief Drops one reference to `p`'s read end (is_read_end != 0) or write
 * end (is_read_end == 0), wakes any processes that might now be able to
 * make progress (a writer waiting on a reader, or a reader waiting on
 * EOF), and frees `p` once both ends have no references left. Called from
 * SYS_close, process exit, and fork-failure cleanup.
 */
void pipe_close_end(pipe_t *p, int is_read_end);

#endif // !PIPE_H
