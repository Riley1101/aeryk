#include <elf.h>
#include <pmm.h>
#include <string.h>
#include <vmm.h>

static inline uint64_t page_align_down(uint64_t addr) {
  return addr & ~(uint64_t)(PAGE_SIZE - 1);
}

static inline uint64_t page_align_up(uint64_t addr) {
  return (addr + PAGE_SIZE - 1) & ~(uint64_t)(PAGE_SIZE - 1);
}

int elf_load(vfs_node_t *file, uint64_t *pml4, uint64_t *out_entry) {
  if (!file || !file->data) {
    return -1;
  }

  Elf64_Ehdr *ehdr = (Elf64_Ehdr *)file->data;

  if (ehdr->e_magic != ELF_MAGIC || ehdr->e_class != ELFCLASS64 ||
      ehdr->e_data != ELFDATA2LSB || ehdr->e_type != ET_EXEC) {
    return -1;
  }

  Elf64_Phdr *phdrs =
      (Elf64_Phdr *)((uint8_t *)file->data + ehdr->e_phoff);

  for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
    Elf64_Phdr *ph = &phdrs[i];
    if (ph->p_type != PT_LOAD) {
      continue;
    }

    uint64_t flags = PTE_PRESENT | PTE_USER;
    if (ph->p_flags & PF_W) {
      flags |= PTE_WRITABLE;
    }
    if (!(ph->p_flags & PF_X)) {
      flags |= PTE_NX;
    }

    uint64_t seg_start = page_align_down(ph->p_vaddr);
    uint64_t seg_end = page_align_up(ph->p_vaddr + ph->p_memsz);

    for (uint64_t page_vaddr = seg_start; page_vaddr < seg_end;
         page_vaddr += PAGE_SIZE) {
      void *phys = pmm_alloc_page();
      if (!phys) {
        return -1;
      }

      void *virt = (void *)((uint64_t)phys + hhdm_offset);
      memset(virt, 0, PAGE_SIZE);

      // Copy any file bytes that fall within this page.
      uint64_t page_end = page_vaddr + PAGE_SIZE;
      uint64_t file_start = ph->p_vaddr;
      uint64_t file_end = ph->p_vaddr + ph->p_filesz;

      uint64_t copy_start = page_vaddr > file_start ? page_vaddr : file_start;
      uint64_t copy_end = page_end < file_end ? page_end : file_end;

      if (copy_start < copy_end) {
        uint64_t copy_len = copy_end - copy_start;
        uint64_t file_offset = ph->p_offset + (copy_start - file_start);
        uint64_t page_offset = copy_start - page_vaddr;

        memcpy((uint8_t *)virt + page_offset,
               (uint8_t *)file->data + file_offset, copy_len);
      }

      vmm_map_page(pml4, page_vaddr, (uint64_t)phys, flags);
    }
  }

  *out_entry = ehdr->e_entry;
  return 0;
}
