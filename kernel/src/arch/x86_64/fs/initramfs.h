#ifndef INITRAMFS_H
#define INITRAMFS_H

// https://products.aspose.com/zip/most-common-archives/what-is-cpio/
#include <stddef.h>

#define MAGIC_NEWC "070701"
#define MAGIC_CRC "070702"

struct cpio_newc_header {
  char c_magic[6]; // this is usually fixed "070701: or "070702"
  char c_ino[8];
  char c_mode[8];
  char c_uid[8];
  char c_gid[8];
  char c_nlink[8];
  char c_mtime[8];
  char c_filesize[8];
  char c_devmajor[8];
  char c_devminor[8];
  char c_rdevmajor[8];
  char c_rdevminor[8];
  char c_namesize[8];
  char c_check[8];
} __attribute__((packed));

void initramfs_init(void *base_address, size_t size);

#endif // !INITRAMFS_H
