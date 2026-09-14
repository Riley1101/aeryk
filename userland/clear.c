#include <stdlib.h>
#include <sys/fb.h>
#include <sys/mman.h>

// Matches BG in kernel/src/arch/x86_64/include/tty.h -- clear.c has no
// access to that header, so this is kept in sync by hand.
#define BG_COLOR 0xff282828

// Shell built-in-style command: clears the screen by mapping the
// framebuffer and painting it over with the background color, same as
// what the kernel's own clear() does at boot.
void main(void) {
  fb_info_t info;
  uint32_t *fb = (uint32_t *)fbmap(&info);
  if (fb == MAP_FAILED) {
    exit(1);
  }

  uint32_t stride = info.pitch / 4;
  for (uint32_t y = 0; y < info.height; y++) {
    for (uint32_t x = 0; x < info.width; x++) {
      fb[x + y * stride] = BG_COLOR;
    }
  }

  munmap(fb, (size_t)info.height * info.pitch);
  exit(0);
}
