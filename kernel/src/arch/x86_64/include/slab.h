#ifndef SLAB_H
#define SLAB_H

#include <stddef.h>

/**
 * @brief Initializes the slab allocator's fixed-size-class caches.
 * Implemented in Rust: crates/kernel/src/slab.rs.
 */
void init_slab(void);

/**
 * @brief Allocates a block of at least `size` bytes from the slab
 * allocator. Returns NULL if `size` is 0, exceeds 2048 bytes, or no memory
 * is available.
 */
void *kmalloc(size_t size);

/**
 * @brief Returns a block previously obtained from kmalloc() to its slab.
 * Passing NULL is a no-op; passing a pointer that isn't a live kmalloc()
 * allocation is caught via a magic-number check and refused.
 */
void kfree(void *ptr);

#endif // !SLAB_H
