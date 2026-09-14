#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static char child_stack[4096];
static int shared_value = 1;

int thread_main(void *arg) {
  // Shared address space: unlike forktest's COW copy, this write is
  // visible to the parent immediately, not just after some IPC.
  int *val = (int *)arg;
  *val = 42;
  printf("clonetest: child running, shared_value=%d\n", *val);
  return 7;
}

void main(void) {
  printf("clonetest: before clone, shared_value=%d\n", shared_value);

  int pid = thread_create(thread_main, child_stack, sizeof(child_stack), &shared_value);
  if (pid < 0) {
    printf("clonetest: thread_create() failed\n");
    exit(1);
  }

  int status = 0;
  int reaped = wait(pid, &status);
  printf("clonetest: parent after wait, shared_value=%d (expect 42) reaped=%d status=%d (expect 7)\n",
         shared_value, reaped, status);

  exit(0);
}
