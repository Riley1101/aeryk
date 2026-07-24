#include <pmm.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <vmm.h>

static uint64_t *kernel_pml4 = NULL;

/**
 * @brief Retrieves the next level of the page table for a given entry index.
 * If the entry is present, it returns the virtual address of the next level.
 * If the entry is not present, it allocates a new page for the next level, initializes it to zero, and updates the current level's entry to point to the new page with the appropriate flags.
 * @param current_level Pointer to the current level of the page table (PML4, PDPT, or PD).
 * @param entry_index The index of the entry in the current level
 * @return Pointer to the next level of the page table, or NULL if allocation fails.
 */
static void *get_next_level(uint64_t *current_level, size_t entry_index) {

  if ((current_level[entry_index] & PTE_PRESENT) != 0) {
    uint64_t phys = current_level[entry_index] & 0x000FFFFFFFFFF000;
    return (void *)(phys + hhdm_offset);
  }

  void *next_level_phys = pmm_alloc_page();
  if (!next_level_phys) {
    return NULL;
  }

  void *next_level_virt = (void *)((uint64_t)next_level_phys + hhdm_offset);
  memset(next_level_virt, 0, PAGE_SIZE);

  current_level[entry_index] =
      (uint64_t)next_level_phys | PTE_PRESENT | PTE_WRITABLE | PTE_USER;

  return next_level_virt;
}

/**
 * @brief Maps a virtual address to a physical address in the specified PML4 with the given flags.
 * @param pml4 Pointer to the PML4 table.
 * @param virtual_addr The virtual address to map.
 * @param physical_addr The physical address to map to.
 * @param flags The flags to set for the page table entry (e.g., PTE
 */
void vmm_map_page(uint64_t *pml4, uint64_t virtual_addr, uint64_t physical_addr,
                  uint64_t flags) {

  size_t pml4_entry = (virtual_addr >> 39) & 0x1FF;
  size_t pdpt_entry = (virtual_addr >> 30) & 0x1FF;
  size_t pd_entry = (virtual_addr >> 21) & 0x1FF;
  size_t pt_entry = (virtual_addr >> 12) & 0x1FF;

  uint64_t *pdpt = get_next_level(pml4, pml4_entry);

  if (!pdpt)
    return;

  uint64_t *pd = get_next_level(pdpt, pdpt_entry);
  if (!pd)
    return;

  uint64_t *pt = get_next_level(pd, pd_entry);
  if (!pt)
    return;
  pt[pt_entry] = physical_addr | flags;
}

/**
 * @brief Marks every level of the page-table walk for virtual_addr as
 * user-accessible. This do notes not modify the existing physical addresses or other flags. It is used to allow ring-3 
 * code/stack to reside on pages that were originally mapped as supervisor-only by Limine.
 *
 * @param pml4 Pointer to the PML4 table.
 * @param virtual_addr The virtual address to mark as user-accessible.
 */
void vmm_set_page_user(uint64_t *pml4, uint64_t virtual_addr) {
  size_t pml4_entry = (virtual_addr >> 39) & 0x1FF;
  size_t pdpt_entry = (virtual_addr >> 30) & 0x1FF;
  size_t pd_entry = (virtual_addr >> 21) & 0x1FF;
  size_t pt_entry = (virtual_addr >> 12) & 0x1FF;

  if (!(pml4[pml4_entry] & PTE_PRESENT))
    return;
  pml4[pml4_entry] |= PTE_USER;
  uint64_t *pdpt =
      (uint64_t *)((pml4[pml4_entry] & 0x000FFFFFFFFFF000) + hhdm_offset);

  if (!(pdpt[pdpt_entry] & PTE_PRESENT))
    return;
  pdpt[pdpt_entry] |= PTE_USER;
  uint64_t *pd =
      (uint64_t *)((pdpt[pdpt_entry] & 0x000FFFFFFFFFF000) + hhdm_offset);

  if (!(pd[pd_entry] & PTE_PRESENT))
    return;
  pd[pd_entry] |= PTE_USER;
  uint64_t *pt =
      (uint64_t *)((pd[pd_entry] & 0x000FFFFFFFFFF000) + hhdm_offset);

  if (!(pt[pt_entry] & PTE_PRESENT))
    return;
  pt[pt_entry] |= PTE_USER;
}

/**
 * @brief Retrieves the kernel's PML4 table, which is stored in a global variable after initialization.
 * @return Pointer to the kernel's PML4 table.
 */
