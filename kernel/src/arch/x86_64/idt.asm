[bits 64]

extern isr_handler

; CPU exceptions without an error code — push dummy 0 first
%macro ISR_NOERR 1
global isr%1
isr%1:
    push qword 0
    push qword %1
    jmp isr_common_stub
%endmacro

; CPU exceptions that already push an error code
%macro ISR_ERR 1
global isr%1
isr%1:
    push qword %1
    jmp isr_common_stub
%endmacro

ISR_NOERR 0   ; #DE divide error
ISR_NOERR 1   ; #DB debug
ISR_NOERR 2   ; NMI
ISR_NOERR 3   ; #BP breakpoint
ISR_NOERR 4   ; #OF overflow
ISR_NOERR 5   ; #BR bound range exceeded
ISR_NOERR 6   ; #UD invalid opcode
ISR_NOERR 7   ; #NM device not available
ISR_ERR   8   ; #DF double fault
ISR_NOERR 9   ; coprocessor segment overrun (legacy)
ISR_ERR   10  ; #TS invalid TSS
ISR_ERR   11  ; #NP segment not present
ISR_ERR   12  ; #SS stack-segment fault
ISR_ERR   13  ; #GP general protection fault
ISR_ERR   14  ; #PF page fault
ISR_NOERR 15  ; reserved
ISR_NOERR 16  ; #MF x87 FPU error
ISR_ERR   17  ; #AC alignment check
ISR_NOERR 18  ; #MC machine check
ISR_NOERR 19  ; #XM SIMD floating-point
ISR_NOERR 20  ; #VE virtualization exception
ISR_ERR   21  ; #CP control protection
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_ERR   29  ; VMM communication exception
ISR_ERR   30  ; #SX security exception
ISR_NOERR 31

; IRQ 0-15 -> vectors 32-47
%assign i 32
%rep 16
    ISR_NOERR i
    %assign i i+1
%endrep

isr_common_stub:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp        ; arg0: pointer to interrupt_frame
    and rsp, -16
    call isr_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16         ; discard int_no + err_code
    iretq
