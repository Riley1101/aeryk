#include "slab.h"
#include "pmm.h"
#include "tty.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SLAB_MAGIC 0x51AB51AB
#define NUM_CACHES 8

static struct slab_cache kmalloc_caches[NUM_CACHES];

static const size_t cache_sizes[NUM_CACHES] = {16,  32,  64,   128,
                                               256, 512, 1024, 2048};

static void list_remove(struct slab **head, struct slab *s) {
  if (s->prev) {
    s->prev->next = s->next;
  }
  if (s->next) {
    s->next->prev = s->prev;
  }
  if (*head == s) {
    *head = s->next;
  }
}

static void list_add(struct slab **head, struct slab *s) {

  s->next = *head;
  s->prev = NULL;
  if (*head) {
    (*head)->prev = s;
  }
  *head = s;
}

void init_slab(void) {
  for (int i = 0; i < NUM_CACHES; i++) {
    kmalloc_caches[i].object_size = cache_sizes[i];
    kmalloc_caches[i].slabs_partial = NULL;
    kmalloc_caches[i].slabs_free = NULL;
    kmalloc_caches[i].slabs_full = NULL;
  }
}

static struct slab *allocate_new_slab(struct slab_cache *cache) {
  void *raw_page_phys = pmm_alloc_page();

  if (!raw_page_phys) {
    return NULL;
  }

  void *raw_page_virt = (void *)((uint64_t)raw_page_phys + hhdm_offset);
  memset(raw_page_virt, 0, PAGE_SIZE);

  struct slab *new_slab = (struct slab *)raw_page_virt;
  new_slab->magic = SLAB_MAGIC;
  new_slab->cache = cache;
  new_slab->next = NULL;
  new_slab->prev = NULL;
  new_slab->free_list = NULL;
  new_slab->free_chunks = 0;

  uint64_t data_start = (uint64_t)raw_page_virt + sizeof(struct slab);
  data_start = (data_start + 15) & ~15;

  uint64_t data_end = (uint64_t)raw_page_virt + PAGE_SIZE;

  size_t obj_size = cache->object_size;

  for (uint64_t chunk_addr = data_start; chunk_addr + obj_size <= data_end;
       chunk_addr += obj_size) {
    struct slab_chunk *chunk = (struct slab_chunk *)chunk_addr;
    chunk->next = new_slab->free_list;
    new_slab->free_list = chunk;
    new_slab->free_chunks++;
  }
  new_slab->total_chunks = new_slab->free_chunks;

  return new_slab;
}

void *kmalloc(size_t size) {
  if (size == 0) {
    return NULL;
  }
  if (size > 2048) {
    print("[!] kmalloc: Allocations > 2048 bytes not supported!\n");
    return NULL;
  }

  struct slab_cache *cache = NULL;

  for (int i = 0; i < NUM_CACHES; i++) {
    if (size <= cache_sizes[i]) {
      cache = &kmalloc_caches[i];
      break;
    }
  }
  if (!cache) {
    return NULL;
  }

  struct slab *target_slab = cache->slabs_partial;

  if (!target_slab) {
    target_slab = cache->slabs_free;
    if (target_slab) {
      list_remove(&cache->slabs_free, target_slab);
      list_add(&cache->slabs_partial, target_slab);
    } else {
      target_slab = allocate_new_slab(cache);
      if (!target_slab) {
        return NULL;
      }
      list_add(&cache->slabs_partial, target_slab);
    }
  }

  struct slab_chunk *chunk = target_slab->free_list;
  target_slab->free_list = chunk->next;
  target_slab->free_chunks--;

  if (target_slab->free_chunks == 0) {
    list_remove(&cache->slabs_partial, target_slab);
    list_add(&cache->slabs_full, target_slab);
  }
  return (void *)chunk;
};

void kfree(void *ptr) {
  if (!ptr) {
    return;
  }

  // Because slabs are aligned exactly to 4KiB pages, we can find the metadata
  // by masking out the lower 12 bits of the virtual address.
  uint64_t page_base = (uint64_t)ptr & ~0xFFF;
  struct slab *slab = (struct slab *)page_base;

  // check slab maigc to make sure this is a slab
  if (slab->magic != SLAB_MAGIC) {
    print("[!] kfree: Invalid pointer or magic mismatch. Memory corruption "
          "avoided.\n");
    return;
  }

  struct slab_cache *cache = slab->cache;

  if (slab->free_chunks == 0) {
    list_remove(&cache->slabs_full, slab);
    list_add(&cache->slabs_partial, slab);
  }

  struct slab_chunk *chunk = (struct slab_chunk *)ptr;
  chunk->next = slab->free_list;
  slab->free_list = chunk;
  slab->free_chunks++;

  if (slab->free_chunks == slab->total_chunks) {
    list_remove(&cache->slabs_partial, slab);

    void *phys = (void *)(page_base - hhdm_offset);
    pmm_free_page(phys);
  }
}
