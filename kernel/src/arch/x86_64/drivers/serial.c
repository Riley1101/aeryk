#include "arch/x86_64/drivers/serial.h"
#include <utils.h>

// https://wiki.osdev.org/Serial_Ports
#define COM1 0x3F8

// Example at https://wiki.osdev.org/Serial_Ports
void init_serial() {
  out_portb(COM1 + 1, 0x00); // Disables all interrupts
  out_portb(COM1 + 3, 0x80); // Enable DLAB
  out_portb(COM1 + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
  out_portb(COM1 + 1, 0x00); //                  (hi byte) 38400 baud
  out_portb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
  out_portb(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
  out_portb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
  out_portb(COM1 + 4, 0x1E); // Set in loopback mode, test serial chip
  out_portb(
      COM1 + 4,
      0xAE); // Test srial chip (send byte 0xAE and check if returns the same)
}

void serial_putchar(char c) {
  while (is_transmit_empty(COM1) == 0) {
    out_portb(COM1, c);
  };
}

void serial_print(const char *c) {
  while (*c) {
    serial_putchar(*c++);
  }
}

int is_transmit_empty(int port) { return in_portb(port) & 0x20; }
