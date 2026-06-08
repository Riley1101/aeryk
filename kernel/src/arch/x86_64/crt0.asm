;; Libc _start

[bits 64]

global _start
extern main
extern exit

section .text
  _start:
   ; 1. Clear the frame pointer to mark the top of the call stack
   xor rbp, rbp

   ; 2. Align the stack to 16-byte boundary
   ; If kernel is push things onto the stack, it might not be aligned.
   and rsp, -16

   ; 3. setup argc, argv and envp
   ; For now let's ignore argc and argv for now
   xor rdi, rdi ; rdi = argc = 0
   xor rsi, rsi ; rsi = argv = NULL
   xor rdx, rdx ; rdx = envp = NULL

   call main

   mov rdi, rax
   call exit

.hang:
   jmp .hang



