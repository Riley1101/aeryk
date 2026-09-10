#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "format.h"

/*
 * @brief Render an unsigned integer into buf in the given base.
 * @param value The value to render.
 * @param buf Destination buffer, must be large enough for the result.
 * @param base Numeric base, between 2 and 16.
 * @param uppercase Whether hex digits above 9 should be uppercase.
 * @return The number of characters written to buf (not NUL-terminated).
 */
static int utoa(unsigned long value, char *buf, unsigned base, bool uppercase) {
  static const char lower[] = "0123456789abcdef";
  static const char upper[] = "0123456789ABCDEF";
  const char *digits = uppercase ? upper : lower;

  char tmp[32];
  int len = 0;
  do {
    tmp[len++] = digits[value % base];
    value /= base;
  } while (value != 0);

  for (int i = 0; i < len; i++) {
    buf[i] = tmp[len - 1 - i];
  }
  return len;
}

int __vcbprintf(format_sink sink, void *ctx, const char *restrict format, va_list parameters) {
  int written = 0;
  while (*format != '\0') {
    size_t maxrem = INT_MAX - written;
    if (format[0] != '%' || format[1] == '%') {
      if (format[0] == '%')
        format++;
      size_t amount = 1;
      while (format[amount] && format[amount] != '%')
        amount++;
      if (maxrem < amount)
        return -1;
      if (!sink(ctx, format, amount))
        return -1;
      format += amount;
      written += amount;
      continue;
    }

    const char *format_begun_at = format++;
    if (*format == 'c') {
      format++;
      char c = (char)va_arg(parameters, int);
      if (!maxrem) {
        return -1;
      }
      if (!sink(ctx, &c, sizeof(c))) {
        return -1;
      }
      written++;
    }
    else if (*format == 'p') {
      format++;
      void *ptr = va_arg(parameters, void *);
      char buffer[2 + sizeof(uintptr_t) * 2];
      buffer[0] = '0';
      buffer[1] = 'x';
      int len = 2 + utoa((unsigned long)(uintptr_t)ptr, buffer + 2, 16, false);
      if (maxrem < (size_t)len) {
        return -1;
      }
      if (!sink(ctx, buffer, len)) {
        return -1;
      }
      written += len;
    }
    else if (*format == 'd') {
      format++;
      int num = va_arg(parameters, int);
      char buffer[12]; // Enough to hold -2147483648 and null terminator
      int len = 0;
      unsigned long mag = (num < 0) ? -(unsigned long)num : (unsigned long)num;
      if (num < 0) {
        buffer[len++] = '-';
      }
      len += utoa(mag, buffer + len, 10, false);
      if (maxrem < (size_t)len) {
        return -1;
      }
      if (!sink(ctx, buffer, len)) {
        return -1;
      }
      written += len;
    }
    else if (*format == 'u') {
      format++;
      unsigned int num = va_arg(parameters, unsigned int);
      char buffer[10]; // Enough to hold 4294967295
      int len = utoa(num, buffer, 10, false);
      if (maxrem < (size_t)len) {
        return -1;
      }
      if (!sink(ctx, buffer, len)) {
        return -1;
      }
      written += len;
    }
    else if (*format == 'x') {
      format++;
      unsigned int num = va_arg(parameters, unsigned int);
      char buffer[8]; // Enough to hold ffffffff
      int len = utoa(num, buffer, 16, false);
      if (maxrem < (size_t)len) {
        return -1;
      }
      if (!sink(ctx, buffer, len)) {
        return -1;
      }
      written += len;
    }
    else if (*format == 's') {
      format++;
      const char *str = va_arg(parameters, const char *);
      size_t len = strlen(str);
      if (maxrem < len) {
        return -1;
      }
      if (!sink(ctx, str, len)) {
        return -1;
      }
      written += len;
    } else {
      format = format_begun_at;
      size_t len = strlen(format);
      if (maxrem < len) {
        return -1;
      }
      if (!sink(ctx, format, len)) {
        return -1;
      }
      written += len;
      format += len;
    }
  }
  return written;
}
