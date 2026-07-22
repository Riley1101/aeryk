;; Libc _start

[bits 64]

global _start
extern main
extern exit

section .text
  _start:
   ; 1. Clear the frame pointer to mark the top of the call stack
   xor rbp, rbp

   ; 2. setup argc, argv and envp
   ; The kernel lays out [argc][argv] as two 8-byte words at the initial
   ; RSP (see setup_user_stack_args() in process.c); argv points at a
   ; NULL-terminated array of pointers into the argument strings below it.
   pop rdi ; rdi = argc
   pop rsi ; rsi = argv
   xor rdx, rdx ; rdx = envp = NULL

   ; 3. Align the stack to 16-byte boundary before the call, per the
   ; SysV ABI. Popping argc/argv above may have broken alignment.
   and rsp, -16

   call main

   mov rdi, rax
   call exit

.hang:
   jmp .hang



