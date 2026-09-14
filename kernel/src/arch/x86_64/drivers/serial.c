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

static int serial_ready = 0;

/**
 * @brief Initializes the serial port (COM1) for communication.
 * Configures the baud rate, data bits, parity, and stop bits, then
 * self-tests the chip in loopback mode before trusting it for output.
 *
 * @see https://wiki.osdev.org/Serial_Ports#Example
 * @return 0 on success, -1 if the loopback self-test failed (chip not
 * present or not responding), in which case serial_putchar() is a no-op.
 */
int init_serial() {
  out_portb(COM1 + 1, 0x00); // Disables all interrupts
  out_portb(COM1 + 3, 0x80); // Enable DLAB
  out_portb(COM1 + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
  out_portb(COM1 + 1, 0x00); //                  (hi byte) 38400 baud
  out_portb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
  out_portb(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
  out_portb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set

  // Loopback self-test: internally wire TX back to RX and confirm a byte
  // written to the data register reads back unchanged.
  out_portb(COM1 + 4, 0x1E);
  out_portb(COM1 + 0, 0xAE);
  if (in_portb(COM1 + 0) != 0xAE) {
    serial_ready = 0;
    return -1;
  }

  // Self-test passed -- leave loopback mode for normal operation
  // (DTR/RTS asserted, OUT1/OUT2 asserted so the line is actually driven).
  out_portb(COM1 + 4, 0x0F);

  serial_ready = 1;
  return 0;
}

/**
 * @brief Reports whether init_serial() completed its self-test successfully.
 * @return int Non-zero if the port is initialized and safe to write to.
 */
int serial_is_ready(void) { return serial_ready; }

/**
 * @brief Sends a character over the serial port (COM1).
 * Waits until the transmit buffer is empty before sending.
 *
 * @param c The character to send.
 * @return void
 */
void serial_putchar(char c) {
  if (!serial_ready) {
    return;
  }
  while (is_transmit_empty(COM1) == 0) {
  };
  out_portb(COM1, c);
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
int is_transmit_empty(int port) { return in_portb(port + 5) & 0x20; }
