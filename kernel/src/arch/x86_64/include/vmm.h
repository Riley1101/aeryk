#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#define PTE_PRESENT (1ull << 0)
#define PTE_WRITABLE (1ull << 1)
#define PTE_USER (1ull << 2)
#define PTE_NX (1ull << 63)
// Bits 9-11 are ignored by the MMU for present entries, so they're free for
// the OS to repurpose. Used to mark a page shared copy-on-write after
// fork(): PTE_WRITABLE is cleared and this bit set on both parent and
// child's mapping, so a write faults and the page-fault handler can decide
// whether to copy or just reclaim sole ownership.
#define PTE_COW (1ull << 9)

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

/**
 * @brief Copy-on-write clones the user half (entries 0-255) of `src_pml4`
 * into a freshly allocated pagetable. Every present writable leaf page is
 * shared (not copied) between parent and child: both mappings have
 * PTE_WRITABLE cleared and PTE_COW set, and the physical frame's refcount
 * is bumped, so the first write by either side takes a page fault that
 * gives it a private copy (see the #PF handler in idt.c). Already
 * read-only pages are shared without the COW marker since neither side can
 * write them. The kernel half (entries 256-511) is shared with the running
 * kernel, same as vmm_new_user_pagetable(). Flushes the TLB before
 * returning, since `src_pml4` is normally the currently-active address
 * space and its entries were just downgraded to read-only in place. Used
 * to implement fork().
 * @param src_pml4 Pointer to the source PML4 table to clone.
 * @return The new PML4's HHDM virtual address, or NULL on allocation failure.
 */
uint64_t *vmm_clone_user_pagetable(uint64_t *src_pml4);

/**
 * @brief Handles a copy-on-write page fault for `fault_vaddr` in `pml4`.
 * If the faulting page is marked PTE_COW, gives the faulting process its
 * own writable copy (or, if it turns out to be the sole remaining owner,
 * simply reclaims write access to the shared frame) and returns 1.
 * Returns 0 if the fault isn't a recognized COW fault (unmapped page, or a
 * write to a genuinely read-only page), so the caller can fall back to
 * treating it as fatal.
 * @param pml4 Pointer to the faulting process's PML4 table.
 * @param fault_vaddr The faulting virtual address (from CR2).
 * @return 1 if the fault was handled, 0 otherwise.
 */
int vmm_handle_cow_fault(uint64_t *pml4, uint64_t fault_vaddr);

#endif /* ifndef VMM_H */

