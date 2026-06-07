#ifndef UTILS
#define UTILS

#include <stdint.h>

void out_portb(uint16_t port, uint8_t value);
uint8_t in_portb(uint16_t port);

void wrmsr(uint32_t msr, uint64_t val);
uint64_t rdmsr(uint32_t msr);

#endif
