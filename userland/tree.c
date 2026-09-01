#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_NAME 64
#define MAX_PATH 128
#define MAX_DEPTH 16

static void print_indent(int depth) {
  for (int i = 0; i < depth; i++) {
    printf("  ");
  }
}

// No sprintf/snprintf in this libc yet, so build "parent/name" by hand.
static void build_child_path(const char *parent, const char *name, char *out,
                              size_t out_size) {
  size_t i = 0;
  size_t plen = strlen(parent);
  for (; i < plen && i < out_size - 1; i++) {
    out[i] = parent[i];
  }
  if (i == 0 || out[i - 1] != '/') {
    if (i < out_size - 1) {
      out[i++] = '/';
    }
  }
  size_t nlen = strlen(name);
  for (size_t k = 0; k < nlen && i < out_size - 1; k++, i++) {
    out[i] = name[k];
  }
  out[i] = '\0';
}

// listdir() packs each entry as "name\n", or "name/\n" for a
// subdirectory (see SYS_readdir in syscall.c) -- parse that back out
// entry by entry, recursing into subdirectories as they're found.
static void tree(const char *path, int depth) {
  if (depth > MAX_DEPTH) {
    return;
  }

  char buf[512];
  ssize_t n = listdir(path, buf, sizeof(buf));
  if (n < 0) {
    return;
  }

  size_t i = 0;
  while (i < (size_t)n) {
    size_t start = i;
    while (i < (size_t)n && buf[i] != '\n') {
      i++;
    }
    size_t len = i - start;
    if (i < (size_t)n) {
      i++; // skip the '\n'
    }

    int is_dir = 0;
    if (len > 0 && buf[start + len - 1] == '/') {
      is_dir = 1;
      len--;
    }
    if (len == 0) {
      continue;
    }

    char name[MAX_NAME];
    if (len >= sizeof(name)) {
      len = sizeof(name) - 1;
    }
    memcpy(name, buf + start, len);
    name[len] = '\0';

    print_indent(depth);
    printf("%s%s\n", name, is_dir ? "/" : "");

    if (is_dir) {
      char child_path[MAX_PATH];
      build_child_path(path, name, child_path, sizeof(child_path));
      tree(child_path, depth + 1);
    }
  }
}

int main(int argc, char *argv[]) {
  const char *path = argc > 1 ? argv[1] : "/";
  printf("%s\n", path);
  tree(path, 1);
  return 0;
}
