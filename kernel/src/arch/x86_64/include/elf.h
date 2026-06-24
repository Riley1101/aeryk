#ifndef ELF_H
#define ELF_H

#include <arch/x86_64/fs/vfs.h>
#include <stdint.h>

#define ELF_MAGIC 0x464C457F // "\x7FELF" little-endian

#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ET_EXEC 2

#define PT_LOAD 1

#define PF_X (1 << 0)
#define PF_W (1 << 1)
#define PF_R (1 << 2)

typedef struct {
  uint32_t e_magic; // 0x7F 'E' 'L' 'F'
  uint8_t e_class;
  uint8_t e_data;
  uint8_t e_version;
  uint8_t e_osabi;
  uint8_t e_abiversion;
  uint8_t e_pad[7];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version2;
  uint64_t e_entry;
  uint64_t e_phoff;
  uint64_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} __attribute__((packed)) Elf64_Ehdr;

typedef struct {
  uint32_t p_type;
  uint32_t p_flags;
  uint64_t p_offset;
  uint64_t p_vaddr;
  uint64_t p_paddr;
  uint64_t p_filesz;
  uint64_t p_memsz;
  uint64_t p_align;
} __attribute__((packed)) Elf64_Phdr;

// Loads the PT_LOAD segments of an ELF64 executable into the given
// pagetable. Returns 0 on success and writes the program entry point
// to *out_entry, or -1 on failure.
int elf_load(vfs_node_t *file, uint64_t *pml4, uint64_t *out_entry);

#endif // !ELF_H
