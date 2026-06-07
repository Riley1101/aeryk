#include "tty.h"
#include <limine.h>
#include <stdint.h>

/* pmm.c computes bitmap = entry->base + hhdm_offset and writes through that
 * pointer directly — there's no real HHDM on the host, so we fake one: a
 * single usable region starting at physical address 0, backed by an actual
 * host buffer, with hhdm_offset pointing straight at that buffer. */
#define FAKE_MEM_PAGES 1024
#define FAKE_MEM_SIZE (FAKE_MEM_PAGES * 4096)
static uint8_t fake_phys_mem[FAKE_MEM_SIZE] __attribute__((aligned(4096)));

static struct limine_memmap_entry usable_entry = {
    .base = 0,
    .length = FAKE_MEM_SIZE,
    .type = LIMINE_MEMMAP_USABLE,
};
static struct limine_memmap_entry *entries[] = {&usable_entry};
static struct limine_memmap_response memmap_response = {
    .revision = 0,
    .entry_count = 1,
    .entries = entries,
};
static struct limine_hhdm_response hhdm_response = {
    .revision = 0,
    .offset = 0, /* patched in by init_pmm_test_env() */
};

volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0,
    .response = &memmap_response,
};
volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
    .response = &hhdm_response,
};

void init_pmm_test_env(void) { hhdm_response.offset = (uint64_t)fake_phys_mem; }
uint64_t fake_hhdm_offset(void) { return (uint64_t)fake_phys_mem; }
uint64_t fake_phys_mem_size(void) { return FAKE_MEM_SIZE; }
int fake_phys_mem_page_count(void) { return FAKE_MEM_PAGES; }

void print(const char *str) { (void)str; }
