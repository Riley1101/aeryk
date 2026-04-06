#include <tty.h>
#include <font.h>
#include <stdint.h>

Renderer *global_renderer;

void init_renderer(Renderer *renderer, FrameBuffer *framebuffer, struct PSF1_FONT *psf1_font)
{
    renderer->color = GREEN;

    renderer->cursor_position.x = 0;
    renderer->cursor_position.y = 0;
    renderer->psf1_font = psf1_font;

    renderer->framebuffer = framebuffer;
    return;
}

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
            break;

        case '\t':
            renderer->cursor_position.x += 8;
            break;

        default:
            put_char(renderer, *chr, renderer->cursor_position.x, renderer->cursor_position.y);
            renderer->cursor_position.x += 8;
            break;
        }

        // TODO! needs scroll
        if (renderer->cursor_position.x + 8 > renderer->framebuffer->width)
        {
            renderer->cursor_position.x = 0;
            renderer->cursor_position.y += 16;
        }

        chr++;
    }

    return;
}

// TODO! Chr is unused for now
void put_char(Renderer *renderer, char chr, unsigned int xOff, unsigned int yOff)
{
    unsigned int *pixPtr = (unsigned int *)renderer->framebuffer->base_address;
    char *fontPtr = (char *)renderer->psf1_font->glyph_buffer + (chr * renderer->psf1_font->psf1_header->charsize);

    for (unsigned long y = yOff; y < yOff + 16; y++)
    {
        for (unsigned long x = xOff; x < xOff + 8; x++)
        {
            if ((*fontPtr & (0b10000000 >> (x - xOff))) > 0)
            {
                *(unsigned int *)(pixPtr + x + (y * renderer->framebuffer->pixels_per_scan_line)) = renderer->color;
            }
        }
        fontPtr++;
    }

    return;
}

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
