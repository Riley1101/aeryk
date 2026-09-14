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

## Userland-side (crates/userland/, a Cargo workspace, linked into userland/<name>.elf)
Same bridge pattern as the kernel side (Rust owns logic, `extern "C"` calls
into the existing, tested C runtime), but a separate crate/target
(`../targets/x86_64-aeryk-user.json`: code-model `small` instead of
`kernel`, everything else identical) since it links against crt0.o +
libc.a instead of the kernel's own linker script. `crates/kernel` lives
*outside* `crates/userland/` entirely (a sibling directory, not nested
under it), so Cargo's upward workspace search from there never finds this
workspace — it just builds standalone with its own Cargo.lock/target dir
via `kernel/GNUmakefile`'s `rust-lib` rule, same as before this workspace
existed. The directory split mirrors the real one: kernel and userland are
different targets, different ABIs, different sys.rs universes, so they
live in different parts of the tree, not just different Cargo packages.

Layout: one crate per *program* (`crt0.asm`'s `call main` just needs a
Rust-exported `main` symbol, no crt0/linker changes needed), plus shared
library crates for what would otherwise be copy-pasted between them:

- `userland/user-rt` — the `extern "C"` FFI bridge into libc.a (syscall
  wrappers, `FbInfo`/`MousePacket` structs) every program needs.
  Originally copy-pasted into each program's own crate; pulled out once
  there were three of them and the duplication cost more than a shared
  dependency does.
- `userland/compositor-protocol` — the compositor<->client wire format
  (`WindowRequest`, `WindowSlot`), shared between `compositor` (reader)
  and `testclient` (writer). Same reasoning as `user-rt`.
- `userland/rusthello`, `userland/compositor`, `userland/testclient` — the
  actual programs, each a thin crate depending on the two above. See
  GNUmakefile's `rust-userland-libs` rule for how a new one gets built
  (add it to `crates/userland/Cargo.toml`'s `members`,
  `RUST_USERLAND_PKGS`, and give it a `userland/<name>.elf` rule mirroring
  rusthello's).

- [x] Bridge scaffolding (`rusthello` — `puts`/`exit` FFI into libc.a via
      `user-rt`, builds + boots + runs under QEMU)
- [x] Compositor MVP (`compositor`) — exclusively fbmap()s the screen,
      paints a background, tracks mouse_read() and draws a cursor sprite.
      No alloc/GlobalAlloc needed (fixed-size arrays only). Verified
      visually under QEMU (monitor screendump). See README.md's
      Compositor section for the full writeup, including a real bug it
      surfaced (the kernel's own text console still writes over the
      compositor's screen).
- [x] Compositor window/surface list (`compositor-protocol`'s
      `WindowSlot`) — a fixed `[WindowSlot; MAX_WINDOWS]` array (no
      `GlobalAlloc` needed; each slot embeds a fixed-size pixel buffer)
      living in an `mmap(MAP_SHARED)` region so it's shared with each
      client's proxy process across `fork()`. No z-order yet (windows
      don't overlap in this MVP) and no dynamic growth -- both deferred
      until they're actually needed.
- [x] Client IPC protocol (`compositor-protocol`'s module docs have the
      full design) + first real client (`testclient`) — a plain-pipe
      handshake-then-frame-pushes protocol, since this kernel has no
      cross-exec shared memory (execve() wipes the user address space --
      see exec_process() in process.c) for a real exec'd client to
      inherit a pointer into. `testclient` is genuinely exec'd fresh, not
      a compositor-internal branch. Verified visually: two independent
      instances render as separate colored rectangles at their assigned
      positions.

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
