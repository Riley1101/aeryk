#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define PAGE_SIZE 4096

extern uint64_t hhdm_offset;

void initPMM(void);
void *pmm_alloc_page(void);
void pmm_free_page(void *page);

#endif // !PMM_H
