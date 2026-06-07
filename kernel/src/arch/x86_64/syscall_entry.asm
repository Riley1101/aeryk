[bits 64]

global syscall_entry
global user_rsp_scratch
global kernel_rsp_scratch

extern syscall_handler_c

section .data
user_rsp_scratch dq 0
kernel_rsp_scratch dq 0

section .text
syscall_entry:
  ; SYSCALL save RIP to RCX and RFLAGS into R11
  ; Swap to the kernel stack safely
  mov [rel user_rsp_scratch], rsp
  mov rsp, [rel kernel_rsp_scratch]

  ; build syscall frame
  push rcx ; Save user RIP
  push r11 ; Save user RFLAGs
  push qword [rel user_rsp_scratch] ; save user RSP

  ; System V AMD64 ABI passes args in: RDI, RSI, RDX, R10, R8, R9 (SYSCALL uses R10 instead of RCX)
  push rdi
  push rsi
  push rdx
  push r10
  push r8
  push r9
  push rax ; syscall number

  ; Pass the frame pointer to C handler
  mov rdi, rsp
  call syscall_handler_c

  ; Restore registers
  pop rax
  pop r9
  pop r8
  pop r10
  pop rdx
  pop rsi
  pop rdi

  ; Restore RSP, RFLAGS and RIP

  mov rcx, [rsp + 16] ; user RIP
  mov r11, [rsp + 8] ; user RFLAGS
  mov rsp, [rsp] ; user RSP

  sysretq