uint64_t *vmm_get_kernel_pml4(void) { return kernel_pml4; }

/**
 * @brief Allocates a fresh PML4 for a user process. The top half (kernel-space
 * entries 256-511) is copied from the kernel PML4 so kernel code/data,
 * HHDM, and the framebuffer stay mapped regardless of which process's
 * pagetable is active. The bottom half is left empty for the loader to
 * populate with the process's segments and stack. Returns the PML4's
 * HHDM virtual address, or NULL on allocation failure.
 */
uint64_t *vmm_new_user_pagetable(void) {
  void *phys = pmm_alloc_page();
  if (!phys) {
    return NULL;
  }

  uint64_t *pml4 = (uint64_t *)((uint64_t)phys + hhdm_offset);
  memset(pml4, 0, PAGE_SIZE);

  // Share the kernel's higher-half mappings (entries 256-511) so kernel
  // code/data and the HHDM remain accessible after a cr3 switch.
  for (size_t i = 256; i < 512; i++) {
    pml4[i] = kernel_pml4[i];
  }

  return pml4;
}

/**
 * @brief Frees every physical frame mapped in the user half (entries 0-255) of
 * pml4, including the intermediate PDPT/PD/PT structure pages themselves,
 * then frees the pml4 page. Safe to call on a partially-populated pagetable
 * (e.g. after a failed elf_load), since unmapped entries are simply skipped.
 * Does not touch the shared kernel half (entries 256-511).
 */
void vmm_destroy_user_pagetable(uint64_t *pml4) {
  if (!pml4) {
    return;
  }

  for (size_t pml4_i = 0; pml4_i < 256; pml4_i++) {
    if (!(pml4[pml4_i] & PTE_PRESENT)) {
      continue;
    }
    uint64_t *pdpt =
        (uint64_t *)((pml4[pml4_i] & 0x000FFFFFFFFFF000) + hhdm_offset);

    for (size_t pdpt_i = 0; pdpt_i < 512; pdpt_i++) {
      if (!(pdpt[pdpt_i] & PTE_PRESENT)) {
        continue;
      }
      uint64_t *pd =
          (uint64_t *)((pdpt[pdpt_i] & 0x000FFFFFFFFFF000) + hhdm_offset);

      for (size_t pd_i = 0; pd_i < 512; pd_i++) {
        if (!(pd[pd_i] & PTE_PRESENT)) {
          continue;
        }
        uint64_t *pt =
            (uint64_t *)((pd[pd_i] & 0x000FFFFFFFFFF000) + hhdm_offset);

        for (size_t pt_i = 0; pt_i < 512; pt_i++) {
          if (pt[pt_i] & PTE_PRESENT) {
            pmm_free_page(
                (void *)(pt[pt_i] & 0x000FFFFFFFFFF000));
          }
        }
        pmm_free_page((void *)(pd[pd_i] & 0x000FFFFFFFFFF000));
      }
      pmm_free_page((void *)(pdpt[pdpt_i] & 0x000FFFFFFFFFF000));
    }
    pmm_free_page((void *)(pml4[pml4_i] & 0x000FFFFFFFFFF000));
  }

  pmm_free_page((void *)((uint64_t)pml4 - hhdm_offset));
}

/**
 * @brief Copy-on-write clones the user half (entries 0-255) of `src_pml4`
 * into a freshly allocated pagetable. See vmm.h for the full behavior.
 * Returns NULL (having freed any partial work) if any allocation along the
 * way fails.
 */
