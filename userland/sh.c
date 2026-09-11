#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LINE_MAX 128
#define BIN_PREFIX "/bin/"
#define MAX_ARGS 8
#define MAX_STAGES 8

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

/**
 * @brief Tokenizes a single pipeline stage in place: each space becomes a
 * '\0', and argv[i] points at the start of the i-th token within s.
 *
 * @return The number of tokens, or 0 if the stage is empty/whitespace-only.
 */
static int tokenize(char *s, char *argv[MAX_ARGS + 1]) {
  while (*s == ' ') {
    s++;
  }

  int argc = 0;
  while (*s && argc < MAX_ARGS) {
    argv[argc++] = s;
    while (*s && *s != ' ') {
      s++;
    }
    if (*s) {
      *s++ = '\0';
    }
    while (*s == ' ') {
      s++;
    }
  }
  argv[argc] = NULL;
  return argc;
}

/**
 * @brief Runs a single pipeline stage: wires its stdin/stdout onto the
 * neighboring pipes (if any), closes every pipe fd, and execve()s it.
 * Never returns: exits 1 if the binary can't be found.
 */
static void run_stage(char *const argv[], const char *path,
                      int pipefds[MAX_STAGES - 1][2], int nstages, int i) {
  if (i > 0) {
    dup2(pipefds[i - 1][0], 0);
  }
  if (i < nstages - 1) {
    dup2(pipefds[i][1], 1);
  }
  for (int j = 0; j < nstages - 1; j++) {
    close(pipefds[j][0]);
    close(pipefds[j][1]);
  }

  execve(path, argv);
  // Only reached if execve() failed.
  printf("sh: %s: not found\n", argv[0]);
  exit(1);
}

void main(void) {
  char line[LINE_MAX];
  char paths[MAX_STAGES][sizeof(BIN_PREFIX) - 1 + LINE_MAX];
  char *argv[MAX_STAGES][MAX_ARGS + 1];

  for (;;) {
    printf("> ");

    int len = read_line(line, sizeof(line));
    if (len == 0) {
      continue;
    }

    // Split the line into pipeline stages on '|'.
    char *stages[MAX_STAGES];
    int nstages = 1;
    stages[0] = line;
    for (char *p = line; *p; p++) {
      if (*p == '|') {
        *p = '\0';
        if (nstages < MAX_STAGES) {
          stages[nstages++] = p + 1;
        }
      }
    }

    bool bad = false;
    for (int i = 0; i < nstages; i++) {
      if (tokenize(stages[i], argv[i]) == 0) {
        bad = true;
      }
    }
    if (bad) {
      printf("sh: syntax error\n");
      continue;
    }

    if (nstages == 1 && strcmp(argv[0][0], "exit") == 0) {
      exit(0);
    }

    for (int i = 0; i < nstages; i++) {
      size_t cmd_len = strlen(argv[i][0]);
      memcpy(paths[i], BIN_PREFIX, sizeof(BIN_PREFIX) - 1);
      memcpy(paths[i] + sizeof(BIN_PREFIX) - 1, argv[i][0], cmd_len + 1);
    }

    int pipefds[MAX_STAGES - 1][2];
    bool pipe_failed = false;
    int npipes = 0;
    for (; npipes < nstages - 1; npipes++) {
      if (pipe(pipefds[npipes]) != 0) {
        printf("sh: pipe failed\n");
        pipe_failed = true;
        break;
      }
    }
    if (pipe_failed) {
      for (int j = 0; j < npipes; j++) {
        close(pipefds[j][0]);
        close(pipefds[j][1]);
      }
      continue;
    }

    int pids[MAX_STAGES];
    for (int i = 0; i < nstages; i++) {
      int pid = fork();
      if (pid < 0) {
        printf("sh: fork failed\n");
        pids[i] = -1;
        continue;
      }
      if (pid == 0) {
        run_stage(argv[i], paths[i], pipefds, nstages, i);
      }
      pids[i] = pid;
    }

    for (int j = 0; j < nstages - 1; j++) {
      close(pipefds[j][0]);
      close(pipefds[j][1]);
    }

    for (int i = 0; i < nstages; i++) {
      if (pids[i] < 0) {
        continue;
      }
      int status = 0;
      wait(pids[i], &status);
    }
  }
}
