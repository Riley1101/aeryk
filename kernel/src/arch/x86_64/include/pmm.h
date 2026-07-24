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
 * This function sets up the allocation bitmap and per-page refcount array
 * for tracking physical memory pages, marks available pages as free, and
 * permanently reserves physical page 0 (see pmm.c for why). It retrieves
 * the memory map and HHDM offset from the Limine bootloader.
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
 * @brief Drops a reference to a physical memory page.
 * Every allocated page starts with a reference count of 1. This decrements
 * it and only marks the page free in the bitmap once the count reaches
 * zero, so pages shared between processes (e.g. copy-on-write frames after
 * fork()) survive until every owner has dropped its reference.
 *
 * @param page The physical address of the page to drop a reference to.
 */
void pmm_free_page(void *page);

/**
 * @brief Takes an extra reference on an already-allocated physical page.
 * Used when a frame becomes shared between processes (copy-on-write after
 * fork()) so pmm_free_page() won't reclaim it while another owner still
 * maps it.
 *
 * @param page The physical address of the page to add a reference to.
 */
void pmm_page_ref_inc(void *page);

/**
 * @brief Returns the current reference count of a physical page.
 * Used by the copy-on-write fault handler to tell whether it's the sole
 * remaining owner of a frame (count == 1, safe to reclaim write access in
 * place) or must copy (count > 1).
 *
 * @param page The physical address of the page to query.
 */
uint16_t pmm_page_refcount(void *page);

#endif // !PMM_H
