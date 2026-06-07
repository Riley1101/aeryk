#include <process.h>
#include <stddef.h>
#include <stdint.h>

process_t *current_process = NULL;

int schedule_call_count = 0;
void reset_schedule_call_count(void) { schedule_call_count = 0; }
void schedule(void) { schedule_call_count++; }

int print_call_count = 0;
const char *print_last_str = NULL;
void reset_print_stub(void)
{
    print_call_count = 0;
    print_last_str = NULL;
}
void print(const char *str)
{
    print_call_count++;
    print_last_str = str;
}

int keyboard_read_call_count = 0;
char *keyboard_read_last_buf = NULL;
int keyboard_read_last_count = 0;
int keyboard_read_return_value = 0;
void reset_keyboard_read_stub(void)
{
    keyboard_read_call_count = 0;
    keyboard_read_last_buf = NULL;
    keyboard_read_last_count = 0;
    keyboard_read_return_value = 0;
}
int keyboard_read(char *buf, int count)
{
    keyboard_read_call_count++;
    keyboard_read_last_buf = buf;
    keyboard_read_last_count = count;
    return keyboard_read_return_value;
}

/* Only referenced from init_syscalls, which the tests never call — minimal
 * stand-ins to satisfy the linker. */
uint64_t rdmsr(uint32_t msr) { (void)msr; return 0; }
void wrmsr(uint32_t msr, uint64_t val) { (void)msr; (void)val; }
void syscall_entry(void) {}
