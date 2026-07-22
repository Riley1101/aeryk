#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LINE_MAX 128
#define BIN_PREFIX "/bin/"

/**
 * @brief Reads one line of input from stdin (fd 0) into buf.
 *
 * The keyboard driver's read() blocks until at least one byte is
 * available and stops early on '\n', so a single read() call is enough
 * to collect a full line. Strips the trailing newline if present.
 *
 * @return The number of characters in the line, excluding the newline.
 */
static int read_line(char *buf, int size) {
  ssize_t n = read(0, buf, size - 1);
  if (n < 0) {
    n = 0;
  }
  if (n > 0 && buf[n - 1] == '\n') {
    n--;
  }
  buf[n] = '\0';
  return (int)n;
}

void main(void) {
  char line[LINE_MAX];
  char cmd[LINE_MAX];
  char path[sizeof(BIN_PREFIX) - 1 + LINE_MAX];

  for (;;) {
    printf("> ");

    int len = read_line(line, sizeof(line));
    if (len == 0) {
      continue;
    }

    char *p = line;
    while (*p == ' ') {
      p++;
    }
    if (*p == '\0') {
      continue;
    }

    char *cmd_start = p;
    while (*p && *p != ' ') {
      p++;
    }
    int cmd_len = (int)(p - cmd_start);

    while (*p == ' ') {
      p++;
    }
    char *args = (*p != '\0') ? p : NULL;

    memcpy(cmd, cmd_start, cmd_len);
    cmd[cmd_len] = '\0';

    if (strcmp(cmd, "exit") == 0) {
      exit(0);
    }

    memcpy(path, BIN_PREFIX, sizeof(BIN_PREFIX) - 1);
    memcpy(path + sizeof(BIN_PREFIX) - 1, cmd, cmd_len + 1);

    int pid = spawn(path, args);
    if (pid < 0) {
      printf("sh: %s: not found\n", cmd);
      continue;
    }

    int status = 0;
    wait(pid, &status);
  }
}
