#include "limine.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <tty.h>
#include <font.h>

// Set base version to 6
__attribute__((used, section(".limine_requests"))) static volatile uint64_t
    limine_base_revision[] = LIMINE_BASE_REVISION(6);

// Limine requests can be load anywhere
__attribute__((
    used,
    section(
        ".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0};

// Start and End markers for Limine requests

__attribute__((used,
               section(".limine_requests_start"))) static volatile uint64_t
    limine_requests_start_maker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end"))) static volatile uint64_t
    limine_requests_end_maker[] = LIMINE_REQUESTS_END_MARKER;

__attribute__((used, section(".limine_requests"))) static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 0};

// Halt and catch fire function
static void hcf(void)
{
    for (;;)
    {
        asm("hlt");
    }
}

// Mian kernel entry point

void kmain(void)
{

    // bootloader to understand our version
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false)
    {
        hcf();
    }

    // make sure we have frame buffer
    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1)
    {
        hcf();
    }

    struct limine_framebuffer *framebuffer =
        framebuffer_request.response->framebuffers[0];

    FrameBuffer f;
    {
        f.base_address = framebuffer->address;
        f.width = framebuffer->width;
        f.height = framebuffer->height;
        f.pixels_per_scan_line = framebuffer->pitch / 4;
        f.buffer_size = framebuffer->height * framebuffer->pitch;
    };

    Renderer renderer;
    global_renderer = &renderer;

    struct PSF1_FONT psf_font;
    struct PSF1_FONT *psf = &psf_font;

    loadPSF1("zap-light16.psf", psf, (struct limine_module_response *)module_request.response);

    init_renderer(global_renderer, &f, psf);

    print(global_renderer, "Hello, Kernel!\n");

    hcf();
}
