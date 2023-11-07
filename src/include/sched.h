/*
Scheduler header
*/

#include <process.h>

//initialize scheduler tree
void sched_init();

//adds node to scheduler tree
void sched_add(struct process *p);

//removes node from scheduler tree - used if process gets manually killed
void sched_remove_process(struct process *p);

//get process with the lowest vruntime
struct process *sched_get_next();