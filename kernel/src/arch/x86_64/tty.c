#include <font.h>
#include <stdint.h>
#include <string.h>
#include <tty.h>
#include <arch/x86_64/drivers/serial.h>

Renderer *global_renderer;
static volatile int dbg_scroll_depth = 0;

/**
 * @brief Scrolls the framebuffer content up by one glyph row (16px):
 * discards the top row and fills the newly exposed bottom row with the
 * background color. Used by print() when it runs out of vertical room,
 * so output behaves like a normal scrolling terminal instead of wiping
 * the whole screen every time it fills up.
 */
static void scroll_up(Renderer *renderer) {
  dbg_scroll_depth++;
  if (dbg_scroll_depth > 1) {
    serial_print("[dbg] scroll_up REENTRANT depth>1\n");
  }
  uint64_t fb_base = (uint64_t)renderer->framebuffer->base_address;
  uint64_t stride = (uint64_t)renderer->framebuffer->pixels_per_scan_line * 4;
  uint64_t row_height = 16;
  uint64_t height = renderer->framebuffer->height;

  memmove((void *)fb_base, (void *)(fb_base + row_height * stride),
          (height - row_height) * stride);

  for (uint64_t y = height - row_height; y < height; y++) {
    for (uint64_t x = 0; x < renderer->framebuffer->width; x++) {
      *(uint32_t *)(fb_base + 4 * x + y * stride) = BG;
    }
  }
  dbg_scroll_depth--;
}

/**
 * @brief Initializes the renderer with the specified framebuffer and PSF1 font.
 * Sets the initial cursor position and color.
 * @param renderer Pointer to the Renderer structure to initialize.
 * @param framebuffer Pointer to the FrameBuffer structure representing the display.
 * @param psf1_font Pointer to the PSF1_FONT structure representing the font to use for rendering text.
 * @return void
 */
void init_renderer(Renderer *renderer, FrameBuffer *framebuffer,
                   struct PSF1_FONT *psf1_font) {
  renderer->color = FG;

  renderer->cursor_position.x = 0;
  renderer->cursor_position.y = 0;
  renderer->psf1_font = psf1_font;

  renderer->framebuffer = framebuffer;
  return;
}

/**
 * @brief Prints a string to the screen.
 * @param str The string to print.
 * @return void
 */
static void print_char_locked(char chr) {
  switch (chr) {
  case '\n':
    global_renderer->cursor_position.x = 0;
    global_renderer->cursor_position.y += 16;
    break;

  case '\t':
    global_renderer->cursor_position.x += 8;
    break;

  default:
    put_char(global_renderer, chr, global_renderer->cursor_position.x,
             global_renderer->cursor_position.y);
    global_renderer->cursor_position.x += 8;
    break;
  }

  if (global_renderer->cursor_position.x + 8 > global_renderer->framebuffer->width) {
    global_renderer->cursor_position.x = 0;
    global_renderer->cursor_position.y += 16;
  }
  if (global_renderer->cursor_position.y + 16 > global_renderer->framebuffer->height) {
    scroll_up(global_renderer);
    global_renderer->cursor_position.y -= 16;
  }
}

void print(const char *str) {
  // on_irq1() (keyboard.c) calls print() directly to echo each
  // keystroke, so this can be re-entered from inside a hardware
  // interrupt while another print() call is mid-update -- e.g. right
  // between scroll_up() and the cursor_position.y -= 16 that follows
  // it. Unlike the old clear()-and-reset-to-(0,0) behavior, that
  // decrement isn't idempotent: a race there silently double-decrements
  // y, and enough of those eventually underflow the unsigned coordinate
  // into a huge value, which put_char() then uses to write far outside
  // the framebuffer. Save/restore (rather than a blind cli/sti) so this
  // stays correct if print() is ever itself called with interrupts
  // already off.
  uint64_t flags;
  asm volatile("pushfq; pop %0; cli" : "=r"(flags)::"memory");

  for (const char *chr = str; *chr != 0; chr++) {
    print_char_locked(*chr);
  }

  asm volatile("push %0; popfq" ::"r"(flags) : "memory", "cc");
  return;
}

void print_n(const char *str, size_t n) {
  uint64_t flags;
  asm volatile("pushfq; pop %0; cli" : "=r"(flags)::"memory");

  for (size_t i = 0; i < n; i++) {
    print_char_locked(str[i]);
  }

  asm volatile("push %0; popfq" ::"r"(flags) : "memory", "cc");
  return;
}

/**
 * @brief Prints a string to the screen with a specified color.
 * @param str The string to print.
 * @param color The color to use for the text.
 * @return void
 */
void put_char(Renderer *renderer, char chr, unsigned int xOff,
              unsigned int yOff) {
  unsigned int *pixPtr = (unsigned int *)renderer->framebuffer->base_address;
  char *fontPtr = (char *)renderer->psf1_font->glyph_buffer +
                  (chr * renderer->psf1_font->psf1_header->charsize);

  for (unsigned long y = yOff; y < yOff + 16; y++) {
    for (unsigned long x = xOff; x < xOff + 8; x++) {
      if ((*fontPtr & (0b10000000 >> (x - xOff))) > 0) {
        *(unsigned int *)(pixPtr + x +
                          (y * renderer->framebuffer->pixels_per_scan_line)) =
            renderer->color;
      }
    }
    fontPtr++;
  }

  return;
}

/**
 * @brief Clears the screen by filling the framebuffer with a specified color.
 * Optionally resets the cursor position to the top-left corner.
 * @param renderer Pointer to the Renderer structure.
 * @param color The color to fill the screen with.
 * @param resetCursor If true, resets the cursor position to (0, 0).
 * @return void
 */
void clear(Renderer *renderer, uint32_t color, bool resetCursor) {
  uint64_t fbBase = (uint64_t)renderer->framebuffer->base_address;
  uint64_t pxlsPerScanline = renderer->framebuffer->pixels_per_scan_line;
  uint64_t fbHeight = renderer->framebuffer->height;

  for (int64_t y = 0; y < renderer->framebuffer->height; y++) {
    for (int64_t x = 0; x < renderer->framebuffer->width; x++) {
      *((uint32_t *)(fbBase + 4 * (x + pxlsPerScanline * y))) = color;
    }
  }

  if (resetCursor) {
    renderer->cursor_position.x = 0;
    renderer->cursor_position.y = 0;
  }

  return;
}
