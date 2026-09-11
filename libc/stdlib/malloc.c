#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief Header prefixed to every block on the free list, sized/aligned by
 * the union so every allocation is naturally aligned for any type. `size`
 * is in units of sizeof(header_t), covering the header itself, so the
 * usable payload after a header is (size - 1) * sizeof(header_t) bytes.
 * `next` links free blocks together in address order (used by free() to
 * find and coalesce with neighbors); it's meaningless for an in-use block.
 */
typedef union header {
  struct {
    union header *next;
    size_t size;
  } s;
  long align; // forces 8-byte alignment, enough for any type this libc hands out
} header_t;

// Minimum number of header_t units to request from sbrk() at a time, so
// small mallocs don't each trigger their own syscall.
#define MIN_ALLOC_UNITS (4096 / sizeof(header_t))

// Sentinel free block of size 0 that free_list always points into,
// giving the allocator a starting point even when nothing has been freed
// yet. Never coalesced away since a real block's size is always >= 1.
static header_t base;
static header_t *free_list = NULL;

/**
 * @brief Asks the kernel for more heap via sbrk(), formats it as one big
 * free block, and links it into the free list via free().
 * @param units Minimum number of header_t units needed; rounded up to
 * MIN_ALLOC_UNITS to batch sbrk() calls.
 * @return The (possibly larger, post-coalesce) free list, or NULL if
 * sbrk() failed.
 */
static header_t *grow_heap(size_t units) {
  if (units < MIN_ALLOC_UNITS) {
    units = MIN_ALLOC_UNITS;
  }

  void *raw = sbrk((int64_t)(units * sizeof(header_t)));
  if (raw == (void *)-1) {
    return NULL;
  }

  header_t *block = (header_t *)raw;
  block->s.size = units;
  free(block + 1);
  return free_list;
}

void *malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }

  size_t units = (size + sizeof(header_t) - 1) / sizeof(header_t) + 1;

  header_t *prev = free_list;
  if (prev == NULL) {
    base.s.next = free_list = prev = &base;
    base.s.size = 0;
  }

  for (header_t *cur = prev->s.next;; prev = cur, cur = cur->s.next) {
    if (cur->s.size >= units) {
      if (cur->s.size == units) {
        prev->s.next = cur->s.next;
      } else {
        cur->s.size -= units;
        cur += cur->s.size;
        cur->s.size = units;
      }
      free_list = prev;
      return (void *)(cur + 1);
    }
    if (cur == free_list) {
      if ((cur = grow_heap(units)) == NULL) {
        return NULL;
      }
    }
  }
}

void free(void *ptr) {
  if (ptr == NULL) {
    return;
  }

  header_t *block = (header_t *)ptr - 1;
  header_t *cur;
  // Find the free block immediately before (or wrapping around past) where
  // `block` belongs in address order.
  for (cur = free_list; !(block > cur && block < cur->s.next); cur = cur->s.next) {
    if (cur >= cur->s.next && (block > cur || block < cur->s.next)) {
      // Either the only block on the list, or `block` sits past the
      // highest-addressed block or before the lowest -- either way this
      // is the insertion point.
      break;
    }
  }

  if (block + block->s.size == cur->s.next) {
    // Merge with the block right after.
    block->s.size += cur->s.next->s.size;
    block->s.next = cur->s.next->s.next;
  } else {
    block->s.next = cur->s.next;
  }

  if (cur + cur->s.size == block) {
    // Merge with the block right before.
    cur->s.size += block->s.size;
    cur->s.next = block->s.next;
  } else {
    cur->s.next = block;
  }

  free_list = cur;
}

void *calloc(size_t nmemb, size_t size) {
  if (nmemb != 0 && size > (size_t)-1 / nmemb) {
    return NULL; // would overflow
  }

  size_t total = nmemb * size;
  void *ptr = malloc(total);
  if (ptr) {
    memset(ptr, 0, total);
  }
  return ptr;
}
