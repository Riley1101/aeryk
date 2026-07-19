#ifndef IDT
#define IDT
#include <stdint.h>
#include <tty.h>

/**
 * @brief Represents an entry in the Interrupt Descriptor Table (IDT).
 *
 * @see https://wiki.osdev.org/Interrupt_Descriptor_Table#Structure_on_x86-64
 */
struct idt_entry_struct
{
    uint16_t base_low;
    uint16_t set;
    uint8_t ist;
    uint8_t type_attributes;
    uint16_t base_middle;
    uint32_t base_high;
    uint32_t reserved;
} __attribute__((packed));

/**
 * @brief Represents a pointer to the Interrupt Descriptor Table (IDT).
 *
 * @see https://wiki.osdev.org/Interrupt_Descriptor_Table#Structure_on_x86-64
 */
struct idt_ptr_struct
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/**
 * @brief Represents the CPU state saved during an interrupt.
 * @see https://wiki.osdev.org/Interrupt_Descriptor_Table#Structure_on_x86-64
 * @note This structure is used to store the CPU state when an interrupt occurs, allowing the interrupt handler to access and modify the CPU registers as needed.
 * @note The order of the registers in this structure matches the order in which they are pushed onto the stack by the interrupt handler.
 *
 * @see idt.asm for the assembly implementation of the interrupt service routines.
 */
struct interrupt_frame
{
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

typedef void (*irq_handler_t)(struct interrupt_frame *frame);

void init_idt();
void set_idt_gate(uint32_t num, uint64_t handler, uint16_t set, uint8_t ist, uint8_t type_attributes);
void isr_handler(struct interrupt_frame *frame);
void irq_install_handler(int irq, irq_handler_t handler);
void irq_uninstall_handler(int irq);

extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);
extern void isr32(void);
extern void isr33(void);
extern void isr34(void);
extern void isr35(void);
extern void isr36(void);
extern void isr37(void);
extern void isr38(void);
extern void isr39(void);
extern void isr40(void);
extern void isr41(void);
extern void isr42(void);
extern void isr43(void);
extern void isr44(void);
extern void isr45(void);
extern void isr46(void);
extern void isr47(void);

#endif // !IDT
