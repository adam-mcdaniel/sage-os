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

#define HART_NONE        (-1U)
#define ON_HART_NONE(p)  (p->hart == HART_NONE)

typedef enum {
    PM_USER,
    PM_SUPERVISOR
} process_mode;

typedef enum {
    PS_DEAD,
    PS_WAITING,
    PS_SLEEPING,
    PS_RUNNING
} process_state;

// Do NOT move or change the fields below. The
// trampoline code expects these to be in the right
// place.
struct trap_frame {
    signed long xregs[32];
    double fregs[32];
    unsigned long sepc;
    unsigned long sstatus;
    unsigned long sie;
    unsigned long satp;
    unsigned long sscratch;
    unsigned long stvec;
    unsigned long trap_satp;
    unsigned long trap_stack;
};

struct page_table;

struct List;
struct Vector;

struct process {
    unsigned short pid;
    unsigned int hart;
    process_mode mode;
    process_state state;
    unsigned long sleep_until;
    unsigned long runtime;
    unsigned long ran_at;
    unsigned long priority;
    
    unsigned long break_size;

    struct trap_frame frame;
    struct page_table *ptable;

    struct List *pages;
    struct Vector *fds;
};

/**
 * Create a new process and return it.
 * mode - Either PM_USER or PM_SUPERVISOR to determine what mode to run in.
*/
struct process *process_new(process_mode mode);
int process_free(struct process *p);

bool process_run(struct process *p, unsigned int hart);
