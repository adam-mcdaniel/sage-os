/*
Scheduler - uses completely fair scheduler approach
*/

#include <process.h>
#include <rbtree.h>
#include <sched.h>


static RBTree *sched_tree;

//initialize scheduler tree
void sched_init(){
    sched_tree = rb_new();
}

//adds node to scheduler tree
void sched_add(struct process *p){
    //ensure runtime is a unique key
    struct process *curr_process;
    while(rb_find(sched_tree, p->runtime, curr_process)){
        p->runtime++;
    }
    rb_insert(sched_tree, p->runtime, p);
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
