#ifndef TTY_H
#define TTY_H 1

#include <stddef.h>
#include <font.h>
#include <stdbool.h>
#include <stdint.h>

// Gruvbox Dark palette
#define BLACK 0xff282828
#define RED 0xffcc241d
#define GREEN 0xff98971a
#define YELLOW 0xffd79921
#define BLUE 0xff458588
#define PURPLE 0xffb16286
#define CYAN 0xff689d6a
#define WHITE 0xffa89984
// Bright variants
#define BBLACK 0xff928374
#define BRED 0xfffb4934
#define BGREEN 0xffb8bb26
#define BYELLOW 0xfffabd2f
#define BBLUE 0xff83a598
#define BPURPLE 0xffd3869b
#define BCYAN 0xff8ec07c
#define BWHITE 0xffebdbb2
// Semantic aliases
#define FG BWHITE
#define BG BLACK
#define ORANGE 0xfffe8019
#define TBLACK 0x00000000

struct Point
{
    unsigned int x;
    unsigned int y;
};

typedef struct FrameBuffer
{
    void *base_address;
    size_t buffer_size;
    unsigned int width;
    unsigned int height;
    unsigned int pixels_per_scan_line;
} FrameBuffer;

typedef struct
{
    FrameBuffer *framebuffer;
    struct Point cursor_position;
    unsigned int color;
    struct PSF1_FONT *psf1_font;
    bool overwrite;
} Renderer;

// TODO! Eventually add custom fonts
void init_renderer(Renderer *render, FrameBuffer *buffer, struct PSF1_FONT *psf1_font);
void print(Renderer *basicrenderer, const char *str);
void put_char(Renderer *basicrenderer, char chr, unsigned int xOff, unsigned int yOff);
void clear(Renderer *basicrenderer, uint32_t color, bool resetCursor);

extern Renderer *global_renderer;

#endif
