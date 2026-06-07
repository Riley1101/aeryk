#include <pmm.h>
#include <stddef.h>
#include <stdint.h>

uint64_t hhdm_offset = 0;

#define POOL_PAGES 16
static uint8_t pool[POOL_PAGES][PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
static int pool_used = 0;
static int pool_freed = 0;

void reset_slab_pmm_pool(void)
{
    pool_used = 0;
    pool_freed = 0;
}
int slab_pmm_pages_used(void) { return pool_used; }
int slab_pmm_pages_freed(void) { return pool_freed; }

void *pmm_alloc_page(void)
{
    if (pool_used >= POOL_PAGES)
        return NULL;
    return pool[pool_used++];
}

void pmm_free_page(void *page)
{
    (void)page;
    pool_freed++;
}

void print(const char *str) { (void)str; }
