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

- [ ] Userland memory management (prerequisite for compositor)
  - [x] Userland heap allocation (brk)
  - [ ] Shared memory mapping between processes (mmap MAP_SHARED) — needed for compositor client/server shared buffers
  - [x] stdlib.c: malloc, free, calloc (libc wrappers over the above)

- [ ] Mouse driver (PS/2) — lands before compositor windowing, not in parallel

- [ ] Compositor (GUI land — the goal before circling back to threads/SMP)
  - [ ] Framebuffer mapped into userland
  - [ ] Word-sized memcpy (currently byte-at-a-time, too slow for full-frame blits)
  - [ ] Write-combining framebuffer mapping (PAT/MTRR)
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
  - [ ] Context switch latency (switch.asm)
  - [ ] Syscall entry/exit overhead (syscall_entry.asm)
  - [ ] Allocator alloc/free latency (slab now, malloc once userland heap lands)
  - [ ] Framebuffer blit throughput — needed to prove the word-sized memcpy fix above actually helps
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
- [ ] stdlib.c: malloc, free, calloc — tracked under "Userland memory management" above
- [ ] stdio.c: sprintf, snprintf (format into a buffer, needed for compositor protocol / error messages)
- [ ] atoi
- [x] errno — tracked under "Syscall hardening" above

## Build

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
