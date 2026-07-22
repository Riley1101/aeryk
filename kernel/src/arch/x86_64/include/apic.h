#ifndef APIC_H
#define APIC_H

/**
 * @file apic.c
 * @brief Implements functions for interacting with the Local APIC and IO APIC on x86
 * @see https://web.archive.org/web/20070112195752/http://developer.intel.com/design/pentium/datashts/24201606.pdf
 */

// local APIC
#include <stdint.h>
#define LAPIC_BASE 0xFEE00000ULL
#define IOAPIC_BASE 0xFEC00000ULL

#define LAPIC_ID 0x020
#define LAPIC_TPR 0x080
#define LAPIC_EOI 0x0B0
#define LAPIC_SVR 0x0F0
#define LAPIC_TIMER_LVT 0x320
#define LAPIC_TIMER_INITCNT 0x380
#define LAPIC_TIMER_CURCNT 0x390
#define LAPIC_TIMER_DIV 0x3E0

/**
 * @brief Initializes the APIC by mapping the LAPIC and IOAPIC into the virtual address space,
 *       masking all interrupts, and enabling the APIC.
 */
void init_apic(void);

/**
 * @brief Writes a value to the specified LAPIC register.
 * @param reg The LAPIC register offset to write to.
 * @param value The value to write to the LAPIC register.
 */
void lapic_write(uint32_t reg, uint32_t value);

/**
 * @brief Reads the value from the specified LAPIC register.
 * @param reg The LAPIC register offset to read from.
 * @return The value read from the LAPIC register.
 */
uint32_t lapic_read(uint32_t reg);

/**
 * @brief Sends an End of Interrupt (EOI) signal to the LAPIC, indicating that the current interrupt has been handled.
 */
void lapic_eoi(void);

// IO apic
/**
 * @brief Writes a value to the specified IOAPIC register.
 * @param reg The IOAPIC register offset to write to.
 * @param value The value to write to the IOAPIC register.
 */
void ioapic_set_irq(uint8_t irq, uint64_t apic_id, uint8_t vector);

/**
 * @brief Writes a value to the specified IOAPIC register.
 * @param reg The IOAPIC register offset to write to.
 * @param value The value to write to the IOAPIC register.
 */
void ioapic_write(uint32_t reg, uint32_t value);

#endif // !APIC_H
