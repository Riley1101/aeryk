#include "limine.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

// Halt and catch fire function
static void hcf(void) {
  for (;;) {
    asm("hlt");
  }
}

// Mian kernel entry point

void kmain(void) {

  // bootloader to understand our version
  if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
    hcf();
  }

  // make sure we have frame buffer
  if (framebuffer_request.response == NULL ||
      framebuffer_request.response->framebuffer_count < 1) {
    hcf();
  }

  struct limine_framebuffer *framebuffer =
      framebuffer_request.response->framebuffers[0];

  // we assume frame buffer is RGB with 32 bit
  for (size_t i = 0; i < 100; i++) {
    volatile uint32_t *fb_ptr = framebuffer->address;
    fb_ptr[i * (framebuffer->pitch / 4) + 1] = 0xffffff;
  }
  hcf();
}
