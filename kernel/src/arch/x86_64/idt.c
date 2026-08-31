#include <apic.h>
#include <idt.h>
#include <pmm.h>
#include <process.h>
#include <stdint.h>
#include <string.h>
#include <utils.h>
#include <vmm.h>

#include <arch/x86_64/drivers/serial.h>

#define IDT_SIZE 256

#define IRQ_COUNT 16

// #PF error code bits (Intel SDM Vol. 3A, 4.7)
#define PF_ERR_WRITE (1u << 1)

/**
 * @file Interrupt Descriptor Table (IDT) initialization
 * and handling for x86_64 architecture.
 *
 * @see https://wiki.osdev.org/Interrupt_Descriptor_Table#Structure_on_x86-64
 */

/**
 * @brief Interrupt Descriptor Table (IDT) entries array.
 * This array keeps all the IDT entries used by the kernel.
 */
struct idt_entry_struct idt_entries[IDT_SIZE];

/**
 * @brief Pointer to the Interrupt Descriptor Table (IDT).
 *
 * This structure holds the base address and limit of the IDT.
 */
struct idt_ptr_struct idt_ptr;

/**
 * @brief Array of IRQ handlers.
 *
 * Each entry corresponds to a standard ISA IRQ.
 *
 * @see https://wiki.osdev.org/Interrupts look at Standard ISA IRQs
 */
static irq_handler_t irq_handlers[IRQ_COUNT];

/**
 * @brief Installs an IRQ handler for a specific IRQ.
 *
 * @param irq The IRQ number.
 * @param handler The handler function to be installed.
 */
void irq_install_handler(int irq, irq_handler_t handler)
{

    irq_handlers[irq] = handler;
}

/**
 * @brief Uninstalls an IRQ handler for a specific IRQ.
 *
 * @param irq The IRQ number.
 */
void irq_uninstall_handler(int irq) { irq_handlers[irq] = 0; }

/**
 * @brief Initializes the Interrupt Descriptor Table (IDT).
 *
 * This function sets up the IDT entries and loads the IDT register.
 *
 * 0-31: CPU exceptions
 * 32-47: IRQs (mapped from PIC)
 *
 * 0x08 is the code segment selector for the kernel code segment in the GDT.
 *
 *
 * @see https://wiki.osdev.org/Interrupt_Descriptor_Table (gate types, exception vectors 0-31)
 * @see https://wiki.osdev.org/Exceptions (CPU exception cheatsheet)
 */
void init_idt()
{
    idt_ptr.limit = sizeof(struct idt_entry_struct) * IDT_SIZE - 1;
    idt_ptr.base = (uint64_t)&idt_entries;

    memset(&idt_entries, 0, sizeof(struct idt_entry_struct) * IDT_SIZE);

    set_idt_gate(0, (uint64_t)isr0, 0x08, 0, 0x8E);
    set_idt_gate(1, (uint64_t)isr1, 0x08, 0, 0x8E);
    set_idt_gate(2, (uint64_t)isr2, 0x08, 0, 0x8E);
    set_idt_gate(3, (uint64_t)isr3, 0x08, 0, 0x8E);
    set_idt_gate(4, (uint64_t)isr4, 0x08, 0, 0x8E);
    set_idt_gate(5, (uint64_t)isr5, 0x08, 0, 0x8E);
    set_idt_gate(6, (uint64_t)isr6, 0x08, 0, 0x8E);
    set_idt_gate(7, (uint64_t)isr7, 0x08, 0, 0x8E);
    set_idt_gate(8, (uint64_t)isr8, 0x08, 0, 0x8E);
    set_idt_gate(9, (uint64_t)isr9, 0x08, 0, 0x8E);
    set_idt_gate(10, (uint64_t)isr10, 0x08, 0, 0x8E);
    set_idt_gate(11, (uint64_t)isr11, 0x08, 0, 0x8E);
    set_idt_gate(12, (uint64_t)isr12, 0x08, 0, 0x8E);
    set_idt_gate(13, (uint64_t)isr13, 0x08, 0, 0x8E);
    set_idt_gate(14, (uint64_t)isr14, 0x08, 0, 0x8E);
    set_idt_gate(15, (uint64_t)isr15, 0x08, 0, 0x8E);
    set_idt_gate(16, (uint64_t)isr16, 0x08, 0, 0x8E);
    set_idt_gate(17, (uint64_t)isr17, 0x08, 0, 0x8E);
    set_idt_gate(18, (uint64_t)isr18, 0x08, 0, 0x8E);
    set_idt_gate(19, (uint64_t)isr19, 0x08, 0, 0x8E);
    set_idt_gate(20, (uint64_t)isr20, 0x08, 0, 0x8E);
    set_idt_gate(21, (uint64_t)isr21, 0x08, 0, 0x8E);
    set_idt_gate(22, (uint64_t)isr22, 0x08, 0, 0x8E);
    set_idt_gate(23, (uint64_t)isr23, 0x08, 0, 0x8E);
    set_idt_gate(24, (uint64_t)isr24, 0x08, 0, 0x8E);
    set_idt_gate(25, (uint64_t)isr25, 0x08, 0, 0x8E);
    set_idt_gate(26, (uint64_t)isr26, 0x08, 0, 0x8E);
    set_idt_gate(27, (uint64_t)isr27, 0x08, 0, 0x8E);
    set_idt_gate(28, (uint64_t)isr28, 0x08, 0, 0x8E);
    set_idt_gate(29, (uint64_t)isr29, 0x08, 0, 0x8E);
    set_idt_gate(30, (uint64_t)isr30, 0x08, 0, 0x8E);
    set_idt_gate(31, (uint64_t)isr31, 0x08, 0, 0x8E);
    set_idt_gate(32, (uint64_t)isr32, 0x08, 0, 0x8E);
    set_idt_gate(33, (uint64_t)isr33, 0x08, 0, 0x8E);
    set_idt_gate(34, (uint64_t)isr34, 0x08, 0, 0x8E);
    set_idt_gate(35, (uint64_t)isr35, 0x08, 0, 0x8E);
    set_idt_gate(36, (uint64_t)isr36, 0x08, 0, 0x8E);
    set_idt_gate(37, (uint64_t)isr37, 0x08, 0, 0x8E);
    set_idt_gate(38, (uint64_t)isr38, 0x08, 0, 0x8E);
    set_idt_gate(39, (uint64_t)isr39, 0x08, 0, 0x8E);
    set_idt_gate(40, (uint64_t)isr40, 0x08, 0, 0x8E);
    set_idt_gate(41, (uint64_t)isr41, 0x08, 0, 0x8E);
    set_idt_gate(42, (uint64_t)isr42, 0x08, 0, 0x8E);
    set_idt_gate(43, (uint64_t)isr43, 0x08, 0, 0x8E);
    set_idt_gate(44, (uint64_t)isr44, 0x08, 0, 0x8E);
    set_idt_gate(45, (uint64_t)isr45, 0x08, 0, 0x8E);
    set_idt_gate(46, (uint64_t)isr46, 0x08, 0, 0x8E);
    set_idt_gate(47, (uint64_t)isr47, 0x08, 0, 0x8E);

// TODO Cross compile these tests to avoid arch dependent flags
#ifdef __x86_64__
    asm volatile("lidt (%0)" : : "r"(&idt_ptr));
#endif
};

