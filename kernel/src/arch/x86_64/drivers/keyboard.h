#ifndef KEYBOARD_H
#define KEYBOARD_H

void initKeyboard(void);

// Reads up to `count` bytes from the keyboard into `buf`, blocking the
// calling process until at least one byte is available. Stops early on '\n'.
// Returns the number of bytes read.
int keyboard_read(char *buf, int count);

#endif // !KEYBOARD_H
