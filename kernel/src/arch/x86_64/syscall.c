#include "arch/x86_64/fs/vfs.h"
#include <stddef.h>
#include <string.h>
#include <utils.h>

#include <arch/x86_64/drivers/keyboard.h>
#include <process.h>
#include <stdint.h>
#include <syscall.h>
#include <tty.h>

struct syscall_frame {
  uint64_t rax;
  uint64_t r9, r8, r10, rdx, rsi, rdi;
  uint64_t user_rsp;
  uint64_t user_rflags;
  uint64_t user_rip;
};

// https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md#x86_64-64_bit
void syscall_handler_c(struct syscall_frame *frame) {
  switch (frame->rax) {
  case 0: // sys_read
    if (frame->rdi == 0) {
      frame->rax = keyboard_read((char *)frame->rsi, (int)frame->rdx);

    } else if (frame->rdi >= 3 && frame->rdi < MAX_FDS) {
      // reading from a file
      file_descriptor_t *fd = &current_process->fd_table[frame->rdi];
      if (fd->node && fd->node->type == VFS_FILE) {
        uint32_t bytes_to_read = frame->rdx;
        if (fd->offset + bytes_to_read > fd->node->size) {
          bytes_to_read = fd->node->size - fd->offset;
        }
        if (bytes_to_read > 0) {
          int _ = memcmp((void *)frame->rsi,
                         (uint8_t *)fd->node->data + fd->offset, bytes_to_read);
          fd->offset += bytes_to_read;
        }
        frame->rax = bytes_to_read;
      } else {
        frame->rax = -1;
      }
    } else {
      frame->rax = -1;
    }
    break;

  case 1: // sys_write
    if (frame->rdi == 1) {
      print((const char *)frame->rsi);
    }
    frame->rax = frame->rdx;
    break;
    // sys_read
  case 2: {
    const char *filename = (const char *)frame->rdi;
    vfs_node_t *file = vfs_find_node(vfs_root, filename);
    if (!file || file->type != VFS_FILE) {
      frame->rax = -1; // file not found
      break;
    }

    int fd_index = -1;
    // starts at 3 because 0,1,2 for stdio
    for (int i = 3; i < MAX_FDS; i++) {
      if (current_process->fd_table[i].node == NULL) {
        fd_index = i;
        break;
      }
    }

    if (fd_index != -1) {
      current_process->fd_table[fd_index].node = file;
      current_process->fd_table[fd_index].offset = 0;
      current_process->fd_table[fd_index].flags = frame->rsi;
    } else {
      frame->rax = -1;
    }
  }
  case 3:
    if (frame->rdi >= 3 && frame->rdi < MAX_FDS) {
      current_process->fd_table[frame->rdi].node = NULL;
      frame->rax = 0;
    } else {
      frame->rax = -1;
    }
    break;
  case 60: // sys_exit
    print("\n[Syscall] Process exited.\n");

    if (current_process) {
      current_process->state = PROCESS_DEAD;
      schedule();
    }

    for (;;) {
      asm volatile("hlt");
    }
    break;

  default:
    print("unknown syscall");
    frame->rax = -1;
    break;
  }
}

void init_syscalls(void) {

  // Enable syscall extension bit
  uint64_t efer = rdmsr(MSR_EFER);
  wrmsr(MSR_EFER, efer | 1);

  // 2. Configure STAR:
  // Bits 32-47: Kernel CS (0x08)
  // bits 48-63: Base for User CS/SS (0x10) -> SYSRET users 0x10+16 = 0x20
  // for CS, 0x10+8=0x18 for SS
  wrmsr(MSR_STAR, ((uint64_t)0x08 << 32) | ((uint64_t)0x10 << 48));

  wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);

  wrmsr(MSR_FMASK, 0x200);
}
