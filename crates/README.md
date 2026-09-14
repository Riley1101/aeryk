# Rust bridge — migration checklist

## Kernel-side (crates/kernel, linked into bin-x86_64/kernel)

### Port first
- [x] Bridge scaffolding (`rust_add` smoke test, linked into `bin-x86_64/kernel`)
- [x] Slab allocator (`init_slab`/`kmalloc`/`kfree`; `GlobalAlloc` impl still open)
- [ ] Buddy allocator (kernel-space PMM, sits under slab — same arithmetic-heavy
      shape as slab, same `pmm_alloc_page`/`pmm_free_page` boundary)
- [ ] libc string functions (`strchr`, `strtok`, `strncmp`, ...)
- [ ] VFS layer
- [ ] ELF loader

### Port when their prerequisite lands
- [ ] `GlobalAlloc` impl over the slab allocator (unblocks everything below)
- [ ] Scheduler/MLFQ ready queues — index-based arena (`Vec<ProcessId>` +
      generational index), not a port of today's intrusive pointer list.
      Gated on `GlobalAlloc`.
- [ ] Threading sync primitives (mutex/futex/spinlock) — RAII guard via
      `Drop` so unlock can't be forgotten/doubled. Gated on threading work
      starting.

### Small, low-risk
- [x] Mouse driver (PS/2) — landed in C (kernel/src/arch/x86_64/drivers/mouse.c),
      not ported to Rust. Was scoped here as a good Rust candidate
      (stateful packet parser, match-on-enum fits well) but was written
      before this crate's scope extended to drivers; revisit if useful as
      a worked example, otherwise leave as-is.

## Userland-side (crates/user, linked into userland/<name>.elf)
Same bridge pattern as the kernel side (Rust owns logic, `extern "C"` calls
into the existing, tested C runtime — see crates/user/src/sys.rs), but a
separate crate/target (`x86_64-aeryk-user.json`: code-model `small` instead
of `kernel`, everything else identical) since it links against crt0.o +
libc.a instead of the kernel's own linker script. One crate = one program
for now (crt0.asm's `call main` just needs a Rust-exported `main` symbol,
no crt0/linker changes needed) — see GNUmakefile's `rust-user-lib` rule for
the pattern to copy when adding another.

- [x] Bridge scaffolding (`crates/user`, `rusthello` — `puts`/`exit` FFI
      into libc.a, builds + boots + runs under QEMU)
- [ ] Compositor window/surface list — index-based arena (z-order, add/remove,
      shared-buffer lifetime). Lives here, not kernel-side, since the
      compositor is a userland process (see the client/server IPC design
      discussed in-session); needs its own crate once compositor work
      starts, following rusthello's pattern.

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
