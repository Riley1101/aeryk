#ifndef _ABI_SYSCALL_H
#define _ABI_SYSCALL_H

/**
 * @brief Syscall number ABI shared between the kernel and userland (libc).
 *
 * This is the single source of truth for syscall numbers. Both
 * `kernel/src/arch/x86_64/include/syscall.h` (kernel side) and
 * `libc/include/sys/syscall.h` (userland side) include this header so the
 * two sides can never disagree on numbering.
 * @see https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md#x86_64-64_bit
 */

#define SYS_read 0
#define SYS_write 1
#define SYS_open 2
#define SYS_close 3
#define SYS_spawn 4
#define SYS_wait 5
#define SYS_readdir 6
#define SYS_fork 57
#define SYS_execve 59
#define SYS_exit 60

#endif // !_ABI_SYSCALL_H
