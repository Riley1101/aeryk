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
 * @brief Initializes the Physical Memory Manager (PMM) by setting up the bitmap and marking available memory pages.
 * This function retrieves the memory map and HHDM offset from the Limine bootloader, and uses this information to manage physical memory allocation.
 * It also protects the memory used by the bitmap itself to prevent it from being overwritten.
 * If the memory map or HHDM offset is missing , the function will print an error message and halt the system.
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
/**
 * @brief Frees a single physical memory page.
 * This function marks the specified page as free in the bitmap.
 * If the freed page has a lower index than the current bitmap index, it updates the index.
 *
 * @param page The physical address of the page to free.
 */
void pmm_free_page(void *page) {
  size_t bit = (size_t)page / PAGE_SIZE;
  if (bitmap_test(bit)) {
    bitmap_clear(bit);
    if (bit < bitmap_index) {
      bitmap_index = bit;
    }
  }
}
