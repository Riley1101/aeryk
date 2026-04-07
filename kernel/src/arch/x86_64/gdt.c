#include <gdt.h>
#include <string.h>

#define GDT_SIZE 7  // index 5 + 6 form the 16-byte 64-bit TSS descriptor

struct gdt_entry_struct gdt_entries[GDT_SIZE];
struct tss_entry_struct tss_entry;
struct gdt_ptr_struct gdt_ptr;

// @see https://wiki.osdev.org/GDT_Tutorial
void initGdt()
{
    gdt_ptr.limit = (sizeof(struct gdt_entry_struct) * GDT_SIZE) - 1;
    gdt_ptr.base = (uint64_t)&gdt_entries;

    setGdtGate(0, 0, 0, 0x00, 0x00); // Null descriptor

    setGdtGate(1, 0, 0xFFFFFFFF, 0x9A, 0xAF); // Kernel Code (64-bit, L bit set)
    setGdtGate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel Data
    setGdtGate(3, 0, 0xFFFFFFFF, 0xFA, 0xAF); // User Code (64-bit, DPL=3)
    setGdtGate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User Data (DPL=3)

    writeTSS(5, 0x0);
    gdt_flush(&gdt_ptr);

    tss_flush();
}

void writeTSS(uint32_t num, uint64_t rsp0)
{
    uint64_t base  = (uint64_t)&tss_entry;
    uint32_t limit = sizeof(struct tss_entry_struct) - 1;

    // Lower 32 bits of base via the standard 8-byte descriptor slot
    setGdtGate(num, (uint32_t)(base & 0xFFFFFFFF), limit, 0x89, 0x00);

    // Upper 32 bits of base go in the next slot (64-bit system descriptors are 16 bytes)
    gdt_entries[num + 1].limit_low   = (base >> 32) & 0xFFFF;
    gdt_entries[num + 1].base_low    = (base >> 48) & 0xFFFF;
    gdt_entries[num + 1].base_middle = 0;
    gdt_entries[num + 1].access      = 0;
    gdt_entries[num + 1].flags       = 0;
    gdt_entries[num + 1].base_high   = 0;

    memset(&tss_entry, 0, sizeof(tss_entry));
    tss_entry.rsp0       = rsp0;
    tss_entry.iomap_base = sizeof(struct tss_entry_struct);
}

void setGdtGate(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt_entries[num].base_low = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low = (limit & 0xFFFF);
    gdt_entries[num].flags = (limit >> 16) & 0x0F;
    gdt_entries[num].flags |= (gran & 0xF0);

    gdt_entries[num].access = access;
}
