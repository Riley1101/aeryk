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

#endif // !_UNISTD_H
