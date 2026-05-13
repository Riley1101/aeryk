#include <apic.h>
#include <idt.h>
#include <stdint.h>
#include <tty.h>
#include <utils.h>

volatile uint64_t ticks;

const uint32_t freq = 100;

void onIrq0(struct interrupt_frame *frame) {
  (void)frame;
  ticks += 1;
}

void initTimer() {
  ticks = 0;

  lapic_write(LAPIC_TIMER_DIV, 0x03);

  lapic_write(LAPIC_TIMER_LVT, 32 | 0x20000);

  lapic_write(LAPIC_TIMER_INITCNT, 0x10000000);

  irq_install_handler(0, onIrq0);
}
