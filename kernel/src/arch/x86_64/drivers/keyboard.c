#include "keyboard.h"
#include <idt.h>
#include <process.h>
#include <scheduler.h>
#include <stdint.h>
#include <tty.h>
#include <utils.h>

#define PS2DATA_PORT 0x60
#define PS2STATUS_PORT 0x64
#define PS2CMD_PORT 0x64

#define KBD_BUF_SIZE 256

// Circular buffer of decoded chars, filled by the IRQ1 handler and drained
// by keyboard_read(). Single-producer (IRQ), single-consumer (sys_read).
static char kbd_buf[KBD_BUF_SIZE];
static uint32_t kbd_head = 0; // next slot to write
static uint32_t kbd_tail = 0; // next slot to read

// FIFO of processes blocked in keyboard_read() waiting for input, linked
// via each process's dedicated wait_next/wait_prev fields (kept separate
// from the MLFQ's queue_next/queue_prev -- see process.h).
static wait_queue_t kbd_waitq = {0};

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

static void kbd_buf_push(char c) {
  uint32_t next = (kbd_head + 1) % KBD_BUF_SIZE;
  if (next == kbd_tail) {
    return; // buffer full, drop the byte
  }
  kbd_buf[kbd_head] = c;
  kbd_head = next;
}

static int kbd_buf_pop(char *out) {
  if (kbd_tail == kbd_head) {
    return 0; // empty
  }
  *out = kbd_buf[kbd_tail];
  kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
  return 1;
}

void on_irq1(struct interrupt_frame *frame) {
  (void)frame;

  uint8_t scancode = in_portb(PS2DATA_PORT);

  if (!(scancode & 0x80)) {
    char c = kbd_us[scancode];
    if (c != 0) {
      char str[2] = {c, '\0'};
      print(str);

      kbd_buf_push(c);

      process_t *waiter = wait_queue_pop(&kbd_waitq);
      if (waiter) {
        mlfq_enqueue(waiter); // sets state back to PROCESS_READY
      }
    }
  }
}

int keyboard_read(char *buf, int count) {
  int n = 0;

  while (n < count) {
    asm volatile("cli");

    char c;
    if (kbd_buf_pop(&c)) {
      asm volatile("sti");
      buf[n++] = c;
      if (c == '\n') {
        break;
      }
      continue;
    }

    // Nothing buffered: register as a waiter and yield. cli/sti around the
    // empty-check + enqueue close the race against on_irq1 firing in between
    // (a wakeup there just re-readies us before schedule() switches away).
    current_process->state = PROCESS_BLOCKED_KBD;
    wait_queue_push(&kbd_waitq, current_process);
    asm volatile("sti");
    schedule();
  }

  return n;
}

void init_keyboard() {
  // Read current i8042 command byte
  ps2_wait_write();
  out_portb(PS2CMD_PORT, 0x20);
  ps2_wait_read();
  uint8_t cmd = in_portb(PS2DATA_PORT);

  cmd |= 0x01 | 0x40;
  cmd &= ~0x10;

  // Write back command byte
  ps2_wait_write();
  out_portb(PS2CMD_PORT, 0x60);
  ps2_wait_write();
  out_portb(PS2DATA_PORT, cmd);

  irq_install_handler(1, on_irq1);
}
