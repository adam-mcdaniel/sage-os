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

#include "trap.h"
#include <stdbool.h>
#include <stdint.h>
#include <list.h>
#include <vector.h>
#include <map.h>
#include <mmu.h>
#include <kmalloc.h>
#include <lock.h>

#define ENV_VARIABLE_MAX_SIZE (1024)

// #define USER_STACK_TOP   0x00000000000e0000UL
// #define USER_STACK_BOTTOM 0x00000000000d0000UL
// Define much larger stack and heap
#define USER_STACK_TOP    0x0000000008000000UL
#define USER_STACK_BOTTOM 0x0000000000100000UL
#define USER_HEAP_TOP    0xfc0ffeef000UL
#define USER_HEAP_BOTTOM 0xfc0ffee0000UL
// #define USER_HEAP_TOP    0x00000000000a0000UL
// #define USER_HEAP_BOTTOM 0x0000000000080000UL

#define ABS(x) ((x) < 0 ? -(x) : (x))

#define USER_STACK_SIZE  (ABS(USER_STACK_TOP - USER_STACK_BOTTOM))
#define USER_HEAP_SIZE   (ABS(USER_HEAP_TOP - USER_HEAP_BOTTOM))


#define MAX_NUM_HARTS    (8) // We are gonna be scheduling 8 harts at most.
#define HART_NONE        (-1U)
#define ON_HART_NONE(p)  (p->hart == HART_NONE)

#define PID_LIMIT (0x500) // Reserved for the kernel

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


void trap_frame_debug(TrapFrame *frame);
TrapFrame *trap_frame_new(bool is_user, PageTable *page_table, uint64_t pid);
void trap_frame_free(TrapFrame *frame);

void trap_frame_set_stack_pointer(TrapFrame *frame, uint64_t stack_pointer);
void trap_frame_set_heap_pointer(TrapFrame *frame, uint64_t heap_pointer);

// Resource Control Block
typedef struct RCB {
    List *image_pages;
    List *stack_pages;
    List *heap_pages;
    List *file_descriptors;
    Map *environemnt;
    PageTable *ptable;
} RCB;

void rcb_init(RCB *rcb);
void rcb_free(RCB *rcb);
void rcb_map(RCB *rcb, uint64_t vaddr, uint64_t paddr, uint64_t size, uint64_t bits);

void rcb_debug(RCB *rcb);

typedef struct Process {
    Mutex lock;

    uint16_t pid;
    uint32_t hart;
    ProcessMode mode;
    ProcessState state;
    TrapFrame *frame;

    
    
    // Process stats
    uint64_t sleep_until;
    uint64_t runtime;
    uint64_t ran_at;
    uint64_t priority;
    uint64_t quantum;

    uint8_t *entry_point;

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

const char *process_get_env(Process *p, const char *var);
void process_put_env(Process *p, const char *var, const char *value);
/**
 * Create a new process and return it.
 * mode - Either PM_USER or PM_SUPERVISOR to determine what mode to run in.
*/
Process *process_new(ProcessMode mode);
int process_free(Process *p);
bool process_run(Process *p, uint32_t hart);

void process_map_init();
void process_map_set(Process *p);
void process_map_remove(uint16_t pid);
Process *process_map_get(uint16_t pid);
bool process_map_contains(uint16_t pid);

void pid_harts_map_init();
void pid_harts_map_set(uint32_t hart, uint16_t pid);
uint16_t pid_harts_map_get(uint32_t hart);
