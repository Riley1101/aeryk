#include "timer.h"
#include <apic.h>
#include <idt.h>
#include <process.h>
#include <scheduler.h>
#include <stdint.h>
#include <tty.h>
#include <utils.h>

#define PIT_CMD 0x43
#define PIT_CHANNEL0 0x40
#define PIT_10MS 11931

volatile uint64_t ticks;
volatile uint64_t tsc_hz;

const uint32_t freq = 100;

void on_irq0(struct interrupt_frame *frame) {
  (void)frame;
  ticks += 1;
  mlfq_on_tick();
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

// Same PIT one-shot 10ms reference as lapic_calibrate(), just bracketed
// with rdtsc() instead of reading the LAPIC's countdown. Kept as its own
// PIT program rather than folded into lapic_calibrate() -- the two
// calibrations don't need to share a single 10ms window, and duplicating
// the few lines is cheaper than threading an extra output through it.
static void calibrate_tsc(void) {
  out_portb(PIT_CMD, 0x30);
  out_portb(PIT_CHANNEL0, PIT_10MS & 0xFF);
  out_portb(PIT_CHANNEL0, PIT_10MS >> 8);

  uint64_t start = rdtsc();

  uint8_t status;
  do {
    out_portb(PIT_CMD, 0xE2);
    status = in_portb(PIT_CHANNEL0);
  } while (!(status & 0x80));

  uint64_t end = rdtsc();

  tsc_hz = (end - start) * 100; // delta was over a 10ms window
}

void init_timer() {
  ticks = 0;

  calibrate_tsc();

  lapic_write(LAPIC_TIMER_DIV, 0x03);

  uint32_t ticks_per_10ms = lapic_calibrate();

  lapic_write(LAPIC_TIMER_LVT, 32 | 0x20000);

  lapic_write(LAPIC_TIMER_INITCNT,
              (uint32_t)((uint64_t)ticks_per_10ms * 100 / freq));

  irq_install_handler(0, on_irq0);
}

