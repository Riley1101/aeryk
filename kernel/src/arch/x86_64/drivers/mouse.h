#ifndef MOUSE_H
#define MOUSE_H

#include <abi/mouse.h>

void init_mouse(void);

// Reads up to `max_packets` decoded mouse packets into `buf`, blocking the
// calling process until at least one is available. Returns the number of
// packets read.
int mouse_read(mouse_packet_t *buf, int max_packets);

#endif // !MOUSE_H
