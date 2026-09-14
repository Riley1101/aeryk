#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/mouse.h>
#include <sys/fb.h>
#include <sys/tsc.h>
#include <abi/clone.h>
#include <errno.h>
#include <stdlib.h>

/**
 * @brief Translates a raw syscall return value into the libc convention.
 *
 * Kernel syscall handlers return -errno directly in rax on failure (see
 * kernel/src/arch/x86_64/syscall.c), matching Linux. This is the userland
 * half of that split: stash the positive code in `errno` and normalize the
 * return to -1, so callers can keep checking `< 0` without knowing the
 * magic negative-range convention.
 */
static long syscall_ret(long ret) {
  if (ret < 0 && ret > -4096) {
    errno = (int)-ret;
    return -1;
  }
  return ret;
}

/**
 * @brief Opens a file.
 * @param path The path to the file.
 * @return Returns the file descriptor or -1 on error (errno set).
 */
int open(const char *path) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_open), "D"(path)
                : "rcx", "r11", "memory");
  return (int)syscall_ret(ret);
}

/**
 * @brief Reads from a file descriptor.
 * @param fd The file descriptor to read from.
 * @param buf The buffer to read into.
 * @param count The number of bytes to read.
 * @return Returns the number of bytes read or -1 on error (errno set).
 */
ssize_t read(int fd, void *buf, size_t count) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_read), "D"(fd), "S"(buf), "d"(count)
                : "rcx", "r11", "memory");
  return syscall_ret(ret);
}

/**
 * @brief Writes to a file descriptor.
 * @param fd The file descriptor to write to.
 * @param buf The buffer to write from.
 * @param count The number of bytes to write.
 * @return Returns the number of bytes written or -1 on error (errno set).
 */
ssize_t write(int fd, const void *buf, size_t count) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_write), "D"(fd), "S"(buf), "d"(count)
                : "rcx", "r11", "memory");
  return syscall_ret(ret);
}

/**
 * @brief Closes a file descriptor.
 * @param fd The file descriptor to close.
 * @return Returns 0 on success or -1 on error (errno set).
 */
int close(int fd) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_close), "D"(fd)
                : "rcx", "r11", "memory");
  return (int)syscall_ret(ret);
}

/**
 * @brief Creates an unnamed pipe.
 * @param fds Filled with fds[0] (read end) and fds[1] (write end).
 * @return Returns 0 on success or -1 on error (errno set).
 */
int pipe(int fds[2]) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_pipe), "D"(fds)
                : "rcx", "r11", "memory");
  return (int)syscall_ret(ret);
}

/**
 * @brief Duplicates a file descriptor onto the lowest-numbered unused fd.
 * @param oldfd The file descriptor to duplicate.
 * @return Returns the new file descriptor or -1 on error (errno set).
 */
int dup(int oldfd) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_dup), "D"(oldfd)
                : "rcx", "r11", "memory");
  return (int)syscall_ret(ret);
}

/**
 * @brief Duplicates a file descriptor onto a specific fd number.
 * @param oldfd The file descriptor to duplicate.
 * @param newfd The file descriptor number to duplicate it onto.
 * @return Returns newfd on success or -1 on error (errno set).
 */
int dup2(int oldfd, int newfd) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_dup2), "D"(oldfd), "S"(newfd)
                : "rcx", "r11", "memory");
  return (int)syscall_ret(ret);
}

/**
 * @brief Forks the calling process.
 * @return Returns 0 in the child, the child's pid in the parent, or -1 on
 * error (errno set).
 */
int fork(void) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_fork)
                : "rcx", "r11", "memory");
  return (int)syscall_ret(ret);
}

/**
 * @brief Raw clone(2)-style syscall. See unistd.h.
 */
int clone(uint64_t flags, void *stack) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_clone), "D"(flags), "S"(stack)
                : "rcx", "r11", "memory");
  return (int)syscall_ret(ret);
}

/**
 * @brief Minimal pthread_create()-alike built on clone(CLONE_VM, ...). See
 * unistd.h.
 */
int thread_create(int (*fn)(void *), void *stack, size_t stack_size, void *arg) {
  // Stack grows down; start the child at the top of the region, 16-byte
  // aligned per the x86_64 SysV ABI.
  uint64_t top = ((uint64_t)stack + stack_size) & ~(uint64_t)0xF;

  int pid = clone(CLONE_VM, (void *)top);
  if (pid == 0) {
    // Child: clone() resumes here at the exact same C statement as the
    // parent (like fork()), just on the new stack -- branch here instead
    // of falling through to whatever the parent does next.
    exit(fn(arg));
  }
  return pid;
}

