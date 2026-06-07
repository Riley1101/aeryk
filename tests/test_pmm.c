#include "unity.h"
#include <pmm.h>
#include <stdint.h>

void init_pmm_test_env(void);
uint64_t fake_hhdm_offset(void);
uint64_t fake_phys_mem_size(void);
int fake_phys_mem_page_count(void);

/* init_pmm() carves the bitmap out of the (fake) usable memory itself and has
 * no reset hook — it's meant to run exactly once at boot, so we do the same
 * here, in main(), before any test runs. Each test below is written to be a
 * self-contained, order-independent invariant of the public API so that
 * shared allocator state between tests doesn't matter — except the exhaustion
 * test, which must run last since it deliberately drains all memory. */

void setUp(void) {}
void tearDown(void) {}

void test_init_pmm_publishes_hhdm_offset(void)
{
    TEST_ASSERT_EQUAL_HEX64(fake_hhdm_offset(), hhdm_offset);
}

void test_alloc_returns_page_aligned_address_in_range(void)
{
    void *p = pmm_alloc_page();
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_HEX64(0, (uint64_t)p % PAGE_SIZE);
    TEST_ASSERT_TRUE((uint64_t)p < fake_phys_mem_size());
}

void test_consecutive_allocations_are_distinct(void)
{
    void *a = pmm_alloc_page();
    void *b = pmm_alloc_page();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_EQUAL_HEX64((uint64_t)a, (uint64_t)b);
}

void test_freeing_most_recent_allocation_makes_it_immediately_reusable(void)
{
    void *a = pmm_alloc_page();
    TEST_ASSERT_NOT_NULL(a);
    pmm_free_page(a);
    void *b = pmm_alloc_page();
    TEST_ASSERT_EQUAL_PTR(a, b);
}

void test_double_free_does_not_corrupt_allocator(void)
{
    void *a = pmm_alloc_page();
    pmm_free_page(a);
    pmm_free_page(a); /* must be a no-op, not double-clear/crash */
    void *b = pmm_alloc_page();
    TEST_ASSERT_EQUAL_PTR(a, b);
}

void test_alloc_returns_null_once_memory_is_exhausted(void)
{
    int allocated = 0;
    while (pmm_alloc_page() != NULL) {
        allocated++;
        TEST_ASSERT_LESS_OR_EQUAL_INT(fake_phys_mem_page_count(), allocated);
    }
    TEST_ASSERT_NULL(pmm_alloc_page()); /* stays exhausted, doesn't wrap */
}

int main(void)
{
    init_pmm_test_env();
    init_pmm();

    UNITY_BEGIN();
    RUN_TEST(test_init_pmm_publishes_hhdm_offset);
    RUN_TEST(test_alloc_returns_page_aligned_address_in_range);
    RUN_TEST(test_consecutive_allocations_are_distinct);
    RUN_TEST(test_freeing_most_recent_allocation_makes_it_immediately_reusable);
    RUN_TEST(test_double_free_does_not_corrupt_allocator);
    RUN_TEST(test_alloc_returns_null_once_memory_is_exhausted);
    return UNITY_END();
}
