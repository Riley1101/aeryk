#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fb.h>
#include <sys/mman.h>
#include <sys/tsc.h>

// Background color to restore before exiting -- matches BG (tty.h) so the
// screen looks untouched once the shell prompt redraws over it.
#define BG_COLOR 0xff282828

// Manual/interactive test: run under a QEMU window (not the headless boot
// smoke test) to see the pattern. Maps the kernel's framebuffer, paints a
// few color bars directly into it, restores the background, then unmaps
// and exits.
void main(void) {
  fb_info_t info;
  uint32_t *fb = (uint32_t *)fbmap(&info);
  if (fb == MAP_FAILED) {
    printf("fbtest: fbmap failed\n");
    exit(1);
  }

  printf("fbtest: mapped %ux%u framebuffer, pitch=%u bpp=%u\n", info.width,
         info.height, info.pitch, info.bpp);

  uint32_t colors[] = {0x00FF0000, 0x0000FF00, 0x000000FF, 0x00FFFF00,
                        0x00FF00FF, 0x0000FFFF};
  int num_bars = 6;
  uint32_t stride = info.pitch / 4;

  for (uint32_t y = 0; y < info.height; y++) {
    int bar = (y * num_bars) / info.height;
    for (uint32_t x = 0; x < info.width; x++) {
      fb[x + y * stride] = colors[bar];
    }
  }
  printf("fbtest: painted color bars\n");

  // Blit throughput benchmark: repeatedly memcpy a full frame from a
  // staging buffer into the mapped (write-combining) framebuffer, timed
  // with rdtsc. Exercises exactly the two things just optimized -- the
  // word-sized memcpy and the PTE_PWT mapping -- so this number is the
  // direct payoff of both.
  size_t fb_bytes = (size_t)info.height * info.pitch;
  uint8_t *staging = malloc(fb_bytes);
  if (staging) {
    memset(staging, 0x55, fb_bytes);

    int iterations = 20;
    uint64_t start = rdtsc();
    for (int i = 0; i < iterations; i++) {
      memcpy(fb, staging, fb_bytes);
    }
    uint64_t cycles = rdtsc() - start;
    free(staging);

    uint64_t tsc_hz = get_tsc_hz();
    uint64_t total_bytes = (uint64_t)fb_bytes * (uint64_t)iterations;
    // MB/s = bytes * Hz / cycles / (1024*1024), kept in this order (multiply
    // before dividing) since there's no float support in this build
    // (-mno-sse etc.) and integer division would otherwise truncate to 0.
    uint64_t mb_per_sec =
        cycles > 0 ? (total_bytes * tsc_hz) / cycles / (1024 * 1024) : 0;
    uint64_t elapsed_ms = tsc_hz > 0 ? (cycles * 1000) / tsc_hz : 0;

    printf("fbtest: blit bench: %d full-frame copies (%u bytes each) in "
           "%u ms, ~%u MB/s\n",
           iterations, (unsigned)fb_bytes, (unsigned)elapsed_ms,
           (unsigned)mb_per_sec);
  } else {
    printf("fbtest: blit bench skipped (malloc failed)\n");
  }

  // The benchmark loop above overwrote the bars with its staging pattern --
  // repaint them so the screen looks right during the delay below.
  for (uint32_t y = 0; y < info.height; y++) {
    int bar = (y * num_bars) / info.height;
    for (uint32_t x = 0; x < info.width; x++) {
      fb[x + y * stride] = colors[bar];
    }
  }

  printf("fbtest: unmapping in 3s...\n");
  for (volatile long i = 0; i < 300000000L; i++) {
  }

  for (uint32_t y = 0; y < info.height; y++) {
    for (uint32_t x = 0; x < info.width; x++) {
      fb[x + y * stride] = BG_COLOR;
    }
  }

  munmap(fb, (size_t)info.height * info.pitch);
  printf("fbtest: done\n");
  exit(0);
}
