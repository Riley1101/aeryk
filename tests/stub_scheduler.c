#include <process.h>
#include <stddef.h>

process_t *current_process = NULL;
process_t *idle_process = NULL;

int schedule_call_count = 0;
void reset_schedule_call_count(void) { schedule_call_count = 0; }
void schedule(void) { schedule_call_count++; }
