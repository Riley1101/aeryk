#include <arch/x86_64/drivers/keyboard.h>
#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/fs/initramfs.h>

#include <apic.h>
#include <font.h>
#include <gdt.h>
#include <idt.h>
#include <limine.h>
#include <pmm.h>
#include <process.h>
#include <slab.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <syscall.h>
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

/**
 * @brief Halts the CPU indefinitely by executing the HLT instruction in an
 * infinite loop.
 */
static void hcf(void) {
  for (;;) {
    asm("hlt");
  }
}

/**
 * @brief Finds the loaded Limine module whose path ends with the specified
 * suffix. Returns NULL if no module matches.
 */
static struct limine_file *
find_module_by_suffix(struct limine_module_response *modules,
                      const char *suffix) {
  if (modules == NULL) {
    return NULL;
  }

  size_t slen = strlen(suffix);
  for (size_t i = 0; i < modules->module_count; i++) {
    struct limine_file *mod = modules->modules[i];
    size_t plen = strlen(mod->path);
    if (plen >= slen && memcmp(mod->path + plen - slen, suffix, slen) == 0) {
      return mod;
    }
  }
  return NULL;
}

/**
 * @brief Initializes the display by setting up the framebuffer-backed renderer
 * and loading the boot font. Halts the system if the framebuffer or font cannot
 * be loaded.
 */
static void init_display(void) {
  if (framebuffer_request.response == NULL ||
      framebuffer_request.response->framebuffer_count < 1) {
    hcf();
  }

  struct limine_framebuffer *framebuffer =
      framebuffer_request.response->framebuffers[0];

  static FrameBuffer f;
  f.base_address = framebuffer->address;
  f.width = framebuffer->width;
  f.height = framebuffer->height;
  f.pixels_per_scan_line = framebuffer->pitch / 4;
  f.buffer_size = framebuffer->height * framebuffer->pitch;

  static Renderer renderer;
  global_renderer = &renderer;

  static struct PSF1_FONT psf;
  if (!load_psf1("cp850-8x16.psf", &psf,
                 (struct limine_module_response *)module_request.response)) {
    hcf();
  }

  init_renderer(global_renderer, &f, &psf);
}

/**
 * @brief Initializes the APIC timer and routes IRQs appropriately.
 * Prints status messages indicating the success or failure of APIC
 * initialization.
 */
static void init_apic_timer(void) {
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
}

/**
 * @brief Runs a basic smoke test for the slab allocator using dynamic kmalloc
 * and kfree. Prints status messages indicating the success or failure of the
 * test.
 */
static void run_slab_smoke_test(void) {
  print("[-] Running slab test with dynamic kmalloc.\n");

  uint32_t *test_ptr = (uint32_t *)kmalloc(sizeof(uint32_t) * 10);
  if (test_ptr != NULL) {
    print("[-] kmalloc allocation successful!\n");
    test_ptr[0] = 1234;
    kfree(test_ptr);
    print("[-] kfree released slab chunk cleanly.\n");
  }
}

/**
 * @brief Builds a synthetic zombie process_t and links it into process_queue
 * as a child of `parent`, without touching the MLFQ ready queues.
 * Mirrors the fields wait_reap_child()/reap_zombies() actually look at:
 * pid, parent, state, exit_code, kernel_stack, cr3.
 */
static process_t *make_test_zombie(process_t *parent, uint64_t pid,
                                   int exit_code) {
  process_t *proc = (process_t *)kmalloc(sizeof(process_t));
  memset(proc, 0, sizeof(process_t));

  void *stack_phys = pmm_alloc_page();
  proc->kernel_stack = (void *)((uint64_t)stack_phys + hhdm_offset);
  proc->pid = pid;
  proc->parent = parent;
  proc->state = PROCESS_DEAD;
  proc->exit_code = exit_code;
  proc->cr3 = (uint64_t)vmm_get_kernel_pml4() - hhdm_offset;

  proc->prev = process_queue->prev;
  proc->next = process_queue;
  process_queue->prev->next = proc;
  process_queue->prev = proc;

  return proc;
}

