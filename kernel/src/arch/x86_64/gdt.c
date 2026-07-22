#include <gdt.h>
#include <string.h>

#define GDT_SIZE 7 // index 5 + 6 form the 16-byte 64-bit TSS descriptor

/**
 * @file Global Descriptor Table (GDT) initialization
 * and handling for x86_64 architecture.
 *
 * @see https://wiki.osdev.org/GDT_Tutorial
 */

/**
 * @brief Global Descriptor Table (GDT) entries array.
 *
 * This array keeps all the GDT entries used by the kernel.
 */
struct gdt_entry_struct gdt_entries[GDT_SIZE];

/**
 * @brief Task State Segment (TSS) entry used by the kernel.
 */
struct tss_entry_struct tss_entry;

struct gdt_ptr_struct gdt_ptr;

/**
 * @brief Initializes the Global Descriptor Table (GDT) and the Task State Segment (TSS).
 *
 * @see https://wiki.osdev.org/GDT_Tutorial
 */
void init_gdt()
{
    gdt_ptr.limit = (sizeof(struct gdt_entry_struct) * GDT_SIZE) - 1;
    gdt_ptr.base = (uint64_t)&gdt_entries;

    set_gdt_gate(0, 0, 0, 0x00, 0x00); // Null descriptor

    set_gdt_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xAF); // Kernel Code (64-bit, L bit set)
    set_gdt_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel Data

    /**
     * AMD found a performance tweak to get the user space without going through GDT, by just reording UserData above User Code
     * For SYSRET
     */
    set_gdt_gate(3, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User Data (DPL=3)

    set_gdt_gate(4, 0, 0xFFFFFFFF, 0xFA, 0xAF); // User Code (64-bit, DPL=3)

    write_tss(5, 0x0);
    gdt_flush(&gdt_ptr);

    tss_flush();
}

/**
 * @brief Writes a Task State Segment (TSS) descriptor into the GDT.
 * 
 * @param num The index in the GDT where the TSS descriptor will be written.
 * @param rsp0 The stack pointer for ring 0 (kernel) to be set in the TSS.
 */
void write_tss(uint32_t num, uint64_t rsp0)
{
    uint64_t base = (uint64_t)&tss_entry;
    uint32_t limit = sizeof(struct tss_entry_struct) - 1;

    // Lower 32 bits of base via the standard 8-byte descriptor slot
    set_gdt_gate(num, (uint32_t)(base & 0xFFFFFFFF), limit, 0x89, 0x00);

    // Upper 32 bits of base go in the next slot (64-bit system descriptors are 16 bytes)
    gdt_entries[num + 1].limit_low = (base >> 32) & 0xFFFF;
    gdt_entries[num + 1].base_low = (base >> 48) & 0xFFFF;
    gdt_entries[num + 1].base_middle = 0;
    gdt_entries[num + 1].access = 0;
    gdt_entries[num + 1].flags = 0;
    gdt_entries[num + 1].base_high = 0;

    memset(&tss_entry, 0, sizeof(tss_entry));
    tss_entry.rsp0 = rsp0;
    tss_entry.iomap_base = sizeof(struct tss_entry_struct);
}

/**
 * @brief Sets the stack pointer for ring 0 (kernel) in the Task State Segment (TSS).
 * 
 * @param rsp0 The stack pointer value to set for ring 0.
 */
void set_kernel_stack(uint64_t rsp0)
{
    tss_entry.rsp0 = rsp0;
}

/**
 * @brief Sets a GDT entry at the specified index with the given parameters.
 * 
 * @param num The index in the GDT where the entry will be set.
 * @param base The base address for the segment.
 * @param limit The limit for the segment.
 * @param access The access flags for the segment.
 * @param gran The granularity flags for the segment.
 */
void set_gdt_gate(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt_entries[num].base_low = (base & 0xFFFF);

    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low = (limit & 0xFFFF);
    gdt_entries[num].flags = (limit >> 16) & 0x0F;
    gdt_entries[num].flags |= (gran & 0xF0);

    gdt_entries[num].access = access;
}
