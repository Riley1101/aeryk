# aeryk

A x86_64 kernel written in C, booted via the [Limine](https://codeberg.org/Limine/Limine) bootloader.

## Timeline

| Date     | Milestone                               |
| -------- | --------------------------------------- |
| Jan 2026 | Bootloader setup with Limine            |
| Feb 2026 | Framebuffer TTY and PSF1 font rendering |
| Mrh 2026 | Global Descriptor Table (GDT)           |
| Apr 2026 | Interrupt Descriptor Table (IDT)        |
| May 2026 | Memory Management UNIT (MMU)            |
| May 2026 | APIC and Keyboard driver                |
| May 2026 | Slab allocator                          |
| Jun 2026 | MLFQ & Process                          |
| Jun 2026 | Serial driver                           |
| Jul 2026 | Syscalls: spawn/wait, libc wrappers     |
| Jul 2026 | Userland shell (sh)                     |
| Aug 2026 | Test users programs, tree, cat, ls      |
| Sep 2026 | Pipes, dup                              |

## Progress

- [x] Boot via Limine
- [x] Framebuffer TTY
- [x] Font rendering (PSF1)
- [x] GDT
- [x] IDT
- [x] Physical memory manager
- [x] Virtual memory / paging
- [x] APIC
- [x] Keyboard driver
- [x] Heap allocator
- [x] Scheduler / processes MLFQ
- [x] Serial driver (loopback self-test verified, mirrors the full boot/console log — used as the CI smoke-test output channel)
- [x] Syscall interface
  - [x] sys_write and sys_exit
  - [x] sys_open, sys_read, sys_close
  - [x] sys_spawn and sys_wait
  - [x] Context switching with MLFQ
  - [x] fork, execve
  - [x] clone (CLONE_VM only: threads share an address space via a refcounted pagetable instead of COW-cloning it; CLONE_FILES and shared brk/heap are not implemented yet)
  - [x] Kill offending process (not halt kernel) on a CPL 3 fault (idt.c isr_handler)
  - [x] copy_from_user / copy_to_user with an exception table, so a bad pointer passed into a syscall (e.g. `read(fd, (void*)0xdeadbeef, 100)`) kills the calling process instead of the kernel. Needed because syscalls run at CPL 0, so the CPL-3 fault check above can't tell a bad user pointer apart from a real kernel bug there; requires tagging user-memory-touching instructions and checking the faulting rip against that table.
- [x] Initramfs
- [x] Filesystem (VFS)
- [x] Elf loader
- [x] libc wrapper for syscalls
  - [x] Some fun libc Programs (init, cat, sh)
- [x] Input and IPC
  - [x] Keyboard wired to sys_read (fd 0, blocking, line-buffered)
  - [x] Pipes / IPC between processes
  - [x] Shell pipelines (`cmd1 | cmd2 | ...`), wiring dup2 across forked stages

- [ ] Syscall hardening
  - [x] errno (kernel syscalls return -errno on failure; libc wrappers translate that into the `errno` global + a -1 return)
  - [x] clone (CLONE_VM only — see above)

- [x] CI / test infra (cheap now, expensive to retrofit after SMP/compositor land)
  - [x] Finish serial driver (currently debug-only, "not complete") — needed as the output channel for a CI smoke test
  - [x] QEMU headless boot + serial-output assert in CI (catches boot hangs / triple faults / taskswitch-class regressions that host-side unit tests can't see)

- [x] Userland memory management (prerequisite for compositor)
  - [x] Userland heap allocation (brk)
  - [x] Shared memory mapping between processes (mmap MAP_SHARED) — needed for compositor client/server shared buffers
  - [x] stdlib.c: malloc, free, calloc (libc wrappers over the above)

- [x] Mouse driver (PS/2) — lands before compositor windowing, not in parallel
  - [x] 8042 controller: enable second PS/2 port, unmask its clock/IRQ12 in the config byte
  - [x] Device init (set defaults, enable data reporting) and IRQ12 → IDT vector 44 routing via the IOAPIC
  - [x] 3-byte packet decode (signed dx/dy, button state, overflow/desync handling) into a ring buffer, mirroring the keyboard driver's producer/consumer + wait-queue design (PROCESS_BLOCKED_MOUSE)
  - [x] SYS_mouse_read syscall + libc sys/mouse.h wrapper
  - [x] userland/mousetest.c — verified interactively under QEMU (monitor-injected mouse_move/mouse_button), decodes correct signed deltas and button press/release

- [ ] Persistent storage (highest-priority gap toward being a "real" OS — everything today lives in an in-memory VFS rebuilt from initramfs.cpio at boot, so nothing a process writes survives reboot)
  - [ ] Block device abstraction (read_block/write_block, request queue)
  - [ ] One disk driver (AHCI or virtio-blk — virtio-blk is far less register/FIS boilerplate under QEMU)
  - [ ] On-disk filesystem (FAT32 first for simplicity/tooling, ext2 later) sitting behind the existing vfs_node_t tree
  - [ ] Wire VFS read/write/open through to the block-backed filesystem instead of the initramfs-only path
  - [ ] Buffer cache (even a trivial one) so every read/write doesn't round-trip to the disk driver

- [ ] Signals (SIGKILL/SIGSEGV/SIGCHLD at minimum) — wait() and the CPL-3 fault killer currently substitute for this, but a real shell needs job control, and userland needs a way to catch/ignore faults instead of just dying
  - [ ] Signal delivery on a pending-signal check at syscall return / scheduler tick
  - [ ] Default dispositions (terminate, ignore, core-dump-equivalent)
  - [ ] sigaction/signal syscalls + libc wrappers
  - [ ] SIGCHLD on child exit (today wait() is the only notification path)

- [ ] Security / isolation hardening — currently ring 0/3 separation and copy_from_user/copy_to_user are the entire security model
  - [ ] Per-segment W^X enforcement on ELF PT_LOAD mappings (verify elf.c/vmm.c aren't mapping any segment RWX)
  - [ ] ASLR (randomize load base / mmap base / stack top)
  - [ ] Guard pages around kernel and user stacks
  - [ ] VFS permission bits (owner/mode) once a real filesystem exists to store them
  - [ ] User/group model (even a minimal uid 0 vs. non-0 distinction)

- [ ] Wall-clock time — currently timer-tick-only, no notion of real time
  - [ ] RTC (CMOS or HPET) read at boot for wall-clock epoch
  - [ ] gettimeofday/clock_gettime syscall + libc wrapper
  - [ ] Filesystem timestamps (depends on the persistent-storage work above)

- [ ] Networking (not previously scoped — decide whether it's in-scope before or after the compositor)
  - [ ] NIC driver (virtio-net is the QEMU-friendly starting point, same rationale as virtio-blk above)
  - [ ] Minimal stack (ARP/IPv4/UDP before TCP) or vendor lwIP
  - [ ] Socket syscalls + libc wrappers

- [ ] Reliability tooling (cheap now, same rationale as the CI smoke test above)
  - [ ] ASan/UBSan (or a freestanding equivalent) build variant for the kernel, run in CI alongside the existing unit tests
  - [ ] Syscall entry fuzzing (malformed/adversarial arguments — copy_from_user's exception table is exactly the kind of code this catches regressions in)

- [ ] Compositor (GUI land — the goal before circling back to threads/SMP)
  - [x] Framebuffer mapped into userland — SYS_fbmap maps the LFB's physical pages (not PMM-owned, so tagged PTE_NOPMM -- see vmm.h) into the caller out of the same MMAP_BASE bump region as SYS_mmap; regular munmap() tears it back down. userland/fbtest.c verified interactively under QEMU (color bars painted directly into the mapped buffer, repeated map/unmap/exit cycles left the PMM/COW machinery intact per malloctest/shmtest afterward)
  - [x] Word-sized memcpy — memcpy/memset/memmove in both kernel/src/arch/x86_64/string.c and libc/string/string.c now move 8 bytes/iteration (via an `aligned(1), may_alias` uint64_t alias, so no strict-aliasing/alignment UB) with a byte-wise tail; memmove's backward-copy path peels the unaligned tail first, then walks whole words down. Verified under QEMU: 30x `ls` to force repeated scroll_up() (kernel memmove), plus malloctest/sprintftest/pipetest/usercopytest/forktest/clonetest/shmtest/fbtest all still pass
  - [x] Write-combining framebuffer mapping (PAT/MTRR) — vmm_init_pat() (called from init_vmm()) reprograms PAT slot 1 from write-through to write-combining, leaving slot 0 (write-back, what every other PTE already resolves to) untouched; SYS_fbmap's mapping is now tagged PTE_PWT to select that slot. Only the userland fbmap mapping changed -- the kernel's own HHDM framebuffer pointer (tty.c) is a separate, Limine-established mapping left as-is. Verified: fbtest still paints/unmaps cleanly post-change
  - [ ] Compositor protocol over IPC (windows, damage rects, input events)
  - [ ] Redraw / vsync trigger off the existing timer
  - [ ] Window/surface data structure (position, z-order, shared buffer)
  - [ ] Compositing loop (blit windows to framebuffer each tick)
  - [ ] Client protocol handshake (create_window, damage, destroy_window)
  - [ ] Input routing (hit-testing, focus)
  - [ ] Cursor rendering
  - [ ] First real client (test window drawing into shared buffer)

- [ ] Threading (clone(CLONE_VM) today is just the primitive a thread library would sit on top of, not a usable threading facility). Comes after the compositor lands: it's the workload that makes SMP worth adding and gives a single-core baseline to benchmark against.
  - [ ] CLONE_FILES (shared fd table between threads — currently each cloned thread gets its own copy, like fork)
  - [ ] Shared brk/heap between CLONE_VM threads (currently snapshotted at clone time, so concurrent sbrk() from two threads can stomp each other's page mapping — blocks real multi-threaded malloc, see the ptmalloc arenas/tcache items below)
  - [ ] Synchronization primitives (mutex/futex/spinlock) — nothing stops two threads racing the same memory today; clonetest.c only avoids it by using wait() as a crude join
  - [ ] Coordinated thread-group teardown (an unhandled fault or exit() in one thread should plausibly kill/signal its siblings, not just itself)

- [ ] SMP (I have no clue what this is) — the actual point of doing Threading above: without SMP, "threads" only means multiple schedulable contexts sharing memory on one CPU, not concurrent execution

- [ ] Benchmarking (rdtsc + serial print, wired into CI as regression guardrails once the QEMU smoke test above exists) — the payoff: single-core vs. SMP-parallel comparisons once both exist
  - [x] rdtsc infra — utils.h's rdtsc() (kernel) / sys/tsc.h's rdtsc() (userland, a plain instruction, no syscall needed to read it) plus tsc_hz calibrated once at boot in timer.c's init_timer() against the same PIT 10ms one-shot reference lapic_calibrate() already used; SYS_get_tsc_hz/get_tsc_hz() lets userland convert its own deltas to real time
  - [ ] Context switch latency (switch.asm)
  - [ ] Syscall entry/exit overhead (syscall_entry.asm)
  - [ ] Allocator alloc/free latency (slab now, malloc once userland heap lands)
  - [x] Framebuffer blit throughput — needed to prove the word-sized memcpy fix above actually helps. userland/fbtest.c times 20 full-frame memcpy()s into the mapped (write-combining) framebuffer: ~2754 MB/s under QEMU TCG (note: TCG's software-emulated display is backed by ordinary host RAM, so the WC-vs-WB differential this measures is architecturally correct but won't show up strongly until run on real hardware/KVM against an actual MMIO LFB)
  - [ ] Single-core vs. SMP compositor throughput once both Threading and SMP land — the comparison this whole sequence is building toward

- [ ] ptmalloc-style allocator (currently a single-free-list K&R allocator; move toward glibc's design) — low priority, orthogonal to the GUI/threading path above
  - [ ] Boundary-tag chunks (size at both ends of a block, so `free()` coalesces with its physical neighbor in O(1) instead of walking a list)
  - [ ] Segregated bins (fastbins for small sizes, size-class bins for mid/large, instead of one linear free list)
  - [ ] Top chunk / wilderness (one chunk always at the high end of the heap absorbing `sbrk()` growth, out of the `malloc()` search path)
  - [ ] mmap threshold for large allocations — needs `mmap`/`munmap` syscalls first (not in the ABI yet)
  - [ ] Arenas + locking — gated on the Threading section above (needs shared brk + real sync primitives, not just clone(CLONE_VM))
  - [ ] tcache (per-thread free lists) — also gated on Threading above

- [ ] Kernel-space buddy allocator (Linux-style, sits under the slab allocator) — low priority, orthogonal to the GUI/threading path above
  - [ ] Replace the flat bitmap PMM with per-order (power-of-2) free lists
  - [ ] Buddy address computation (XOR with block size) for O(1) coalescing on free
  - [ ] Block splitting on alloc when the requested order has no free block
  - [ ] Keep `pmm_alloc_page`/`pmm_free_page` as the order-0 case so `slab.c` needs no changes
  - [ ] Multi-page contiguous allocation API (`pmm_alloc_order(n)`) for callers needing >1 page (e.g. framebuffer, large DMA-style buffers)
  - [ ] Carry per-page refcounts through at order-0 granularity (COW stays page-level even once buddy lands)

## Libc Notes

- [x] string.c
- [x] stdio.c (printf, putchar, puts)
- [x] unistd.c (open/read/write/close/spawn/wait)
- [x] stdlib.c (exit)
- [ ] string.c: strchr, strtok, strncmp, strcpy, strncpy, strcat (needed for shell parsing, e.g. `|`)
- [ ] ctype.h: isspace, isdigit, isalpha (needed for shell tokenizing)
- [x] unistd.c: dup, dup2, pipe() (wrappers for the SYS_pipe work)
- [x] stdlib.c: malloc, free, calloc — tracked under "Userland memory management" above
- [ ] stdio.c: sprintf, snprintf (format into a buffer, needed for compositor protocol / error messages)
- [ ] atoi
- [x] errno — tracked under "Syscall hardening" above

## Build

The slab allocator (`crates/kernel`) is written in Rust and cross-compiled
against a freestanding `x86_64-aeryk` target (`crates/targets/x86_64-aeryk.json`),
so a nightly Rust toolchain is required alongside the C toolchain. The
`rust-lib` GNUmakefile target builds it with `cargo +nightly build
-Zbuild-std=core -Zjson-target-spec` and stages the resulting
`libaeryk_kernel.a` for the linker; `make`/`make run`/`make clean` drive this
automatically, no separate step needed.


WHY? 

> BECAUSE IT's FUN!!!!!!!

```sh
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
rustup toolchain install nightly --component rust-src
```

(`rust-toolchain.toml` pins the nightly channel and `rust-src` component, so
`rustup` will pick these up automatically once installed.)

**macOS**

```sh
brew install make x86_64-elf-gcc qemu xorriso
```

```sh
make TOOLCHAIN_PREFIX=x86_64-elf-        # build ISO
make TOOLCHAIN_PREFIX=x86_64-elf- run    # run in QEMU (UEFI)
make TOOLCHAIN_PREFIX=x86_64-elf- run-bios  # run in QEMU (BIOS)
make clean
```

**Linux (Debian/Ubuntu)**

```sh
sudo apt install build-essential gcc qemu-system-x86 xorriso
```

```sh
make        # build ISO
make run    # run in QEMU (UEFI)
make run-bios  # run in QEMU (BIOS)
make clean
```

## LSP

You can use bear to generate a compile_commands.json file for LSP support. This is required for some features of the LSP to work, such as "Go to definition" and "Find references".

```sh
bear -- make
```

Note: `make` skips recompiling files that are already up to date, and bear only
records commands that actually run. If `obj-userland/` is already built, the
above won't capture userland/libc compile commands. To include userland and
libc (which use different flags than the kernel, e.g. no `-nostdinc`), force a
rebuild of those targets under bear:

```sh
rm -rf obj-userland userland/*.elf
bear --append -- make kernel initramfs.cpio
```

`--append` merges into the existing compile_commands.json instead of
overwriting the kernel entries. Re-run this whenever you add a new
userland/libc source file.

## Sandbox

This is somewhat cleaned repo. A full sandbox is available where I test around is in [oands](https://github.com/Riley1101/oands)
