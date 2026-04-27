#ifndef STDIO_H
#define STDIO_H 1

#include <sys/cdef.h>
#include <stdbool.h>

#define EOF (-1)

static bool printf(const char *restrict format, ...);
bool putchar(int);
bool puts(const char *restrict format);

#endif