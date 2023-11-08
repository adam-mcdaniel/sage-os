/*
Scheduler - uses completely fair scheduler approach
*/

#include <process.h>
#include <rbtree.h>
#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <lock.h>
#include <sbi.h>
#include <hart.h>
#include <csr.h>


static RBTree *sched_tree;
uint64_t sched_mutex;


//initialize scheduler tree
void sched_init(){
    sched_tree = rb_new();
}

//adds node to scheduler tree
void sched_add(struct process *p) {  
    mutex_spinlock(sched_mutex);  
    //NOTE: process key is runtime * priority
    rb_insert(sched_tree, p->runtime * p->priority, p);
    mutex_unlock(sched_mutex);
}

//get (pop) process with the lowest vruntime
struct process *sched_get_next(){
    mutex_spinlock(sched_mutex);
    struct process *min_process;
    //implementation of async process freeing
    while(min_process->state != PS_DEAD){
        bool search_success = rb_min_val_ptr(sched_tree, min_process);
        rb_delete(sched_tree, min_process->runtime * min_process->priority);
        if (!search_success){
            min_process = NULL;
            break;
        }
    }
    mutex_unlock(sched_mutex);
    return min_process;
}
/*
 NOTE: async process freeing is where if the process is killed while it is idle 
 (its still in the CFS tree) the system just sets its status to dead. The scheduler
 encounters it while getting the next process to schedule, sees the status, then removes it 
 and finds another process.
*/

/* Duplicate purpose of above method*/
// Function to choose the next process to run based on CFS
// struct process *sched_choose_next() {
//     mutex_spinlock(sched_mutex);
//     struct process *next_process;
//     if (rb_min_val_ptr(sched_tree, &next_process)) {
//         rb_delete(sched_tree, next_process->runtime);
//         mutex_unlock(sched_mutex);
//         return next_process;
//     }
    
//     mutex_unlock(sched_mutex);
//     return NULL; // No process is ready to run
// }


/* for scheduling, we pop a process from the CFS tree when we want to run.
 when the hart get interrupted, we should put the process back into the tree with 
 its new run time. That way, we don't need to update vruntime while it's in the tree.
 
 Also, this method changes the runtime before removing from the tree, 
 which messes up the search
  */
// Function to update the vruntime of a process
// void sched_update_vruntime(struct process *p, unsigned long delta_time) {
//     p->runtime += delta_time;
//     // Rebalance the tree if necessary
//     rb_delete(sched_tree, p->runtime);
//     rb_insert(sched_tree, p->runtime, (uint64_t)p);
// }

// Function to handle the timer interrupt for context switching
void sched_handle_timer_interrupt(int hart) {

    //put process currently on the hart back in the scheduler to recalc priority
    sched_add(hart);

    //get an idle process
    struct process *next_process = sched_get_next(); // Implement this function to get the currently running process

    //execute process until next interrupt
    if(next_process != NULL){
        //set timer
        sbi_add_timer(hart, CONTEXT_SWITCH_TIMER * next_process->quantum);
        process_run(next_process, hart);
    }
    else{
        //run idle process (WFI loop)
        sbi_add_timer(hart, CONTEXT_SWITCH_TIMER);
        WFI();
    }

    // unsigned long time_slice = get_time_slice(); // Implement this function based on the timer configuration
    // sched_update_vruntime(current_process, time_slice);
    // struct process *next_process = sched_choose_next();
    // if (next_process != current_process) {
    //     context_switch(current_process, next_process); // Implement context_switch function
    // }
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

//amount of time before hart is interrupted
unsigned long get_time_slice(void) {
    return 
}

void set_current_process(struct process *proc) {
}


