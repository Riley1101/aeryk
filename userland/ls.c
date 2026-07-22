#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  const char *path = (argc > 1) ? argv[1] : "/";

  char buf[512];
  ssize_t n = listdir(path, buf, sizeof(buf));
  if (n < 0) {
    printf("ls: %s: No such directory\n", path);
    return 1;
  }

  write(1, buf, n);
  return 0;
}
