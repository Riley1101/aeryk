[bits 64]

global gdt_flush
global tss_flush

; void gdt_flush(struct gdt_ptr_struct *ptr)
; rdi = pointer to GDT descriptor (System V AMD64 ABI)

gdt_flush:
    lgdt [rdi]                      ; load GDT register

                                    ; Reload CS via far return trick
    push 0x08                       ; kernel code segment selector
    lea  rax, [rel .reload_cs]
    push rax
    retfq

.reload_cs:
    mov ax, 0x10                    ; kernel data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

; void tss_flush(void)
; Loads the TSS selector (GDT index 5 = 0x28) into the Task Register
tss_flush:
    mov ax, 0x28                    ; TSS descriptor selector
    ltr ax                          ; load task register
    ret
