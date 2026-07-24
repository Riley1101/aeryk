#include <limine.h>
#include <pmm.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <tty.h>

/**
 * @file pmm.c
 * @brief Physical Memory Manager (PMM) implementation for the Aeryk kernel.
 */

/**
 * @brief Offset for the Higher Half Direct Mapping (HHDM).
 * This value is used to translate physical addresses to virtual addresses in the higher half of the kernel
 */
uint64_t hhdm_offset = 0;

/**
 * @brief Bitmap representing the allocation status of physical memory pages.
 * Each bit corresponds to a page, where 0 indicates free and 1 indicates allocated.
 */
static uint8_t *bitmap = NULL;

/**
 * @brief Size of the bitmap in bytes.
 */
static size_t bitmap_size = 0;

/**
 * @brief Highest physical address available in the system.
 */
static size_t highest_address = 0;

/**
 * @brief Index in the bitmap to start searching for free pages.
 * This helps optimize the allocation process by avoiding scanning from the beginning every time.
 */
static size_t bitmap_index = 0;

/**
 * @brief Per-page reference counts, indexed by page frame number.
 * A freshly allocated page starts at 1. fork()'s copy-on-write path bumps
 * this when a frame becomes shared between a parent and child's
 * pagetables; pmm_free_page() only returns the frame to the bitmap once
 * the count drops to 0.
 */
static uint16_t *refcounts = NULL;

/**
 * @brief Sets a bit in the bitmap to indicate that the corresponding page is allocated.
 *
 * @param bit The index of the bit to set in the bitmap.
 */
static void bitmap_set(size_t bit) { bitmap[bit / 8] |= (1 << (bit % 8)); }

/**
 * @brief Clears a bit in the bitmap to indicate that the corresponding page is free.
 *
 * @param bit The index of the bit to clear in the bitmap.
 */
static void bitmap_clear(size_t bit) { bitmap[bit / 8] &= ~(1 << (bit % 8)); }

/**
 * @brief Tests whether a bit in the bitmap is set, indicating that the corresponding page is allocated.
 *
 * @param bit The index of the bit to test in the bitmap.
 * @return true if the page is allocated, false otherwise.
 */
static bool bitmap_test(size_t bit) {
  return (bitmap[bit / 8] & (1 << (bit % 8))) != 0;
}

/**
 * @brief Limine request structure for retrieving the memory map from the bootloader.
 * This structure is used to request the memory map information during kernel initialization.
 */
extern volatile struct limine_memmap_request memmap_request;

/**
 * @brief Limine request structure for retrieving the Higher Half Direct Mapping (HHDM) offset from the bootloader.
 * This structure is used to request the HHDM offset information during kernel initialization.
 */
extern volatile struct limine_hhdm_request hhdm_request;

/**
 * @brief Initializes the Physical Memory Manager (PMM) by setting up the
 * allocation bitmap and per-page refcount array, and marking available
 * memory pages free.
 * This function retrieves the memory map and HHDM offset from the Limine
 * bootloader, and uses this information to manage physical memory
 * allocation. The bitmap and refcount array are placed together in the
 * first usable region large enough to hold both, and those pages are
 * protected from being handed out. Physical page 0 is also permanently
 * reserved, since its address (0) is indistinguishable from a NULL
 * allocation-failure return to any caller that checks with `if (!ptr)`.
 * If the memory map or HHDM offset is missing, the function will print an
 * error message and halt the system.
 */
void init_pmm(void) {
  if (!memmap_request.response || !hhdm_request.response) {
    print("PANIC: Missing limine memory map or HHDM!\n");
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

  size_t refcounts_size = (highest_address / PAGE_SIZE) * sizeof(uint16_t);

  // Find a place for the bitmap and the refcount array together, so they
  // never overlap: the refcount array is placed right after the bitmap in
  // the same usable region.
  for (size_t i = 0; i < memmap->entry_count; i++) {
    struct limine_memmap_entry *entry = memmap->entries[i];
    if (entry->type == LIMINE_MEMMAP_USABLE &&
        entry->length >= bitmap_size + refcounts_size) {
      bitmap = (uint8_t *)(entry->base + hhdm_offset);
      memset(bitmap, 0xFF, bitmap_size);
      refcounts = (uint16_t *)((uint8_t *)bitmap + bitmap_size);
      memset(refcounts, 0, refcounts_size);
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

  // Protect the memory used by the bitmap and refcount array themselves
  size_t bitmap_phys = (size_t)bitmap - hhdm_offset;
  for (size_t i = 0; i < bitmap_size + refcounts_size; i += PAGE_SIZE) {
    bitmap_set((bitmap_phys + i) / PAGE_SIZE);
  }

  // Never hand out physical page 0: its address is 0, which is
  // indistinguishable from a NULL allocation-failure return to any caller
  // that checks the result with `if (!ptr)` (e.g. vmm.c's get_next_level).
  // Reserving it here is the standard fix rather than auditing every call
  // site for that ambiguity.
  bitmap_set(0);
  refcounts[0] = 1;
}

/**
 * @brief Allocates a single physical memory page.
 * This function searches the bitmap for a free page, marks it as allocated, and returns its
 * physical address. If no free pages are available, it returns NULL.
 */
void *pmm_alloc_page(void) {
  for (size_t i = bitmap_index; i < highest_address / PAGE_SIZE; i++) {
    if (!bitmap_test(i)) {
      bitmap_set(i);
      bitmap_index = i;
      refcounts[i] = 1;
      return (void *)(i * PAGE_SIZE);
    }
  }
  for (size_t i = 0; i < bitmap_index; i++) {
    if (!bitmap_test(i)) {
      bitmap_set(i);
      bitmap_index = i;
      refcounts[i] = 1;
      return (void *)(i * PAGE_SIZE);
    }
  }
  return NULL;
}

/**
 * @brief Drops a reference to a physical memory page.
 * Decrements the page's reference count and only marks it free in the
 * bitmap once the count reaches 0, so pages shared via copy-on-write
 * survive until every owner has released it.
 */
void pmm_free_page(void *page) {
  size_t bit = (size_t)page / PAGE_SIZE;
  if (!bitmap_test(bit)) {
    return;
  }
  if (refcounts[bit] > 0) {
    refcounts[bit]--;
  }
  if (refcounts[bit] == 0) {
    bitmap_clear(bit);
    if (bit < bitmap_index) {
      bitmap_index = bit;
    }
  }
}

/**
 * @brief Takes an extra reference on an already-allocated physical page.
 */
void pmm_page_ref_inc(void *page) {
  size_t bit = (size_t)page / PAGE_SIZE;
  refcounts[bit]++;
}

/**
 * @brief Returns the current reference count of a physical page.
 */
uint16_t pmm_page_refcount(void *page) {
  size_t bit = (size_t)page / PAGE_SIZE;
  return refcounts[bit];
}
