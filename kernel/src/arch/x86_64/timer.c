#include <stdint.h>
#include <utils.h>
#include <tty.h>
#include <idt.h>

volatile uint64_t ticks;

const uint32_t freq = 100;

void onIrq0(struct interrupt_frame *frame)
{
    (void)frame;
    ticks += 1;
    print(global_renderer, "M");
}

void init_timer()
{
    ticks = 0;

    // https://wiki.osdev.org/Programmable_Interval_Timer#Channel_0
    // 1193182 Hz base frequency divided by desired frequency
    uint32_t divisor = 1193182 / freq;

    // Set mode register to square wave generator
    out_portb(0x43, 0x36);

    out_portb(0x40, (uint8_t)(divisor & 0xFF));
    out_portb(0x40, (uint8_t)((divisor >> 8) & 0xFF));

    irq_install_handler(0, onIrq0);
}