/**
 * @brief Array of exception messages corresponding to CPU exception vectors 0-31.
 */
static const char *exception_messages[32] = {
    "Division By Zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved",
};

/**
 * @brief Handles CPU exceptions and IRQs.
 *
 * This function is called by the interrupt service routines (ISRs) and
 * dispatches the appropriate handler based on the interrupt number.
 *
 * A write-triggered page fault (#PF, vector 14) is first offered to
 * vmm_handle_cow_fault() so a copy-on-write page from fork() can be
 * resolved transparently; if it isn't a recognized COW fault, execution
 * falls through to the same fatal handling as every other exception below.
 *
 * @param frame Pointer to the interrupt frame containing CPU state.
 */
void isr_handler(struct interrupt_frame *frame)
{
    if (frame->int_no == 14 && (frame->err_code & PF_ERR_WRITE) &&
        current_process)
    {
        uint64_t fault_vaddr;
        asm volatile("mov %%cr2, %0" : "=r"(fault_vaddr));

        uint64_t *pml4 = (uint64_t *)(current_process->cr3 + hhdm_offset);
        if (vmm_handle_cow_fault(pml4, fault_vaddr))
        {
            return;
        }
    }

    if (frame-> int_no < 32)
    {
        // CPL 3 means the faulting instruction was running as user-mode
        // code (as opposed to a kernel bug, or a bad user pointer
        // dereferenced from inside a syscall, which runs at CPL 0). Only
        // that process is misbehaving, so kill it and keep the rest of the
        // system running instead of halting everything.
        int from_userland = current_process && (frame->cs & 3) == 3;

        if (from_userland)
        {
            serial_print("\n[Fault] Killing user process due to exception: ");
            serial_print(exception_messages[frame->int_no]);
            serial_print("\n");

            current_process->exit_code = -1;
            current_process->state = PROCESS_DEAD;

            // switch_task() saves/restores RFLAGS per-task, so the process we
            // switch into resumes with its own IF state -- no manual sti()
            // needed here even though exceptions enter with IF=0.
            schedule();

            // // schedule() does not return for a dead process; halt
            // // defensively if it somehow does.
            // for (;;)
            // {
            //     asm volatile("hlt");
            // }
        }

        serial_print("\n KERNEL PANIC **\n");
        serial_print("EXCEPTION:");
        serial_print(exception_messages[frame->int_no]);

// TODO! Cross compile these tests to avoid arch dependent flags
#ifdef __x86_64__
        asm volatile("cli; hlt");
#else
        while (1)
        {
        }
#endif
    }
    else
    {
        int irq = (int)frame->int_no - 32;

        lapic_eoi();

        if (irq >= 0 && irq < IRQ_COUNT && irq_handlers[irq])
            irq_handlers[irq](frame);
    }
}

/**
 * @brief Sets an entry in the Interrupt Descriptor Table (IDT).
 *
 * @param num The IDT entry number.
 * @param handler The address of the interrupt handler function.
 * @param set The segment selector for the handler.
 * @param ist The Interrupt Stack Table (IST) index.
 * @param type_attributes The type and attributes of the IDT entry.
 *
 * @see https://wiki.osdev.org/Interrupt_Descriptor_Table#Structure_on_x86-64
 */
void set_idt_gate(uint32_t num, uint64_t handler, uint16_t set, uint8_t ist,
                  uint8_t type_attributes)
{
    idt_entries[num].base_low = handler & 0xFFFF;
    idt_entries[num].set = set;
    idt_entries[num].ist = ist & 0x07;
    idt_entries[num].type_attributes = type_attributes;
    idt_entries[num].base_middle = (handler >> 16) & 0xFFFF;
    idt_entries[num].base_high = (handler >> 32) & 0xFFFFFFFF;
    idt_entries[num].reserved = 0;
}
