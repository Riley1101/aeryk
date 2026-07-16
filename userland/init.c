#include <stdio.h>
#include <unistd.h>

/**
 * @brief The main function of the init process.
 *
 * This function is the entry point of the init process. It writes a banner message to stdout,
 * attempts to open and read from a file named "/hello.txt", and writes its contents to stdout.
 *
 * @return Returns 0 on success.
 */
int main(void) {
  printf("Hello from /bin/init (ring 3)!\n");

  char buffer[128];
  int fd = open("/hello.txt");
  if (fd >= 0) {
    ssize_t n = read(fd, buffer, sizeof(buffer));
    if (n > 0) {
      printf("Contents of /hello.txt:\n");
      write(1, buffer, n);
    }
    close(fd);
  }

  return 0;
}
