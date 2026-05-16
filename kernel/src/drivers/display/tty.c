#include "font.h"
#include "tty.h"
#include <stdint.h>

Renderer *global_renderer;

/**
 * Initializes the renderer with the given framebuffer and font. Sets the default
 * text color to FG and the cursor position to the top-left corner of the screen.
 *
 * @param renderer Pointer to the Renderer structure to initialize.
 * @param framebuffer Pointer to the FrameBuffer structure containing framebuffer information.
 * @param psf1_font Pointer to the PSF1_FONT structure containing font information.
 * @return void
 */
void init_renderer(Renderer *renderer, FrameBuffer *framebuffer,
                   struct PSF1_FONT *psf1_font)
{
    renderer->color = FG;

    renderer->cursor_position.x = 0;
    renderer->cursor_position.y = 0;
    renderer->psf1_font = psf1_font;

    renderer->framebuffer = framebuffer;
    return;
}

/**
 * Prints a null-terminated string to the screen using the provided Renderer.
 * Handles newline and tab characters for proper formatting. Automatically scrolls the screen when the cursor reaches the bottom of the framebuffer.
 *
 * @param renderer Pointer to the Renderer structure used for rendering text.
 * @param str Null-terminated string to be printed on the screen.
 * @return void
 */
void print(Renderer *renderer, const char *str)
{
    char *chr = (char *)str;
    while (*chr != 0)
    {
        switch (*chr)
        {
        case '\n':
            renderer->cursor_position.x = 0;
            renderer->cursor_position.y += 16;
            if (renderer->cursor_position.y + 16 > renderer->framebuffer->height)
                scroll(renderer);
            break;

        case '\t':
            renderer->cursor_position.x += 8;
            break;

        default:
            put_char(renderer, *chr, renderer->cursor_position.x,
                     renderer->cursor_position.y);
            renderer->cursor_position.x += 8;
            break;
        }

        if (renderer->cursor_position.x + 8 > renderer->framebuffer->width)
        {
            renderer->cursor_position.x = 0;
            renderer->cursor_position.y += 16;
        }

        if (renderer->cursor_position.y + 16 > renderer->framebuffer->height)
        {
            scroll(renderer);
        }

        chr++;
    }

    return;
}

/**
 * Renders a single character on the screen at the specified offset using the provided Renderer.
 * The character is drawn using the font glyphs defined in the PSF1_FONT structure.
 * The function calculates the appropriate pixel positions based on the character's bitmap and updates the framebuffer accordingly.
 *
 * @param renderer Pointer to the Renderer structure used for rendering the character.
 * @param chr The character to be rendered on the screen.
 * @param xOff The horizontal offset (in pixels) from the left edge of the screen
 * @param yOff The vertical offset (in pixels) from the top edge of the screen
 * @return void
 */
void put_char(Renderer *renderer, char chr, unsigned int xOff,
              unsigned int yOff)
{
    unsigned int *pixPtr = (unsigned int *)renderer->framebuffer->base_address;
    char *fontPtr = (char *)renderer->psf1_font->glyph_buffer +
                    (chr * renderer->psf1_font->psf1_header->charsize);

    for (unsigned long y = yOff; y < yOff + 16; y++)
    {
        for (unsigned long x = xOff; x < xOff + 8; x++)
        {
            if ((*fontPtr & (0b10000000 >> (x - xOff))) > 0)
            {
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
 * Scrolls the screen up by one line.
 * This function is called when the cursor reaches the bottom of the framebuffer.
 *
 * @param renderer Pointer to the Renderer structure used for rendering text.
 * @return void
 */
void scroll(Renderer *renderer)
{
    uint32_t *fb = (uint32_t *)renderer->framebuffer->base_address;
    uint64_t ppsl = renderer->framebuffer->pixels_per_scan_line;
    uint64_t height = renderer->framebuffer->height;
    uint64_t width = renderer->framebuffer->width;

    // shift every pixel up by 16 pixels
    for (uint64_t y = 0; y + 16 < height; y++)
        for (uint64_t x = 0; x < ppsl; x++)
            fb[x + y * ppsl] = fb[x + (y + 16) * ppsl];

    // clearning the remaining
    for (uint64_t y = height - 16; y < height; y++)
        for (uint64_t x = 0; x < width; x++)
            fb[x + y * ppsl] = BG;

    renderer->cursor_position.y -= 16;
}

/**
 * Clears the screen by filling the entire framebuffer with the specified color.
 * Optionally resets the cursor position to the top-left corner of the screen.
 *
 * @param renderer Pointer to the Renderer structure used for rendering text.
 * @param color The color to fill the screen with, specified as a 32-bit unsigned
 * integer in ARGB format (0xAARRGGBB).
 * @param resetCursor A boolean flag indicating whether to reset the cursor position to (0, 0) after clearing the screen.
 * @return void
 */
void clear(Renderer *renderer, uint32_t color, bool resetCursor)
{
    uint64_t fbBase = (uint64_t)renderer->framebuffer->base_address;
    uint64_t pxlsPerScanline = renderer->framebuffer->pixels_per_scan_line;
    uint64_t fbHeight = renderer->framebuffer->height;

    for (int64_t y = 0; y < renderer->framebuffer->height; y++)
    {
        for (int64_t x = 0; x < renderer->framebuffer->width; x++)
        {
            *((uint32_t *)(fbBase + 4 * (x + pxlsPerScanline * y))) = color;
        }
    }

    if (resetCursor)
    {
        renderer->cursor_position.x = 0;
        renderer->cursor_position.y = 0;
    }

    return;
}
