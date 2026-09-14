#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#define PTE_PRESENT (1ull << 0)
#define PTE_WRITABLE (1ull << 1)
#define PTE_USER (1ull << 2)
#define PTE_NX (1ull << 63)
// PWT (page write-through), bit 3. Combined with PCD=0 (bit 4, left clear)
// and the PAT bit (bit 7, also left clear -- out of reach for 4KB pages
// without it), this selects PAT slot 1 -- see vmm_init_pat(). Used only on
// the framebuffer mapping (SYS_fbmap) to get write-combining instead of
// the write-back every other page implicitly gets via PAT slot 0.
#define PTE_PWT (1ull << 3)
// Bits 9-11 are ignored by the MMU for present entries, so they're free for
// the OS to repurpose. Used to mark a page shared copy-on-write after
// fork(): PTE_WRITABLE is cleared and this bit set on both parent and
// child's mapping, so a write faults and the page-fault handler can decide
// whether to copy or just reclaim sole ownership.
#define PTE_COW (1ull << 9)
// Marks a page mapped via mmap(MAP_SHARED): unlike an ordinary writable
// page, vmm_clone_user_pagetable() must NOT turn this into a COW mapping
// across fork(), since a MAP_SHARED region is required to stay one shared
// physical frame -- visible to (and writable by) every process mapping it,
// parent and child alike -- forever, not just until the first write.
#define PTE_SHARED (1ull << 10)
// Marks a page whose physical frame is NOT owned by the PMM (e.g. the
// framebuffer's LFB, mapped by fb_map() straight from the physical address
// Limine handed the kernel, not from pmm_alloc_page()). The PMM's bitmap
// and refcount arrays are sized off the highest USABLE memmap entry, and
// an MMIO/LFB physical address commonly sits above that -- calling
// pmm_free_page()/pmm_page_ref_inc() on it would index those arrays out of
// bounds. Every path that would otherwise touch the PMM for a leaf frame
// (vmm_unmap_page, vmm_destroy_user_pagetable, vmm_clone_user_pagetable)
// must check this bit first and skip the PMM call -- the frame is only
// ever unmapped, never freed or refcounted, and is shared as-is (like
// PTE_SHARED) across fork rather than becoming a COW mapping.
#define PTE_NOPMM (1ull << 11)

/**
 * @brief Initializes the virtual memory manager by retrieving the kernel's PML4
 * from the CR3 register and storing it in a global variable for later use.
 */
void init_vmm(void);

/**
 * @brief Reprograms PAT slot 1 (selected by a PTE with PWT=1, PCD=0) from
 * its power-on default of write-through to write-combining (WC), leaving
 * slot 0 (PWT=0, PCD=0 -- what every other present PTE in this kernel
 * implicitly uses, having never set PWT/PCD/PAT) at write-back. WC lets
 * the CPU buffer and burst writes to the framebuffer instead of either
 * serializing every store (uncached) or paying cache-coherency overhead
 * for a region the CPU never reads back (write-back) -- the standard
 * choice for a linear framebuffer. Must run before anything maps a page
 * with PTE_PWT set (SYS_fbmap), so this is called once from init_vmm().
 */
void vmm_init_pat(void);

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
 * write them. A page marked PTE_SHARED (mmap MAP_SHARED) is the third case:
 * it's shared with its writable bit left intact and no COW marker added, so
 * parent and child keep writing straight through to the same physical frame
 * instead of forking off private copies. The kernel half (entries 256-511) is shared with the running
 * kernel, same as vmm_new_user_pagetable(). Flushes the TLB before
 * returning, since `src_pml4` is normally the currently-active address
 * space and its entries were just downgraded to read-only in place. Used
 * to implement fork().
 * @param src_pml4 Pointer to the source PML4 table to clone.
 * @return The new PML4's HHDM virtual address, or NULL on allocation failure.
 */
uint64_t *vmm_clone_user_pagetable(uint64_t *src_pml4);

/**
 * @brief Unmaps a single page at `virtual_addr` in `pml4`, if present:
 * drops the physical frame's refcount (freeing it once no other mapping
 * holds it, same as any other pmm_free_page() call) and clears the leaf
 * page-table entry. A no-op at whichever level the walk finds the page
 * already unmapped -- safe to call on an address that was never mapped, or
 * only partially mapped. Does not free now-empty intermediate PDPT/PD/PT
 * pages (unlike vmm_destroy_user_pagetable(), this is for unmapping one
 * region out of a still-live address space, e.g. SYS_munmap). Flushes the
 * TLB for `virtual_addr`.
 * @param pml4 Pointer to the PML4 table.
 * @param virtual_addr The virtual address of the page to unmap.
 */
void vmm_unmap_page(uint64_t *pml4, uint64_t virtual_addr);

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

