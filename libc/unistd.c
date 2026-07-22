#include <unistd.h>
#include <sys/syscall.h>

/**
 * @brief Opens a file.
 * @param path The path to the file.
 * @return Returns the file descriptor or -1 on error.
 */
int open(const char *path) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_open), "D"(path)
                : "rcx", "r11", "memory");
  return (int)ret;
}

/**
 * @brief Reads from a file descriptor.
 * @param fd The file descriptor to read from.
 * @param buf The buffer to read into.
 * @param count The number of bytes to read.
 * @return Returns the number of bytes read or -1 on error.
 */
ssize_t read(int fd, void *buf, size_t count) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_read), "D"(fd), "S"(buf), "d"(count)
                : "rcx", "r11", "memory");
  return ret;
}

/**
 * @brief Writes to a file descriptor.
 * @param fd The file descriptor to write to.
 * @param buf The buffer to write from.
 * @param count The number of bytes to write.
 * @return Returns the number of bytes written or -1 on error.
 */
ssize_t write(int fd, const void *buf, size_t count) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_write), "D"(fd), "S"(buf), "d"(count)
                : "rcx", "r11", "memory");
  return ret;
}

/**
 * @brief Closes a file descriptor.
 * @param fd The file descriptor to close.
 * @return Returns 0 on success or -1 on error.
 */
int close(int fd) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_close), "D"(fd)
                : "rcx", "r11", "memory");
  return (int)ret;
}

/**
 * @brief Spawns a new user process from an executable path.
 * @param path The path to the executable.
 * @return Returns the pid of the new process or -1 on error.
 */
int spawn(const char *path) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_spawn), "D"(path)
                : "rcx", "r11", "memory");
  return (int)ret;
}

/**
 * @brief Waits for a child process to terminate.
 * @param pid The process ID of the child to wait for.
 * @param status A pointer to an integer where the exit status will be stored.
 * @return Returns the pid of the terminated child or -1 on error.
 */
int wait (int pid, int *status) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_wait), "D"(pid), "S"(status)
                : "rcx", "r11", "memory");
  return (int)ret;
}


