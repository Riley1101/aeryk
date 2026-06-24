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
  -  [x] Context switching with MLFQ 
- [x] Initramfs 
- [x] Filesystem (VFS)
- [x] Elf loader 
- [ ] libc wrapper for syscalls
    -  [ ] Some fun libc Programs
- [ ] Input and IPC
- [ ] Compositor

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

You can use bear to generate a compile command.json file for LSP support. This is required for some features of the LSP to work, such as "Go to definition" and "Find references".

```sh
bear -- make 
```

## Sandbox

This is somewhat cleaned repo. A full sandbox is available where I test around is in [oands](https://github.com/Riley1101/oands)

