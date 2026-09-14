#include <stdint.h>
#include <string.h>

// Unaligned-safe 8-byte access: `aligned(1)` lets it sit at any byte offset
// (x86_64 doesn't fault on unaligned loads/stores, so no correctness issue,
// just avoids UB from asserting 8-byte alignment we haven't checked), and
// `may_alias` exempts it from strict-aliasing so reading/writing through
// this type when the underlying bytes came from an unrelated object (any
// buffer memcpy/memset/memmove is handed) isn't undefined behavior.
typedef uint64_t u64_unaligned __attribute__((aligned(1), may_alias));

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
  uint8_t *restrict pdest = (uint8_t *restrict)dest;
  const uint8_t *restrict psrc = (const uint8_t *restrict)src;

  // Framebuffer blits (the kernel's own scroll_up, and eventually a
  // userland compositor via libc's copy of this same routine) move
  // hundreds of KB per call -- copying 8 bytes/iteration instead of 1
  // cuts the loop-overhead-dominated cost roughly 8x.
  size_t words = n / sizeof(uint64_t);
  u64_unaligned *restrict wdest = (u64_unaligned *restrict)pdest;
  const u64_unaligned *restrict wsrc = (const u64_unaligned *restrict)psrc;
  for (size_t i = 0; i < words; i++) {
    wdest[i] = wsrc[i];
  }

  for (size_t i = words * sizeof(uint64_t); i < n; i++) {
    pdest[i] = psrc[i];
  }

  return dest;
}

void *memset(void *dest, int c, size_t n) {
  uint8_t *pdest = (uint8_t *)dest;
  uint8_t byte = (uint8_t)c;

  uint64_t pattern = byte;
  pattern |= pattern << 8;
  pattern |= pattern << 16;
  pattern |= pattern << 32;

  size_t words = n / sizeof(uint64_t);
  u64_unaligned *wdest = (u64_unaligned *)pdest;
  for (size_t i = 0; i < words; i++) {
    wdest[i] = pattern;
  }

  for (size_t i = words * sizeof(uint64_t); i < n; i++) {
    pdest[i] = byte;
  }

  return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
  uint8_t *pdest = (uint8_t *)dest;
  const uint8_t *psrc = (const uint8_t *)src;

  if (psrc > pdest) {
    size_t words = n / sizeof(uint64_t);
    u64_unaligned *wdest = (u64_unaligned *)pdest;
    const u64_unaligned *wsrc = (const u64_unaligned *)psrc;
    for (size_t i = 0; i < words; i++) {
      wdest[i] = wsrc[i];
    }
    for (size_t i = words * sizeof(uint64_t); i < n; i++) {
      pdest[i] = psrc[i];
    }
  } else if (psrc < pdest) {
    // Destination overlaps forward into source: must copy back-to-front so
    // no byte is overwritten before it's read. Peel off the tail bytes
    // that don't form a full word first (safe -- they're the highest
    // addresses, nothing above them left to preserve), then copy the
    // remaining whole words in decreasing order.
    size_t tail_bytes = n % sizeof(uint64_t);
    for (size_t i = n; i > n - tail_bytes; i--) {
      pdest[i - 1] = psrc[i - 1];
    }
    size_t words = (n - tail_bytes) / sizeof(uint64_t);
    u64_unaligned *wdest = (u64_unaligned *)pdest;
    const u64_unaligned *wsrc = (const u64_unaligned *)psrc;
    for (size_t i = words; i > 0; i--) {
      wdest[i - 1] = wsrc[i - 1];
    }
  }

  return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const uint8_t *p1 = (const uint8_t *)s1;
  const uint8_t *p2 = (const uint8_t *)s2;

  for (size_t i = 0; i < n; i++) {
    if (p1[i] != p2[i]) {
      return p1[i] < p2[i] ? -1 : 1;
    }
  }

  return 0;
}

size_t strlen(const char *str) {
  size_t len = 0;

  while (str[len]) {
    len++;
  }

  return len;
}
