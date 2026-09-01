# Nuke built-in rules.
.SUFFIXES:

# Target architecture to build for. Default to x86_64.
ARCH := x86_64

# Default user QEMU flags. These are appended to the QEMU command calls.
QEMUFLAGS := -m 2G -d guest_errors,int -D qemu.log -serial stdio

override IMAGE_NAME := template-$(ARCH)

# Toolchain for building the 'limine' executable for the host.
HOST_CC := cc
HOST_CFLAGS := -g -O2 -pipe
HOST_CPPFLAGS :=
HOST_LDFLAGS :=
HOST_LIBS :=

# Toolchain for building userland (ring 3) programs. Honors the same
# TOOLCHAIN_PREFIX used for the kernel (e.g. TOOLCHAIN_PREFIX=x86_64-elf-).
USER_CC := $(TOOLCHAIN_PREFIX)gcc
USER_LD := $(TOOLCHAIN_PREFIX)ld
USER_AR := $(TOOLCHAIN_PREFIX)ar

override USERLAND_CFLAGS := -g -O2 -pipe \
    -m64 -march=x86-64 -mabi=sysv \
    -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone \
    -ffreestanding -fno-stack-protector -fno-pic -fno-pie \
    -Ikernel/freestnd-c-hdrs/include \
    -Ilibc/include \
    -Iabi/include

.PHONY: all
all: $(IMAGE_NAME).iso

.PHONY: all-hdd
all-hdd: $(IMAGE_NAME).hdd

.PHONY: run
run: run-$(ARCH)

.PHONY: run-hdd
run-hdd: run-hdd-$(ARCH)

.PHONY: run-x86_64
run-x86_64: edk2-ovmf $(IMAGE_NAME).iso
	qemu-system-$(ARCH) \
		-M q35 \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
		-cdrom $(IMAGE_NAME).iso \
		$(QEMUFLAGS)

.PHONY: run-hdd-x86_64
run-hdd-x86_64: edk2-ovmf $(IMAGE_NAME).hdd
	qemu-system-$(ARCH) \
		-M q35 \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
		-hda $(IMAGE_NAME).hdd \
		$(QEMUFLAGS)

.PHONY: run-aarch64
run-aarch64: edk2-ovmf $(IMAGE_NAME).iso
	qemu-system-$(ARCH) \
		-M virt \
		-cpu cortex-a72 \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-tablet \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
		-cdrom $(IMAGE_NAME).iso \
		$(QEMUFLAGS)

.PHONY: run-hdd-aarch64
run-hdd-aarch64: edk2-ovmf $(IMAGE_NAME).hdd
	qemu-system-$(ARCH) \
		-M virt \
		-cpu cortex-a72 \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-tablet \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
		-hda $(IMAGE_NAME).hdd \
		$(QEMUFLAGS)

.PHONY: run-riscv64
run-riscv64: edk2-ovmf $(IMAGE_NAME).iso
	qemu-system-$(ARCH) \
		-M virt \
		-cpu rv64 \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-tablet \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
		-cdrom $(IMAGE_NAME).iso \
		$(QEMUFLAGS)

.PHONY: run-hdd-riscv64
run-hdd-riscv64: edk2-ovmf $(IMAGE_NAME).hdd
	qemu-system-$(ARCH) \
		-M virt \
		-cpu rv64 \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-tablet \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
		-hda $(IMAGE_NAME).hdd \
		$(QEMUFLAGS)

.PHONY: run-loongarch64
run-loongarch64: edk2-ovmf $(IMAGE_NAME).iso
	qemu-system-$(ARCH) \
		-M virt \
		-cpu la464 \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-tablet \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
		-cdrom $(IMAGE_NAME).iso \
		$(QEMUFLAGS)

.PHONY: run-hdd-loongarch64
run-hdd-loongarch64: edk2-ovmf $(IMAGE_NAME).hdd
	qemu-system-$(ARCH) \
		-M virt \
		-cpu la464 \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-tablet \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
		-hda $(IMAGE_NAME).hdd \
		$(QEMUFLAGS)


.PHONY: run-bios
run-bios: $(IMAGE_NAME).iso
	qemu-system-$(ARCH) \
		-M q35 \
		-cdrom $(IMAGE_NAME).iso \
		-boot d \
		$(QEMUFLAGS)

.PHONY: run-hdd-bios
run-hdd-bios: $(IMAGE_NAME).hdd
	qemu-system-$(ARCH) \
		-M q35 \
		-hda $(IMAGE_NAME).hdd \
		$(QEMUFLAGS)

edk2-ovmf:
	curl -L https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/edk2-ovmf.tar.gz | gunzip | tar -xf -

