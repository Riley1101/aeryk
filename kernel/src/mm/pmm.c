#include "pmm.h"
#include <drivers/display/tty.h>
#include <limine.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

uint64_t hhdm_offset = 0;

static uint8_t *bitmap = NULL;
static size_t bitmap_size = 0;
static size_t highest_address = 0;
static size_t bitmap_index = 0;

static void bitmap_set(size_t bit) { bitmap[bit / 8] |= (1 << (bit % 8)); }
static void bitmap_clear(size_t bit) { bitmap[bit / 8] &= ~(1 << (bit % 8)); }
static bool bitmap_test(size_t bit) {
  return (bitmap[bit / 8] & (1 << (bit % 8))) != 0;
}

extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_hhdm_request hhdm_request;

void initPMM(void) {
  if (!memmap_request.response || !hhdm_request.response) {
    print(global_renderer, "PANIC: Missing limine memory map or HHDM!\n");
    for (;;)
      asm("hlt");
  }

  hhdm_offset = hhdm_request.response->offset;
  struct limine_memmap_response *memmap = memmap_request.response;

  // Find the highest physical address
  for (size_t i = 0; i < memmap->entry_count; i++) {
    struct limine_memmap_entry *entry = memmap->entries[i];
    if (entry->type == LIMINE_MEMMAP_USABLE) {
      size_t top = entry->base + entry->length;
      if (top > highest_address) {
        highest_address = top;
      }
    }
  }

  bitmap_size = (highest_address / PAGE_SIZE) / 8;
  if (bitmap_size % 8 != 0) {
    bitmap_size++;
  }

  // Find a place for the bitmap
  for (size_t i = 0; i < memmap->entry_count; i++) {
    struct limine_memmap_entry *entry = memmap->entries[i];
    if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size) {
      bitmap = (uint8_t *)(entry->base + hhdm_offset);
      memset(bitmap, 0xFF, bitmap_size);
      break;
    }
  }

  // Clear available memory
  for (size_t i = 0; i < memmap->entry_count; i++) {
    struct limine_memmap_entry *entry = memmap->entries[i];
    if (entry->type == LIMINE_MEMMAP_USABLE) {
      for (size_t j = 0; j < entry->length; j += PAGE_SIZE) {
        bitmap_clear((entry->base + j) / PAGE_SIZE);
      }
    }
  }

  // Protect the memory used by the bitmap itself
  size_t bitmap_phys = (size_t)bitmap - hhdm_offset;
  for (size_t i = 0; i < bitmap_size; i += PAGE_SIZE) {
    bitmap_set((bitmap_phys + i) / PAGE_SIZE);
  }
}

void *pmm_alloc_page(void) {
  for (size_t i = bitmap_index; i < highest_address / PAGE_SIZE; i++) {
    if (!bitmap_test(i)) {
      bitmap_set(i);
      bitmap_index = i;
      return (void *)(i * PAGE_SIZE);
    }
  }
  for (size_t i = 0; i < bitmap_index; i++) {
    if (!bitmap_test(i)) {
      bitmap_set(i);
      bitmap_index = i;
      return (void *)(i * PAGE_SIZE);
    }
  }
  return NULL;
}

void pmm_free_page(void *page) {
  size_t bit = (size_t)page / PAGE_SIZE;
  if (bitmap_test(bit)) {
    bitmap_clear(bit);
    if (bit < bitmap_index) {
      bitmap_index = bit;
    }
  }
}
