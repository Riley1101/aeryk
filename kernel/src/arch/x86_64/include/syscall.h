#ifndef SYSCALL_H
#define SYSCALL_H

#include <abi/syscall.h>
#include <stdint.h>

/**
 * @brief Kernel System call numbers for x86_64 architecture.
 *
 * This header file defines the system MSR registers and kernel syscall declearations.
 * Syscall numbers (SYS_*) come from <abi/syscall.h>, the shared kernel/userland ABI.
 */

#define MSR_EFER 0xC0000080
#define MSR_STAR 0xC0000081
#define MSR_LSTAR 0xC0000082
#define MSR_FMASK 0xC0000084

extern uint64_t kernel_rsp_scratch;

extern void syscall_entry(void);

void init_syscalls(void);

/**
 * @brief Releases the calling-process-exclusive SYS_fbmap lock if `pid`
 * currently holds it (a no-op otherwise). Must be called from every path
 * that tears a process down -- both process_release_fds() call sites
 * (SYS_exit and the CPL-3 fault killer in idt.c) -- so a crashed or
 * exited compositor doesn't permanently lock out any future one. See the
 * SYS_fbmap case in syscall.c for why the lock exists: only one process
 * should ever own the real screen at a time.
 */
void fbmap_release_owner(uint64_t pid);

#endif // !SYSCALL_H
