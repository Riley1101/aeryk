#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

extern volatile uint64_t ticks;

void initTimer(void);

#endif // TIMER_H
