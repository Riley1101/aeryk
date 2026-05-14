#ifndef GDT
#define GDT

#include <stdint.h>

// @see https://wiki.osdev.org/Task_State_Segment (64-bit layout)
struct tss_entry_struct
{
    uint32_t reserved0;
    uint64_t rsp0;          // kernel stack for ring 0
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];        // interrupt stack table
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

struct gdt_entry_struct
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t flags;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr_struct
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

void initGdt();
void setGdtGate(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void writeTSS(uint32_t num, uint64_t rsp0);

void gdt_flush(struct gdt_ptr_struct *ptr);

void tss_flush();

#endif
