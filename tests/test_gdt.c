#include "unity.h"
#include <gdt.h>
#include <stddef.h>

extern struct gdt_entry_struct gdt_entries[];
extern struct tss_entry_struct tss_entry;

void setUp(void) {}
void tearDown(void) {}

/* --- set_gdt_gate tests --- */

void test_set_gdt_gate_packs_base_correctly(void)
{
    set_gdt_gate(1, 0x12345678, 0xFFFFFFFF, 0x9A, 0xAF);
    TEST_ASSERT_EQUAL_HEX16(0x5678, gdt_entries[1].base_low);
    TEST_ASSERT_EQUAL_HEX8(0x34, gdt_entries[1].base_middle);
    TEST_ASSERT_EQUAL_HEX8(0x12, gdt_entries[1].base_high);
}

void test_set_gdt_gate_packs_limit_and_flags(void)
{
    set_gdt_gate(1, 0, 0x000FFFFF, 0x9A, 0xA0);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, gdt_entries[1].limit_low);
    TEST_ASSERT_EQUAL_HEX8(0xAF, gdt_entries[1].flags);
}

void test_set_gdt_gate_sets_access(void)
{
    set_gdt_gate(2, 0, 0, 0x92, 0xCF);
    TEST_ASSERT_EQUAL_HEX8(0x92, gdt_entries[2].access);
}

/* --- write_tss tests (64-bit TSS spans two 8-byte descriptor slots) --- */

void test_write_tss_lower_descriptor(void)
{
    write_tss(5, 0xCAFEBABE);
    TEST_ASSERT_EQUAL_HEX8(0x89, gdt_entries[5].access);
    TEST_ASSERT_EQUAL_HEX16(sizeof(struct tss_entry_struct) - 1, gdt_entries[5].limit_low);
}

void test_write_tss_upper_descriptor_holds_high_base_bits(void)
{
    uint64_t base = (uint64_t)&tss_entry;
    write_tss(5, 0);
    TEST_ASSERT_EQUAL_HEX16((base >> 32) & 0xFFFF, gdt_entries[6].limit_low);
    TEST_ASSERT_EQUAL_HEX16((base >> 48) & 0xFFFF, gdt_entries[6].base_low);
    TEST_ASSERT_EQUAL_HEX8(0, gdt_entries[6].access);
    TEST_ASSERT_EQUAL_HEX8(0, gdt_entries[6].flags);
}

void test_write_tss_initializes_tss_entry(void)
{
    write_tss(5, 0xDEADBEEF);
    TEST_ASSERT_EQUAL_HEX64(0xDEADBEEF, tss_entry.rsp0);
    TEST_ASSERT_EQUAL_UINT16(sizeof(struct tss_entry_struct), tss_entry.iomap_base);
}

void test_set_kernel_stack_updates_rsp0(void)
{
    write_tss(5, 0);
    set_kernel_stack(0x1000);
    TEST_ASSERT_EQUAL_HEX64(0x1000, tss_entry.rsp0);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_set_gdt_gate_packs_base_correctly);
    RUN_TEST(test_set_gdt_gate_packs_limit_and_flags);
    RUN_TEST(test_set_gdt_gate_sets_access);
    RUN_TEST(test_write_tss_lower_descriptor);
    RUN_TEST(test_write_tss_upper_descriptor_holds_high_base_bits);
    RUN_TEST(test_write_tss_initializes_tss_entry);
    RUN_TEST(test_set_kernel_stack_updates_rsp0);
    return UNITY_END();
}
