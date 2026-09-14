#ifndef _ABI_ERRNO_H
#define _ABI_ERRNO_H

/**
 * @brief Errno codes shared between the kernel and userland (libc).
 *
 * Numbering follows Linux so the syscall ABI stays a strict subset (a
 * syscall returns -errno on failure, exactly like Linux does), even though
 * only a handful of codes are actually produced by this kernel so far.
 *
 * A kernel syscall handler returns `-EFOO` directly in rax on failure. Each
 * libc syscall wrapper (see libc/unistd.c) checks for a small negative
 * return, stores the positive code in the userland `errno` global, and
 * returns -1 to the caller — the same split Linux uses between the raw
 * syscall and its libc wrapper.
 */

#define EPERM 1   /* Operation not permitted */
#define ENOENT 2  /* No such file or directory */
#define ESRCH 3   /* No such process */
#define EBADF 9   /* Bad file descriptor */
#define ECHILD 10 /* No child processes */
#define EAGAIN 11 /* Try again */
#define ENOMEM 12 /* Out of memory */
#define EFAULT 14 /* Bad address */
#define EEXIST 17 /* File exists */
#define ENOTDIR 20 /* Not a directory */
#define EINVAL 22 /* Invalid argument */
#define EMFILE 24 /* Too many open files */
#define ENOSYS 38 /* Function not implemented */

#endif // !_ABI_ERRNO_H
