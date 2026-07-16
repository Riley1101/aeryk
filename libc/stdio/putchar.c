#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

/**
 * @brief Writes a character to stdout.
 * @param ic The character to write.
 * @return Returns the character written as an unsigned char cast to an int or EOF on error
 */
int putchar(int ic) {
    unsigned char c = (unsigned char)ic;
    if (write(1, &c, 1) == 1) {
        return (int)c;
    }

 return EOF;
}
