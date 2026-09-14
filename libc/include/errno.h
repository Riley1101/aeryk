#ifndef _ERRNO_H
#define _ERRNO_H 1

#include <abi/errno.h>

/**
 * @brief Last error set by a failing syscall wrapper in this process.
 *
 * Single global rather than thread-local: there is no userland threading
 * yet, and each process has its own address space, so this is already
 * per-process.
 */
extern int errno;

#endif // !_ERRNO_H