limine/limine:
	rm -rf limine
	git clone https://codeberg.org/Limine/Limine.git limine --branch=v11.x-binary --depth=1
	$(MAKE) -C limine \
		CC="$(HOST_CC)" \
		CFLAGS="$(HOST_CFLAGS)" \
		CPPFLAGS="$(HOST_CPPFLAGS)" \
		LDFLAGS="$(HOST_LDFLAGS)" \
		LIBS="$(HOST_LIBS)"

kernel/.deps-obtained:
	./kernel/get-deps

# Every userland program is userland/<name>.c -> userland/<name>.elf, linked
# against crt0.o and the common libc.a. To add a new program, just drop its
# name in USERLAND_PROGS -- no other Makefile changes needed.
override USERLAND_PROGS := init cat sh ls forktest exectest crashtest usercopytest

override USERLAND_LIBC_SRCS := \
    libc/stdlib/exit.c \
    libc/unistd.c \
    libc/stdio/putchar.c \
    libc/stdio/puts.c \
    libc/stdio/printf.c \
    libc/string/strlen.c \
    libc/string/strcmp.c \
    libc/string/string.c

override USERLAND_LIBC_OBJS := $(patsubst libc/%.c,obj-userland/libc/%.o,$(USERLAND_LIBC_SRCS))
override USERLAND_ELFS := $(addprefix userland/,$(addsuffix .elf,$(USERLAND_PROGS)))

# Keep chained pattern-rule intermediates (e.g. obj-userland/init.c.o) around
# instead of letting make delete them as throwaway intermediates.
.SECONDARY: $(USERLAND_LIBC_OBJS) obj-userland/crt0.o obj-userland/libc.a \
	$(addprefix obj-userland/,$(addsuffix .c.o,$(USERLAND_PROGS)))

obj-userland/libc/%.o: libc/%.c
	mkdir -p $(dir $@)
	$(USER_CC) $(USERLAND_CFLAGS) -c $< -o $@

obj-userland/libc.a: $(USERLAND_LIBC_OBJS)
	mkdir -p $(dir $@)
	$(USER_AR) rcs $@ $^

obj-userland/crt0.o: kernel/src/arch/x86_64/crt0.asm
	mkdir -p $(dir $@)
	nasm -f elf64 $< -o $@

obj-userland/%.c.o: userland/%.c
	mkdir -p $(dir $@)
	$(USER_CC) $(USERLAND_CFLAGS) -c $< -o $@

userland/%.elf: obj-userland/%.c.o userland/linker.lds obj-userland/crt0.o obj-userland/libc.a
	$(USER_LD) -nostdlib -static -m elf_x86_64 -T userland/linker.lds \
		obj-userland/crt0.o $< obj-userland/libc.a -o $@

initramfs.cpio: $(USERLAND_ELFS)
	mkdir -p initramfs_root/bin
	echo "Hello from the aeryk cpio initramfs!" > initramfs_root/hello.txt
	$(foreach elf,$(USERLAND_ELFS),cp -v $(elf) initramfs_root/bin/$(basename $(notdir $(elf)));)
	cd initramfs_root && find . | cpio -o -H newc > ../initramfs.cpio

.PHONY: kernel
kernel: kernel/.deps-obtained
	$(MAKE) -C kernel

