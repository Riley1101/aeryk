#ifndef UTILS
#define UTILS

#include <stdint.h>

void out_portb(uint16_t port, uint8_t value);
uint8_t in_portb(uint16_t port);

void wrmsr(uint32_t msr, uint64_t val);
uint64_t rdmsr(uint32_t msr);

/**
 * @brief Reads the CPU's timestamp counter: a free-running cycle count
 * with no syscall/IRQ overhead, the standard way to time a short span of
 * code (bracket it with two calls and take the difference). Divide by
 * tsc_hz (timer.h, calibrated once at boot) to convert to seconds.
 */
static inline uint64_t rdtsc(void) {
  uint32_t lo, hi;
  asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
}

#endif
