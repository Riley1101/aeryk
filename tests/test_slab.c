#include "unity.h"
#include <pmm.h>
#include <slab.h>
#include <stddef.h>
#include <stdint.h>

void reset_slab_pmm_pool(void);
int slab_pmm_pages_used(void);
int slab_pmm_pages_freed(void);

void setUp(void)
{
    reset_slab_pmm_pool();
    init_slab();
}

void tearDown(void) {}

/* Mirrors allocate_new_slab's layout math so the test stays correct
 * regardless of struct slab's exact size/padding. */
static size_t chunks_per_slab(size_t obj_size)
{
    size_t data_start_off = (sizeof(struct slab) + 15) & ~(size_t)15;
    return (PAGE_SIZE - data_start_off) / obj_size;
}

void test_kmalloc_zero_returns_null(void)
{
    TEST_ASSERT_NULL(kmalloc(0));
}

void test_kmalloc_oversized_returns_null(void)
{
    TEST_ASSERT_NULL(kmalloc(2049));
}

void test_kmalloc_max_supported_size_succeeds(void)
{
    TEST_ASSERT_NOT_NULL(kmalloc(2048));
}

void test_kmalloc_allocates_backing_page_lazily(void)
{
    TEST_ASSERT_EQUAL_INT(0, slab_pmm_pages_used());
    void *p = kmalloc(16);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_INT(1, slab_pmm_pages_used());
}

void test_kmalloc_packs_small_allocations_into_one_page(void)
{
    void *a = kmalloc(16);
    void *b = kmalloc(16);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_EQUAL_HEX64((uint64_t)a, (uint64_t)b);
    TEST_ASSERT_EQUAL_INT(1, slab_pmm_pages_used());
}

void test_kfree_reuses_chunk_lifo(void)
{
    void *a = kmalloc(16);
    kmalloc(16); /* keep a second live chunk so the slab stays partial */
    kfree(a);
    void *c = kmalloc(16);
    TEST_ASSERT_EQUAL_PTR(a, c);
}

void test_kfree_rejects_pointer_without_slab_magic(void)
{
    int not_a_slab_chunk = 0;
    int freed_before = slab_pmm_pages_freed();
    kfree(&not_a_slab_chunk); /* must not corrupt or free anything */
    TEST_ASSERT_EQUAL_INT(freed_before, slab_pmm_pages_freed());
}

void test_kfree_ignores_null(void)
{
    kfree(NULL); /* must not crash */
    TEST_ASSERT_EQUAL_INT(0, slab_pmm_pages_freed());
}

void test_kmalloc_starts_new_slab_once_current_is_full(void)
{
    size_t n = chunks_per_slab(16);
    for (size_t i = 0; i < n; i++)
        TEST_ASSERT_NOT_NULL(kmalloc(16));
    TEST_ASSERT_EQUAL_INT(1, slab_pmm_pages_used());

    TEST_ASSERT_NOT_NULL(kmalloc(16));
    TEST_ASSERT_EQUAL_INT(2, slab_pmm_pages_used());
}

void test_kfree_returns_fully_freed_slab_to_pmm(void)
{
    size_t n = chunks_per_slab(16);
    void *objs[512];
    TEST_ASSERT_LESS_OR_EQUAL(512, n);

    for (size_t i = 0; i < n; i++)
        objs[i] = kmalloc(16);
    TEST_ASSERT_EQUAL_INT(1, slab_pmm_pages_used());
    TEST_ASSERT_EQUAL_INT(0, slab_pmm_pages_freed());

    for (size_t i = 0; i < n; i++)
        kfree(objs[i]);

    TEST_ASSERT_EQUAL_INT(1, slab_pmm_pages_freed());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_kmalloc_zero_returns_null);
    RUN_TEST(test_kmalloc_oversized_returns_null);
    RUN_TEST(test_kmalloc_max_supported_size_succeeds);
    RUN_TEST(test_kmalloc_allocates_backing_page_lazily);
    RUN_TEST(test_kmalloc_packs_small_allocations_into_one_page);
    RUN_TEST(test_kfree_reuses_chunk_lifo);
    RUN_TEST(test_kfree_rejects_pointer_without_slab_magic);
    RUN_TEST(test_kfree_ignores_null);
    RUN_TEST(test_kmalloc_starts_new_slab_once_current_is_full);
    RUN_TEST(test_kfree_returns_fully_freed_slab_to_pmm);
    return UNITY_END();
}
