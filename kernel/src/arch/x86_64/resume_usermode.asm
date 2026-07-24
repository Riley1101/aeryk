[bits 64]

global resume_usermode

; void resume_usermode(trapframe_t *tf) __attribute__((noreturn));
; tf is passed in rdi per the System V AMD64 ABI.
;
; Restores a full user-mode register snapshot and iretq's into ring 3. Used
; to land a freshly fork()'d child exactly where its parent was when it
; called fork(), with all callee-saved registers intact. Field order must
; match trapframe_t in process.h:
;   0: rdi   8: rsi   16: rdx  24: r10  32: r8   40: r9
;   48: rbx  56: rbp  64: r12  72: r13  80: r14  88: r15
;   96: rax  104: rip 112: rflags 120: rsp
section .text
resume_usermode:
  ; Build the iretq frame first, while rdi still points at tf and rax/rcx
  ; are free scratch registers (their pre-syscall values were never
  ; preserved across a syscall instruction in the first place).
  push qword 0x1B          ; user SS
  mov rax, [rdi + 120]     ; user RSP
  push rax
  mov rax, [rdi + 112]     ; user RFLAGS
  push rax
  push qword 0x23          ; user CS
  mov rax, [rdi + 104]     ; user RIP
  push rax

  ; Load general-purpose registers. rdi is loaded last since it is our base
  ; pointer until then.
  mov rsi, [rdi + 8]
  mov rdx, [rdi + 16]
  mov r10, [rdi + 24]
  mov r8,  [rdi + 32]
  mov r9,  [rdi + 40]
  mov rbx, [rdi + 48]
  mov rbp, [rdi + 56]
  mov r12, [rdi + 64]
  mov r13, [rdi + 72]
  mov r14, [rdi + 80]
  mov r15, [rdi + 88]
  mov rax, [rdi + 96]
  mov rdi, [rdi + 0]

  iretq
