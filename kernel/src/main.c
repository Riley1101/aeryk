#include "limine.h"
#include <apic.h>
#include <font.h>
#include <gdt.h>
#include <idt.h>
#include <pmm.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <timer.h>
#include <tty.h>
#include <vmm.h>

// --- START MARKER ---
__attribute__((used,
               section(".limine_requests_start"))) static volatile uint64_t
    limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

// --- LIMINE REQUESTS ---
__attribute__((used, section(".limine_requests"))) static volatile uint64_t
    limine_base_revision[] = LIMINE_BASE_REVISION(6);

__attribute__((
    used,
    section(
        ".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0};

__attribute__((
    used,
    section(".limine_requests"))) static volatile struct limine_module_request
    module_request = {.id = LIMINE_MODULE_REQUEST_ID, .revision = 0};

__attribute__((
    used, section(".limine_requests"))) volatile struct limine_memmap_request
    memmap_request = {.id = LIMINE_MEMMAP_REQUEST_ID, .revision = 0};

__attribute__((used,
               section(".limine_requests"))) volatile struct limine_hhdm_request
    hhdm_request = {.id = LIMINE_HHDM_REQUEST_ID, .revision = 0};

// --- END MARKER ---
__attribute__((used, section(".limine_requests_end"))) static volatile uint64_t
    limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

static void hcf(void) {
  for (;;) {
    asm("hlt");
  }
}

// Main kernel entry point
void kmain(void) {
  initGdt();
  initIdt();

  if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
    hcf();
  }

  if (framebuffer_request.response == NULL ||
      framebuffer_request.response->framebuffer_count < 1) {
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

  loadPSF1("cp850-8x16.psf", psf,
           (struct limine_module_response *)module_request.response);

  // Initialize the screen FIRST
  init_renderer(global_renderer, &f, psf);

  initPMM();
  initVMM();

  initAPIC();

  uint32_t svr = lapic_read(LAPIC_SVR);

  if ((svr & 0x100) != 0) {
    print(global_renderer, "[2] APIC verified online.\n");
  } else {
    print(global_renderer, "[!] ERROR: APIC failed to enable.\n");
  }

  initTimer();

  asm volatile("sti");

  print(global_renderer, "Hello, Kernel! Memory active.\n");

  hcf();
}
