#include <stddef.h>
#include <string.h>
#include <utils.h>

#include <arch/x86_64/fs/vfs.h>
#include <arch/x86_64/drivers/keyboard.h>

#include <process.h>
#include <stdint.h>
#include <syscall.h>
#include <tty.h>

/**
 * @brief Structure representing the state of registers during a system call.
 * 
 * This structure is used to capture the values of registers when a user process
 * invokes a system call. It contains the values of general-purpose registers,
 * as well as the user stack pointer, flags, and instruction pointer.
 */
struct syscall_frame {
  uint64_t rax;
  uint64_t r9, r8, r10, rdx, rsi, rdi;
  uint64_t user_rsp;
  uint64_t user_rflags;
  uint64_t user_rip;
};

/**
 * @brief Handles system calls invoked by user processes.
 * 
 * This function is called when a user process invokes a system call using the `syscall` instruction.
 * It takes a pointer to a `syscall_frame` structure that contains the register values at
 * the time of the system call. The function processes the system call based on the value in the `rax` register,
 * which indicates the system call number. It supports various system calls such as reading from
 * the keyboard, writing to the console, opening and closing files, and exiting the process.
 */
// https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md#x86_64-64_bit
void syscall_handler_c(struct syscall_frame *frame) {
  switch (frame->rax) {
  case SYS_read:
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
          memcpy((void *)frame->rsi,
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

  case SYS_write:
    if (frame->rdi == 1) {
      print((const char *)frame->rsi);
    }
    frame->rax = frame->rdx;
    break;
  case SYS_open: {
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
      frame->rax = fd_index;
    } else {
      frame->rax = -1;
    }
    break;
  }
  case SYS_spawn: {
    const char *user_path = (const char *)frame->rdi;
    if (!user_path) {
      frame->rax = -1;
      break;
    }

    char path[128];
    size_t i = 0;
    for (; i < sizeof(path) - 1 && user_path[i] != '\0'; i++) {
      path[i] = user_path[i];
    }
    path[i] = '\0';

    if (i == 0) {
      frame->rax = -1;
      break;
    }

    process_t *child = create_user_process(path);
    frame->rax = child ? (int64_t)child->pid : -1;
    break;
  }
  case SYS_close:
    if (frame->rdi >= 3 && frame->rdi < MAX_FDS) {
      current_process->fd_table[frame->rdi].node = NULL;
      frame->rax = 0;
    } else {
      frame->rax = -1;
    }
    break;
  case SYS_exit:
    print("\n[Syscall] Process exited.\n");

    if (current_process) {
      current_process->exit_code = (int)frame->rdi;
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
