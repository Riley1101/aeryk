
#include <stdint.h>

void exit(int status) {
  asm volatile("mov $60, %%rax \n"
               "syscall \n"
               :
               : "D"((uint64_t)status)
               : "rax", "rcx", "r11"

  );

  // tell compiler this loop never ends
  __builtin_unreachable();
}
