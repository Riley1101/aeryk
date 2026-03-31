#ifndef STRING_H
#define STRING_H 1

#include <sys/cdef.h>
#include <stddef.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n);
void *memset(void *dest, int src, size_t count);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);


size_t strlen(const char* str);
#endif
