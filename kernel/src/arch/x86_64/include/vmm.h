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

#endif /* ifndef VMM_H */