/**
 * @brief Smoke test for wait_reap_child(): reaps two synthetic dead
 * children one at a time, then confirms ECHILD once the parent has none
 * left. Runs before any real user process exists so it can't race the
 * scheduler or step on a real zombie.
 */
static void run_wait_smoke_test(void) {
  print("[-] Running SYS_wait smoke test.\n");

  process_t *parent = current_process;
  process_t *child_a = make_test_zombie(parent, 9001, 7);
  process_t *child_b = make_test_zombie(parent, 9002, 42);
  // Capture pids up front: wait_reap_child() kfree()s the reaped process_t,
  // and kfree() overwrites the chunk's first 8 bytes (the pid field) with
  // its free-list pointer, so reading child_a/child_b->pid after reaping
  // them is a use-after-free that returns garbage, not the original pid.
  uint64_t child_a_pid = child_a->pid;
  uint64_t child_b_pid = child_b->pid;

  int status = -1;
  int reaped = wait_reap_child(parent, child_a_pid, &status);
  if (reaped == (int)child_a_pid && status == 7) {
    print("[-] wait_reap_child: reaped specific pid with correct exit code.\n");
  } else {
    print("[!] ERROR: wait_reap_child pid-specific reap failed.\n");
  }

  status = -1;
  reaped = wait_reap_child(parent, 0, &status);
  if (reaped == (int)child_b_pid && status == 42) {
    print("[-] wait_reap_child: reaped remaining child with pid<=0.\n");
  } else {
    print("[!] ERROR: wait_reap_child any-child reap failed.\n");
  }

  reaped = wait_reap_child(parent, 0, &status);
  if (reaped == -1) {
    print("[-] wait_reap_child: correctly returned ECHILD once childless.\n");
  } else {
    print("[!] ERROR: wait_reap_child should have returned ECHILD.\n");
  }
}

/**
 * @brief Mounts and initializes the initramfs from the Limine module.
 * Prints status messages indicating success or failure.
 */
static void mount_initramfs(void) {
  struct limine_file *initramfs_mod = find_module_by_suffix(
      (struct limine_module_response *)module_request.response,
      "initramfs.cpio");
  if (initramfs_mod != NULL) {
    print("[5] Parsing initramfs...\n");
    initramfs_init(initramfs_mod->address, initramfs_mod->size);
  } else {
    print("[!] ERROR: initramfs.cpio module not found\n");
  }
}

/**
 * @brief Spawns the initial user-space process, /bin/sh.
 * Prints status messages indicating success or failure for each process.
 */
static void spawn_initial_processes(void) {

  char *sh_argv[] = {"/bin/sh", NULL};
  if (create_user_process("/bin/sh", 1, sh_argv) == NULL) {
    print("[!] ERROR: failed to load /bin/sh\n");
  } else {
    print("[8] /bin/sh loaded, switching to Ring 3...\n");
  }
}

/**
 * @brief Main kernel entry point.
 * Initializes essential subsystems, mounts the initramfs, and spawns initial
 * user-space processes. Enters the scheduler and halts if no runnable process
 * exists.
 */
void kmain(void) {
  init_gdt();
  init_idt();

  if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
    hcf();
  }

  init_display();
  print("[-] Display initialized.\n");

  init_serial();
  print("[-] Serial initialized.\n");

  init_pmm();
  print("[0] PMM Initialized\n");

  init_vmm();
  print("[1] VMM Initialized.\n");

  init_apic_timer();
  print("[4] Slab Allocator kmalloc online.\n");

  init_slab();
  print("[-] Slab Allocator initialized.\n");

  init_scheduler();

  print("[5] Scheduler initialized.\n");

  run_slab_smoke_test();

  run_wait_smoke_test();

  mount_initramfs();

  init_keyboard();

  print("[6] IRQ1 keyboard listening...\n");

  init_syscalls();

  print("[7] Syscalls initialized...\n");

  spawn_initial_processes();

  schedule();

  // Only reached if schedule() picked idle_process as current_process
  // was already idle (i.e. no runnable process existed yet) — a normal
  // switch_task() into a real process does not return here at all,
  // since kmain is not itself a process on the switched-away stack.
  asm volatile("sti");
  hcf();
}
