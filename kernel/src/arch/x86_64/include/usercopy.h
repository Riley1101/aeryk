#ifndef USERCOPY_H
#define USERCOPY_H

#include <stddef.h>

/**
 * @brief Copies `n` bytes from a user-space pointer into a kernel buffer.
 *
 * Runs at CPL 0 like any other syscall body, but if `usrc` (or any byte in
 * the following `n` bytes) faults -- because userland passed a bad,
 * unmapped, or otherwise malicious pointer -- isr_handler() recovers by
 * unwinding back into this function instead of treating it as a kernel
 * bug. See usercopy.c for how the recovery point is registered.
 * @param dst Kernel destination buffer, at least `n` bytes.
 * @param usrc User-space source pointer.
 * @param n Number of bytes to copy.
 * @return 0 on success, -1 if the copy faulted partway through (contents
 * of `dst` are then unspecified).
 */
int copy_from_user(void *dst, const void *usrc, size_t n);

/**
 * @brief Copies `n` bytes from a kernel buffer into a user-space pointer.
 * Fault recovery works the same way as copy_from_user(); see there.
 * @param udst User-space destination pointer.
 * @param src Kernel source buffer, at least `n` bytes.
 * @param n Number of bytes to copy.
 * @return 0 on success, -1 if the copy faulted partway through (contents
 * at `udst` are then unspecified).
 */
int copy_to_user(void *udst, const void *src, size_t n);

/**
 * @brief Called from isr_handler() on a page fault taken at CPL 0.
 * If a copy_from_user()/copy_to_user() call is currently in flight, this
 * unwinds execution straight back to it (which then returns -1) instead
 * of letting the fault fall through to the kernel-panic path. Never
 * returns when it recovers a fault.
 */
void usercopy_recover_or_return(void);

#endif
