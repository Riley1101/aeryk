#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// LAPIC timer IRQ0 frequency (init_timer() programs the LAPIC to this),
// i.e. `ticks` advances by 1 every 1000/TIMER_HZ ms. Shared with syscall.c
// for converting a userland sleep_ms() argument to a tick count.
#define TIMER_HZ 100

extern volatile uint64_t ticks;

/**
 * @brief CPU cycles per second, calibrated once at boot in init_timer()
 * against the same PIT 10ms one-shot reference lapic_calibrate() uses.
 * 0 until init_timer() has run. Divide an rdtsc() (utils.h) delta by this
 * to convert a benchmark's cycle count to seconds.
 */
extern volatile uint64_t tsc_hz;

void init_timer(void);

#endif // TIMER_H
