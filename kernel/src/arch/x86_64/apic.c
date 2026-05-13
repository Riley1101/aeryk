#include <apic.h>
#include <pmm.h>
#include <stdint.h>
#include <utils.h>
#include <vmm.h>

void lapic_write(uint32_t reg, uint32_t value) {
  *((volatile uint32_t *)(LAPIC_BASE + hhdm_offset + reg)) = value;
}

uint32_t lapic_read(uint32_t reg) {
  return *((volatile uint32_t *)(LAPIC_BASE + hhdm_offset + reg));
}

void lapic_eoi(void) { lapic_write(LAPIC_EOI, 0); }

// io apic

void ioapic_write(uint32_t reg, uint32_t value) {
  volatile uint32_t *base = (volatile uint32_t *)(IOAPIC_BASE + hhdm_offset);
  base[0] = reg;
  base[4] = value;
}

void ioapic_set_irq(uint8_t irq, uint64_t apic_id, uint8_t vector) {
  uint32_t low_index = 0x10 + irq * 2;
  uint32_t high_index = 0x11 + irq * 2;

  ioapic_write(low_index, vector);
  ioapic_write(high_index, apic_id << 24);
}

void initAPIC() {
  vmm_map_page(vmm_get_kernel_pml4(), LAPIC_BASE + hhdm_offset, LAPIC_BASE,
               PTE_PRESENT | PTE_WRITABLE);

  vmm_map_page(vmm_get_kernel_pml4(), IOAPIC_BASE + hhdm_offset, IOAPIC_BASE,
               PTE_PRESENT | PTE_WRITABLE);

  out_portb(0x21, 0xFF);
  out_portb(0xA1, 0xFF);

  lapic_write(LAPIC_SVR, lapic_read(LAPIC_SVR) | 0x1FF);
  lapic_write(LAPIC_TPR, 0);
}
