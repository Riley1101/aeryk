#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void main(void) {
  printf("exectest: before fork\n");

  int pid = fork();

  if (pid == 0) {
    printf("exectest: child execing /bin/ls\n");
    execve("/bin/ls", "/bin");
    // Only reached if execve() failed.
    printf("exectest: execve() failed\n");
    exit(1);
  } else if (pid > 0) {
    int status = 0;
    int reaped = wait(pid, &status);
    printf("exectest: parent reaped pid=%d status=%d\n", reaped, status);
  } else {
    printf("exectest: fork() failed\n");
  }

  exit(0);
}
