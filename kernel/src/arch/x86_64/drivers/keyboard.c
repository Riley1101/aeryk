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

#define SC_LSHIFT 0x2A
#define SC_RSHIFT 0x36

#define KBD_BUF_SIZE 256
#define LINE_BUF_SIZE KBD_BUF_SIZE

// Circular buffer of decoded chars, filled by the IRQ1 handler and drained
// by keyboard_read(). Single-producer (IRQ), single-consumer (sys_read).
static char kbd_buf[KBD_BUF_SIZE];
static uint32_t kbd_head = 0; // next slot to write
static uint32_t kbd_tail = 0; // next slot to read

// FIFO of processes blocked in keyboard_read() waiting for input, linked
// via each process's dedicated wait_next/wait_prev fields (kept separate
// from the MLFQ's queue_next/queue_prev -- see process.h).
static wait_queue_t kbd_waitq = {0};

// Canonical-mode line editing: keystrokes accumulate here (with backspace
// removing the last one) and are only handed to kbd_buf -- and thus to a
// blocked keyboard_read() -- as a whole line, once '\n' arrives or the
// line fills up. Without this, backspace would need to reach back into
// kbd_buf to erase a byte, but keyboard_read() typically drains each byte
// into the caller before the next key is even pressed, so there'd usually
// be nothing left in kbd_buf to erase.
static char line_buf[LINE_BUF_SIZE];
static uint32_t line_len = 0;

static int shift_down = 0;

static void ps2_wait_write(void) {
  while (in_portb(PS2STATUS_PORT) & 0x02)
    ;
}

static void ps2_wait_read(void) {
  while (!(in_portb(PS2STATUS_PORT) & 0x01))
    ;
}

static const char kbd_us[128] = {
    0,   27,   '1',  '2', '3',  '4', '5', '6', '7', '8', '9', '0', '-',
    '=', '\b', '\t', 'q', 'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']',  '\n', 0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    ';', '\'', '`',  0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
    '.', '/',  0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   '-', 0,   0,   0,
    '+', 0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0};

// Maps an unshifted char to what it becomes with Shift held. Derived from
// kbd_us rather than kept as a parallel 128-entry table so the two can't
// drift out of sync.
static char shift_char(char c) {
  if (c >= 'a' && c <= 'z') {
    return (char)(c - ('a' - 'A'));
  }
  switch (c) {
  case '1': return '!';
  case '2': return '@';
  case '3': return '#';
  case '4': return '$';
  case '5': return '%';
  case '6': return '^';
  case '7': return '&';
  case '8': return '*';
  case '9': return '(';
  case '0': return ')';
  case '-': return '_';
  case '=': return '+';
  case '[': return '{';
  case ']': return '}';
  case ';': return ':';
  case '\'': return '"';
  case '`': return '~';
  case '\\': return '|';
  case ',': return '<';
  case '.': return '>';
  case '/': return '?';
  default: return c;
  }
}

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
  uint8_t code = scancode & 0x7F;
  int is_break = scancode & 0x80;

  // Shift only updates modifier state -- it never produces a char, and we
  // need its release (break code) too, unlike every other key below.
  if (code == SC_LSHIFT || code == SC_RSHIFT) {
    shift_down = !is_break;
    return;
  }

  if (is_break) {
    return;
  }

  char c = kbd_us[scancode];
  if (c == 0) {
    return;
  }
  if (shift_down) {
    c = shift_char(c);
  }

  if (c == '\b') {
    if (line_len > 0) {
      line_len--;
      print("\b \b"); // move back, blank the char, move back again
    }
    return;
  }

  char str[2] = {c, '\0'};
  print(str);

  if (line_len < LINE_BUF_SIZE) {
    line_buf[line_len++] = c;
  }

  // Hand the line to kbd_buf (and wake a blocked keyboard_read()) only once
  // it's complete, so a waiting reader gets the whole edited line rather
  // than every keystroke as it happens.
  if (c == '\n' || line_len == LINE_BUF_SIZE) {
    for (uint32_t i = 0; i < line_len; i++) {
      kbd_buf_push(line_buf[i]);
    }
    line_len = 0;

    process_t *waiter = wait_queue_pop(&kbd_waitq);
    if (waiter) {
      mlfq_enqueue(waiter); // sets state back to PROCESS_READY
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
