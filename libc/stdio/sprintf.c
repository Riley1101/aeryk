#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "format.h"

struct sprintf_ctx {
  char *buf;
};

/*
 * @brief Sink that writes formatted output into a caller-supplied buffer.
 */
static bool buffer_sink(void *ctx, const char *data, size_t length) {
  struct sprintf_ctx *sctx = (struct sprintf_ctx *)ctx;
  memcpy(sctx->buf, data, length);
  sctx->buf += length;
  return true;
}

/*
 * @brief Print a formatted string into a buffer.
 * @param str Destination buffer. Must be large enough to hold the result
 * plus a terminating NUL; the caller is responsible for sizing it.
 * @param format The format string. Supported format specifiers are %c, %s,
 * %d, %u, %x and %p.
 * @param ... Additional arguments for formatting.
 * @return The number of characters written (excluding the NUL), or -1 on
 * error.
 */
int sprintf(char *restrict str, const char *restrict format, ...) {
  va_list parameters;
  va_start(parameters, format);
  struct sprintf_ctx ctx = {.buf = str};
  int written = __vcbprintf(buffer_sink, &ctx, format, parameters);
  va_end(parameters);
  if (written >= 0) {
    ctx.buf[0] = '\0';
  }
  return written;
}
