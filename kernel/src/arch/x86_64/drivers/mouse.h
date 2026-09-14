#ifndef MOUSE_H
#define MOUSE_H

#include <abi/mouse.h>

void init_mouse(void);

// Reads up to `max_packets` decoded mouse packets into `buf`. If `nonblock`
// is 0, blocks the calling process until at least one is available (same
// as before this param existed); if nonzero, returns immediately with
// whatever's buffered (possibly 0) instead of blocking -- used by a
// caller (e.g. the compositor's timer-driven redraw loop) that wants to
// poll the mouse without giving up its own scheduling turn. Returns the
// number of packets read.
int mouse_read(mouse_packet_t *buf, int max_packets, int nonblock);

#endif // !MOUSE_H
