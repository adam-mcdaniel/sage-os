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

#define DEFAULT_HEAP_SIZE (0x10000) // 16 KB
#define DEFAULT_STACK_SIZE (0x2000) // 8 KB

#define MAX_NUM_HARTS    (8) // We are gonna be scheduling 8 harts at most.
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

void trap_frame_debug(TrapFrame *frame);

// Resource Control Block
typedef struct RCB {
    List *image_pages;
    List *stack_pages;
    List *heap_pages;
    List *file_descriptors;
    Map *environemnt;
    PageTable *ptable;
} RCB;

void rcb_debug(RCB *rcb);

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

    // Memory
    uint8_t *image;
    uint64_t image_size;

    uint8_t *text, *text_vaddr;
    uint64_t text_size;
    uint8_t *bss, *bss_vaddr;
    uint64_t bss_size;
    uint8_t *rodata, *rodata_vaddr;
    uint64_t rodata_size;
    uint8_t *data, *data_vaddr;
    uint64_t data_size;

    uint8_t *stack, *stack_vaddr;
    uint64_t stack_size;
    uint8_t *heap, *heap_vaddr;
    uint64_t heap_size;
    
    // Resources
    RCB rcb;
    uint64_t break_size;
} Process;

void process_debug(Process *p);

/**
 * Create a new process and return it.
 * mode - Either PM_USER or PM_SUPERVISOR to determine what mode to run in.
*/
Process *process_new(ProcessMode mode);
int process_free(Process *p);
bool process_run(Process *p, uint32_t hart);

void process_map_init();
void process_map_set(Process *p);
Process *process_map_get(uint16_t pid);

void pid_harts_map_init();
void pid_harts_map_set(uint32_t hart, uint16_t pid);
uint16_t pid_harts_map_get(uint32_t hart);
