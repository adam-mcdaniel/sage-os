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
#include <csr.h>
#include <debug.h>

static RBTree *sched_tree;
Mutex sched_lock;


//initialize scheduler tree
void sched_init() {
    sched_tree = rb_new();
}

//adds node to scheduler tree
void sched_add(Process *p) {  
    mutex_spinlock(&sched_lock);  
    //NOTE: Process key is runtime * priority
    rb_insert_ptr(sched_tree, p->runtime * p->priority, p);
    mutex_unlock(&sched_lock);
}

//get (pop) Process with the lowest vruntime
Process *sched_get_next() {
    mutex_spinlock(&sched_lock);
    Process *min_process;
    //implementation of async Process freeing
    while (min_process->state != PS_DEAD) {
        bool search_success = rb_min_val_ptr(sched_tree, min_process);
        rb_delete(sched_tree, min_process->runtime * min_process->priority);
        if (!search_success) {
            min_process = NULL;
            break;
        }
    }
    mutex_unlock(&sched_lock);
    return min_process;
}
/*
 NOTE: async Process freeing is where if the Process is killed while it is idle 
 (its still in the CFS tree) the system just sets its status to dead. The scheduler
 encounters it while getting the next Process to schedule, sees the status, then removes it 
 and finds another Process.
*/

/* Duplicate purpose of above method*/
// Function to choose the next Process to run based on CFS
// Process *sched_choose_next() {
//     mutex_spinlock(&sched_lock);
//     Process *next_process;
//     if (rb_min_val_ptr(sched_tree, &next_process)) {
//         rb_delete(sched_tree, next_process->runtime);
//         mutex_unlock(&sched_lock);
//         return next_process;
//     }
    
//     mutex_unlock(&sched_lock);
//     return NULL; // No Process is ready to run
// }


/* for scheduling, we pop a Process from the CFS tree when we want to run.
 when the hart get interrupted, we should put the Process back into the tree with 
 its new run time. That way, we don't need to update vruntime while it's in the tree.
 
 Also, this method changes the runtime before removing from the tree, 
 which messes up the search
  */
// Function to update the vruntime of a Process
// void sched_update_vruntime(Process *p, unsigned long delta_time) {
//     p->runtime += delta_time;
//     // Rebalance the tree if necessary
//     rb_delete(sched_tree, p->runtime);
//     rb_insert(sched_tree, p->runtime, (uint64_t)p);
// }

// Function to handle the timer interrupt for context switching
void sched_handle_timer_interrupt(int hart) {

    //put Process currently on the hart back in the scheduler to recalc priority
    uint16_t pid = pid_harts_map_get(hart);
    Process *current_proc = process_map_get(pid);
    sched_add(current_proc);

    //get an idle Process
    Process *next_process = sched_get_next(); // Implement this function to get the currently running Process

    //execute Process until next interrupt
    if (next_process != NULL) {
        //set timer
        sbi_add_timer(hart, CONTEXT_SWITCH_TIMER * next_process->quantum);
        process_run(next_process, hart);
    } else {
        if (hart == 0) {
            //run idle Process (WFI loop)
            sbi_add_timer(hart, CONTEXT_SWITCH_TIMER);
            debugf("sched_handle_timer_interrupt: Running idle process\n");
            WFI_LOOP();
        } else {
            sbi_hart_stop();
        }
    }

    // unsigned long time_slice = get_time_slice(); // Implement this function based on the timer configuration
    // sched_update_vruntime(current_process, time_slice);
    // Process *next_process = sched_choose_next();
    // if (next_process != current_process) {
    //     context_switch(current_process, next_process); // Implement context_switch function
    // }
}

void context_switch(Process *from, Process *to) {
    // Save the state of the current Process
    save_state(&from->frame);

    // Load the state of the next Process
    load_state(&to->frame);

    // Update the current Process pointer
    set_current_process(to);

    // Perform the actual switch
    switch_to(&to->frame);
}


void save_state(TrapFrame *state) {
}

void load_state(TrapFrame *state) {
}

void switch_to(TrapFrame *state) {
}

Process *sched_get_current(void) {
}

//amount of time before hart is interrupted
unsigned long get_time_slice(void) {
}

void set_current_process(Process *proc) {
}
