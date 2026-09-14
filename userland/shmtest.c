#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

// Exercises SYS_mmap/SYS_munmap: unlike forktest's COW copy, a MAP_SHARED
// mapping stays backed by the same physical pages across fork(), so a write
// by the child shows up in the parent without going through a pipe or any
// other IPC -- exactly the primitive a compositor needs for a client/server
// shared framebuffer.

void main(void) {
  printf("shmtest: start\n");

  int *shared = (int *)mmap(0, sizeof(int), PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (shared == MAP_FAILED) {
    printf("shmtest: mmap failed\n");
    exit(1);
  }
  *shared = 1;
  printf("shmtest: before fork, shared=%d\n", *shared);

  int pid = fork();
  if (pid == 0) {
    // Child: mutate the shared page and exit; the parent should see this
    // write without any explicit IPC.
    *shared = 42;
    printf("shmtest: child wrote shared=%d\n", *shared);
    exit(0);
  }
  if (pid < 0) {
    printf("shmtest: fork() failed\n");
    exit(1);
  }

  int status = 0;
  int reaped = wait(pid, &status);
  printf("shmtest: parent after wait, shared=%d (expect 42) reaped=%d status=%d (expect 0)\n",
         *shared, reaped, status);

  if (munmap(shared, sizeof(int)) != 0) {
    printf("shmtest: munmap failed\n");
    exit(1);
  }

  exit(0);
}
