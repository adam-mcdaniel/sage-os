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
#include <map.h>
#include <util.h>


extern const unsigned long trampoline_thread_start;
extern const unsigned long trampoline_trap_start;

#define STACK_PAGES 2
#define STACK_SIZE  (STACK_PAGES * PAGE_SIZE)
#define STACK_TOP   0xfffffffc0ffee000UL

static uint16_t pid = 1; // Start from 1, 0 is reserved

static uint16_t generate_unique_pid(void) {
    ++pid;
    if (pid == PID_LIMIT)
        warnf("process.c (generate_unique_pid): Reached PID_LIMIT\n");
    return pid;
}

void rcb_debug(RCB *rcb) {
    debugf("RCB:\n");
    // List *image_pages;
    // List *stack_pages;
    // List *heap_pages;
    // List *file_descriptors;
    // Map *environemnt;
    // PageTable *ptable;
    debugf("  image_pages:\n");
    struct ListElem *e;
    list_for_each(rcb->image_pages, e) {
        debugf("    %p\n", list_elem_value_ptr(e));
    }
    debugf("  stack_pages:\n");
    list_for_each(rcb->stack_pages, e) {
        debugf("    %p\n", list_elem_value_ptr(e));
    }
    debugf("  heap_pages:\n");
    list_for_each(rcb->heap_pages, e) {
        debugf("    %p\n", list_elem_value_ptr(e));
    }
    debugf("  file_descriptors:\n");
    list_for_each(rcb->file_descriptors, e) {
        debugf("    %p\n", list_elem_value_ptr(e));
    }
    debugf("  environemnt:\n");
    struct List *keys = map_get_keys(rcb->environemnt);
    struct ListElem *k;
    list_for_each(keys, k) {
        char *key = list_elem_value_ptr(k);
        char *value;
        map_get(rcb->environemnt, key, (MapValue *)&value);
        debugf("    %s=%s\n", key, value);
    }
    map_free_get_keys(keys);
    debugf("  ptable:\n");
    debugf("    %p\n", rcb->ptable);
}


void trap_frame_debug(TrapFrame *tf) {
    debugf("TrapFrame:\n");
    // int64_t xregs[32];
    // double fregs[32];
    // uint64_t sepc;
    // uint64_t sstatus;
    // uint64_t sie;
    // uint64_t satp;
    // uint64_t sscratch;
    // uint64_t stvec;
    // uint64_t trap_satp;
    // uint64_t trap_stack;
    debugf("  xregs:\n");
    for (int i = 0; i < 32; i++) {
        debugf("    x%d: %d\n", i, tf->xregs[i]);
    }
    debugf("  fregs:\n");
    for (int i = 0; i < 32; i++) {
        debugf("    f%d: %d\n", i, tf->fregs[i]);
    }
    debugf("  sepc: 0x%p\n", tf->sepc);
    debugf("  sstatus: 0x%p\n", tf->sstatus);
    debugf("  sie: 0x%p\n", tf->sie);
    debugf("  satp: 0x%p\n", tf->satp);
    debugf("  sscratch: 0x%p\n", tf->sscratch);
    debugf("  stvec: 0x%p\n", tf->stvec);
    debugf("  trap_satp: 0x%p\n", tf->trap_satp);
    debugf("  trap_stack: 0x%p\n", tf->trap_stack);
}

void process_debug(Process *p) {
    debugf("Process:\n");
    // uint16_t pid;
    // uint32_t hart;
    // ProcessMode mode;
    // ProcessState state;
    // TrapFrame frame;
    debugf("  pid: %d\n", p->pid);
    debugf("  hart: %d\n", p->hart);
    debugf("  mode: %d\n", p->mode);
    debugf("  state: %d\n", p->state);
    trap_frame_debug(&p->frame);
    
    // // Process stats
    // uint64_t sleep_until;
    // uint64_t runtime;
    // uint64_t ran_at;
    // uint64_t priority;
    // uint64_t quantum;
    debugf("  sleep_until: %d\n", p->sleep_until);
    debugf("  runtime: %d\n", p->runtime);
    debugf("  ran_at: %d\n", p->ran_at);
    debugf("  priority: %d\n", p->priority);
    debugf("  quantum: %d\n", p->quantum);

    if (p->image) {
        debugf("  image: %p\n", p->image);
        debugf("  image_size: 0x%X (%d pages)\n", p->image_size, ALIGN_UP_POT(p->image_size, PAGE_SIZE_4K) / PAGE_SIZE_4K);
    } else {
        debugf("  image: NULL\n");
    }
    if (p->text) {
        debugf("  text: %p (physical address = %p)\n", p->text_vaddr, mmu_translate(p->rcb.ptable, (uintptr_t)p->text_vaddr));
        debugf("  text_size: 0x%X (%d pages)\n", p->text_size, ALIGN_UP_POT(p->text_size, PAGE_SIZE_4K) / PAGE_SIZE_4K);
    } else {
        debugf("  text: NULL\n");
    }
    if (p->bss) {
        debugf("  bss: %p (physical address = %p)\n", p->bss_vaddr, mmu_translate(p->rcb.ptable, (uintptr_t)p->bss_vaddr));
        // debugf("  bss_size: 0x%X (%d pages)\n", p->bss_size, p->bss_size / PAGE_SIZE);
        debugf("  bss_size: 0x%X (%d pages)\n", p->bss_size, ALIGN_UP_POT(p->bss_size, PAGE_SIZE_4K) / PAGE_SIZE_4K);
        
    } else {
        debugf("  bss: NULL\n");
    }
    if (p->rodata) {
        debugf("  rodata: %p (physical address = %p)\n", p->rodata_vaddr, mmu_translate(p->rcb.ptable, (uintptr_t)p->rodata_vaddr));
        debugf("  rodata_size: 0x%X (%d pages)\n", p->rodata_size, ALIGN_UP_POT(p->rodata_size, PAGE_SIZE_4K) / PAGE_SIZE_4K);
    } else {
        debugf("  rodata: NULL\n");
    }
    if (p->data) {
        debugf("  data: %p (physical address = %p)\n", p->data_vaddr, mmu_translate(p->rcb.ptable, (uintptr_t)p->data_vaddr));
        debugf("  data_size: 0x%X (%d pages)\n", p->data_size, ALIGN_UP_POT(p->data_size, PAGE_SIZE_4K) / PAGE_SIZE_4K);
    } else {
        debugf("  data: NULL\n");
    }

    // // Resources
    // RCB rcb;
    rcb_debug(&p->rcb);
    // uint64_t break_size;
}

Process *process_new(ProcessMode mode)
{
    Process *p = (Process *)kzalloc(sizeof(*p));
    debugf("process.c (process_new): Process address: 0x%08x\n", p);

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
    
    p->frame.trap_stack = (uint64_t)kmalloc(0x10000); 

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

    // Map trap frame to user's page table
    uintptr_t trans_frame = kernel_mmu_translate((uintptr_t)&p->frame);
    mmu_map(p->rcb.ptable, (uintptr_t)&p->frame, trans_frame, MMU_LEVEL_4K, PB_READ | PB_WRITE | PB_EXECUTE);

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
    MapValue val;
    map_get_int(processes, pid, &val);
    return (Process *)val;
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
    MapValue val;
    map_get_int(pid_on_harts, hart, &val);
    return (uint16_t)val;
}