uint64_t *vmm_clone_user_pagetable(uint64_t *src_pml4) {
  uint64_t *dst_pml4 = vmm_new_user_pagetable();
  if (!dst_pml4) {
    return NULL;
  }

  for (size_t pml4_i = 0; pml4_i < 256; pml4_i++) {
    if (!(src_pml4[pml4_i] & PTE_PRESENT)) {
      continue;
    }
    uint64_t *src_pdpt =
        (uint64_t *)((src_pml4[pml4_i] & 0x000FFFFFFFFFF000) + hhdm_offset);

    for (size_t pdpt_i = 0; pdpt_i < 512; pdpt_i++) {
      if (!(src_pdpt[pdpt_i] & PTE_PRESENT)) {
        continue;
      }
      uint64_t *src_pd =
          (uint64_t *)((src_pdpt[pdpt_i] & 0x000FFFFFFFFFF000) + hhdm_offset);

      for (size_t pd_i = 0; pd_i < 512; pd_i++) {
        if (!(src_pd[pd_i] & PTE_PRESENT)) {
          continue;
        }
        uint64_t *src_pt =
            (uint64_t *)((src_pd[pd_i] & 0x000FFFFFFFFFF000) + hhdm_offset);

        for (size_t pt_i = 0; pt_i < 512; pt_i++) {
          if (!(src_pt[pt_i] & PTE_PRESENT)) {
            continue;
          }

          uint64_t flags = src_pt[pt_i] & 0xFFF0000000000FFF;
          uint64_t phys = src_pt[pt_i] & 0x000FFFFFFFFFF000;

          if (flags & PTE_WRITABLE) {
            flags = (flags & ~PTE_WRITABLE) | PTE_COW;
            src_pt[pt_i] = phys | flags;
          }

          pmm_page_ref_inc((void *)phys);

          uint64_t virtual_addr = ((uint64_t)pml4_i << 39) |
                                   ((uint64_t)pdpt_i << 30) |
                                   ((uint64_t)pd_i << 21) |
                                   ((uint64_t)pt_i << 12);
          vmm_map_page(dst_pml4, virtual_addr, phys, flags);
        }
      }
    }
  }

  // src_pml4 is normally the currently-active address space and entries
  // in it were just downgraded to read-only in place; flush stale
  // writable TLB entries by reloading CR3.
  uint64_t cr3;
  asm volatile("mov %%cr3, %0" : "=r"(cr3));
  asm volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");

  return dst_pml4;
}

/**
 * @brief Handles a copy-on-write page fault. See vmm.h for the full
 * behavior.
 */
int vmm_handle_cow_fault(uint64_t *pml4, uint64_t fault_vaddr) {
  size_t pml4_i = (fault_vaddr >> 39) & 0x1FF;
  size_t pdpt_i = (fault_vaddr >> 30) & 0x1FF;
  size_t pd_i = (fault_vaddr >> 21) & 0x1FF;
  size_t pt_i = (fault_vaddr >> 12) & 0x1FF;

  if (!(pml4[pml4_i] & PTE_PRESENT)) {
    return 0;
  }
  uint64_t *pdpt =
      (uint64_t *)((pml4[pml4_i] & 0x000FFFFFFFFFF000) + hhdm_offset);
  if (!(pdpt[pdpt_i] & PTE_PRESENT)) {
    return 0;
  }
  uint64_t *pd =
      (uint64_t *)((pdpt[pdpt_i] & 0x000FFFFFFFFFF000) + hhdm_offset);
  if (!(pd[pd_i] & PTE_PRESENT)) {
    return 0;
  }
  uint64_t *pt = (uint64_t *)((pd[pd_i] & 0x000FFFFFFFFFF000) + hhdm_offset);
  if (!(pt[pt_i] & PTE_PRESENT) || !(pt[pt_i] & PTE_COW)) {
    return 0;
  }

  uint64_t phys = pt[pt_i] & 0x000FFFFFFFFFF000;
  uint64_t flags = (pt[pt_i] & 0xFFF0000000000FFF & ~PTE_COW) | PTE_WRITABLE;

  if (pmm_page_refcount((void *)phys) == 1) {
    // We're the sole remaining owner; no one else can be writing to this
    // frame, so just reclaim write access in place.
    pt[pt_i] = phys | flags;
  } else {
    void *new_phys = pmm_alloc_page();
    if (!new_phys) {
      return 0;
    }
    memcpy((void *)((uint64_t)new_phys + hhdm_offset),
           (void *)(phys + hhdm_offset), PAGE_SIZE);
    pt[pt_i] = (uint64_t)new_phys | flags;
    pmm_free_page((void *)phys);
  }

  asm volatile("invlpg (%0)" : : "r"(fault_vaddr & ~(uint64_t)0xFFF) : "memory");
  return 1;
}

/**
 * @brief Initializes the virtual memory manager by retrieving the kernel's PML4
 * from the CR3 register and storing it in a global variable for later use.
 */
void init_vmm(void) {
  uint64_t cr3;
  asm volatile("mov %%cr3, %0" : "=r"(cr3));

  // The mask 0x000FFFFFFFFFF000 clears those flag bits and ensures the address
  // is page-aligned
  kernel_pml4 = (uint64_t *)((cr3 & 0x000FFFFFFFFFF000) + hhdm_offset);
}
