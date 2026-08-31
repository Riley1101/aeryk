[bits 64]
global switch_task

; void switch_task(process_t* prev, process_t* next);
; System V API pass arguments in RDI = prev, RSI = next

switch_task:
  ; 1. Save callee-save registers and RFLAGS (incl. IF) for the current thread.
  ; RFLAGS must be saved per-task here -- it is not restored by this ret-based
  ; switch the way an iretq would restore it, so without pushfq/popfq, IF just
  ; carries over from whichever task last ran, regardless of which task
  ; resumes next.
  pushfq
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

  ; 3b. Switch address space to next->cr3 (offset 16 in process_t)
  mov rax, [rsi + 16]
  mov cr3, rax

  ; 4. Restore the callee-save registers and RFLAGS for the next thread
  pop r15
  pop r14
  pop r13
  pop r12
  pop rbp
  pop rbx
  popfq

  ; 5. Return to next thread's execution point
  ret

