#include <stdint.h>
#include <usercopy.h>

/**
 * @brief Hand-rolled setjmp/longjmp pair (usercopy_ctx.asm) used instead
 * of __builtin_setjmp/__builtin_longjmp -- the builtins are documented
 * as only supported for implementing libc's own setjmp/longjmp, and did
 * not survive being resumed from deep inside an interrupt handler's
 * call stack at -O2 in practice (a repeating fault storm instead of a
 * clean recovery). This version only touches registers explicitly, so
 * there's nothing for the optimizer to get wrong.
 * @return 0 on the initial call; a nonzero `retval` when resumed via
 * usercopy_restore_context().
 */
extern long usercopy_save_context(void *buf);

/**
 * @brief Restores the registers saved by usercopy_save_context(buf) and
 * jumps straight back to that call site, making it return `retval`.
 * Never returns to its own caller.
 */
extern void usercopy_restore_context(void *buf, long retval);

/**
 * @brief Recovery point for the copy currently in flight: [rbx, rbp,
 * r12, r13, r14, r15, rsp, rip], saved by usercopy_save_context(). Only
 * one copy can be in flight at a time: the kernel is single-core (no
 * SMP yet) and copy_from_user()/copy_to_user() never call back into
 * themselves, so a single global buffer is enough -- there is no other
 * execution context that could be mid-copy when a fault lands.
 */
static uint64_t user_copy_ctx[8];

/**
 * @brief Set only while a copy_from_user()/copy_to_user() call is
 * between its usercopy_save_context() and the end of the byte loop.
 * Checked by usercopy_recover_or_return() to tell a genuine
 * bad-user-pointer fault apart from an unrelated kernel bug faulting at
 * CPL 0.
 */
static volatile int user_copy_in_flight = 0;

int copy_from_user(void *dst, const void *usrc, size_t n)
{
    if (usercopy_save_context(user_copy_ctx))
    {
        user_copy_in_flight = 0;
        return -1;
    }

    user_copy_in_flight = 1;
    unsigned char *d = dst;
    const volatile unsigned char *s = usrc;
    for (size_t i = 0; i < n; i++)
    {
        d[i] = s[i];
    }
    user_copy_in_flight = 0;
    return 0;
}

int copy_to_user(void *udst, const void *src, size_t n)
{
    if (usercopy_save_context(user_copy_ctx))
    {
        user_copy_in_flight = 0;
        return -1;
    }

    user_copy_in_flight = 1;
    volatile unsigned char *d = udst;
    const unsigned char *s = src;
    for (size_t i = 0; i < n; i++)
    {
        d[i] = s[i];
    }
    user_copy_in_flight = 0;
    return 0;
}

void usercopy_recover_or_return(void)
{
    if (user_copy_in_flight)
    {
        user_copy_in_flight = 0;
        usercopy_restore_context(user_copy_ctx, 1);
    }
}
