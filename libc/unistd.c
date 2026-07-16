#include <unistd.h>
#include <sys/syscall.h>

int open(const char *path) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_open), "D"(path)
                : "rcx", "r11", "memory");
  return (int)ret;
}

ssize_t read(int fd, void *buf, size_t count) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_read), "D"(fd), "S"(buf), "d"(count)
                : "rcx", "r11", "memory");
  return ret;
}

ssize_t write(int fd, const void *buf, size_t count) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_write), "D"(fd), "S"(buf), "d"(count)
                : "rcx", "r11", "memory");
  return ret;
}

int close(int fd) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_close), "D"(fd)
                : "rcx", "r11", "memory");
  return (int)ret;
}
