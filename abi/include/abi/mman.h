#ifndef _ABI_MMAN_H
#define _ABI_MMAN_H

/**
 * @brief mmap(2) prot/flags ABI shared between the kernel and userland
 * (libc), values matching Linux x86_64 so existing intuition/documentation
 * transfers directly.
 */

#define PROT_NONE 0x0
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4

#define MAP_SHARED 0x01
#define MAP_PRIVATE 0x02
#define MAP_ANONYMOUS 0x20

#define MAP_FAILED ((void *)-1)

#endif // !_ABI_MMAN_H
