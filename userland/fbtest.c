#include <stdio.h>
#include <stdlib.h>
#include <sys/fb.h>
#include <sys/mman.h>

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

  printf("fbtest: painted color bars, unmapping in 3s...\n");
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
