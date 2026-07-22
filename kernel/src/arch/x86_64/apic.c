#include <apic.h>
#include <pmm.h>
#include <stdint.h>
#include <utils.h>
#include <vmm.h>

/**
 * @file apic.c
 * @brief Implements functions for interacting with the Local APIC and IO APIC on x86
 * @see https://web.archive.org/web/20070112195752/http://developer.intel.com/design/pentium/datashts/24201606.pdf
 */

/**
 * @brief Writes a value to a specific register in the Local APIC.
 * @param reg The register to write to.
 * @param value The value to write.
 */
void lapic_write(uint32_t reg, uint32_t value) {
  *((volatile uint32_t *)(LAPIC_BASE + hhdm_offset + reg)) = value;
}

/**
 * @brief Reads the value from a specific register in the Local APIC.
 * @param reg The register to read from.
 * @return The value read from the specified register.
 */
uint32_t lapic_read(uint32_t reg) {
  return *((volatile uint32_t *)(LAPIC_BASE + hhdm_offset + reg));
}

/**
 * @brief Sends an End of Interrupt (EOI) signal to the Local APIC, indicating that the current interrupt has been handled.
 * This function writes a value of 0 to the EOI register of the Local APIC, which is located at the LAPIC_EOI offset from the LAPIC_BASE address. 
 * It is typically called at the end of an interrupt service routine to inform the APIC that it can process further interrupts.
 */
void lapic_eoi(void) { lapic_write(LAPIC_EOI, 0); }

// io apic

/**
 * @brief Writes a value to a specific register in the IO APIC.
 * @param reg The register to write to.
 * @param value The value to write.
 */
void ioapic_write(uint32_t reg, uint32_t value) {
  volatile uint32_t *base = (volatile uint32_t *)(IOAPIC_BASE + hhdm_offset);
  base[0] = reg;
  base[4] = value;
}

/**
 * @brief Sets the mapping of a specific IRQ to a given APIC ID and interrupt vector in the IO APIC.
 * @param irq The IRQ number to configure (0-23).
 * @param apic_id The APIC ID of the target CPU to which the IRQ should be routed.
 * @param vector The interrupt vector number to which the IRQ should be mapped (32-255).
 */
void ioapic_set_irq(uint8_t irq, uint64_t apic_id, uint8_t vector) {
  uint32_t low_index = 0x10 + irq * 2;
  uint32_t high_index = 0x11 + irq * 2;

  ioapic_write(low_index, vector);
  ioapic_write(high_index, apic_id << 24);
}

/**
 * @brief Initializes the Local APIC and IO APIC by mapping their physical addresses to virtual addresses, disabling legacy PICs, enabling the Local APIC, 
 * and setting the Task Priority Register (TPR) to allow all interrupts.
 */
void init_apic() {
  vmm_map_page(vmm_get_kernel_pml4(), LAPIC_BASE + hhdm_offset, LAPIC_BASE,
               PTE_PRESENT | PTE_WRITABLE);

  vmm_map_page(vmm_get_kernel_pml4(), IOAPIC_BASE + hhdm_offset, IOAPIC_BASE,
               PTE_PRESENT | PTE_WRITABLE);

  out_portb(0x21, 0xFF);
  out_portb(0xA1, 0xFF);

  lapic_write(LAPIC_SVR, lapic_read(LAPIC_SVR) | 0x1FF);
  lapic_write(LAPIC_TPR, 0);
}
