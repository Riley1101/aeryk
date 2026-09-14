#ifndef _ABI_CLONE_H
#define _ABI_CLONE_H

/**
 * @brief Flags for SYS_clone, shared between the kernel and userland.
 *
 * Numbered to match Linux so this stays a strict subset, even though only
 * CLONE_VM is actually recognized so far (see clone_process() in
 * kernel/src/arch/x86_64/process.c). An unrecognized flag bit is silently
 * ignored rather than rejected, same as an early Linux would tolerate a
 * newer libc passing bits it doesn't understand yet.
 */
#define CLONE_VM 0x00000100 /* Share the address space (threads) instead of COW-cloning it (fork) */

#endif // !_ABI_CLONE_H
