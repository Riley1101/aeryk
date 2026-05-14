#include "timer.h"
#include <arch/x86_64/apic/apic.h>
#include <arch/x86_64/cpu/idt.h>
#include <drivers/display/tty.h>
#include <lib/utils.h>
#include <stdint.h>

volatile uint64_t ticks;
const uint32_t freq = 100;

#define PIT_CHANNEL0 0x40
#define PIT_CMD 0x43
#define PIT_10MS 11931 // 1193182 Hz * 10ms / 1000

void onIrq0(struct interrupt_frame *frame)
{
    (void)frame;
    ticks += 1;
}

static uint32_t lapic_calibrate(void)
{
    // Use LAPIC timer as a stopwatch: one-shot, masked (no interrupt)
    lapic_write(LAPIC_TIMER_LVT, 0x10000);
    lapic_write(LAPIC_TIMER_INITCNT, 0xFFFFFFFF);

    // Program PIT channel 0: mode 0 (one-shot), lo/hi byte, binary
    out_portb(PIT_CMD, 0x30);
    out_portb(PIT_CHANNEL0, PIT_10MS & 0xFF);
    out_portb(PIT_CHANNEL0, PIT_10MS >> 8);

    // Poll until PIT output pin goes high (countdown complete)
    uint8_t status;
    do
    {
        out_portb(PIT_CMD, 0xE2); // read-back: latch status for channel 0
        status = in_portb(PIT_CHANNEL0);
    } while (!(status & 0x80));

    // Return LAPIC ticks elapsed during ~10ms
    return 0xFFFFFFFF - lapic_read(LAPIC_TIMER_CURCNT);
}

void initTimer()
{
    ticks = 0;
    lapic_write(LAPIC_TIMER_DIV, 0x03); // divide by 16

    uint32_t ticks_per_10ms = lapic_calibrate();

    // Program periodic timer at freq Hz:
    // initial_count = ticks_per_10ms * (1000ms / 10ms) / freq
    lapic_write(LAPIC_TIMER_LVT, 32 | 0x20000); // vector 32, periodic
    lapic_write(LAPIC_TIMER_INITCNT,
                (uint32_t)((uint64_t)ticks_per_10ms * 100 / freq));

    irq_install_handler(0, onIrq0);
}
