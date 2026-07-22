#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#define PTE_PRESENT (1ull << 0)
#define PTE_WRITABLE (1ull << 1)
#define PTE_USER (1ull << 2)
#define PTE_NX (1ull << 63)

/**
 * @brief Initializes the virtual memory manager by retrieving the kernel's PML4
 * from the CR3 register and storing it in a global variable for later use.
 */
void init_vmm(void);

/**
 * @brief Maps a virtual address to a physical address in the specified PML4 with the given flags.
 * @param pml4 Pointer to the PML4 table.
 * @param virtual_addr The virtual address to map.
 * @param physical_addr The physical address to map to.
 * @param flags The flags to set for the page table entry (e.g., PTE_PRESENT, PTE_WRITABLE, PTE_USER).
 */
void vmm_map_page(uint64_t *pml4, uint64_t virtual_addr, uint64_t physical_addr,
                  uint64_t flags);

/**
 * @brief Marks every level of the page-table walk for virtual_addr as
 * user-accessible.
 * @param pml4 Pointer to the PML4 table.
 * @param virtual_addr The virtual address to mark as user-accessible.
 */
void vmm_set_page_user(uint64_t *pml4, uint64_t virtual_addr);

/**
 * @brief Retrieves the kernel's PML4 table, which is stored in a global variable after initialization.
 * @return Pointer to the kernel's PML4 table.
 */
uint64_t *vmm_get_kernel_pml4(void);

/**
 * @brief Allocates a fresh PML4 for a user process. The top half (kernel-space
 * entries 256-511) is copied from the kernel PML4 so kernel code/data,
 * HHDM, and the framebuffer stay mapped regardless of which process's
 * pagetable is active. The bottom half is left empty for the loader to
 * populate with the process's segments and stack. Returns the PML4's
 * HHDM virtual address, or NULL on allocation failure.
 */
uint64_t *vmm_new_user_pagetable(void);

/**
 * @brief Frees every physical frame mapped in the user half (entries 0-255) of
 * pml4, including the intermediate PDPT/PD/PT structure pages themselves,
 * then frees the pml4 page. Safe to call on a partially-populated pagetable
 * (e.g. after a failed elf_load), since unmapped entries are simply skipped.
 * Does not touch the shared kernel half (entries 256-511).
 * @param pml4 Pointer to the PML4 table to destroy.
 */
void vmm_destroy_user_pagetable(uint64_t *pml4);

#endif /* ifndef VMM_H */

