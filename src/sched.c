/*
Scheduler - uses completely fair scheduler approach
*/

#include <process.h>
#include <rbtree.h>
#include <sched.h>
#include <stddef.h>
#include <stdint.h>


static RBTree *sched_tree;


//initialize scheduler tree
void sched_init(){
    sched_tree = rb_new();
}

//adds node to scheduler tree
void sched_add(struct process *p) {
    struct process *curr_process = NULL;
    uint64_t curr_process_key;

    while(rb_find(sched_tree, p->runtime, &curr_process_key)) {
        curr_process = (struct process *)curr_process_key;
        p->runtime++;
    }
    uint64_t process_key = (uint64_t)p;
    rb_insert(sched_tree, p->runtime, process_key);
}

//removes node from scheduler tree - used if process gets manually killed
void sched_remove_process(struct process *p){
    rb_delete(sched_tree, p->runtime);
}

//get process with the lowest vruntime
struct process *sched_get_next(){
    struct process *min_process;
    rb_min_val_ptr(sched_tree, min_process);
    return min_process;
}

// Function to choose the next process to run based on CFS
struct process *sched_choose_next() {
    struct process *next_process;
    if (rb_min_val_ptr(sched_tree, &next_process)) {
        return next_process;
    }
    return NULL; // No process is ready to run
}

// Function to update the vruntime of a process
void sched_update_vruntime(struct process *p, unsigned long delta_time) {
    p->runtime += delta_time;
    // Rebalance the tree if necessary
    rb_delete(sched_tree, p->runtime);
    rb_insert(sched_tree, p->runtime, (uint64_t)p);
}

// Function to handle the timer interrupt for context switching
void sched_handle_timer_interrupt() {
    struct process *current_process = sched_get_current(); // Implement this function to get the currently running process
    unsigned long time_slice = get_time_slice(); // Implement this function based on the timer configuration
    sched_update_vruntime(current_process, time_slice);
    struct process *next_process = sched_choose_next();
    if (next_process != current_process) {
        context_switch(current_process, next_process); // Implement context_switch function
    }
}

void context_switch(struct process *from, struct process *to) {
    // Save the state of the current process
    save_state(&from->frame);

    // Load the state of the next process
    load_state(&to->frame);

    // Update the current process pointer
    set_current_process(to);

    // Perform the actual switch
    switch_to(&to->frame);
}


void save_state(struct trap_frame *state) {
}

void load_state(struct trap_frame *state) {
}

void switch_to(struct trap_frame *state) {
}

struct process *sched_get_current(void) {
}

unsigned long get_time_slice(void) {
}

void set_current_process(struct process *proc) {
}


