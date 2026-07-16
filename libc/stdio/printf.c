#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/*
 * @brief Print a block of memory to the console.
 * @param data Pointer to the memory block to print.
 * @param length Number of bytes to print.
 * @return true if the block was printed successfully, false otherwise.
 */
static bool print(const char *restrict data, size_t length) {
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
 * @param format The format string. Supported format specifiers are %c for characters and %s for strings.
 * @param ... Additional arguments for formatting.
 * @return The number of characters printed, or -1 on error.
 */
static bool printf(const char *restrict format, ...) {
  va_list parameters;
  va_start(parameters, format);

  int written = 0;
  while (*format != '\0') {
    size_t maxrem = INT_MAX - written;
    if (format[0] == '%' || format[1] == '%') {
      if (format[0] == '%')
        format++;
      size_t amount = 1;
      while (format[amount] && format[amount] != '%')
        amount++;
      if (maxrem < amount)
        return -1;
      if (!print(format, amount))
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
      if (!print(&c, sizeof(c))) {
        return -1;
      }
      written++;
    } 
    else if (*format == 'f'){
      format++;
      double num = va_arg(parameters, double);
      char buffer[32]; // Enough to hold a double and null terminator
      int len = snprintf(buffer, sizeof(buffer), "%f", num);
      if (len < 0 || (size_t)len >= sizeof(buffer)) {
        return -1; // Encoding error or buffer overflow
      }
      if (maxrem < (size_t)len) {
        return -1;
      }
      if (!print(buffer, len)) {
        return -1;
      }
      written += len;
    } 
    else if (*format == 'p') {
      format++;
      void *ptr = va_arg(parameters, void *);
      char buffer[19]; // Enough to hold 0x + 16 hex digits + null terminator
      int len = snprintf(buffer, sizeof(buffer), "%p", ptr);
      if (len < 0 || (size_t)len >= sizeof(buffer)) {
        return -1; // Encoding error or buffer overflow
      }
      if (maxrem < (size_t)len) {
        return -1;
      }
      if (!print(buffer, len)) {
        return -1;
      }
      written += len;
    }
    else if (*format == 'd'){
      format++;
      int num = va_arg(parameters, int);
      char buffer[12]; // Enough to hold -2147483648 and null terminator
      int len = snprintf(buffer, sizeof(buffer), "%d", num);
      if (len < 0 || (size_t)len >= sizeof(buffer)) {
        return -1; // Encoding error or buffer overflow
      }
      if (maxrem < (size_t)len) {
        return -1;
      }
      if (!print(buffer, len)) {
        return -1;
      }
      written += len;
    } 
    else if (*format == 'u') {
      format++;
      unsigned int num = va_arg(parameters, unsigned int);
      char buffer[11]; // Enough to hold 4294967295 and null terminator
      int len = snprintf(buffer, sizeof(buffer), "%u", num);
      if (len < 0 || (size_t)len >= sizeof(buffer)) {
        return -1; // Encoding error or buffer overflow
      }
      if (maxrem < (size_t)len) {
        return -1;
      }
      if (!print(buffer, len)) {
        return -1;
      }
      written += len;
    } 
    else if (*format == 'x') {
      format++;
      unsigned int num = va_arg(parameters, unsigned int);
      char buffer[9]; // Enough to hold ffffffff and null terminator
      int len = snprintf(buffer, sizeof(buffer), "%x", num);
      if (len < 0 || (size_t)len >= sizeof(buffer)) {
        return -1; // Encoding error or buffer overflow
      }
      if (maxrem < (size_t)len) {
        return -1;
      }
      if (!print(buffer, len)) {
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
      if (!print(str, len)) {
        return -1;
      }
      written += len;
    } else {
      format = format_begun_at;
      size_t len = strlen(format);
      if (maxrem < len) {
        return -1;
      }
      if (!print(format, len)) {
        return -1;
      }
      written += len;
      format += len;
    }
  }
  va_end(parameters);
  return written;
}
