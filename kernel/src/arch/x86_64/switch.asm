[bits 64]
global switch_task

; void switch_task(process_t* prev, process_t* next);
; System V API pass arguments in RDI = prev, RSI = next

switch_task:
  ; 1. Save callee-save registers for the current thread
  push rbx
  push rbp
  push r12
  push r13
  push r14
  push r15

  ; 2. Save current stack pointer into prev->rsp
  ; prev is in RDI and rsp is offset 8 in the process_t struct
  mov [rdi + 8], rsp

  ; 3. Switch stack pointer to next->rsp
  ; next is in RSI
  mov rsp, [rsi + 8]

  ; 4. Restore the callee-save register for the next thread
  pop r15
  pop r14
  pop r13
  pop r12
  pop rbp
  pop rbx

  ; 5. Return to next thread's execution point
  ret

