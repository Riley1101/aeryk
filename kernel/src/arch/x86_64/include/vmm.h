#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#define PTE_PRESENT (1ull << 0)
#define PTE_WRITABLE (1ull << 1)
#define PTE_USER (1ull << 2)
#define PTE_NX (1ull << 63)

void init_vmm(void);
void vmm_map_page(uint64_t *pml4, uint64_t virtual_addr, uint64_t physical_addr,
                  uint64_t flags);
void vmm_set_page_user(uint64_t *pml4, uint64_t virtual_addr);
uint64_t *vmm_get_kernel_pml4(void);

// Allocates a fresh PML4 for a user process. The top half (kernel-space
// entries 256-511) is copied from the kernel PML4 so kernel code/data,
// HHDM, and the framebuffer stay mapped regardless of which process's
// pagetable is active. The bottom half is left empty for the loader to
// populate with the process's segments and stack. Returns the PML4's
// HHDM virtual address, or NULL on allocation failure.
uint64_t *vmm_new_user_pagetable(void);

// Frees every physical frame mapped in the user half (entries 0-255) of
// pml4, including the intermediate PDPT/PD/PT structure pages themselves,
// then frees the pml4 page. Safe to call on a partially-populated pagetable
// (e.g. after a failed elf_load), since unmapped entries are simply skipped.
// Does not touch the shared kernel half (entries 256-511).
void vmm_destroy_user_pagetable(uint64_t *pml4);

#endif /* ifndef VMM_H */

