#ifndef _ABI_MOUSE_H
#define _ABI_MOUSE_H

#include <stdint.h>

/**
 * @brief One decoded PS/2 mouse movement/button-state update, shared
 * between the kernel (mouse.c, decoded from raw 3-byte PS/2 packets) and
 * userland (SYS_mouse_read callers). `dx`/`dy` are relative motion since
 * the previous packet; per the PS/2 convention, positive `dy` means the
 * mouse moved up, not down -- callers drawing to a framebuffer (Y growing
 * downward) need to negate it.
 */
typedef struct {
  int16_t dx;
  int16_t dy;
  uint8_t buttons;
} mouse_packet_t;

#define MOUSE_BTN_LEFT 0x01
#define MOUSE_BTN_RIGHT 0x02
#define MOUSE_BTN_MIDDLE 0x04

#endif // !_ABI_MOUSE_H
