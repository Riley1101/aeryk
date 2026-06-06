#ifndef SERIAL_H
#define SERIAL_H
void init_serial();
void serial_putchar(char c);
void serial_print(const char *c);

int is_transmit_empty(int port);

#endif // !SERIAL_H
