#ifndef _SYS_TSC_H
#define _SYS_TSC_H 1

#include <stdint.h>

/**
 * @brief Reads the CPU's timestamp counter: a free-running cycle count,
 * readable directly from ring 3 (no syscall) since the kernel never sets
 * CR4.TSD. Bracket a span of code with two calls and take the difference,
 * then divide by get_tsc_hz() to convert to seconds -- the same pattern
 * kernel/src/arch/x86_64/include/utils.h's rdtsc() is for.
 */
static inline uint64_t rdtsc(void) {
  uint32_t lo, hi;
  asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
}

/**
 * @brief Returns the CPU's timestamp counter frequency in Hz, as
 * calibrated once at boot (kernel/src/arch/x86_64/timer.c). Divide an
 * rdtsc() delta by this to get seconds.
 */
uint64_t get_tsc_hz(void);

#endif // !_SYS_TSC_H
