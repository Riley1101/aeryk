#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Exercises SYS_pipe/SYS_dup/SYS_dup2: a child dup2()s the pipe's write end
// onto stdout and prints through it (the shell `cmd1 | cmd2` pattern), the
// parent reads the pipe's read end directly, and a second round trip
// confirms a blocking read (parent reads before the child has written
// anything) actually parks on pipe_read()'s wait queue instead of spinning
// or returning early.

void main(void)
{
    printf("pipetest: start\n");

    int fds[2];
    if (pipe(fds) != 0)
    {
        printf("pipetest: pipe() failed\n");
        exit(1);
    }

    int pid = fork();
    if (pid == 0)
    {
        // Child: writer end only, redirected onto stdout.
        close(fds[0]);
        if (dup2(fds[1], 1) != 1)
        {
            printf("pipetest: child dup2 failed\n");
            exit(1);
        }
        close(fds[1]);
        printf("hello through the pipe\n");
        exit(0);
    }

    if (pid < 0)
    {
        printf("pipetest: fork() failed\n");
        exit(1);
    }

    // Parent: reader end only. Read before the child necessarily got to run
    // -- this should block in pipe_read() until the child writes, not return
    // empty.
    close(fds[1]);
    char buf[128];
    int n = read(fds[0], buf, sizeof(buf) - 1);
    buf[n > 0 ? n : 0] = '\0';

    int status = 0;
    int reaped = wait(pid, &status);

    printf("pipetest: read %d bytes: %s", n, buf);
    printf("pipetest: reaped=%d status=%d (want 0)\n", reaped, status);

    // EOF check: writer end is gone (child closed and exited), so a second
    // read must return 0, not block forever.
    int n2 = read(fds[0], buf, sizeof(buf) - 1);
    printf("pipetest: second read after writer closed returned %d (want 0, EOF)\n", n2);

    close(fds[0]);
    printf("pipetest: done\n");
    exit(0);
}
