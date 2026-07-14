
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
    for (size_t i = 0; i < len; i++)
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
    for (size_t i = 0; i < n; i++)
    {
        p[i] = (uint8_t)c;
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
        for (size_t i = 0; i < n; i++)
        {
            pdest[i] = psrc[i];
        }
    }
    else if ((uintptr_t)src < (uintptr_t)dest)
    {
        for (size_t i = n; i > 0; i--)
        {
            pdest[i - 1] = psrc[i - 1];
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