$(IMAGE_NAME).iso: limine/limine kernel initramfs.cpio
	rm -rf iso_root
	mkdir -p iso_root/boot
	cp -v external/* iso_root/boot/
	cp -v kernel/bin-$(ARCH)/kernel iso_root/boot/
	cp -v initramfs.cpio iso_root/boot/
	mkdir -p iso_root/boot/limine
	cp -v ./boot/limine.conf iso_root/boot/limine/
	mkdir -p iso_root/EFI/BOOT

ifeq ($(ARCH),x86_64)
	cp -v limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/boot/limine/
	cp -v limine/BOOTX64.EFI iso_root/EFI/BOOT/
	cp -v limine/BOOTIA32.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
	./limine/limine bios-install $(IMAGE_NAME).iso
endif
ifeq ($(ARCH),aarch64)
	cp -v limine/limine-uefi-cd.bin iso_root/boot/limine/
	cp -v limine/BOOTAA64.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs -R -r -J \
		-hfsplus -apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
endif
ifeq ($(ARCH),riscv64)
	cp -v limine/limine-uefi-cd.bin iso_root/boot/limine/
	cp -v limine/BOOTRISCV64.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs -R -r -J \
		-hfsplus -apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
endif
ifeq ($(ARCH),loongarch64)
	cp -v limine/limine-uefi-cd.bin iso_root/boot/limine/
	cp -v limine/BOOTLOONGARCH64.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs -R -r -J \
		-hfsplus -apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
endif
	rm -rf iso_root

$(IMAGE_NAME).hdd: limine/limine kernel initramfs.cpio
	rm -f $(IMAGE_NAME).hdd
	dd if=/dev/zero bs=1M count=0 seek=64 of=$(IMAGE_NAME).hdd
ifeq ($(ARCH),x86_64)
	PATH=$$PATH:/usr/sbin:/sbin sgdisk $(IMAGE_NAME).hdd -n 1:2048 -t 1:ef00 -m 1
	./limine/limine bios-install $(IMAGE_NAME).hdd
else
	PATH=$$PATH:/usr/sbin:/sbin sgdisk $(IMAGE_NAME).hdd -n 1:2048 -t 1:ef00
endif
	mformat -i $(IMAGE_NAME).hdd@@1M
	mmd -i $(IMAGE_NAME).hdd@@1M ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	mcopy -i $(IMAGE_NAME).hdd@@1M kernel/bin-$(ARCH)/kernel ::/boot
	mcopy -i $(IMAGE_NAME).hdd@@1M initramfs.cpio ::/boot
	mcopy -i $(IMAGE_NAME).hdd@@1M limine.conf ::/boot/limine
ifeq ($(ARCH),x86_64)
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/limine-bios.sys ::/boot/limine
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTX64.EFI ::/EFI/BOOT
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTIA32.EFI ::/EFI/BOOT
endif

unity/src/unity.c:
	git clone https://github.com/ThrowTheSwitch/Unity.git unity --depth=1

override TEST_CFLAGS := $(HOST_CFLAGS) -std=gnu11 \
    -I kernel/src/arch/x86_64/include \
    -I kernel/src \
    -I kernel/limine-protocol/include \
    -I abi/include \
    -I unity/src

.PHONY: test
test: unity/src/unity.c
	mkdir -p tests/bin
	$(HOST_CC) $(TEST_CFLAGS) tests/stubs.c tests/test_idt.c kernel/src/arch/x86_64/idt.c unity/src/unity.c -o tests/bin/test_idt
	$(HOST_CC) $(TEST_CFLAGS) tests/stub_gdt.c tests/test_gdt.c kernel/src/arch/x86_64/gdt.c unity/src/unity.c -o tests/bin/test_gdt
	$(HOST_CC) $(TEST_CFLAGS) tests/stub_vmm.c tests/test_vmm.c kernel/src/arch/x86_64/vmm.c unity/src/unity.c -o tests/bin/test_vmm
	$(HOST_CC) $(TEST_CFLAGS) tests/stub_slab.c tests/test_slab.c kernel/src/arch/x86_64/slab.c unity/src/unity.c -o tests/bin/test_slab
	$(HOST_CC) $(TEST_CFLAGS) tests/stub_scheduler.c tests/test_scheduler.c kernel/src/arch/x86_64/scheduler.c unity/src/unity.c -o tests/bin/test_scheduler
	$(HOST_CC) $(TEST_CFLAGS) tests/stub_syscall.c tests/test_syscall.c kernel/src/arch/x86_64/syscall.c unity/src/unity.c -o tests/bin/test_syscall
	$(HOST_CC) $(TEST_CFLAGS) tests/stub_pmm.c tests/test_pmm.c kernel/src/arch/x86_64/pmm.c unity/src/unity.c -o tests/bin/test_pmm
	./tests/bin/test_idt
	./tests/bin/test_gdt
	./tests/bin/test_vmm
	./tests/bin/test_slab
	./tests/bin/test_scheduler
	./tests/bin/test_syscall
	./tests/bin/test_pmm

.PHONY: clean
clean:
	$(MAKE) -C kernel clean
	rm -rf iso_root $(IMAGE_NAME).iso $(IMAGE_NAME).hdd tests/bin \
		obj-userland $(USERLAND_ELFS) initramfs_root/bin initramfs.cpio

.PHONY: docs
docs:
	doxygen Doxyfile

.PHONY: docs-clean
docs-clean:
	rm -rf docs

.PHONY: distclean
distclean:
	$(MAKE) -C kernel distclean
	rm -rf iso_root *.iso *.hdd limine edk2-ovmf unity tests/bin docs

.PHONY: debug
debug: edk2-ovmf $(IMAGE_NAME).iso
	qemu-system-$(ARCH) \
		-M q35 \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
		-cdrom $(IMAGE_NAME).iso \
		-s -S \
		$(QEMUFLAGS)
