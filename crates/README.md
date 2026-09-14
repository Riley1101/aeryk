# Rust bridge — migration checklist

## Port first
- [x] Bridge scaffolding (`rust_add` smoke test, linked into `bin-x86_64/kernel`)
- [x] Slab allocator (`init_slab`/`kmalloc`/`kfree`; `GlobalAlloc` impl still open)
- [ ] Buddy allocator (kernel-space PMM, sits under slab — same arithmetic-heavy
      shape as slab, same `pmm_alloc_page`/`pmm_free_page` boundary)
- [ ] libc string functions (`strchr`, `strtok`, `strncmp`, ...)
- [ ] VFS layer
- [ ] ELF loader

## Port when their prerequisite lands
- [ ] `GlobalAlloc` impl over the slab allocator (unblocks everything below)
- [ ] Scheduler/MLFQ ready queues — index-based arena (`Vec<ProcessId>` +
      generational index), not a port of today's intrusive pointer list.
      Gated on `GlobalAlloc`.
- [ ] Compositor window/surface list — index-based arena from day one
      (z-order, add/remove, shared-buffer lifetime). Gated on `GlobalAlloc`.
- [ ] Threading sync primitives (mutex/futex/spinlock) — RAII guard via
      `Drop` so unlock can't be forgotten/doubled. Gated on threading work
      starting.

## Small, low-risk, unwritten yet
- [ ] Mouse driver (PS/2) — stateful packet parser, `match`-on-enum fits well

## Leave in C
- [ ] Boot, GDT/IDT, `switch.asm`/`syscall_entry.asm`, ISR stubs
- [ ] `copy_from_user`/exception-table mechanism
- [ ] Serial driver

## Later / low priority
- [ ] ptmalloc-style allocator (userland malloc) — real value, far out

## Infra
- [ ] Host-side `cargo test` for pure-logic pieces (cache-size selection,
      buddy order math, MLFQ priority) — faster than the planned QEMU
      headless boot + serial-assert loop, doesn't replace it
