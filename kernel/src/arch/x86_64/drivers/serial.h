#ifndef SERIAL_H
#define SERIAL_H

/**
 * @brief Initializes the serial port for communication.
 * Configures the serial port settings and prepares it for data transmission.
 * @return 0 on success, -1 if the loopback self-test failed.
 */
int init_serial();

/**
 * @brief Reports whether init_serial() completed its self-test successfully.
 * @return Non-zero if the port is initialized and safe to write to.
 */
int serial_is_ready(void);

/**
 * @brief Sends a single character over the serial port.
 * @param c The character to send.
 */
void serial_putchar(char c);

/**
 * @brief Sends a null-terminated string over the serial port.
 * @param c The string to send.
 */
void serial_print(const char *c);

/**
 * @brief Checks if the serial port's transmit buffer is empty.
 * @param port The I/O port address of the serial port.
 * @return 1 if the transmit buffer is empty, 0 otherwise.
 */
int is_transmit_empty(int port);

#endif // !SERIAL_H
