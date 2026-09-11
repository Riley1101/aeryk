#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  // With no file argument, read from stdin -- this is what makes `cat`
  // useful as the tail end of a pipeline (e.g. `ls | cat`).
  int fd = 0;
  if (argc >= 2) {
    fd = open(argv[1]);
    if (fd < 0) {
      printf("cat: %s: No such file\n", argv[1]);
      return 1;
    }
  }

  char buf[128];
  ssize_t n;
  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    write(1, buf, n);
  }

  if (fd != 0) {
    close(fd);
  }
  return 0;
}
