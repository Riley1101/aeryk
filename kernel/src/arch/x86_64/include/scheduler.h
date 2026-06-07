#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <process.h>

void mlfq_init();
void mlfq_enqueue(process_t * proc);
process_t* mlfq_pick_next(void);
void mlfq_on_tick(void);

#endif // !SCHEDULER_H
