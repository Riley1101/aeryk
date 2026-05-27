#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <process.h>

void mlfqInit();
void mlfqEnqueue(process_t * proc);
process_t* mlfqPickNext(void);
void mlfqOnTick(void);

#endif // !SCHEDULER_H
