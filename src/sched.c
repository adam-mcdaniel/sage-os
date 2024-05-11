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
#include <lock.h>
#include <compiler.h>
#include <config.h>

// #define DEBUG_SCHED
#ifdef DEBUG_SCHED
#define debugf(...) debugf(__VA_ARGS__)
#else
#define debugf(...)
#endif

static RBTree *sched_tree;
static Mutex sched_lock = MUTEX_UNLOCKED;

static Process *idle_process = NULL, *current_process = NULL;

void sbi_print(char *c) {
    while (*c != '\0') {
        sbi_putchar(*c);
        c++;
    }
}

static uint64_t start_time = -1ULL;
void idle_process_main() {
    // if (start_time == -1ULL) {
    //     start_time = sbi_get_time();
    // }
    // uint64_t time = sbi_get_time();
    // if (time - start_time > 10000000) {
    //     sbi_print("Idle woke up!\n");
    //     start_time = time;
    // }

    #ifdef DEBUG_SCHED
    // sbi_print("Idle woke up!\n");
    #endif
    while (1) {
        #ifdef DEBUG_SCHED
        sbi_print("Idle woke up!\n");
        #endif
        // infof("Idle woke up!\n");
        WFI();
    }
}

Process *sched_get_idle_process() {
    return idle_process;
}


