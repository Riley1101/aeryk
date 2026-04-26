# aeryk

A x86_64 kernel written in C, booted via the [Limine](https://codeberg.org/Limine/Limine) bootloader.

## Features

- Limine bootprotocol (revision 6)
- Global Descriptor Table (GDT)
- Interrupt Descriptor Table (IDT)
- Framebuffer-based TTY renderer
- PSF1 font rendering (CP850 8x16)

## Timeline

| Date     | Milestone                               |
| -------- | --------------------------------------- |
| Jan 2026 | Bootloader setup with Limine            |
| Feb 2026 | Framebuffer TTY and PSF1 font rendering |
| Mrh 2026 | Global Descriptor Table (GDT)           |
| Apr 2026 | Interrupt Descriptor Table (IDT)        |

## Progress

- [x] Boot via Limine
- [x] Framebuffer TTY
- [x] Font rendering (PSF1)
- [x] GDT
- [x] IDT
- [ ] Physical memory manager
- [ ] Virtual memory / paging
- [ ] Heap allocator
- [ ] Keyboard driver
- [ ] Scheduler / processes
- [ ] Syscall interface
- [ ] Filesystem (VFS)

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

## Sandbox

This is somewhat cleaned repo. A full sandbox is available where I test around is in [oands](https://github.com/Riley1101/oands)
