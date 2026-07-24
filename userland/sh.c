#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LINE_MAX 128
#define BIN_PREFIX "/bin/"
#define MAX_ARGS 8

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
  char path[sizeof(BIN_PREFIX) - 1 + LINE_MAX];
  char *argv[MAX_ARGS + 1];

  for (;;) {
    printf("> ");

    int len = read_line(line, sizeof(line));
    if (len == 0) {
      continue;
    }

    // Tokenize the line into argv in place: each space becomes a '\0',
    // and argv[i] points at the start of the i-th token within line.
    char *p = line;
    while (*p == ' ') {
      p++;
    }
    if (*p == '\0') {
      continue;
    }

    int argc = 0;
    while (*p && argc < MAX_ARGS) {
      argv[argc++] = p;
      while (*p && *p != ' ') {
        p++;
      }
      if (*p) {
        *p++ = '\0';
      }
      while (*p == ' ') {
        p++;
      }
    }
    argv[argc] = NULL;

    if (strcmp(argv[0], "exit") == 0) {
      exit(0);
    }

    size_t cmd_len = strlen(argv[0]);
    memcpy(path, BIN_PREFIX, sizeof(BIN_PREFIX) - 1);
    memcpy(path + sizeof(BIN_PREFIX) - 1, argv[0], cmd_len + 1);

    int pid = fork();
    if (pid < 0) {
      printf("sh: fork failed\n");
      continue;
    }

    if (pid == 0) {
      execve(path, argv);
      // Only reached if execve() failed.
      printf("sh: %s: not found\n", argv[0]);
      exit(1);
    }

    int status = 0;
    wait(pid, &status);
  }
}
