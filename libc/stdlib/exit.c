
#include <stdint.h>
#include <sys/syscall.h>

/**
 * @brief Terminate the calling process with the given status code.
 * @param status The exit status code to return to the operating system.
 */
void exit(int status) {
  asm volatile("syscall"
               :
               : "a"((uint64_t)SYS_exit), "D"((uint64_t)status)
               : "rcx", "r11", "memory"

  );

  // tell compiler this loop never ends
  __builtin_unreachable();
}
