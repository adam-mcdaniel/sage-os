/**
 * @file process.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Process structures and routines.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <list.h>
#include <vector.h>
#include <map.h>
#include <mmu.h>

#define HART_NONE        (-1U)
#define ON_HART_NONE(p)  (p->hart == HART_NONE)

#define PID_LIMIT (0xFFFF) // Reserved for the kernel

typedef enum ProcessMode {
    PM_USER,
    PM_SUPERVISOR
} ProcessMode;

typedef enum ProcessState {
    PS_DEAD,
    PS_WAITING,
    PS_SLEEPING,
    PS_RUNNING
} ProcessState;

// Do NOT move or change the fields below. The
// trampoline code expects these to be in the right
// place.
typedef struct TrapFrame {
    int64_t xregs[32];
    double fregs[32];
    uint64_t sepc;
    uint64_t sstatus;
    uint64_t sie;
    uint64_t satp;
    uint64_t sscratch;
    uint64_t stvec;
    uint64_t trap_satp;
    uint64_t trap_stack;
} TrapFrame;

// Resource Control Block
typedef struct RCB {
    List *image_pages;
    List *stack_pages;
    List *heap_pages;
    List *file_descriptors;
    Map *environemnt;
    PageTable *ptable;
} RCB;

typedef struct Process {
    uint16_t pid;
    uint32_t hart;
    ProcessMode mode;
    ProcessState state;
    TrapFrame frame;
    
    // Process stats
    uint64_t sleep_until;
    uint64_t runtime;
    uint64_t ran_at;
    uint64_t priority;
    uint64_t quantum;
    
    // Resources
    RCB rcb;
    uint64_t break_size;
} Process;

/**
 * Create a new process and return it.
 * mode - Either PM_USER or PM_SUPERVISOR to determine what mode to run in.
*/
Process *process_new(ProcessMode mode);
int process_free(Process *p);
bool process_run(Process *p, uint32_t hart);

static uint16_t generate_unique_pid(void);
