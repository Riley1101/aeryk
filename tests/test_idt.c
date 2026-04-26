#include "unity.h"
#include <idt.h>

#define IDT_SIZE 256
extern struct idt_entry_struct idt_entries[IDT_SIZE];

void setUp(void) {}
void tearDown(void) {}

void test_setIdtGate_packs_base_correctly(void)
{
    uint64_t handler = 0x0000DEADCAFE1234ULL;
    setIdtGate(5, handler, 0x08, 0, 0x8E);
    TEST_ASSERT_EQUAL_HEX16(0x1234, idt_entries[5].base_low);
    TEST_ASSERT_EQUAL_HEX16(0xCAFE, idt_entries[5].base_middle);
    TEST_ASSERT_EQUAL_HEX32(0x0000DEAD, idt_entries[5].base_high);
}

void test_setIdtGate_sets_selector_and_attributes(void)
{
    setIdtGate(3, 0x1000, 0x08, 2, 0x8E);
    TEST_ASSERT_EQUAL_HEX16(0x08, idt_entries[3].set);
    TEST_ASSERT_EQUAL_UINT8(2 & 0x07, idt_entries[3].ist);
    TEST_ASSERT_EQUAL_HEX8(0x8E, idt_entries[3].type_attributes);
}

void test_setIdtGate_reserved_is_zero(void)
{
    setIdtGate(0, 0xDEADBEEF, 0x08, 0, 0x8E);
    TEST_ASSERT_EQUAL_UINT32(0, idt_entries[0].reserved);
}

void test_setIdtGate_ist_masked_to_3_bits(void)
{
    setIdtGate(0, 0x1000, 0x08, 0xFF, 0x8E);
    TEST_ASSERT_EQUAL_UINT8(0x07, idt_entries[0].ist);
}

static int handler_called;
static void dummy_handler(struct interrupt_frame *frame)
{
    (void)frame;
    handler_called++;
}

void test_irq_install_and_dispatch(void)
{
    handler_called = 0;
    irq_install_handler(1, dummy_handler);
    struct interrupt_frame frame = {0};
    frame.int_no = 33; /* IRQ 1 = 32 + 1 */
    isr_handler(&frame);
    TEST_ASSERT_EQUAL_INT(1, handler_called);
}

void test_irq_uninstall_stops_dispatch(void)
{
    handler_called = 0;
    irq_install_handler(1, dummy_handler);
    irq_uninstall_handler(1);
    struct interrupt_frame frame = {0};
    frame.int_no = 33;
    isr_handler(&frame);
    TEST_ASSERT_EQUAL_INT(0, handler_called);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_setIdtGate_packs_base_correctly);
    RUN_TEST(test_setIdtGate_sets_selector_and_attributes);
    RUN_TEST(test_setIdtGate_reserved_is_zero);
    RUN_TEST(test_setIdtGate_ist_masked_to_3_bits);
    RUN_TEST(test_irq_install_and_dispatch);
    RUN_TEST(test_irq_uninstall_stops_dispatch);
    return UNITY_END();
}
