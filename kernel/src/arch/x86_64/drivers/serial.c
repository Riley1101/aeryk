#include "serial.h"
#include <utils.h>

/**
 * @file Serial port driver for x86_64 architecture.
 * 
 * @see https://wiki.osdev.org/Serial_Ports
 * @see https://wiki.osdev.org/Serial_Ports#Example
 * 
 * @brief Initializes the serial port (COM1) for communication.
 * Provides functions to send characters and strings over the serial port.
 * 
 * @note This driver is designed for the x86_64 architecture and uses I/O port access.
 * And this is used for debugging purposes, 
 * as it allows the kernel to output messages to a serial console.
 */

// https://wiki.osdev.org/Serial_Ports
#define COM1 0x3F8

/**
 * @brief Initializes the serial port (COM1) for communication.
 * Configures the baud rate, data bits, parity, and stop bits.
 * 
 * @see https://wiki.osdev.org/Serial_Ports#Example
 * @return void
 */
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

/**
 * @brief Sends a character over the serial port (COM1).
 * Waits until the transmit buffer is empty before sending.
 * 
 * @param c The character to send.
 * @return void
 */
void serial_putchar(char c) {
  while (is_transmit_empty(COM1) == 0) {
    out_portb(COM1, c);
  };
}

/**
 * @brief Prints a string over the serial port (COM1).
 * 
 * @param c The string to print.
 * @return void
 */
void serial_print(const char *c) {
  while (*c) {
    serial_putchar(*c++);
  }
}

/**
 * @brief Checks if the transmit buffer of the serial port (COM1) is empty.
 * 
 * @param port The I/O port address of the serial port.
 * @return int Returns non-zero if the transmit buffer is empty, zero otherwise.
 */
int is_transmit_empty(int port) { return in_portb(port) & 0x20; }
