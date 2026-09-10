#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "format.h"

/*
 * @brief Sink that writes formatted output to the console via putchar.
 */
static bool console_sink(void *ctx, const char *data, size_t length) {
  (void)ctx;
  const unsigned char *bytes = (const unsigned char *)data;
  for (size_t i = 0; i < length; i++) {
    if (putchar(bytes[i]) == EOF) {
      return false;
    }
  }
  return true;
}

/*
 * @brief Print a formatted string to the console.
 * @param format The format string. Supported format specifiers are %c, %s,
 * %d, %u, %x and %p.
 * @param ... Additional arguments for formatting.
 * @return The number of characters printed, or -1 on error.
 */
int printf(const char *restrict format, ...) {
  va_list parameters;
  va_start(parameters, format);
  int written = __vcbprintf(console_sink, NULL, format, parameters);
  va_end(parameters);
  return written;
}
