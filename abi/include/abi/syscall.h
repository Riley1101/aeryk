#ifndef _ABI_SYSCALL_H
#define _ABI_SYSCALL_H

/**
 * @brief Syscall number ABI shared between the kernel and userland (libc).
 *
 * This is the single source of truth for syscall numbers. Both
 * `kernel/src/arch/x86_64/include/syscall.h` (kernel side) and
 * `libc/include/sys/syscall.h` (userland side) include this header so the
 * two sides can never disagree on numbering.
 */

#define SYS_read  0
#define SYS_write 1
#define SYS_open  2
#define SYS_close 3
#define SYS_exit  60

#endif // !_ABI_SYSCALL_H
