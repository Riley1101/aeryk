#ifndef SLAB_H
#define SLAB_H

#include <stddef.h>
#include <stdint.h>
struct slab_chunk {
  struct slab_chunk *next;
};

struct slab {
  uint32_t magic;
  struct slab *next;
  struct slab *prev;
  struct slab_cache *cache;
  struct slab_chunk *free_list;
  size_t free_chunks;
  size_t total_chunks;
};

struct slab_cache {
  size_t object_size;
  struct slab *slabs_partial;
  struct slab *slabs_free;
  struct slab *slabs_full;
};

void initSlab(void);
void *kmalloc(size_t size);
void kfree(void *ptr);

#endif // !SLAB_H
