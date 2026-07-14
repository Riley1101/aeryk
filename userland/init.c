/**
 *  TODO! Temporary init process for the userland.
 *  Replicate everything you did here in libc/unistd.c and remove this file.
 */

#include <sys/syscall.h>

/**
 * @brief Opens a file.
 * @param path The path to the file.
 * @return Returns the file descriptor or -1
 **/
static long sys_open(const char *path) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_open), "D"(path)
                : "rcx", "r11", "memory");
  return ret;
}


/**
 * @brief Reads from a file descriptor.
 * 
 * @param fd The file descriptor to read from.
 * @param buf The buffer to read into.
 * @param len The number of bytes to read.
 * @return Returns the number of bytes read or -1 on error.
 */
static long sys_read(long fd, void *buf, long len) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_read), "D"(fd), "S"(buf), "d"(len)
                : "rcx", "r11", "memory");
  return ret;
}

/**
 * @brief Writes to a file descriptor.
 * @param fd The file descriptor to write to.
 * @param buf The buffer to write from.
 * @param len The number of bytes to write.
 * @return Returns the number of bytes written or -1 on error.
 */
static long sys_write(long fd, const void *buf, long len) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_write), "D"(fd), "S"(buf), "d"(len)
                : "rcx", "r11", "memory");
  return ret;
}

/**
 * @brief Closes a file descriptor.
 * @param fd The file descriptor to close.
 * @return Returns 0 on success or -1 on error.
 */
static long sys_close(long fd) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_close), "D"(fd)
                : "rcx", "r11", "memory");
  return ret;
}

/**
 * @brief Calculates the length of a string.
 * @param s The string to calculate the length of.
 * @return Returns the length of the string.
 */
static long str_len(const char *s) {
  long n = 0;
  while (s[n]) {
    n++;
  }
  return n;
}

/**
 * @brief The main function of the init process.
 * 
 * This function is the entry point of the init process. It writes a banner message to stdout,
 * attempts to open and read from a file named "/hello.txt", and writes its contents to stdout.
 * 
 * @return Returns 0 on success.
 */
int main(void) {
  const char *banner = "Hello from /bin/init (ring 3)!\n";
  sys_write(1, banner, str_len(banner));

  char buffer[128];
  long fd = sys_open("/hello.txt");
  if (fd >= 0) {
    long n = sys_read(fd, buffer, sizeof(buffer));
    if (n > 0) {
      sys_write(1, buffer, n);
    }
  }

  return 0;
}
