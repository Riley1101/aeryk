#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Exercises sprintf()'s format specifiers (%c %s %d %u %x %p %%) against the
// shared __vcbprintf core it now shares with printf(), plus NUL-termination
// and the returned character count.

static int failures = 0;

static void check(const char *label, const char *got, const char *want) {
  if (strcmp(got, want) != 0) {
    printf("sprintftest: FAIL %s: got \"%s\" want \"%s\"\n", label, got, want);
    failures++;
  } else {
    printf("sprintftest: ok %s -> \"%s\"\n", label, got);
  }
}

void main(void) {
  printf("sprintftest: start\n");

  char buf[64];

  int n = sprintf(buf, "%s=%d", "answer", 42);
  check("%s=%d", buf, "answer=42");
  if (n != 9) {
    printf("sprintftest: FAIL return value: got %d want 9\n", n);
    failures++;
  }

  sprintf(buf, "%c%c%c", 'f', 'o', 'o');
  check("%c%c%c", buf, "foo");

  sprintf(buf, "%u", 4294967295U);
  check("%u", buf, "4294967295");

  sprintf(buf, "%d", -12345);
  check("%d", buf, "-12345");

  sprintf(buf, "%x", 0xdeadbeefU);
  check("%x", buf, "deadbeef");

  sprintf(buf, "100%%");
  check("100%%", buf, "100%");

  sprintf(buf, "%p", (void *)0x1000);
  check("%p", buf, "0x1000");

  // Writing a second, shorter string must not leave stale bytes past the new
  // NUL terminator.
  sprintf(buf, "%s", "this is a longer string");
  sprintf(buf, "%s", "short");
  check("reuse buffer", buf, "short");

  if (failures == 0) {
    printf("sprintftest: all checks passed\n");
  } else {
    printf("sprintftest: %d check(s) failed\n", failures);
  }

  exit(failures == 0 ? 0 : 1);
}
