#include "unity.h"
#include <stdint.h>
#include <string.h>
#include <vmm.h>

#define ENTRY_MASK 0x000FFFFFFFFFF000ULL
#define FLAGS (PTE_PRESENT | PTE_WRITABLE | PTE_USER)

extern uint64_t hhdm_offset;
void reset_vmm_pmm_pool(void);
int vmm_pmm_pages_used(void);

static uint64_t pml4[512] __attribute__((aligned(4096)));

void setUp(void)
{
    memset(pml4, 0, sizeof(pml4));
    reset_vmm_pmm_pool();
}

void tearDown(void) {}

static uint64_t *table_at(uint64_t entry)
{
    return (uint64_t *)((entry & ENTRY_MASK) + hhdm_offset);
}

void test_map_page_builds_full_table_chain(void)
{
    uint64_t va = 0x0000123456789000ULL;
    uint64_t pa = 0x0000000040000000ULL;

    vmm_map_page(pml4, va, pa, FLAGS);

    size_t i4 = (va >> 39) & 0x1FF;
    size_t i3 = (va >> 30) & 0x1FF;
    size_t i2 = (va >> 21) & 0x1FF;
    size_t i1 = (va >> 12) & 0x1FF;

    TEST_ASSERT_TRUE(pml4[i4] & PTE_PRESENT);
    uint64_t *pdpt = table_at(pml4[i4]);
    TEST_ASSERT_TRUE(pdpt[i3] & PTE_PRESENT);
    uint64_t *pd = table_at(pdpt[i3]);
    TEST_ASSERT_TRUE(pd[i2] & PTE_PRESENT);
    uint64_t *pt = table_at(pd[i2]);

    TEST_ASSERT_EQUAL_HEX64(pa | FLAGS, pt[i1]);
}

void test_map_page_intermediate_levels_get_rwu_flags(void)
{
    uint64_t va = 0x0000000000001000ULL; /* all indices land in slot 0 except pt */
    vmm_map_page(pml4, va, 0x1000, FLAGS);

    uint64_t *pdpt = table_at(pml4[0]);
    uint64_t *pd = table_at(pdpt[0]);

    TEST_ASSERT_EQUAL_HEX64(PTE_PRESENT | PTE_WRITABLE | PTE_USER,
                            pml4[0] & 0xFFF);
    TEST_ASSERT_EQUAL_HEX64(PTE_PRESENT | PTE_WRITABLE | PTE_USER,
                            pdpt[0] & 0xFFF);
    TEST_ASSERT_EQUAL_HEX64(PTE_PRESENT | PTE_WRITABLE | PTE_USER,
                            pd[0] & 0xFFF);
}

void test_map_page_reuses_tables_for_addresses_in_same_region(void)
{
    /* Two pages 4 KiB apart share PML4/PDPT/PD/PT, differing only in PT slot */
    vmm_map_page(pml4, 0x0000000000000000ULL, 0x10000, FLAGS);
    int used_after_first = vmm_pmm_pages_used();

    vmm_map_page(pml4, 0x0000000000001000ULL, 0x20000, FLAGS);
    int used_after_second = vmm_pmm_pages_used();

    /* second mapping reuses every level of the existing chain — no new pages */
    TEST_ASSERT_EQUAL_INT(used_after_first, used_after_second);

    uint64_t *pdpt = table_at(pml4[0]);
    uint64_t *pd = table_at(pdpt[0]);
    uint64_t *pt = table_at(pd[0]);

    TEST_ASSERT_EQUAL_HEX64(0x10000 | FLAGS, pt[0]);
    TEST_ASSERT_EQUAL_HEX64(0x20000 | FLAGS, pt[1]);
}

void test_map_page_distinct_pml4_slots_get_distinct_chains(void)
{
    uint64_t va_a = 0x0000000000000000ULL;        /* pml4 index 0 */
    uint64_t va_b = 0x0000800000000000ULL;        /* pml4 index 256 */

    vmm_map_page(pml4, va_a, 0x1000, FLAGS);
    vmm_map_page(pml4, va_b, 0x2000, FLAGS);

    TEST_ASSERT_TRUE(pml4[0] & PTE_PRESENT);
    TEST_ASSERT_TRUE(pml4[256] & PTE_PRESENT);
    TEST_ASSERT_NOT_EQUAL_HEX64(pml4[0] & ENTRY_MASK, pml4[256] & ENTRY_MASK);
}

void test_map_page_bails_out_when_pmm_exhausted(void)
{
    /* Each mapping below needs a fresh PDPT/PD/PT chain (distinct PML4 slots),
     * draining the 8-page pool exactly. */
    for (int i = 0; i < 8; i++) {
        vmm_map_page(pml4, (uint64_t)i << 39, 0x1000, FLAGS);
    }
    TEST_ASSERT_EQUAL_INT(8, vmm_pmm_pages_used());

    /* pool is now empty: a brand new chain cannot be built and must not crash */
    vmm_map_page(pml4, (uint64_t)8 << 39, 0xABC000, FLAGS);
    TEST_ASSERT_FALSE(pml4[8] & PTE_PRESENT);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_map_page_builds_full_table_chain);
    RUN_TEST(test_map_page_intermediate_levels_get_rwu_flags);
    RUN_TEST(test_map_page_reuses_tables_for_addresses_in_same_region);
    RUN_TEST(test_map_page_distinct_pml4_slots_get_distinct_chains);
    RUN_TEST(test_map_page_bails_out_when_pmm_exhausted);
    return UNITY_END();
}
