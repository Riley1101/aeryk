
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Unaligned-safe 8-byte access: `aligned(1)` lets it sit at any byte offset
// (x86_64 doesn't fault on unaligned loads/stores, so no correctness issue,
// just avoids UB from asserting 8-byte alignment we haven't checked), and
// `may_alias` exempts it from strict-aliasing so reading/writing through
// this type when the underlying bytes came from an unrelated object (any
// buffer memcpy/memset/memmove is handed) isn't undefined behavior.
typedef uint64_t u64_unaligned __attribute__((aligned(1), may_alias));

/**
 * @brief Copy a block of memory from source to destination.
 *
 * @param dest Pointer to the destination memory block.
 * @param src Pointer to the source memory block.
 * @param len Number of bytes to copy.
 * @return Pointer to the destination memory block.
 */
void *memcpy(void *restrict dest, const void *restrict src, size_t len)
{
    uint8_t *restrict pdest = (uint8_t *restrict)dest;
    const uint8_t *restrict psrc = (uint8_t *restrict)src;

    // A compositor blitting a client's shared buffer into the framebuffer
    // moves hundreds of KB per call -- copying 8 bytes/iteration instead
    // of 1 cuts the loop-overhead-dominated cost roughly 8x.
    size_t words = len / sizeof(uint64_t);
    u64_unaligned *restrict wdest = (u64_unaligned *restrict)pdest;
    const u64_unaligned *restrict wsrc = (const u64_unaligned *restrict)psrc;
    for (size_t i = 0; i < words; i++)
    {
        wdest[i] = wsrc[i];
    }

    for (size_t i = words * sizeof(uint64_t); i < len; i++)
    {
        pdest[i] = psrc[i];
    }
    return dest;
}

/**
 * @brief Set a block of memory to a specified value.
 *
 * @param s Pointer to the memory block to set.
 * @param c Value to set each byte of the memory block to.
 * @param n Number of bytes to set.
 * @return Pointer to the memory block.
 */
void *memset(void *s, int c, size_t n)
{
    uint8_t *p = (uint8_t *)s;
    uint8_t byte = (uint8_t)c;

    uint64_t pattern = byte;
    pattern |= pattern << 8;
    pattern |= pattern << 16;
    pattern |= pattern << 32;

    size_t words = n / sizeof(uint64_t);
    u64_unaligned *wp = (u64_unaligned *)p;
    for (size_t i = 0; i < words; i++)
    {
        wp[i] = pattern;
    }

    for (size_t i = words * sizeof(uint64_t); i < n; i++)
    {
        p[i] = byte;
    }
    return s;
}

/**
 * @brief Move a block of memory from source to destination.
 *
 * @param dest Pointer to the destination memory block.
 * @param src Pointer to the source memory block.
 * @param n Number of bytes to move.
 * @return Pointer to the destination memory block.
 */
void *memmove(void *dest, const void *src, size_t n)
{
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;
    if ((uintptr_t)src > (uintptr_t)dest)
    {
        size_t words = n / sizeof(uint64_t);
        u64_unaligned *wdest = (u64_unaligned *)pdest;
        const u64_unaligned *wsrc = (const u64_unaligned *)psrc;
        for (size_t i = 0; i < words; i++)
        {
            wdest[i] = wsrc[i];
        }
        for (size_t i = words * sizeof(uint64_t); i < n; i++)
        {
            pdest[i] = psrc[i];
        }
    }
    else if ((uintptr_t)src < (uintptr_t)dest)
    {
        // Destination overlaps forward into source: must copy back-to-front
        // so no byte is overwritten before it's read. Peel off the tail
        // bytes that don't form a full word first (safe -- they're the
        // highest addresses, nothing above them left to preserve), then
        // copy the remaining whole words in decreasing order.
        size_t tail_bytes = n % sizeof(uint64_t);
        for (size_t i = n; i > n - tail_bytes; i--)
        {
            pdest[i - 1] = psrc[i - 1];
        }
        size_t words = (n - tail_bytes) / sizeof(uint64_t);
        u64_unaligned *wdest = (u64_unaligned *)pdest;
        const u64_unaligned *wsrc = (const u64_unaligned *)psrc;
        for (size_t i = words; i > 0; i--)
        {
            wdest[i - 1] = wsrc[i - 1];
        }
    }
    return dest;
}

/**
 * @brief Compare two blocks of memory.
 * 
 * @param s1 Pointer to the first memory block.
 * @param s2 Pointer to the second memory block.
 * @param n Number of bytes to compare.
 * @return 0 if the blocks are identical, negative if the first block is less than the second, positive otherwise.
 */
int memcmp(const void *s1, const void *s2, size_t n)
{
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++)
    {
        if (p1[i] != p2[i])
        {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }
    return 0;
}
