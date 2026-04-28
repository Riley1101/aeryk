#include <stdint.h>
#include <tty.h>
#include <utils.h>

Renderer *global_renderer = NULL;
void print(Renderer *r, const char *str)
{
    (void)r;
    (void)str;
}

typedef struct {
    uint16_t port;
    uint8_t value;
} PortWrite;

#define MAX_PORT_LOG 16
PortWrite portb_log[MAX_PORT_LOG];
int portb_log_count = 0;

void reset_portb_log(void) { portb_log_count = 0; }

void out_portb(uint16_t port, uint8_t value)
{
    if (portb_log_count < MAX_PORT_LOG) {
        portb_log[portb_log_count].port = port;
        portb_log[portb_log_count].value = value;
        portb_log_count++;
    }
}

/* Stub ISR handlers — referenced by initIdt, needed to satisfy the linker */
#define DEF_ISR(n) \
    void isr##n(void) {}
DEF_ISR(0)
DEF_ISR(1)
DEF_ISR(2)
DEF_ISR(3)
DEF_ISR(4)
DEF_ISR(5)
DEF_ISR(6)
DEF_ISR(7)
DEF_ISR(8)
DEF_ISR(9)
DEF_ISR(10)
DEF_ISR(11)
DEF_ISR(12)
DEF_ISR(13)
DEF_ISR(14)
DEF_ISR(15)
DEF_ISR(16)
DEF_ISR(17)
DEF_ISR(18)
DEF_ISR(19)
DEF_ISR(20)
DEF_ISR(21)
DEF_ISR(22)
DEF_ISR(23)
DEF_ISR(24)
DEF_ISR(25)
DEF_ISR(26)
DEF_ISR(27)
DEF_ISR(28)
DEF_ISR(29)
DEF_ISR(30)
DEF_ISR(31)
DEF_ISR(32)
DEF_ISR(33)
DEF_ISR(34)
DEF_ISR(35)
DEF_ISR(36)
DEF_ISR(37)
DEF_ISR(38)
DEF_ISR(39)
DEF_ISR(40)
DEF_ISR(41)
DEF_ISR(42)
DEF_ISR(43)
DEF_ISR(44)
DEF_ISR(45)
DEF_ISR(46)
DEF_ISR(47)
