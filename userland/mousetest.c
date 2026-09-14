#include <stdio.h>
#include <stdlib.h>
#include <sys/mouse.h>

// Manual/interactive test: run under a QEMU window (not the headless boot
// smoke test) and move the mouse to see decoded packets stream in.
void main(void) {
  printf("mousetest: move the mouse to generate packets (reading 10)...\n");

  for (int i = 0; i < 10; i++) {
    mouse_packet_t pkt;
    int n = mouse_read(&pkt, 1, 0);
    if (n != 1) {
      printf("mousetest: mouse_read failed\n");
      exit(1);
    }
    printf("mousetest: dx=%d dy=%d buttons=%d\n", pkt.dx, pkt.dy, pkt.buttons);
  }

  printf("mousetest: done\n");
  exit(0);
}
