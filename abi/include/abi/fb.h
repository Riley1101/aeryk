#ifndef _ABI_FB_H
#define _ABI_FB_H

#include <stdint.h>

/**
 * @brief Geometry of the kernel's framebuffer, filled in by SYS_fbmap
 * alongside the mapping itself so a userland caller knows how to address
 * the pixels it just got mapped in. `pitch` is bytes per scanline (may
 * exceed width * bpp / 8 -- always stride by `pitch`, never assume the
 * buffer is tightly packed). `bpp` is always 32 today; the kernel's own
 * framebuffer code (tty.c) hardcodes 4-byte pixels throughout.
 */
typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t pitch;
  uint32_t bpp;
} fb_info_t;

#endif // !_ABI_FB_H
