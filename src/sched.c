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
#include <mmu.h>
#include <trap.h>
#include <debug.h>
#include <kmalloc.h>

static RBTree *sched_tree;
static Mutex sched_lock;

static Process *idle_process, *current_process;

void idle_process_main() {
    while (1) {
        debugf("idle_process_main: Idle Process started\n");
        WFI_LOOP();
    }
}


//initialize scheduler tree
void sched_init() {
    //create idle Process
    Process *p = process_new(PM_SUPERVISOR);
    process_map_set(p);
    p->state = PS_RUNNING;
    debugf("sched_init: Idle Process created with pid %d\n", p->pid);
    p->runtime = 0;
    p->priority = 0;
    kfree(p->rcb.ptable);
    p->rcb.ptable = kernel_mmu_table;
    p->frame = *kernel_trap_frame;
    p->frame.sscratch = (uint64_t) p->rcb.ptable;
    p->frame.sepc = (uint64_t) idle_process_main;
    CSR_READ(p->frame.sstatus, "sstatus");
    

    sched_tree = rb_new();
    set_current_process(p);
    //add idle Process to scheduler tree
    sched_add(p);

    // Print the next Process to run
    Process *next_process = sched_get_next();
    if (next_process != NULL) {
        debugf("sched_init: Next Process to run is %d\n", next_process->pid);
    } else {
        debugf("sched_init: No Process to run\n");
    }
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
    Process *min_process = NULL;
    if (!rb_min_val_ptr(sched_tree, &min_process)) {
        debugf("sched_get_next: No Process to run\n");
        mutex_unlock(&sched_lock);
        return NULL;
    }
    
    //implementation of async Process freeing
    while (min_process != NULL && min_process->state != PS_RUNNING) {
        bool search_success = rb_min_val_ptr(sched_tree, min_process);
        if (!search_success) {
            min_process = NULL;
            break;
        }
        // If the process is dead, remove it from the tree
        // if (min_process->state == PS_DEAD) {
        //     rb_delete(sched_tree, min_process->runtime * min_process->priority);
        // }
        rb_delete(sched_tree, min_process->runtime * min_process->priority);
    }
    debugf("sched_get_next: Next Process to run is %d\n", min_process->pid);
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
    infof("sched_handle_timer_interrupt: hart %d\n", hart);
    //put Process currently on the hart back in the scheduler to recalc priority
    uint16_t pid = pid_harts_map_get(hart);
    Process *current_proc = process_map_get(pid);
    // remove_process(current_proc);
    debugf("sched_handle_timer_interrupt: Putting Process %d back in scheduler\n", current_proc->pid);
    sched_add(current_proc);
    // Print the program counter of the Process that was interrupted
    debugf("pc: %lx\n", current_proc->frame.sepc);

    //get an idle Process
    Process *next_process = sched_get_next(); // Implement this function to get the currently running Process
    debugf("sched_handle_timer_interrupt: Next Process to run is %d\n", next_process->pid);
    //execute Process until next interrupt
    if (next_process != NULL) {
        //set timer
        sbi_add_timer(hart, CONTEXT_SWITCH_TIMER * next_process->quantum);
        debugf("sched_handle_timer_interrupt: Running Process %d\n", next_process->pid);
        // load_state(&next_process->frame);
        process_run(next_process, hart);
    } else {
        debugf("sched_handle_timer_interrupt: No Process to run\n");
    }

    // unsigned long time_slice = get_time_slice(); // Implement this function based on the timer configuration
    // sched_update_vruntime(current_process, time_slice);
    // Process *next_process = sched_choose_next();
    // if (next_process != current_process) {
    //     context_switch(current_process, next_process); // Implement context_switch function
    // }
    debugf("sched_handle_timer_interrupt: hart %d done\n", hart);
}

// void context_switch(Process *from, Process *to) {
//     // Save the state of the current Process
//     save_state(&from->frame);

//     // Load the state of the next Process
//     load_state(&to->frame);

//     // Update the current Process pointer
//     set_current_process(to);

//     // Perform the actual switch
//     switch_to(&to->frame);
// }


// void save_state(TrapFrame *state) {
//     // Save the current Process's page table
//     // TODO: turn on the MMU when you've written the src/mmu.c functions
//     // CSR_WRITE("satp", state->satp); 
//     // SFENCE_ALL();
// }

// void load_state(TrapFrame *state) {
//     // debugf("load_state: Loading page table %d\n", state->satp);
//     // // Load the next Process's page table
//     // CSR_WRITE("satp", state->satp); 
//     // SFENCE_ALL();
// }

// void switch_to(TrapFrame *state) {
// }

// Process *sched_get_current(void) {
//     return current_process;
// }

// //amount of time before hart is interrupted
// unsigned long get_time_slice(void) {
// }

void set_current_process(Process *proc) {
    pid_harts_map_set(sbi_whoami(), proc->pid);
    current_process = proc;
}
