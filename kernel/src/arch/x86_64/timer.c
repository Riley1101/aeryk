#include <apic.h>
#include <idt.h>
#include <stdint.h>
#include <tty.h>
#include <utils.h>

#define PIT_CMD 0x43
#define PIT_CHANNEL0 0x40
#define PIT_10MS 11931

volatile uint64_t ticks;

const uint32_t freq = 100;

void onIrq0(struct interrupt_frame *frame) {
  (void)frame;
  ticks += 1;
}

// https://wiki.osdev.org/APIC_Timer
static uint32_t lapic_calibrate(void) {
  lapic_write(LAPIC_TIMER_LVT, 0x10000);
  lapic_write(LAPIC_TIMER_INITCNT, 0xFFFFFFFF);

  out_portb(PIT_CMD, 0x30);
  out_portb(PIT_CHANNEL0, PIT_10MS & 0xFF);
  out_portb(PIT_CHANNEL0, PIT_10MS >> 8);

  uint8_t status;
  do {
    out_portb(PIT_CMD, 0xE2);
    status = in_portb(PIT_CHANNEL0);
  } while (!(status & 0x80));

  return 0xFFFFFFFF - lapic_read(LAPIC_TIMER_CURCNT);
}

void initTimer() {
  ticks = 0;

  lapic_write(LAPIC_TIMER_DIV, 0x03);

  uint32_t ticks_per_10ms = lapic_calibrate();

  lapic_write(LAPIC_TIMER_LVT, 32 | 0x20000);

  lapic_write(LAPIC_TIMER_INITCNT,
              (uint32_t)((uint64_t)ticks_per_10ms * 100 / freq));

  irq_install_handler(0, onIrq0);
}
