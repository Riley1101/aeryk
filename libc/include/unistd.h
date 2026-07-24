#ifndef _UNISTD_H
#define _UNISTD_H 1

#include <stddef.h>

typedef long ssize_t;

/**
 * @brief Opens a file.
 * @param path The path to the file.
 * @return Returns the file descriptor or -1 on error.
 */
int open(const char *path);

/**
 * @brief Reads from a file descriptor.
 * @param fd The file descriptor to read from.
 * @param buf The buffer to read into.
 * @param count The number of bytes to read.
 * @return Returns the number of bytes read or -1 on error.
 */
ssize_t read(int fd, void *buf, size_t count);

/**
 * @brief Writes to a file descriptor.
 * @param fd The file descriptor to write to.
 * @param buf The buffer to write from.
 * @param count The number of bytes to write.
 * @return Returns the number of bytes written or -1 on error.
 */
ssize_t write(int fd, const void *buf, size_t count);

/**
 * @brief Closes a file descriptor.
 * @param fd The file descriptor to close.
 * @return Returns 0 on success or -1 on error.
 */
int close(int fd);

/**
 * @brief Forks the calling process, creating a near-identical copy that
 * resumes right after this call.
 * @return Returns 0 in the child, the child's pid in the parent, or -1 on
 * error.
 */
int fork(void);

/**
 * @brief Replaces the calling process's image with a new executable,
 * keeping the same pid, parent, and open file descriptors. Named after
 * Linux's execve syscall (number 59), though this simplifies the
 * signature to leave out envp (no environment variables yet).
 * @param path The path to the executable.
 * @param argv NULL-terminated array of argument strings; conventionally
 * argv[0] is the program name, but that's the caller's responsibility.
 * @return Returns -1 on error. Does not return on success.
 */
int execve(const char *path, char *const argv[]);

/**
 * @brief Waits for a child process to terminate.
 * @param pid The process ID of the child to wait for.
 * @param status A pointer to an integer where the exit status will be stored.
 * @return Returns the pid of the terminated child or -1 on error.
 */
int wait(int pid, int *status);

/**
 * @brief Lists the entries of a directory into a buffer.
 * Each entry is written as its name followed by '\n', with a trailing
 * '/' appended for subdirectories.
 * @param path The path to the directory.
 * @param buf Destination buffer.
 * @param size The size of buf.
 * @return The number of bytes written, or -1 if path is not a directory.
 */
ssize_t listdir(const char *path, char *buf, size_t size);

#endif // !_UNISTD_H
