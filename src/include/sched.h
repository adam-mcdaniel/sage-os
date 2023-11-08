/*
Scheduler header
*/

#ifndef SCHED_H
#define SCHED_H

#include <process.h>

//initialize scheduler tree
void sched_init();

//adds node to scheduler tree
void sched_add(struct process *p);

//removes node from scheduler tree - used if process gets manually killed
void sched_remove_process(struct process *p);

//get process with the lowest vruntime
struct process *sched_get_next();

// Get the currently running process
struct process *sched_get_current(void);

// Get the time slice for the current process
unsigned long get_time_slice(void);

// Perform a context switch from one process to another
void context_switch(struct process *from, struct process *to);

// Save the state of the current process
void save_state(struct trap_frame *state);

// Load the state of the next process
void load_state(struct trap_frame *state);

// Set the current process in the scheduler
void set_current_process(struct process *proc);

// Perform the actual switch to the new process state
void switch_to(struct trap_frame *state);

#endif // SCHED_H