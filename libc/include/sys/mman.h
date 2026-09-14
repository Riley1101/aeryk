#ifndef _SYS_MMAN_H
#define _SYS_MMAN_H 1

#include <stddef.h>
#include <stdint.h>

#include <abi/mman.h>

/**
 * @brief Maps a new anonymous region of `length` bytes into the calling
 * process's address space. Only MAP_ANONYMOUS mappings are supported (no
 * file-backed mmap yet), so `fd` must be -1 and `offset` is ignored.
 *
 * With MAP_SHARED, the mapping stays backed by the same physical pages
 * across a fork(): parent and child (and further descendants) all read and
 * write through to the same memory, which is what makes this usable for a
 * compositor's client/server shared framebuffer-style buffers. With
 * MAP_PRIVATE, the mapping is an ordinary process-private region --
 * copy-on-write split from any child the same as heap or stack pages.
 * @param addr Ignored; the kernel always chooses the placement itself
 * (equivalent to always passing NULL on Linux).
 * @param length Number of bytes to map; rounded up to a whole number of
 * pages.
 * @param prot Bitwise OR of PROT_READ/PROT_WRITE/PROT_EXEC/PROT_NONE.
 * @param flags Bitwise OR of MAP_ANONYMOUS and exactly one of
 * MAP_SHARED/MAP_PRIVATE.
 * @param fd Must be -1 (anonymous mappings only).
 * @param offset Ignored (anonymous mappings only).
 * @return Pointer to the start of the new mapping, or MAP_FAILED on error
 * (errno set).
 */
void *mmap(void *addr, size_t length, int prot, int flags, int fd, int64_t offset);

/**
 * @brief Unmaps the pages covering [addr, addr + length), freeing each
 * physical frame no longer referenced by any mapping. `addr` must be a
 * page-aligned address previously returned by mmap().
 * @param addr Start of the region to unmap.
 * @param length Number of bytes to unmap; rounded up to a whole number of
 * pages.
 * @return 0 on success, or -1 on error (errno set).
 */
int munmap(void *addr, size_t length);

#endif // !_SYS_MMAN_H
