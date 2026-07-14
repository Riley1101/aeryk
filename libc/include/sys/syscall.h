#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H 1

// Syscall numbers shared between the kernel's dispatcher
// (kernel/src/arch/x86_64/syscall.c) and userland/libc callers. Keep this
// file as the single source of truth -- do not hardcode these numbers
// anywhere else.
//
// Numbering follows the Linux x86_64 syscall ABI where practical, so the
// values below aren't arbitrary picks.

#define SYS_read 0
#define SYS_write 1
#define SYS_open 2
#define SYS_close 3
#define SYS_exit 60

#endif // !_SYS_SYSCALL_H
