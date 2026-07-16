#ifndef STDIO_H
#define STDIO_H 1

#include <stdbool.h>

#define EOF (-1)

static int printf(const char *restrict format, ...);

int putchar(int);
int puts(const char *restrict format);

#endif