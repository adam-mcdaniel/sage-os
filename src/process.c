#include <csr.h>
#include <debug.h>
#include <kmalloc.h>
#include <list.h>
#include <mmu.h>
#include <page.h>
#include <process.h>
#include <sbi.h>
#include <stdbool.h>
#include <vector.h>
#include <stddef.h>
#include <stdint.h>
#include "map.h"

extern const unsigned long trampoline_thread_start;
extern const unsigned long trampoline_trap_start;

#define STACK_PAGES 2
#define STACK_SIZE  (STACK_PAGES * PAGE_SIZE)
#define STACK_TOP   0xfffffffc0ffee000UL

static uint16_t pid = 1; // Start from 1, 0 is reserved

static uint16_t generate_unique_pid(void) {
    pid++;
    if (pid == PID_LIMIT)
        warnf("process (generate_unique_pid): Reached PID_LIMIT\n");
    return pid++;
}

Process *process_new(ProcessMode mode)
{
    Process *p = (Process *)kzalloc(sizeof(*p));

    p->pid = generate_unique_pid();
    p->hart = -1U;
    p->mode = mode;
    p->state = PS_WAITING;
    
    process_map_set(p);

    // Initialize the Resource Control Block
    p->rcb.image_pages = list_new();
    p->rcb.stack_pages = list_new();
    p->rcb.heap_pages = list_new();
    p->rcb.file_descriptors = list_new();
    p->rcb.environemnt = map_new();
    p->rcb.ptable = mmu_table_create();

    // Set the trap frame and create all necessary structures.
    // p->frame.sepc = filled_in_by_ELF_loader
    p->frame.sstatus = SSTATUS_SPP_BOOL(mode) | SSTATUS_FS_INITIAL | SSTATUS_SPIE;
    p->frame.sie = SIE_SEIE | SIE_SSIE | SIE_STIE;
    p->frame.satp = SATP(p->rcb.ptable, p->pid);
    p->frame.sscratch = (unsigned long)&p->frame;
    p->frame.stvec = trampoline_trap_start;
    p->frame.trap_satp = SATP_KERNEL;
    // p->frame.trap_stack = filled_in_by_SCHEDULER

    // We need to keep track of the stack itself in the kernel, so we can free it
    // later, but the user process will interact with the stack via the SP register.
    p->frame.xregs[XREG_SP] = STACK_TOP + STACK_SIZE;
    for (unsigned long i = 0; i < STACK_PAGES; i += 1) {
        void *stack = page_zalloc();
        list_add_ptr(p->rcb.stack_pages, stack);
        mmu_map(p->rcb.ptable, STACK_TOP + PAGE_SIZE * i, (unsigned long)stack,
                MMU_LEVEL_4K, mode == PM_USER ? PB_USER : 0 | PB_READ | PB_WRITE);
    }

    // We need to map certain kernel portions into the user's page table. Notice
    // that the PB_USER is NOT set, but it needs to be there because we need to execute
    // the trap/start instructions while using the user's page table until we change SATP.
    unsigned long trans_trampoline_start = mmu_translate(kernel_mmu_table, trampoline_thread_start);
    unsigned long trans_trampoline_trap  = mmu_translate(kernel_mmu_table, trampoline_trap_start);
    mmu_map(p->rcb.ptable, trampoline_thread_start, trans_trampoline_start, MMU_LEVEL_4K,
            PB_READ | PB_EXECUTE);
    mmu_map(p->rcb.ptable, trampoline_trap_start, trans_trampoline_trap, MMU_LEVEL_4K,
            PB_READ | PB_EXECUTE);

    SFENCE_ASID(p->pid);

    return p;
}

int process_free(Process *p)
{
    struct ListElem *e;

    if (!p || !ON_HART_NONE(p)) {
        // Process is invalid or running somewhere, or this is stale.
        return -1;
    }

    // Free all resources allocated to the process.
    if (p->rcb.image_pages) {
        list_for_each(p->rcb.image_pages, e) {
            page_free(list_elem_value_ptr(e));
        }
        list_free(p->rcb.image_pages);
    }

    if (p->rcb.stack_pages) {
        list_for_each(p->rcb.stack_pages, e) {
            page_free(list_elem_value_ptr(e));
        }
        list_free(p->rcb.stack_pages);
    }

    if (p->rcb.heap_pages) {
        list_for_each(p->rcb.heap_pages, e) {
            page_free(list_elem_value_ptr(e));
        }
        list_free(p->rcb.heap_pages);
    }

    if (p->rcb.file_descriptors) {
        list_for_each(p->rcb.file_descriptors, e) {
            page_free(list_elem_value_ptr(e));
        }
        list_free(p->rcb.file_descriptors);
    }

    if (p->rcb.environemnt) {
        map_free_get_keys(map_get_keys(p->rcb.environemnt));
    }

    if (p->rcb.ptable) {
        mmu_free(p->rcb.ptable);
        SFENCE_ASID(p->pid);
    }

    kfree(p);
    return 0;
}

bool process_run(Process *p, unsigned int hart)
{
    void process_asm_run(void *frame_addr);
    unsigned int me = sbi_whoami();

    if (me == hart) {
        pid_harts_map_set(hart, p->pid);
        process_asm_run(&p->frame);
        // process_asm_run should not return, but if it does
        // something went wrong.
        return false;
    }

    return sbi_hart_start(hart, trampoline_thread_start, (unsigned long)&p->frame, p->frame.satp);
}

// Map of all the processes, key is PID.
static Map *processes;

// Initialize the processes map, needs to be called before creating the
// first process.
void process_map_init()
{
    processes = map_new();
}

// Store a process on a map as its PID as the key.
void process_map_set(Process *p)
{
    map_set_int(processes, p->pid, (MapValue)p);
}

// Get process stored on the process map using the PID as the key.
Process *process_map_get(uint16_t pid) 
{
    MapValue *val;
    map_get_int(processes, pid, val);
    return (Process *)*val; 
}

// Keep track of the PIDs running on each hart.
static Map *pid_on_harts;

// Initialize the PID on harts map, needs to be called before creating the
// first process.
void pid_harts_map_init()
{
    pid_on_harts = map_new();
}

// Associate the PID running to hart
void pid_harts_map_set(uint32_t hart, uint16_t pid)
{
    if (hart > MAX_NUM_HARTS - 1)
        fatalf("set_pid_on_hart: Invalid hart number\n");
    map_set_int(pid_on_harts, hart, pid);
}

// Get the PID running on hart
uint16_t pid_harts_map_get(uint32_t hart)
{
    if (hart > MAX_NUM_HARTS - 1)
        fatalf("get_pid_on_hart: Invalid hart number\n");
    MapValue *val;
    map_get_int(pid_on_harts, hart, val);
    return (uint16_t)*val;
}
