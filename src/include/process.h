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
#include <mmu.h>

#define HART_NONE        (-1U)
#define ON_HART_NONE(p)  (p->hart == HART_NONE)

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

uint16_t generate_unique_pid(void);

// Resource Control Block
typedef struct RCB {
    List *image_pages;
    List *heap_pages;
} RCB;

typedef struct Process {
    uint16_t pid;
    uint32_t hart;
    ProcessMode mode;
    ProcessState state;
    uint64_t sleep_until;
    uint64_t runtime;
    uint64_t ran_at;
    uint64_t priority;
    uint64_t quantum;
    
    uint64_t break_size;

    TrapFrame frame;
    PageTable *ptable;

    List *pages;
    Vector *fds;
    RCB rcb;
} Process;

/**
 * Create a new process and return it.
 * mode - Either PM_USER or PM_SUPERVISOR to determine what mode to run in.
*/
Process *process_new(ProcessMode mode);
int process_free(Process *p);

bool process_run(Process *p, uint32_t hart);

