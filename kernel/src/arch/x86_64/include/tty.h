#ifndef TTY_H
#define TTY_H 1

#include <stddef.h>
#include <font.h>
#include <stdbool.h>
#include <stdint.h>

#define WHITE 0xffffffff
#define SILVER 0xffc0c0c0
#define GRAY 0xff808080
#define BGRAY 0xffC0C0C0
#define DGRAY 0xff404040
#define BLACK 0xff000000
#define PINK 0xffFF1493
#define GREEN 0xff008000
#define RED 0xff800000
#define PURPLE 0xff800080
#define ORANGE 0xffFF4500
#define CYAN 0xff008080
#define YELLOW 0xffFFD700
#define BROWN 0xffA52A2A
#define BLUE 0xff000080
#define DBLUE 0xff000030
#define BRED 0xffFF0000
#define BBLUE 0xff0000FF
#define BGREEN 0xff00FF00
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
