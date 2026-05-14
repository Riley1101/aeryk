#include "keyboard.h"
#include <arch/x86_64/cpu/idt.h>
#include <drivers/display/tty.h>
#include <lib/utils.h>
#include <stdint.h>

#define PS2DATA_PORT 0x60
#define PS2STATUS_PORT 0x64
#define PS2CMD_PORT 0x64

static void ps2_wait_write(void) {
  while (in_portb(PS2STATUS_PORT) & 0x02)
    ;
}

static void ps2_wait_read(void) {
  while (!(in_portb(PS2STATUS_PORT) & 0x01))
    ;
}

const char kbd_us[128] = {
    0,   27,   '1',  '2', '3',  '4', '5', '6', '7', '8', '9', '0', '-',
    '=', '\b', '\t', 'q', 'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']',  '\n', 0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    ';', '\'', '`',  0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
    '.', '/',  0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   '-', 0,   0,   0,
    '+', 0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0};

void onIrq1(struct interrupt_frame *frame) {
  (void)frame;

  uint8_t scancode = in_portb(PS2DATA_PORT);

  if (!(scancode & 0x80)) {
    char c = kbd_us[scancode];
    if (c != 0) {
      char str[2] = {c, '\0'};
      print(global_renderer, str);
    }
  }
}

void initKeyboard() {
  // Read current i8042 command byte
  ps2_wait_write();
  out_portb(PS2CMD_PORT, 0x20);
  ps2_wait_read();
  uint8_t cmd = in_portb(PS2DATA_PORT);

    // Enable keyboard interrupt (bit 0), scancode translation (bit 6)
    // Clear keyboard disable (bit 4)
    cmd |= 0x01 | 0x40;
    cmd &= ~0x10;

  // Write back command byte
  ps2_wait_write();
  out_portb(PS2CMD_PORT, 0x60);
  ps2_wait_write();
  out_portb(PS2DATA_PORT, cmd);

  irq_install_handler(1, onIrq1);
}