static bool is_init = false;
//initialize scheduler tree
void sched_init() {
    mutex_spinlock(&sched_lock);
    process_map_init();
    pid_harts_map_init();
    sched_tree = rb_new();
    //create idle Process
    idle_process = process_new(PM_SUPERVISOR);
    
    idle_process->state = PS_RUNNING;
    idle_process->hart = sbi_whoami();
    debugf("sched_init: Idle Process created with pid %d\n", idle_process->pid);
    idle_process->runtime = 1;
    idle_process->quantum = 4;
    idle_process->priority = 20;
    // idle_process->frame->xregs
    // kfree(p->rcb.ptable);
    // p->rcb.ptable = kernel_mmu_table;
    // p->frame = *kernel_trap_frame;
    // memset(&p->frame, 0, sizeof(TrapFrame));
    // uint64_t permission_bits = PB_READ | PB_EXECUTE | PB_WRITE | PB_USER;


    // p->frame.sepc = kernel_mmu_translate(idle_process_main);
    // p->frame.sstatus = 0x0000000000000000;
    // p->frame.stvec = trampoline_trap_start;
    // p->frame.sscratch = kernel_mmu_translate(&p->frame);
    // p->frame.satp = SATP(kernel_mmu_translate(p->rcb.ptable), p->pid % 4094 + 2);
    // p->frame.trap_satp = SATP_KERNEL;
    // p->frametrap_stack = kzalloc(0x1000);
    // mmu_map_range(p->rcb.ptable, 
    //             p->frame.trap_stack, 
    //             p->frame.trap_stack + 0x10000, 
    //             kernel_mmu_translate(p->frame.trap_stack), 
    //             MMU_LEVEL_4K,
    //             permission_bits);
    // mmu_map_range(p->rcb.ptable, 
    //             p->frame.sscratch, 
    //             p->frame.sscratch + 0x10000, 
    //             kernel_mmu_translate(p->frame.sscratch),
    //             MMU_LEVEL_4K,
    //             permission_bits);
    // mmu_map_range(p->rcb.ptable,
    //             p->rcb.ptable,
    //             p->rcb.ptable + 0x10000,
    //             kernel_mmu_translate(p->rcb.ptable),
    //             MMU_LEVEL_4K,
    //             permission_bits);
    // mmu_map_range(p->rcb.ptable,
    //             &p->frame,
    //             &p->frame + 0x10000,
    //             kernel_mmu_translate(&p->frame),
    //             MMU_LEVEL_4K,
    //             permission_bits);

    // // Add the trap handler to the process's page table
    // mmu_map_range(p->rcb.ptable,
    //             p->frame.stvec,
    //             p->frame.stvec + 0x10000,
    //             kernel_mmu_translate(p->frame.stvec),
    //             MMU_LEVEL_4K,
    //             permission_bits);

    // void process_asm_run(void);
    // unsigned long trans_trampoline_start = mmu_translate(kernel_mmu_table, trampoline_thread_start);
    // unsigned long trans_trampoline_trap  = mmu_translate(kernel_mmu_table, trampoline_trap_start);
    // unsigned long trans_process_asm_run  = mmu_translate(kernel_mmu_table, process_asm_run);
    // unsigned long trans_os_trap_handler  = mmu_translate(kernel_mmu_table, os_trap_handler);
    // mmu_map(p->rcb.ptable, trampoline_thread_start, trans_trampoline_start, MMU_LEVEL_4K,
    //         PB_READ | PB_EXECUTE | PB_USER);
    // mmu_map(p->rcb.ptable, trampoline_trap_start, trans_trampoline_trap, MMU_LEVEL_4K,
    //         PB_READ | PB_EXECUTE | PB_USER);
    // // Map the trap stack
    // mmu_map(p->rcb.ptable, process_asm_run, trans_process_asm_run, MMU_LEVEL_4K,
    //         PB_READ | PB_EXECUTE | PB_USER);
    // mmu_map(p->rcb.ptable, os_trap_handler, trans_os_trap_handler, MMU_LEVEL_4K,
    //         PB_READ | PB_EXECUTE | PB_USER);
    // // Map trap frame to user's page table
    // uintptr_t trans_frame = kernel_mmu_translate((uintptr_t)&p->frame);
    // mmu_map(p->rcb.ptable, (uintptr_t)&p->frame, trans_frame, MMU_LEVEL_4K, PB_READ | PB_WRITE | PB_EXECUTE | PB_USER);
    rcb_map(&idle_process->rcb, idle_process_main, idle_process_main, 0x1000, PB_READ | PB_EXECUTE);
    // mmu_translate(p->rcb.ptable, p->frame.stvec);
    // CSR_READ(p->frame->sie, "sie");
    
    // uint64_t  gpregs[32]; 
    // double    fpregs[32]; 
    // uint64_t  sepc;     
    // uint64_t  sstatus;  
    // uint64_t  sie;      
    // uint64_t  satp;     
    // uint64_t  sscratch; 
    // uint64_t  stvec;    
    // uint64_t  trap_satp;
    // uint64_t  trap_stack;

    // memcpy(idle_process->frame->xregs, kernel_trap_frame->xregs, sizeof(kernel_trap_frame->xregs));
    // rcb_map(&idle_process->rcb, idle_process_main, idle_process_main, 0x1000, PB_READ | PB_EXECUTE);
    // rcb_map(&idle_process->rcb, sbi_putchar, sbi_putchar, 0x1000, PB_READ | PB_EXECUTE);
    // rcb_map(&idle_process->rcb, sbi_get_time, sbi_get_time, 0x1000, PB_READ | PB_EXECUTE);
    // rcb_map(&idle_process->rcb, sbi_print, sbi_print, 0x1000, PB_READ | PB_EXECUTE);
    // rcb_map(&idle_process->rcb, &start_time, &start_time, 0x1000, PB_READ | PB_WRITE | PB_EXECUTE);
    debugf("sched_init: Idle Process RCB initialized\n");
    // rcb_map(&idle_process->rcb, TE)

    // rcb_map(&idle_process->rcb, sym_start(text), , sym_start(text), MMU_LEVEL_1G,
    //               PB_READ | PB_WRITE | PB_EXECUTE);
    // mmu_map(p->rcb.ptable, idle_process_main, idle_process_main, MMU_LEVEL_4K, PB_READ | PB_WRITE | PB_EXECUTE | PB_USER);

    // p->frame.sepc = (uint64_t) idle_process_main;
    // CSR_READ(p->frame.sstatus, "sstatus");
    
    idle_process->runtime += CONTEXT_SWITCH_TIMER;
    // process_map_set(p);
    set_current_process(idle_process);
    //add idle Process to scheduler tree
    mutex_unlock(&sched_lock);
    sched_add(idle_process);
    mutex_spinlock(&sched_lock);

    // process_debug(p);
    // Print the next Process to run
    mutex_unlock(&sched_lock);
    Process *next_process = sched_get_next();
    mutex_spinlock(&sched_lock);

    if (next_process == NULL) {
        debugf("sched_init: No Process to run\n");
    } else if (next_process == sched_get_idle_process()) {
        debugf("sched_init: First Process to run is idle\n");
    } else {
        debugf("sched_init: First Process to run is %d\n", next_process->pid);
    }

    debugf("sched_init: Scheduler initialized\n");
    mutex_unlock(&sched_lock);
}

void sched_print_processes() {
    mutex_spinlock(&sched_lock);
    debugf("sched_print_processes: Printing scheduler tree\n");
    
    mutex_unlock(&sched_lock);
}

static int total_processes = 0;