/**
 * @brief Replaces the calling process's image with a new executable.
 * Named after Linux's execve syscall (number 59), though this simplifies
 * the signature to leave out envp (no environment variables yet).
 * @param path The path to the executable.
 * @param argv NULL-terminated array of argument strings; conventionally
 * argv[0] is the program name, but that's the caller's responsibility.
 * @return Returns -1 on error (errno set). Does not return on success.
 */
int execve(const char *path, char *const argv[]) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_execve), "D"(path), "S"(argv)
                : "rcx", "r11", "memory");
  return (int)syscall_ret(ret);
}

/**
 * @brief Waits for a child process to terminate.
 * @param pid The process ID of the child to wait for.
 * @param status A pointer to an integer where the exit status will be stored.
 * @return Returns the pid of the terminated child or -1 on error (errno
 * set).
 */
int wait (int pid, int *status) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_wait), "D"(pid), "S"(status)
                : "rcx", "r11", "memory");
  return (int)syscall_ret(ret);
}

/**
 * @brief Sets the process's program break.
 * @param addr The new break address.
 * @return The resulting break address.
 */
uint64_t brk(uint64_t addr) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_brk), "D"(addr)
                : "rcx", "r11", "memory");
  return (uint64_t)ret;
}

/**
 * @brief Grows (or shrinks) the process's heap by `increment` bytes.
 * @param increment Bytes to extend the break by; 0 just queries the
 * current break.
 * @return The break address before the call, or (void *)-1 on failure.
 */
void *sbrk(int64_t increment) {
  uint64_t old_brk = brk(0);
  if (increment == 0) {
    return (void *)old_brk;
  }
  if (increment < 0) {
    return (void *)-1;
  }

  uint64_t new_brk = brk(old_brk + (uint64_t)increment);
  if (new_brk != old_brk + (uint64_t)increment) {
    return (void *)-1;
  }
  return (void *)old_brk;
}

/**
 * @brief Lists the entries of a directory into a buffer.
 * @param path The path to the directory.
 * @param buf Destination buffer.
 * @param size The size of buf.
 * @return The number of bytes written, or -1 on error (errno set).
 */
ssize_t listdir(const char *path, char *buf, size_t size) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_readdir), "D"(path), "S"(buf), "d"(size)
                : "rcx", "r11", "memory");
  return syscall_ret(ret);
}

/**
 * @brief Maps a new anonymous region into this process's address space.
 * See sys/mman.h.
 */
void *mmap(void *addr, size_t length, int prot, int flags, int fd, int64_t offset) {
  (void)addr;
  long ret;
  register long r10 asm("r10") = flags;
  register long r8 asm("r8") = fd;
  register long r9 asm("r9") = offset;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_mmap), "D"(0), "S"(length), "d"(prot), "r"(r10), "r"(r8), "r"(r9)
                : "rcx", "r11", "memory");
  ret = syscall_ret(ret);
  return ret < 0 ? MAP_FAILED : (void *)ret;
}

/**
 * @brief Unmaps a previously mmap()'d region. See sys/mman.h.
 */
int munmap(void *addr, size_t length) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_munmap), "D"(addr), "S"(length)
                : "rcx", "r11", "memory");
  return (int)syscall_ret(ret);
}

/**
 * @brief Reads decoded PS/2 mouse packets. See sys/mouse.h.
 */
int mouse_read(mouse_packet_t *buf, int max_packets, int nonblock) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_mouse_read), "D"(buf), "S"(max_packets), "d"(nonblock)
                : "rcx", "r11", "memory");
  return (int)syscall_ret(ret);
}

/**
 * @brief Returns the calibrated TSC frequency in Hz. See sys/tsc.h.
 */
uint64_t get_tsc_hz(void) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_get_tsc_hz)
                : "rcx", "r11", "memory");
  return (uint64_t)syscall_ret(ret);
}

/**
 * @brief Blocks the calling process for at least `ms` milliseconds. See
 * sys/tsc.h.
 */
int sleep_ms(uint32_t ms) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_sleep_ms), "D"(ms)
                : "rcx", "r11", "memory");
  return (int)syscall_ret(ret);
}

/**
 * @brief Maps the kernel's framebuffer into this process. See sys/fb.h.
 */
void *fbmap(fb_info_t *info) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(SYS_fbmap), "D"(info)
                : "rcx", "r11", "memory");
  ret = syscall_ret(ret);
  return ret < 0 ? MAP_FAILED : (void *)ret;
}
