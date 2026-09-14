#include "mouse.h"
#include <idt.h>
#include <process.h>
#include <scheduler.h>
#include <stdint.h>
#include <utils.h>

#define PS2DATA_PORT 0x60
#define PS2STATUS_PORT 0x64
#define PS2CMD_PORT 0x64

#define MOUSE_BUF_SIZE 64

// Circular buffer of decoded packets, filled by the IRQ12 handler and
// drained by mouse_read(). Single-producer (IRQ), single-consumer
// (sys_mouse_read) -- same shape as keyboard.c's kbd_buf.
static mouse_packet_t mouse_buf[MOUSE_BUF_SIZE];
static uint32_t mouse_head = 0; // next slot to write
static uint32_t mouse_tail = 0; // next slot to read

// FIFO of processes blocked in mouse_read() waiting for a packet. Kept
// separate from kbd_waitq/pipe wait queues (see PROCESS_BLOCKED_MOUSE in
// process.h for why a dedicated state+queue is required here).
static wait_queue_t mouse_waitq = {0};

// Byte-accumulation state for the raw 3-byte PS/2 packet currently being
// assembled by on_irq12().
static uint8_t packet_bytes[3];
static int packet_idx = 0;

static void ps2_wait_write(void) {
  while (in_portb(PS2STATUS_PORT) & 0x02)
    ;
}

static void ps2_wait_read(void) {
  while (!(in_portb(PS2STATUS_PORT) & 0x01))
    ;
}

// Sends `data` to the mouse (the second PS/2 port), prefixed with the
// controller's "next byte goes to port 2" command, and discards the
// device's ACK (0xFA) byte that follows.
static void mouse_write(uint8_t data) {
  ps2_wait_write();
  out_portb(PS2CMD_PORT, 0xD4);
  ps2_wait_write();
  out_portb(PS2DATA_PORT, data);
  ps2_wait_read();
  in_portb(PS2DATA_PORT);
}

static void mouse_buf_push(mouse_packet_t pkt) {
  uint32_t next = (mouse_head + 1) % MOUSE_BUF_SIZE;
  if (next == mouse_tail) {
    return; // buffer full, drop the packet
  }
  mouse_buf[mouse_head] = pkt;
  mouse_head = next;
}

static int mouse_buf_pop(mouse_packet_t *out) {
  if (mouse_tail == mouse_head) {
    return 0; // empty
  }
  *out = mouse_buf[mouse_tail];
  mouse_tail = (mouse_tail + 1) % MOUSE_BUF_SIZE;
  return 1;
}

void on_irq12(struct interrupt_frame *frame) {
  (void)frame;

  uint8_t data = in_portb(PS2DATA_PORT);

  // Byte 0 of a standard 3-byte packet always has bit 3 set. If we see a
  // byte without it while expecting a byte 0, the stream is desynced (e.g.
  // a byte got dropped) -- drop it and wait for a plausible start instead
  // of building a garbage packet out of it.
  if (packet_idx == 0 && !(data & 0x08)) {
    return;
  }

  packet_bytes[packet_idx++] = data;
  if (packet_idx < 3) {
    return;
  }
  packet_idx = 0;

  uint8_t flags = packet_bytes[0];
  if (flags & 0xC0) {
    return; // X or Y overflow -- the delta isn't trustworthy, drop it
  }

  mouse_packet_t pkt;
  // Sign-extend the 8-bit deltas using the sign bits carried in `flags`
  // (bit 4 for X, bit 5 for Y), per the standard PS/2 packet format.
  pkt.dx = (flags & 0x10) ? (int16_t)packet_bytes[1] - 256 : packet_bytes[1];
  pkt.dy = (flags & 0x20) ? (int16_t)packet_bytes[2] - 256 : packet_bytes[2];
  pkt.buttons = flags & 0x07;

  mouse_buf_push(pkt);

  process_t *waiter = wait_queue_pop(&mouse_waitq);
  if (waiter) {
    mlfq_enqueue(waiter); // sets state back to PROCESS_READY
  }
}

int mouse_read(mouse_packet_t *buf, int max_packets) {
  int n = 0;

  while (n < max_packets) {
    asm volatile("cli");

    mouse_packet_t pkt;
    if (mouse_buf_pop(&pkt)) {
      asm volatile("sti");
      buf[n++] = pkt;
      continue;
    }

    if (n > 0) {
      // Already have at least one packet buffered up for the caller --
      // hand those back now rather than blocking for more, same
      // "return what's available" convention as a typical read().
      asm volatile("sti");
      break;
    }

    // Nothing buffered: register as a waiter and yield. cli/sti around the
    // empty-check + enqueue close the race against on_irq12 firing in
    // between (a wakeup there just re-readies us before schedule()
    // switches away).
    current_process->state = PROCESS_BLOCKED_MOUSE;
    wait_queue_push(&mouse_waitq, current_process);
    asm volatile("sti");
    schedule();
  }

  return n;
}

void init_mouse(void) {
  // Enable the second PS/2 port (the mouse channel) at the controller.
  ps2_wait_write();
  out_portb(PS2CMD_PORT, 0xA8);

  // Read the controller command byte, set bit 1 (enable IRQ12 on port-2
  // output-buffer-full) and clear bit 5 (unmask the second port's clock --
  // mirrors how init_keyboard() clears bit 4 for the first port).
  ps2_wait_write();
  out_portb(PS2CMD_PORT, 0x20);
  ps2_wait_read();
  uint8_t cmd = in_portb(PS2DATA_PORT);

  cmd |= 0x02;
  cmd &= ~0x20;

  ps2_wait_write();
  out_portb(PS2CMD_PORT, 0x60);
  ps2_wait_write();
  out_portb(PS2DATA_PORT, cmd);

  mouse_write(0xF6); // set defaults
  mouse_write(0xF4); // enable data reporting

  irq_install_handler(12, on_irq12);
}
