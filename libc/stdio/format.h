#ifndef LIBC_STDIO_FORMAT_H
#define LIBC_STDIO_FORMAT_H 1

#include <stdarg.h>
#include <stddef.h>

/*
 * @brief Sink callback used by __vcbprintf to emit formatted output.
 * @param ctx Opaque context passed through from the caller.
 * @param data Pointer to the bytes to emit.
 * @param length Number of bytes to emit.
 * @return true if the bytes were accepted, false to abort formatting.
 */
typedef bool (*format_sink)(void *ctx, const char *data, size_t length);

/*
 * @brief Shared printf-style format parser used by printf and sprintf.
 * @param sink Callback invoked with each chunk of formatted output.
 * @param ctx Opaque context forwarded to sink.
 * @param format The format string. Supported format specifiers are %c, %s,
 * %d, %u, %x and %p.
 * @param parameters Arguments for formatting, already started with va_start.
 * @return The number of characters formatted, or -1 on error.
 */
int __vcbprintf(format_sink sink, void *ctx, const char *restrict format, va_list parameters);

#endif
