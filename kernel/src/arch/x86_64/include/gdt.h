#ifndef GDT
#define GDT

#include <stdint.h>

/**
 * @brief Task State Segment (TSS) structure for the x86_64 architecture.
 *
 * @see https://wiki.osdev.org/Task_State_Segment#Long_Mode
 *
 * @note This structure is used by the CPU to manage task-specific information,
 * including stack pointers for different privilege levels and the interrupt stack table (IST).
 */
struct tss_entry_struct
{
    /** @brief Reserved field (must be zero) */
    uint32_t reserved0;

    /** @brief Stack pointer for ring 0 (kernel) */
    uint64_t rsp0;

    /** @brief Stack pointer for ring 1 */
    uint64_t rsp1;

    /** @brief Stack pointer for ring 2 */
    uint64_t rsp2;
    /** @brief Reserved field (must be zero) */
    uint64_t reserved1;

    /** @brief Interrupt Stack Table (IST) entries */
    uint64_t ist[7];

    /** @brief Reserved field (must be zero) */
    uint64_t reserved2;
    /** @brief Reserved field (must be zero) */
    uint16_t reserved3;
    /** @brief Reserved field (must be zero) */
    uint16_t iomap_base;
} __attribute__((packed));

/**
 * @brief Global Descriptor Table (GDT) entry structure for the x86_64 architecture.
 *
 * @note This structure represents an 8-byte GDT entry.
 */
struct gdt_entry_struct
{
    /** @brief Segment limit low 16 bits */
    uint16_t limit_low;

    /** @brief Base address low 16 bits */
    uint16_t base_low;

    /** @brief Base address middle 8 bits */
    uint8_t base_middle;

    /** @brief Access flags */
    uint8_t access;

    /** @brief Flags and high 4 bits of limit */
    uint8_t flags;

    /** @brief Base address high 8 bits */
    uint8_t base_high;

} __attribute__((packed));

/**
 * @brief Pointer structure for the Global Descriptor Table (GDT).
 *
 * @note This structure is used to load the GDT using the lgdt instruction.
 */
struct gdt_ptr_struct
{
    /** @brief Size of the GDT (limit) */
    uint16_t limit;
    /** @brief Base address of the GDT */
    uint64_t base;
} __attribute__((packed));

/**
 * @brief Initialize the Global Descriptor Table (GDT) and the Task State Segment (TSS).
 */
void init_gdt();

/**
 * @brief Set a GDT entry.
 *
 * @param num The index of the GDT entry to set.
 * @param base The base address of the segment.
 * @param limit The limit of the segment.
 * @param access The access flags for the segment.
 * @param gran The granularity flags for the segment.
 */
void set_gdt_gate(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

/**
 * @brief Write to the Task State Segment (TSS) for a specific GDT entry.
 *
 * @param num The index of the TSS entry in the GDT.
 * @param rsp0 The stack pointer for ring 0 (kernel).
 * @note This function updates the TSS entry in the GDT with the provided stack pointer.
 */
void write_tss(uint32_t num, uint64_t rsp0);

/**
 * @brief Set the kernel stack pointer in the Task State Segment (TSS).
 *
 * @param rsp0 The stack pointer for ring 0 (kernel).
 * @note This function updates the TSS with the new kernel stack pointer.
 */
void set_kernel_stack(uint64_t rsp0);

/**
 * @brief Flush the Global Descriptor Table (GDT) by loading the new GDT pointer.
 *
 * @param ptr Pointer to the GDT pointer structure.
 * @note This function uses the lgdt instruction to load the new GDT.
 */
void gdt_flush(struct gdt_ptr_struct *ptr);

/**
 * @brief Flush the Task State Segment (TSS) by loading the new TSS selector.
 *
 * @note This function uses the ltr instruction to load the new TSS.
 */
void tss_flush();

#endif
