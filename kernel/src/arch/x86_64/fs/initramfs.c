#include <arch/x86_64/fs/vfs.h>
#include <arch/x86_64/fs/initramfs.h>
#include <stdint.h>
#include <string.h>
#include <tty.h>

#define ALIGN4(val) (((val) + 3) & ~3)

static uint32_t parse_hex_str(const char *str, int len) {
  uint32_t val = 0;
  for (int i = 0; i < len; i++) {
    val <<= 4;
    if (str[i] >= '0' && str[i] <= '9') {
      val |= (str[i] - '0');
    } else if (str[i] >= 'A' && str[i] <= 'F') {
      val |= (str[i] - 'A' + 10);
    } else if (str[i] >= 'a' && str[i] <= 'f') {
      val |= (str[i] - 'a' + 10);
    }
  }
  return val;
}

void initramfs_init(void *base_address, size_t size) {
  init_vfs();

  uint64_t current_addr = (uint64_t)base_address;
  uint64_t end_addr = current_addr + size;
  while (current_addr < end_addr) {
    struct cpio_newc_header *header = (struct cpio_newc_header *)current_addr;
    if (memcmp(header->c_magic, MAGIC_NEWC, 6) != 0 &&
        memcmp(header->c_magic, MAGIC_CRC, 6) != 0) {
      print("[initramfs] Error: Invalid cpio magic.\n");
      break;
    }

    uint32_t filesize = parse_hex_str(header->c_filesize, 8);
    uint32_t namesize = parse_hex_str(header->c_namesize, 8);
    uint32_t mode = parse_hex_str(header->c_mode, 8);

    const char *filename =
        (const char *)current_addr + sizeof(struct cpio_newc_header);

    if (memcmp(filename, "TRAILER!!!", 10) == 0) {
      break;
    }

    uint64_t data_addr =
        ALIGN4(current_addr + sizeof(struct cpio_newc_header) + namesize);
    if (filename[0] != '.' || filename[1] != '\0') {
      vfs_node_t *parent_dir = create_directories(filename);
      const char *leaf_name = filename;

      for (int i = 0; filename[i]; i++) {
        if (filename[i] == '/') {
          leaf_name = &filename[i + 1];
        }
      }

      // S_IFDIR = 0x4000 S_IFREG=0x80000
      // // if dir
      if ((mode & 0xF000) == 0x4000) {
        if (!vfs_find_node(parent_dir, leaf_name)) {
          vfs_add_child(parent_dir, vfs_create_node(leaf_name, VFS_DIRECTORY));
        }
      } else if ((mode & 0xF000) == 0x8000) {
        vfs_node_t *file = vfs_create_node(leaf_name, VFS_FILE);
        file->size = filesize;
        // point to RAM
        file->data = (void *)data_addr;
        vfs_add_child(parent_dir, file);
      }
    }
    current_addr = ALIGN4(data_addr + filesize);
  }
  print("[-] Initramfs Mounted.\n");
}