//adds node to scheduler tree
void sched_add(Process *p) {  
    mutex_spinlock(&sched_lock);
    if (p->state == PS_DEAD) {
        warnf("sched_add: Process %d is dead\n", p->pid);
        mutex_unlock(&sched_lock);
        return;
    }

    mutex_spinlock(&p->lock);
    total_processes++;
    //NOTE: Process key is runtime * priority
    rb_insert_ptr(sched_tree, p->runtime * p->priority, p);
    // process_map_set(p);
    // pid_harts_map_set(p->hart, p->pid);
    debugf("Scheduled Process %d with runtime %d and priority %d\n", p->pid, p->runtime, p->priority);
    mutex_unlock(&p->lock);
    mutex_unlock(&sched_lock);
}

void sched_invoke(Process *p, int hart) {
    is_init = true;
    debugf("sched_invoke: Invoking scheduler on hart %d\n", hart);
    p->hart = hart;
    sched_add(p);
    process_run(p, hart);
}

//get (pop) Process with the lowest vruntime
Process *sched_get_next() {
    debugf("sched_get_next: Getting next Process to run\n");
    mutex_spinlock(&sched_lock);
    Process *min_process = NULL;
    bool search_success = rb_min_val_ptr(sched_tree, &min_process);
    
    //implementation of async Process freeing
    uint64_t i = 0;
    while (min_process == NULL || min_process->state != PS_RUNNING) {
        if (i++ > 20) {
            // Retuning idle Process
            // warnf("sched_get_next: No Process to run\n");
            mutex_unlock(&sched_lock);
            return sched_get_idle_process();
        }

        debugf("Min Process is %d\n", min_process->pid);
        if (!process_map_contains(min_process->pid)) {
            warnf("sched_get_next: Process %d not found in process map\n", min_process->pid);
            min_process = NULL;
            break;
        }

        if (!rb_contains(sched_tree, min_process->runtime * min_process->priority)) {
            warnf("sched_get_next: Process %d is not in the scheduler\n", min_process->pid);
            if (!search_success) {
                min_process = NULL;
                break;
            }
        }
        // If the process is dead, remove it from the tree
        if (min_process->state == PS_DEAD) {
            rb_delete(sched_tree, min_process->runtime * min_process->priority);
            total_processes-=1;
        }

        if (min_process->state == PS_SLEEPING) {
            // Check if the Process is ready to run
            debugf("sched_get_next: Process %d is sleeping until %d\n", min_process->pid, min_process->sleep_until);
            if (min_process->sleep_until < sbi_get_time()) {
                debugf("sched_get_next: Process %d is ready to run\n", min_process->pid);
                min_process->state = PS_RUNNING;
                break;
            } else {
                debugf("sched_get_next: Process %d is not ready to run\n", min_process->pid);
            }
        }
        search_success = rb_min_val_ptr(sched_tree, &min_process);
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
    if (!is_init) {
        warnf("sched_handle_timer_interrupt: Scheduler not initialized\n");
        return;
    }

    debugf("sched_handle_timer_interrupt: hart %d\n", hart);
    //put Process currently on the hart back in the scheduler to recalc priority
    // uint16_t pid = pid_harts_map_get(hart);
    // Process *current_proc = process_map_get(pid);
    Process *current_proc = sched_get_current();
    if (current_proc == NULL) {
        debugf("sched_handle_timer_interrupt: No Process to interrupt\n");
        return;
    }

    if (current_proc == sched_get_idle_process()) {
        debugf("sched_handle_timer_interrupt: Idle Process interrupted\n");
    } else {
        debugf("sched_handle_timer_interrupt: Process %d interrupted\n", current_proc->pid);
    }
    // Remove the Process from the tree
    rb_delete(sched_tree, current_proc->runtime * current_proc->priority);
    if (current_proc->state == PS_RUNNING) {
        current_proc->hart = HART_NONE;
        // Update the Process's runtime
        sbi_add_timer(sbi_whoami(), CONTEXT_SWITCH_TIMER);

        current_proc->runtime += CONTEXT_SWITCH_TIMER;
        current_proc->priority = 1;
        debugf("sched_handle_timer_interrupt: Process %d quantum is now %d\n", current_proc->pid, current_proc->quantum);
        debugf("sched_handle_timer_interrupt: Process %d runtime is now %d\n", current_proc->pid, current_proc->runtime);
        // Add the Process back to the tree
        debugf("sched_handle_timer_interrupt: Putting Process %d back in scheduler\n", current_proc->pid);
        
        // remove_process(current_proc);
        debugf("sched_handle_timer_interrupt: Putting Process %d back in scheduler\n", current_proc->pid);
        // sched_add(current_proc);
        // Print the program counter of the Process that was interrupted
        debugf("pc: %lx\n", current_proc->frame->sepc);
        // Print map size
        // debugf("sched_handle_timer_interrupt: Map size is %d\n", process_map_size());
    }

    if (current_proc->state != PS_DEAD) {
        debugf("sched_handle_timer_interrupt: Putting Process %d back in scheduler\n", current_proc->pid);
        rb_insert_ptr(sched_tree, current_proc->runtime * current_proc->priority, current_proc);
    } else {
        total_processes-=1;
        warnf("sched_handle_timer_interrupt: Process %d is dead\n", current_proc->pid);
    }

    //get an idle Process
    debugf("sched_handle_timer_interrupt: Getting next Process to run\n");
    Process *next_process = sched_get_next(); // Implement this function to get the currently running Process
    if (next_process == NULL) {
        debugf("sched_handle_timer_interrupt: No Process to run\n");
        next_process = sched_get_idle_process();
    }

    if (next_process == sched_get_idle_process() && total_processes > 1 && current_proc != sched_get_idle_process() && current_proc != NULL && current_proc->state != PS_DEAD) {
        debugf("Short circuiting idle process to execute Process %d\n", current_proc->pid);
        next_process = current_proc;
    }
    
    if (next_process == sched_get_idle_process()) {
        next_process->frame->sepc = (uint64_t) idle_process_main;
        debugf("sched_handle_timer_interrupt: Next Process to run is idle\n");
    } else {
        debugf("sched_handle_timer_interrupt: Next Process to run is %d\n", next_process->pid);
    }
    //execute Process until next interrupt
    if (next_process != NULL) {
        set_current_process(next_process);
        //set timer
        sbi_add_timer(hart, CONTEXT_SWITCH_TIMER);
        // debugf("sched_handle_timer_interrupt: Running Process %d\n", next_process->pid);
        // load_state(&next_process->frame);
        process_run(next_process, hart);
    } else {
        debugf("sched_handle_timer_interrupt: No Process to run\n");
    }
    // CSR_READ(current_proc->frame->sepc, "sepc");

    // unsigned long time_slice = get_time_slice(); // Implement this function based on the timer configuration
    // sched_update_vruntime(current_process, time_slice);
    // Process *next_process = sched_choose_next();
    // if (next_process != current_process) {
    //     context_switch(current_process, next_process); // Implement context_switch function
    // }
    debugf("sched_handle_timer_interrupt: hart %d done\n", hart);
}

void sched_remove(Process *p) {
    mutex_spinlock(&sched_lock);
    debugf("sched_remove: Removing Process %d from scheduler\n", p->pid);
    // rb_delete(sched_tree, p->runtime * p->priority);
    // Find the process in the tree
    Process *found_process;
    rb_find(sched_tree, p->runtime * p->priority, (uint64_t*)&found_process);
    if (found_process == NULL) {
        debugf("sched_remove: Process %d not found in scheduler\n", p->pid);
        mutex_unlock(&sched_lock);
        return;
    }
    if (found_process == p) {
        debugf("sched_remove: Removing Process %d from scheduler\n", p->pid);
        rb_delete(sched_tree, p->runtime * p->priority);
    } else {
        debugf("sched_remove: Process %d not found in scheduler\n", p->pid);
    }

    mutex_unlock(&sched_lock);
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

Process *sched_get_current(void) {
    mutex_spinlock(&sched_lock);
    if (!is_init) {
        debugf("sched_get_current: Scheduler not initialized\n");
        mutex_unlock(&sched_lock);
        return NULL;
    }
    // return current_process;
    int hart = sbi_whoami();
    uint16_t pid = pid_harts_map_get(hart);
    if (!process_map_contains(pid)) {
        fatalf("sched_get_current: Process %d not found on hart %d\n", pid, hart);
    }
    Process *p = process_map_get(pid);
    mutex_unlock(&sched_lock);
    return p;
}

// //amount of time before hart is interrupted
// unsigned long get_time_slice(void) {
// }

void set_current_process(Process *proc) {
    if (proc == NULL) {
        warnf("set_current_process: Process is NULL\n");
        return;
    }
    mutex_spinlock(&proc->lock);
    
    if (!process_map_contains(proc->pid)) {
        fatalf("set_current_process: Process %d not found\n", proc->pid);
    } else {
        // debugf("set_current_process: Process %d found\n", proc->pid);
    }
    pid_harts_map_set(sbi_whoami(), proc->pid);
    proc->hart = sbi_whoami();
    current_process = proc;

    mutex_unlock(&proc->lock);
}
