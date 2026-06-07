#ifndef APIC_H
#define APIC_H

// https://web.archive.org/web/20070112195752/http://developer.intel.com/design/pentium/datashts/24201606.pdf
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

void init_apic(void);
void lapic_write(uint32_t reg, uint32_t value);
uint32_t lapic_read(uint32_t reg);
void lapic_eoi(void);

// IO apic
void ioapic_set_irq(uint8_t irq, uint64_t apic_id, uint8_t vector);
void ioapic_write(uint32_t reg, uint32_t value);

#endif // !APIC_H
