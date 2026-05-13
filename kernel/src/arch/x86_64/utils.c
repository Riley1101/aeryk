#include <stdint.h>
#include <utils.h>

// write some data to port chips
void out_portb(uint16_t port, uint8_t value) {
  asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

// write some data to port chips
uint8_t in_portb(uint16_t port) {
  char rv;
  asm volatile("inb %1, %0" : "=a"(rv) : "dN"(port));
  return rv;
}
