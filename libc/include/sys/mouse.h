#ifndef _SYS_MOUSE_H
#define _SYS_MOUSE_H 1

#include <stddef.h>

#include <abi/mouse.h>

/**
 * @brief Reads up to `max_packets` decoded PS/2 mouse packets into `buf`.
 * Returns fewer than `max_packets` if that's all that's currently
 * buffered, rather than blocking further once at least one packet has
 * been read.
 * @param buf Destination array of at least `max_packets` entries.
 * @param max_packets Maximum number of packets to read.
 * @param nonblock If 0, blocks until at least one packet is available
 * (like the classic behavior); if nonzero, returns immediately with
 * whatever's buffered, possibly 0.
 * @return The number of packets read, or -1 on error (errno set).
 */
int mouse_read(mouse_packet_t *buf, int max_packets, int nonblock);

#endif // !_SYS_MOUSE_H
