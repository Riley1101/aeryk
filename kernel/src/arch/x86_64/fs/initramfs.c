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

    const char *filename =
        (const char *)current_addr + sizeof(struct cpio_newc_header);

    if (memcmp(filename, "TRAILER!!!", 10) == 0) {
      break;
    }

    print("  -> Found: ");
    print(filename);
    print("\n");

    uint64_t data_addr =
        ALIGN4(current_addr + sizeof(struct cpio_newc_header) + namesize);
    current_addr = ALIGN4(data_addr + filesize);
  }
}
