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
- [x] Serial driver (not complete one, just for debugging instructions to GDB)
- [x] Syscall interface
  -  [x] sys_write and sys_exit
  -  [x] sys_open, sys_read, sys_close
  -  [x] sys_spawn and sys_wait
  -  [x] Context switching with MLFQ 
  -  [x] fork,  execve
  -  [ ] clone
  -  [x] Kill offending process (not halt kernel) on a CPL 3 fault (idt.c isr_handler)
  -  [ ] copy_from_user / copy_to_user with an exception table, so a bad pointer passed into a syscall (e.g. `read(fd, (void*)0xdeadbeef, 100)`) kills the calling process instead of the kernel. Needed because syscalls run at CPL 0, so the CPL-3 fault check above can't tell a bad user pointer apart from a real kernel bug there; requires tagging user-memory-touching instructions and checking the faulting rip against that table.
- [x] Initramfs 
- [x] Filesystem (VFS)
- [x] Elf loader 
- [x] libc wrapper for syscalls
    -  [x] Some fun libc Programs (init, cat, sh)
- [ ] Input and IPC
    -  [x] Keyboard wired to sys_read (fd 0, blocking, line-buffered)
    -  [ ] Pipes / IPC between processes

- [ ] SMP 

- [ ] Compositor
    -  [ ] Userland heap allocation (brk / anonymous mmap)
    -  [ ] Shared memory mapping between processes (mmap MAP_SHARED)
    -  [ ] Framebuffer mapped into userland
    -  [ ] Word-sized memcpy (currently byte-at-a-time, too slow for full-frame blits)
    -  [ ] Write-combining framebuffer mapping (PAT/MTRR)

    -  [ ] Mouse driver (PS/2)

    -  [ ] Compositor protocol over IPC (windows, damage rects, input events)
    -  [ ] Redraw / vsync trigger off the existing timer
    -  [ ] Window/surface data structure (position, z-order, shared buffer)
    -  [ ] Compositing loop (blit windows to framebuffer each tick)
    -  [ ] Client protocol handshake (create_window, damage, destroy_window)
    -  [ ] Input routing (hit-testing, focus)
    -  [ ] Cursor rendering
    -  [ ] First real client (test window drawing into shared buffer)


## Libc Notes
- [x] string.c
- [x] stdio.c (printf, putchar, puts)
- [x] unistd.c (open/read/write/close/spawn/wait)
- [x] stdlib.c (exit)
- [ ] string.c: strchr, strtok, strncmp, strcpy, strncpy, strcat (needed for shell parsing, e.g. `|`)
- [ ] ctype.h: isspace, isdigit, isalpha (needed for shell tokenizing)
- [ ] unistd.c: dup, dup2, pipe() (wrappers for the SYS_pipe work)
- [ ] stdlib.c: malloc, free, calloc (needed once brk/mmap lands)
- [ ] stdio.c: sprintf, snprintf (format into a buffer, needed for compositor protocol / error messages)
- [ ] atoi
- [ ] errno (syscalls currently collapse all failures to -1)

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
