#include <stdio.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief Writes a string followed by a newline to stdout.
 * @param s The string to write.
 * @return Returns 0 on success or EOF on error.
 */
int puts(const char *restrict s) {
  size_t len = strlen(s);
  if (write(1, s, len) != (ssize_t)len) {
    return EOF;
  }
  if (putchar('\n') == EOF) {
    return EOF;
  }
  return 0;
}
