#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Exercises copy_to_user()'s fault recovery (kernel/src/arch/x86_64/
// usercopy.c): a syscall handed a bad-but-canonical user pointer should
// fault at CPL 0 while the kernel is mid-copy, recover, and report -1 --
// not panic the kernel.
//
// Before that recovery existed, each of these
// calls would have taken down the whole system instead of just failing.
//
// The address is deliberately unmapped but far from both the ELF image
// (loaded low) and the stack (USER_STACK_TOP, up near 0x7000_0000_0000),
// so it can't accidentally land on a real page.

static void *const bad_ptr = (void *)0x600000000ULL;

// Non-canonical: bits 63:47 aren't all equal, so the CPU raises #GP
// (vector 13) rather than #PF (vector 14) -- it rejects the address
// before ever consulting the page tables.
static void *const noncanonical_ptr = (void *)0x0000800000000000ULL;

void main(void) {
  printf("usercopytest: start\n");

  int fd = open("/hello.txt");
  if (fd < 0) {
    printf("usercopytest: open failed\n");
    exit(1);
  }

  ssize_t r = read(fd, bad_ptr, 16);
  printf("usercopytest: read into bad buffer returned %d (want -1)\n", (int)r);
  close(fd);

  fd = open("/hello.txt");
  ssize_t r2 = read(fd, noncanonical_ptr, 16);
  printf("usercopytest: read into non-canonical buffer returned %d (want -1)\n", (int)r2);
  close(fd);

  ssize_t rd = listdir("/bin", bad_ptr, 128);
  printf("usercopytest: listdir into bad buffer returned %d (want -1)\n", (int)rd);

  int pid = fork();
  if (pid == 0) {
    exit(42);
  }

  int w = wait(pid, bad_ptr);

  printf("usercopytest: wait with bad status ptr returned %d (want -1)\n", w);

  printf("usercopytest: still alive, kernel did not panic\n");

  exit(0);
}
