#include "arch/x86_64/drivers/keyboard.h"
#include "arch/x86_64/drivers/serial.h"
#include "arch/x86_64/fs/initramfs.h"
#include "limine.h"
#include "process.h"
#include "syscall.h"
#include <apic.h>
#include <font.h>
#include <gdt.h>
#include <idt.h>
#include <pmm.h>
#include <slab.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
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

void test_user_program() {
    const char *filepath = "/hello.txt";
    char buffer[128];

    // sys_open
    long fd;
    asm volatile("mov $2, %%rax \n" "mov %1, %%rdi \n" "syscall \n" : "=a"(fd) : "r"(filepath) : "rdi", "rcx", "r11");

    if (fd >= 0) {
        // sys_read
        long bytes_read;
        asm volatile("mov $0, %%rax \n" "mov %1, %%rdi \n" "mov %2, %%rsi \n" "mov $128, %%rdx \n" "syscall \n"
            : "=a"(bytes_read) : "r"(fd), "r"(buffer) : "rdi", "rsi", "rdx", "rcx", "r11");

        // sys_write to stdout (1)
        if (bytes_read > 0) {
            asm volatile("mov $1, %%rax \n" "mov $1, %%rdi \n" "mov %0, %%rsi \n" "mov %1, %%rdx \n" "syscall \n"
                : : "r"(buffer), "r"(bytes_read) : "rax", "rdi", "rsi", "rdx", "rcx", "r11");
        }
    }

    const char *log = "LOG";
asm volatile("mov $1, %%rax \n" "mov $1, %%rdi \n" "mov %0, %%rsi \n" "mov $3, %%rdx \n" "syscall \n"
    : : "r"(log) : "rax", "rdi", "rsi", "rdx", "rcx", "r11");

    // sys_exit
    asm volatile("mov $60, %%rax \n" "syscall \n" : : : "rax", "rcx", "r11");
}

// Main kernel entry point
void kmain(void) {
  init_gdt();
  init_idt();

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

  load_psf1("cp850-8x16.psf", psf,
            (struct limine_module_response *)module_request.response);

  // Initialize the screen FIRST
  init_renderer(global_renderer, &f, psf);

  // init_serial();
  init_serial();

  init_pmm();

  print("[0] PMM Initialized\n");

  init_vmm();
  print("[1] VMM Initialized.\n");

  init_apic();

  // Route irq 1 to idt 33
  ioapic_set_irq(1, 0, 33);

  uint32_t svr = lapic_read(LAPIC_SVR);

  if ((svr & 0x100) != 0) {
    print("[2] APIC verified online.\n");
  } else {
    print("[!] ERROR: APIC failed to enable.\n");
  }

  init_timer();

  print("[3] IRQ0 PIT Timer calibration started.\n");

  print("[4] Slab Allocator kmalloc online.\n");
  init_slab();

  init_scheduler();

  print("[5] Running slab test with dynamic kmalloc.\n");

  uint32_t *test_ptr = (uint32_t *)kmalloc(sizeof(uint32_t) * 10);
  if (test_ptr != NULL) {
    print("[-] kmalloc allocation successful!\n");
    test_ptr[0] = 1234;
    kfree(test_ptr);
    print("[-] kfree released slab chunk cleanly.\n");
  }

  if (module_request.response != NULL) {
    const char *suffix = "initramfs.cpio";
    size_t slen = 14;
    for (size_t i = 0; i < module_request.response->module_count; i++) {
      struct limine_file *mod = module_request.response->modules[i];
      size_t plen = strlen(mod->path);
      if (plen >= slen && memcmp(mod->path + plen - slen, suffix, slen) == 0) {
        print("[5] Parsing initramfs...\n");
        initramfs_init(mod->address, mod->size);
        break;
      }
    }
  }

  init_keyboard();

  print("[6] IRQ1 keyboard listening...\n");

  asm volatile("sti");

  init_syscalls();

  print("[7] Syscalls initialized...\n");

  void *user_stack = pmm_alloc_page();

  uint64_t user_stack_top = (uint64_t)user_stack + hhdm_offset + PAGE_SIZE;

  void *kernel_stack = pmm_alloc_page();
  kernel_rsp_scratch = (uint64_t)kernel_stack + hhdm_offset + PAGE_SIZE;

  print("[-] Jumping into Ring 3... \n");

  // test_user_program still lives in kernel .text and the user stack
  // is backed by an HHDM page; both were mapped supervisor-only by
  // Limine. Mark the pages ring-3 can touch as user-accessible so the
  // iret to CPL3 doesn't immediately #PF on fetch/stack access.
  uint64_t *pml4 = vmm_get_kernel_pml4();
  uint64_t func_addr = (uint64_t)test_user_program;
  for (uint64_t addr = func_addr & ~(uint64_t)(PAGE_SIZE - 1);
       addr < func_addr + 0x2000; addr += PAGE_SIZE) {
    vmm_set_page_user(pml4, addr);
  }
  vmm_set_page_user(pml4, (uint64_t)user_stack + hhdm_offset);

  enter_usermode((uint64_t)test_user_program, user_stack_top);

  hcf();
}
