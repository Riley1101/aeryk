#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void main(void) {
  // Basic alloc + write-back-and-check.
  char *a = malloc(64);
  if (!a) {
    printf("malloctest: malloc(64) failed\n");
    exit(1);
  }
  memset(a, 'A', 64);

  // Multiple concurrent allocations shouldn't alias each other.
  char *b = malloc(128);
  char *c = malloc(32);
  if (!b || !c) {
    printf("malloctest: malloc failed\n");
    exit(1);
  }
  memset(b, 'B', 128);
  memset(c, 'C', 32);

  int ok = 1;
  for (int i = 0; i < 64; i++) if (a[i] != 'A') ok = 0;
  for (int i = 0; i < 128; i++) if (b[i] != 'B') ok = 0;
  for (int i = 0; i < 32; i++) if (c[i] != 'C') ok = 0;
  printf("malloctest: no-alias check %s\n", ok ? "PASS" : "FAIL");

  // free() + coalescing: freeing all three then asking for something
  // bigger than any individual block should succeed by merging them.
  free(a);
  free(b);
  free(c);
  char *d = malloc(200);
  printf("malloctest: post-free coalesced alloc %s\n", d ? "PASS" : "FAIL");
  if (d) {
    memset(d, 'D', 200);
    free(d);
  }

  // calloc() zeroes memory.
  int *nums = calloc(16, sizeof(int));
  ok = nums != NULL;
  if (nums) {
    for (int i = 0; i < 16; i++) if (nums[i] != 0) ok = 0;
    free(nums);
  }
  printf("malloctest: calloc zero check %s\n", ok ? "PASS" : "FAIL");

  // A big allocation forces grow_heap() through more than one sbrk().
  char *big = malloc(64 * 1024);
  printf("malloctest: large alloc (64KiB) %s\n", big ? "PASS" : "FAIL");
  if (big) {
    memset(big, 'X', 64 * 1024);
    free(big);
  }

  printf("malloctest: done\n");
  exit(0);
}
