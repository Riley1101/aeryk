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

/**
 * @brief Represents a point in 2D space with x and y coordinates.
 * 
 * @struct Point
 * @member x The x-coordinate of the point.
 * @member y The y-coordinate of the point.
 */
struct Point
{
    unsigned int x;
    unsigned int y;
};

/**
 * @brief Represents a framebuffer with its properties.
 * 
 * @struct FrameBuffer
 * @member base_address Pointer to the base address of the framebuffer.
 * @member buffer_size Size of the framebuffer in bytes.
 * @member width Width of the framebuffer in pixels.
 * @member height Height of the framebuffer in pixels.
 * @member pixels_per_scan_line Number of pixels per scan line (row) in the framebuffer, 
 * this is used to calculate the offset of a pixel in the framebuffer.
 */
typedef struct FrameBuffer
{
    void *base_address;
    size_t buffer_size;
    unsigned int width;
    unsigned int height;
    unsigned int pixels_per_scan_line;
} FrameBuffer;

/**
 * @brief Represents a renderer that handles drawing to the framebuffer.
 * 
 * @struct Renderer
 * @member framebuffer Pointer to the associated FrameBuffer.
 * @member cursor_position Current position of the cursor in the framebuffer.
 * @member color Current color used for rendering text and graphics.
 * @member psf1_font Pointer to the PSF1 font used for rendering text.
 * @member overwrite Flag indicating whether to overwrite existing content when rendering.
 */
typedef struct
{
    FrameBuffer *framebuffer;
    struct Point cursor_position;
    unsigned int color;
    struct PSF1_FONT *psf1_font;
    bool overwrite;
} Renderer;

void init_renderer(Renderer *render, FrameBuffer *buffer, struct PSF1_FONT *psf1_font);
/**
 * @brief Prints a string to the screen.
 * @param str The string to print.
 * @return void
 */
void print(const char *str);

/**
 * @brief Prints exactly `n` bytes to the screen, regardless of NUL bytes.
 * Used for syscall writes, where the caller-supplied length is the only
 * reliable bound -- unlike print(), which stops at the first '\0' and so
 * is only safe for real NUL-terminated C strings.
 * @param str The bytes to print.
 * @param n Number of bytes to print.
 * @return void
 */
void print_n(const char *str, size_t n);

/**
 * @brief Prints a string to the screen with a specified color.
 * @param str The string to print.
 * @param color The color to use for the text.
 * @return void
 */
void put_char(Renderer *basicrenderer, char chr, unsigned int xOff, unsigned int yOff);

/**
 * @brief Clears the screen by filling the framebuffer with a specified color.
 * Optionally resets the cursor position to the top-left corner.
 * @param renderer Pointer to the Renderer structure.
 * @param color The color to fill the screen with.
 * @param resetCursor If true, resets the cursor position to (0, 0).
 * @return void
 */ 
void clear(Renderer *basicrenderer, uint32_t color, bool resetCursor);

extern Renderer *global_renderer;

#endif
