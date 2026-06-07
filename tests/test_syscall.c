#include "unity.h"
#include <stdint.h>
#include <string.h>

/* Mirrors the private struct syscall_frame layout in syscall.c — only the
 * fields the dispatcher reads/writes matter for these tests. */
struct syscall_frame {
    uint64_t rax;
    uint64_t r9, r8, r10, rdx, rsi, rdi;
    uint64_t user_rsp;
    uint64_t user_rflags;
    uint64_t user_rip;
};

void syscall_handler_c(struct syscall_frame *frame);

extern int print_call_count;
extern const char *print_last_str;
void reset_print_stub(void);

extern int keyboard_read_call_count;
extern char *keyboard_read_last_buf;
extern int keyboard_read_last_count;
extern int keyboard_read_return_value;
void reset_keyboard_read_stub(void);

void setUp(void)
{
    reset_print_stub();
    reset_keyboard_read_stub();
}

void tearDown(void) {}

static struct syscall_frame make_frame(uint64_t rax, uint64_t rdi, uint64_t rsi, uint64_t rdx)
{
    struct syscall_frame f = {0};
    f.rax = rax;
    f.rdi = rdi;
    f.rsi = rsi;
    f.rdx = rdx;
    return f;
}

/* --- sys_read (0) --- */

void test_sys_read_from_stdin_delegates_to_keyboard_read(void)
{
    char buf[32];
    keyboard_read_return_value = 5;

    struct syscall_frame frame = make_frame(0, 0 /* fd = stdin */, (uint64_t)buf, sizeof(buf));
    syscall_handler_c(&frame);

    TEST_ASSERT_EQUAL_INT(1, keyboard_read_call_count);
    TEST_ASSERT_EQUAL_PTR(buf, keyboard_read_last_buf);
    TEST_ASSERT_EQUAL_INT((int)sizeof(buf), keyboard_read_last_count);
    TEST_ASSERT_EQUAL_HEX64(5, frame.rax);
}

void test_sys_read_from_non_stdin_returns_error(void)
{
    char buf[8];
    struct syscall_frame frame = make_frame(0, 1 /* fd = stdout, not readable */, (uint64_t)buf, sizeof(buf));
    syscall_handler_c(&frame);

    TEST_ASSERT_EQUAL_INT(0, keyboard_read_call_count);
    TEST_ASSERT_EQUAL_HEX64((uint64_t)-1, frame.rax);
}

/* --- sys_write (1) --- */

void test_sys_write_to_stdout_prints_and_returns_length(void)
{
    const char *msg = "hello";
    struct syscall_frame frame = make_frame(1, 1 /* fd = stdout */, (uint64_t)msg, strlen(msg));
    syscall_handler_c(&frame);

    TEST_ASSERT_EQUAL_INT(1, print_call_count);
    TEST_ASSERT_EQUAL_STRING(msg, print_last_str);
    TEST_ASSERT_EQUAL_HEX64(strlen(msg), frame.rax);
}

void test_sys_write_to_non_stdout_skips_print_but_reports_length(void)
{
    const char *msg = "hello";
    struct syscall_frame frame = make_frame(1, 2 /* fd = stderr, not forwarded to print */, (uint64_t)msg, strlen(msg));
    syscall_handler_c(&frame);

    TEST_ASSERT_EQUAL_INT(0, print_call_count);
    TEST_ASSERT_EQUAL_HEX64(strlen(msg), frame.rax);
}

/* --- unknown syscall --- */

void test_unknown_syscall_warns_and_returns_error(void)
{
    struct syscall_frame frame = make_frame(999, 0, 0, 0);
    syscall_handler_c(&frame);

    TEST_ASSERT_EQUAL_INT(1, print_call_count);
    TEST_ASSERT_EQUAL_HEX64((uint64_t)-1, frame.rax);
}

/* sys_exit (60) is intentionally not covered here: on a real failure it never
 * returns — it halts via `for (;;) asm("hlt")`, which is a privileged
 * instruction that SIGSEGVs (and would hang/crash the test runner) on a
 * hosted x86_64 build. */

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sys_read_from_stdin_delegates_to_keyboard_read);
    RUN_TEST(test_sys_read_from_non_stdin_returns_error);
    RUN_TEST(test_sys_write_to_stdout_prints_and_returns_length);
    RUN_TEST(test_sys_write_to_non_stdout_skips_print_but_reports_length);
    RUN_TEST(test_unknown_syscall_warns_and_returns_error);
    return UNITY_END();
}
