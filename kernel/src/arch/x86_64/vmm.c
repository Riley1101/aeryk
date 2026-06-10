#include "pmm.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <vmm.h>

static uint64_t *kernel_pml4 = NULL;

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

// Marks every level of the page-table walk for virtual_addr as
// user-accessible, without touching the existing physical
// addresses or other flags. Used to let ring-3 code/stack live on
// pages that were originally mapped supervisor-only by Limine.
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

uint64_t *vmm_get_kernel_pml4(void) { return kernel_pml4; }

void init_vmm(void) {
  uint64_t cr3;
  asm volatile("mov %%cr3, %0" : "=r"(cr3));

  // The mask 0x000FFFFFFFFFF000 clears those flag bits and ensures the address
  // is page-aligned
  kernel_pml4 = (uint64_t *)((cr3 & 0x000FFFFFFFFFF000) + hhdm_offset);
}
