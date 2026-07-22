#ifndef PMM_H
#define PMM_H

#include <stdint.h>

/**
 * @brief Defines the size of a memory page in bytes.
 * This value is used for memory management operations, 
 * such as allocating and freeing pages.
 */
#define PAGE_SIZE 4096

/**
 * @brief Offset for the Higher Half Direct Mapping (HHDM).
 * This value is used to translate physical addresses to virtual addresses in the higher half of the kernel.
 */
extern uint64_t hhdm_offset;

/**
 * @brief Initializes the Physical Memory Manager (PMM).
 * This function sets up the bitmap for tracking physical memory pages and marks available pages as free.
 * It retrieves the memory map and HHDM offset from the Limine bootloader.
 */
void init_pmm(void);

/**
 * @brief Allocates a single physical memory page.
 * This function searches the bitmap for a free page, marks it as allocated, and returns its physical address.
 * If no free pages are available, it returns NULL.
 *
 * @return A pointer to the allocated page, or NULL if no pages are available.
 */
void *pmm_alloc_page(void);

/**
 * @brief Frees a single physical memory page.
 * This function marks the specified page as free in the bitmap.
 * If the freed page has a lower index than the current bitmap index, it updates the index.
 *
 * @param page The physical address of the page to free.
 */
void pmm_free_page(void *page);

#endif // !PMM_H
