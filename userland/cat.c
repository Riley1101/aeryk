#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("usage: cat <file>\n");
    return 1;
  }

  int fd = open(argv[1]);
  if (fd < 0) {
    printf("cat: %s: No such file\n", argv[1]);
    return 1;
  }

  char buf[128];
  ssize_t n;
  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    write(1, buf, n);
  }

  close(fd);
  return 0;
}
