#include "unity.h"
#include <idt.h>

#define IDT_SIZE 256
extern struct idt_entry_struct idt_entries[IDT_SIZE];

typedef struct { uint16_t port; uint8_t value; } PortWrite;
extern PortWrite portb_log[];
extern int portb_log_count;
void reset_portb_log(void);

void setUp(void) {}

void tearDown(void)
{
    for (int i = 0; i < 16; i++)
        irq_uninstall_handler(i);
    reset_portb_log();
}

/* --- setIdtGate tests --- */

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

/* --- IRQ handler tests --- */

static int handler_called;
static void dummy_handler(struct interrupt_frame *frame)
{
    (void)frame;
    handler_called++;
}

static int handler_a_called;
static void handler_a(struct interrupt_frame *frame)
{
    (void)frame;
    handler_a_called++;
}

static int handler_b_called;
static void handler_b(struct interrupt_frame *frame)
{
    (void)frame;
    handler_b_called++;
}

static struct interrupt_frame *received_frame;
static void frame_capturing_handler(struct interrupt_frame *frame)
{
    received_frame = frame;
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

void test_irq_dispatch_irq0(void)
{
    handler_called = 0;
    irq_install_handler(0, dummy_handler);
    struct interrupt_frame frame = {0};
    frame.int_no = 32; /* IRQ 0 = 32 + 0 */
    isr_handler(&frame);
    TEST_ASSERT_EQUAL_INT(1, handler_called);
}

void test_irq_dispatch_irq15(void)
{
    handler_called = 0;
    irq_install_handler(15, dummy_handler);
    struct interrupt_frame frame = {0};
    frame.int_no = 47; /* IRQ 15 = 32 + 15 */
    isr_handler(&frame);
    TEST_ASSERT_EQUAL_INT(1, handler_called);
}

void test_irq_no_handler_no_call(void)
{
    handler_called = 0;
    /* IRQ 5 has no handler registered */
    struct interrupt_frame frame = {0};
    frame.int_no = 37; /* IRQ 5 = 32 + 5 */
    isr_handler(&frame);
    TEST_ASSERT_EQUAL_INT(0, handler_called);
}

void test_irq_reinstall_replaces_handler(void)
{
    handler_a_called = 0;
    handler_b_called = 0;
    irq_install_handler(3, handler_a);
    irq_install_handler(3, handler_b);
    struct interrupt_frame frame = {0};
    frame.int_no = 35; /* IRQ 3 = 32 + 3 */
    isr_handler(&frame);
    TEST_ASSERT_EQUAL_INT(0, handler_a_called);
    TEST_ASSERT_EQUAL_INT(1, handler_b_called);
}

void test_irq_handler_receives_correct_frame(void)
{
    received_frame = NULL;
    irq_install_handler(2, frame_capturing_handler);
    struct interrupt_frame frame = {0};
    frame.int_no = 34; /* IRQ 2 = 32 + 2 */
    frame.rax = 0xDEADBEEFULL;
    isr_handler(&frame);
    TEST_ASSERT_EQUAL_PTR(&frame, received_frame);
    TEST_ASSERT_EQUAL_HEX64(0xDEADBEEFULL, received_frame->rax);
}

void test_irq_multiple_independent_slots(void)
{
    handler_called = 0;
    handler_a_called = 0;
    irq_install_handler(0, dummy_handler);
    irq_install_handler(7, handler_a);

    struct interrupt_frame frame = {0};
    frame.int_no = 32; /* IRQ 0 */
    isr_handler(&frame);
    TEST_ASSERT_EQUAL_INT(1, handler_called);
    TEST_ASSERT_EQUAL_INT(0, handler_a_called);

    frame.int_no = 39; /* IRQ 7 */
    isr_handler(&frame);
    TEST_ASSERT_EQUAL_INT(1, handler_called);
    TEST_ASSERT_EQUAL_INT(1, handler_a_called);
}

/* --- PIC EOI routing tests --- */

void test_irq_master_eoi_only_for_irq0_to_7(void)
{
    /* Vectors 32-39 (IRQ 0-7) must send EOI only to master PIC (port 0x20) */
    struct interrupt_frame frame = {0};
    frame.int_no = 32; /* IRQ 0, lowest master-only vector */
    isr_handler(&frame);
    TEST_ASSERT_EQUAL_INT(1, portb_log_count);
    TEST_ASSERT_EQUAL_HEX16(0x20, portb_log[0].port);
    TEST_ASSERT_EQUAL_HEX8(0x20, portb_log[0].value);
}

void test_irq_slave_and_master_eoi_for_irq8_to_15(void)
{
    /* Vectors 40-47 (IRQ 8-15) must send EOI to slave PIC (0xA0) then master (0x20) */
    struct interrupt_frame frame = {0};
    frame.int_no = 40; /* IRQ 8, lowest slave vector */
    isr_handler(&frame);
    TEST_ASSERT_EQUAL_INT(2, portb_log_count);
    TEST_ASSERT_EQUAL_HEX16(0xA0, portb_log[0].port);
    TEST_ASSERT_EQUAL_HEX8(0x20, portb_log[0].value);
    TEST_ASSERT_EQUAL_HEX16(0x20, portb_log[1].port);
    TEST_ASSERT_EQUAL_HEX8(0x20, portb_log[1].value);
}

void test_irq_eoi_boundary_vector39(void)
{
    /* Vector 39 (IRQ 7) is the last master-only vector — must NOT send slave EOI */
    struct interrupt_frame frame = {0};
    frame.int_no = 39;
    isr_handler(&frame);
    TEST_ASSERT_EQUAL_INT(1, portb_log_count);
    TEST_ASSERT_EQUAL_HEX16(0x20, portb_log[0].port);
}

void test_irq_eoi_boundary_vector40(void)
{
    /* Vector 40 (IRQ 8) is the first slave vector — must send slave EOI first */
    struct interrupt_frame frame = {0};
    frame.int_no = 40;
    isr_handler(&frame);
    TEST_ASSERT_EQUAL_INT(2, portb_log_count);
    TEST_ASSERT_EQUAL_HEX16(0xA0, portb_log[0].port);
    TEST_ASSERT_EQUAL_HEX16(0x20, portb_log[1].port);
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
    RUN_TEST(test_irq_dispatch_irq0);
    RUN_TEST(test_irq_dispatch_irq15);
    RUN_TEST(test_irq_no_handler_no_call);
    RUN_TEST(test_irq_reinstall_replaces_handler);
    RUN_TEST(test_irq_handler_receives_correct_frame);
    RUN_TEST(test_irq_multiple_independent_slots);
    RUN_TEST(test_irq_master_eoi_only_for_irq0_to_7);
    RUN_TEST(test_irq_slave_and_master_eoi_for_irq8_to_15);
    RUN_TEST(test_irq_eoi_boundary_vector39);
    RUN_TEST(test_irq_eoi_boundary_vector40);
    return UNITY_END();
}
