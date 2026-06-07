#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#define MSR_EFER 0xC0000080
#define MSR_STAR 0xC0000081
#define MSR_LSTAR 0xC0000082
#define MSR_FMASK 0xC0000084

extern uint64_t kernel_rsp_scratch;

extern void syscall_entry(void);

void init_syscalls(void);

#endif // !SYSCALL_H
