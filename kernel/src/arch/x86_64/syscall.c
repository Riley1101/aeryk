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

void syscall_handler_c(struct syscall_frame *frame) {
  switch (frame->rax) {
  case 0: // sys_read
    if (frame->rdi == 0) {
      frame->rax = keyboard_read((char *)frame->rsi, (int)frame->rdx);
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

void initSyscalls(void) {

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
