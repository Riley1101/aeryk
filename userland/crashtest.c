#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Exercises the exception handler's per-process fault isolation
// (idt.c isr_handler): a user-mode fault should kill only the faulting
// process, not halt the kernel. Run under a fork so the parent can
// confirm it (and the rest of the system) survived the child's crash.
static void crash(const char *kind) {
  if (strcmp(kind, "div0") == 0) {
    // Both operands must be opaque to the optimizer. A literal `1 / zero`
    // (even with zero volatile) lets GCC strength-reduce division-by-constant
    // -1/0/1 into a branchless compare that just defines 1/0 as 0, sidestepping
    // idiv entirely -- no trap, ever. Producing both operands via inline asm
    // forces a real idiv (and #DE) at runtime.
    int one, zero;
    asm volatile("mov $1, %0" : "=r"(one));
    asm volatile("xor %0, %0" : "=r"(zero));
    volatile int result = one / zero;
    (void)result;
  } else if (strcmp(kind, "ud2") == 0) {
    asm volatile("ud2");
  } else {
    // Default: null pointer write -> page fault.
    volatile int *p = (volatile int *)0;
    *p = 1;
  }
}

void main(int argc, char *argv[]) {

  const char *kind = argc > 1 ? argv[1] : "null";

  printf("crashtest: before fork, crash kind=%s\n", kind);

  int pid = fork();

  if (pid == 0) {
    printf("crashtest: child about to crash\n");
    crash(kind);
    // Should never reach here.
    printf("crashtest: child survived the crash?!\n");
    exit(1);
  } else if (pid > 0) {
    int status = 0;
    int reaped = wait(pid, &status);
    printf("crashtest: parent alive after wait, reaped=%d status=%d\n", reaped, status);
  } else {
    printf("crashtest: fork() failed\n");
  }

  exit(0);
}
