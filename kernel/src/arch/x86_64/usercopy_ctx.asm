[bits 64]
global usercopy_save_context
global usercopy_restore_context

; long usercopy_save_context(void *buf);
;
; Saves the callee-saved registers, RSP, and the return address (i.e.
; everything needed to resume execution right after this call) into the
; 8-slot buf: [rbx, rbp, r12, r13, r14, r15, rsp, rip]. Returns 0 the
; first time it's called normally; usercopy_restore_context() later
; jumps back here and makes this same call site "return" a nonzero
; value instead, without ever unwinding through the intervening call
; frames (isr_handler and everything it called).
;
; A hand-rolled setjmp/longjmp instead of __builtin_setjmp/
; __builtin_longjmp: the builtins are documented as only supported for
; implementing libc's own setjmp/longjmp, and in practice do not survive
; being resumed from deep inside an unrelated call stack (an interrupt
; handler) at -O2 -- observed as a repeating fault storm when used that
; way here. This version only touches registers explicitly, so there's
; nothing for the optimizer to get wrong.
usercopy_save_context:
    mov [rdi + 0], rbx
    mov [rdi + 8], rbp
    mov [rdi + 16], r12
    mov [rdi + 24], r13
    mov [rdi + 32], r14
    mov [rdi + 40], r15
    lea rax, [rsp + 8]      ; caller's rsp, i.e. rsp after this call returns
    mov [rdi + 48], rax
    mov rax, [rsp]          ; return address pushed by the `call`
    mov [rdi + 56], rax
    xor eax, eax
    ret

; void usercopy_restore_context(void *buf, long retval);
;
; Restores the registers saved by usercopy_save_context(buf) and jumps
; straight to the saved return address with RAX set to `retval`, so the
; original usercopy_save_context() call appears to return `retval`
; instead of 0. Never returns to its own caller.
usercopy_restore_context:
    mov r11, [rdi + 56]     ; saved rip -- caller-saved scratch, safe to
                             ; clobber since we never return from here
    mov rbx, [rdi + 0]
    mov rbp, [rdi + 8]
    mov r12, [rdi + 16]
    mov r13, [rdi + 24]
    mov r14, [rdi + 32]
    mov r15, [rdi + 40]
    mov rsp, [rdi + 48]
    mov rax, rsi
    jmp r11
