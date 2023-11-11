/*
Scheduler header
*/

#ifndef SCHED_H
#define SCHED_H

#include <process.h>

//initialize scheduler tree
void sched_init();

//adds node to scheduler tree
void sched_add(Process *p);

//removes node from scheduler tree - used if process gets manually killed
void sched_remove_process(Process *p);

//get process with the lowest vruntime
Process *sched_get_next();

// Get the currently running process
Process *sched_get_current(void);

// Get the time slice for the current process
unsigned long get_time_slice(void);

// Perform a context switch from one process to another
void context_switch(Process *from, Process *to);

// Save the state of the current process
void save_state(TrapFrame *state);

// Load the state of the next process
void load_state(TrapFrame *state);

// Set the current process in the scheduler
void set_current_process(Process *proc);

// Perform the actual switch to the new process state
void switch_to(TrapFrame *state);

#endif // SCHED_H