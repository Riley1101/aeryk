#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <stddef.h>

/**
 * @brief Terminate the calling process with the given status code.
 * @param status The exit status code to return to the operating system.
 */
void exit(int status);

/**
 * @brief Allocates `size` bytes of uninitialized heap memory.
 * Grows the process's heap via sbrk() on demand, in a free list built out
 * of the memory sbrk() hands back (see malloc.c). Not safe to call from
 * more than one thread of execution at a time -- there's no locking, but
 * this kernel doesn't have userland threads yet either.
 * @param size Number of bytes requested.
 * @return Pointer to the allocated block, or NULL if the heap couldn't be
 * grown far enough.
 */
void *malloc(size_t size);

/**
 * @brief Returns a block previously obtained from malloc()/calloc() to the
 * free list, coalescing it with adjacent free blocks where possible.
 * Freeing NULL, or a pointer not obtained from malloc()/calloc(), is
 * undefined behavior (the latter will corrupt the free list).
 * @param ptr Pointer previously returned by malloc()/calloc(), or NULL
 * (a no-op).
 */
void free(void *ptr);

/**
 * @brief Allocates memory for `nmemb` elements of `size` bytes each,
 * zeroed. Equivalent to malloc(nmemb * size) followed by memset(0), except
 * it also catches the nmemb*size multiplication overflowing.
 * @param nmemb Number of elements.
 * @param size Size of each element, in bytes.
 * @return Pointer to the zeroed allocation, or NULL on failure (including
 * overflow).
 */
void *calloc(size_t nmemb, size_t size);

#endif // !_STDLIB_H
