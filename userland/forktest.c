#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int counter = 1;

void main(void) {
  printf("forktest: before fork, counter=%d\n", counter);

  int pid = fork();

  if (pid == 0) {
    // Child: write to the shared-before-fork page. Under COW this must
    // fault, get a private copy, and not affect the parent's value.
    counter = 99;
    printf("forktest: child running, counter=%d\n", counter);
    exit(42);
  } else if (pid > 0) {
    int status = 0;
    int reaped = wait(pid, &status);
    printf("forktest: parent after wait, counter=%d (expect 1) reaped=%d status=%d\n",
           counter, reaped, status);
  } else {
    printf("forktest: fork() failed\n");
  }

  exit(0);
}
