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
#include <symbols.h>
#include <trap.h>
#include <sched.h>

#define DEBUG_PROCESS
#ifdef DEBUG_PROCESS
#define debugf(...) debugf(__VA_ARGS__)
#else
#define debugf(...)
#endif

// extern const unsigned long trampoline_thread_start;
// extern const unsigned long trampoline_trap_start;
// extern const unsigned long process_asm_run;


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
    // debugf("  fregs:\n");
    // for (int i = 0; i < 32; i++) {
    //     debugf("    f%d: %d\n", i, tf->fregs[i]);
    // }
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
    trap_frame_debug(p->frame);
    
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

    // if (p->frame->sscratch != kernel_mmu_translate((unsigned long)p->frame)) {
    //     warnf("`sscratch` (0x%08lx) is not pointing to the trap frame (0x%08lx)!\n", p->frame->sscratch, (unsigned long)p->frame);
    // }

    if (p->frame->trap_satp != SATP_KERNEL) {
        warnf("  trap_satp: 0x%p (physical address = %p)\n", p->frame->trap_satp, mmu_translate(p->rcb.ptable, (uintptr_t)p->frame->trap_satp));
    }
}

TrapFrame *trap_frame_new(bool is_user, PageTable *page_table) {
    TrapFrame *frame = (TrapFrame *)kzalloc(sizeof(TrapFrame));
    memset(frame, 0, sizeof(TrapFrame));
    frame->sstatus = SSTATUS_SPP_BOOL(!is_user) | SSTATUS_FS_INITIAL | SSTATUS_SPIE;
    frame->satp = SATP(kernel_mmu_translate(page_table), 0);
    frame->sscratch = frame;
    frame->stvec = trampoline_trap_start;
    frame->trap_satp = SATP_KERNEL;
    frame->trap_stack = (unsigned long)kzalloc(0x10000);
    uint64_t permission_bits = PB_READ | PB_EXECUTE | PB_WRITE;
    mmu_map_range(page_table, 
                frame->trap_stack, 
                frame->trap_stack + 0x10000, 
                kernel_mmu_translate(frame->trap_stack), 
                MMU_LEVEL_4K,
                permission_bits);
    mmu_map_range(page_table,
                frame->sscratch,
                frame->sscratch + 0x1000,
                kernel_mmu_translate((unsigned long)frame),
                MMU_LEVEL_4K,
                permission_bits);
    mmu_map_range(page_table,
                page_table,
                page_table + 0x1000,
                kernel_mmu_translate(page_table),
                MMU_LEVEL_4K,
                permission_bits);
    mmu_map_range(page_table,
                frame,
                frame + 0x1000,
                kernel_mmu_translate(frame),
                // (uint64_t)frame,
                MMU_LEVEL_4K,
                permission_bits);
    mmu_map_range(page_table,
                trampoline_trap_start,
                trampoline_trap_start + 0x1000,
                kernel_mmu_translate(trampoline_trap_start),
                MMU_LEVEL_4K,
                permission_bits);

    debugf("process.c (trap_frame_new): TrapFrame address: 0x%08x\n", frame);
    return frame;
}

void trap_frame_free(TrapFrame *frame) {
    kfree((void *)frame->trap_stack);
    kfree(frame);
}

void rcb_init(RCB *rcb) {
    rcb->image_pages = list_new();
    rcb->stack_pages = list_new();
    rcb->heap_pages = list_new();
    rcb->file_descriptors = list_new();
    rcb->environemnt = map_new();
    rcb->ptable = mmu_table_create();
}
void rcb_free(RCB *rcb) {
    list_free(rcb->image_pages);
    list_free(rcb->stack_pages);
    list_free(rcb->heap_pages);
    list_free(rcb->file_descriptors);
    map_free(rcb->environemnt);
    page_free(rcb->ptable);
}

void rcb_map(RCB *rcb, uint64_t vaddr, uint64_t paddr, uint64_t size, uint64_t bits) {
    debugf("rcb_map: vaddr = 0x%08lx, paddr = 0x%08lx, size = 0x%08lx, bits = 0x%08lx\n", vaddr, paddr, size, bits);
    if (size % PAGE_SIZE_4K != 0) {
        warnf("rcb_map: size is not a multiple of PAGE_SIZE_4K\n");
        size = ALIGN_UP_POT(size, PAGE_SIZE_4K);
        size += PAGE_SIZE_4K;
    }

    uint64_t alignment = ~(PAGE_SIZE_4K - 1);
    for (uint64_t i = 0; i < size; i += PAGE_SIZE_4K) {
        debugf("rcb_map: Mapping 0x%08lx to 0x%08lx\n", (vaddr + i) & alignment, (paddr + i) & alignment);
        mmu_map(rcb->ptable, (vaddr + i) & alignment, (paddr + i) & alignment, MMU_LEVEL_4K, bits);
    }
}

Process *process_new(ProcessMode mode)
{
    Process *p = (Process *)kzalloc(sizeof(Process));
    debugf("process.c (process_new): Process address: 0x%08x\n", p);

    p->pid = generate_unique_pid();
    p->hart = -1U;
    p->mode = mode;
    p->state = PS_WAITING;
    p->quantum = 10;
    p->priority = 10;
    
    process_map_set(p);

    // Initialize the Resource Control Block
    rcb_init(&p->rcb);

    // Set the trap frame and create all necessary structures.
    // p->frame->sepc = filled_in_by_ELF_loader
    // p->frame->sstatus = SSTATUS_SPP_BOOL(mode) | SSTATUS_FS_INITIAL | SSTATUS_SPIE;
    // p->frame->satp = SATP(p->rcb.ptable, p->pid);
    // p->frame->sscratch = (unsigned long)&p->frame;
    // p->frame->stvec = trampoline_trap_start;
    // p->frame->trap_satp = SATP_KERNEL;

    p->frame = trap_frame_new(mode == PM_USER, p->rcb.ptable);


    // p->frame->sie = SIE_SEIE | SIE_SSIE | SIE_STIE;
    // p->frame->sstatus = SSTATUS_SPP_BOOL(mode) | SSTATUS_FS_INITIAL | SSTATUS_SPIE;
    // p->frame->stvec = trampoline_trap_start;
    // p->frame->sscratch = kernel_mmu_translate(&p->frame);
    // p->frame->satp = SATP(kernel_mmu_translate(p->rcb.ptable), p->pid % 0xffff);
    // p->frame->trap_satp = SATP_KERNEL;


    // p->frame->trap_stack = kzalloc(0x1000);
    // uint64_t permission_bits = PB_READ | PB_EXECUTE | PB_WRITE | PB_USER;
    // mmu_map_range(p->rcb.ptable, 
    //             p->frame->trap_stack, 
    //             p->frame->trap_stack + 0x10000, 
    //             kernel_mmu_translate(p->frame->trap_stack), 
    //             MMU_LEVEL_4K,
    //             permission_bits);
    // mmu_map_range(p->rcb.ptable, 
    //             p->frame->sscratch, 
    //             p->frame->sscratch + 0x10000, 
    //             kernel_mmu_translate(p->frame->sscratch),
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
    //             // (uint64_t)&p->frame,
    //             MMU_LEVEL_4K,
    //             permission_bits);

    // Add the trap handler to the process's page table
    // mmu_map_range(p->rcb.ptable,
    //             p->frame->stvec & ~0xfff,
    //             p->frame->stvec & ~0xfff + 0x1000,
    //             p->frame->stvec & ~0xfff,
    //             MMU_LEVEL_4K,
    //             permission_bits);
    // p->frame->trap_stack = filled_in_by_SCHEDULER
    
    // p->frame->trap_stack = (uint64_t)kzalloc(0x10000); 

    // We need to keep track of the stack itself in the kernel, so we can free it
    // later, but the user process will interact with the stack via the SP register.
    // for (unsigned long i = 0; i < STACK_PAGES; i += 1) {
    //     void *stack = page_zalloc();
    //     list_add_ptr(p->rcb.stack_pages, stack);
    //     rcb_map(&p->rcb, STACK_TOP + PAGE_SIZE * i, (unsigned long)stack, PAGE_SIZE, PB_READ | PB_WRITE | PB_USER);
    // }

    // for (unsigned long i = 0; i < STACK_PAGES; i += 1) {
    //     void *stack = page_zalloc();
    //     list_add_ptr(p->rcb.stack_pages, stack);
    //     rcb_map(&p->rcb, STACK_TOP + PAGE_SIZE * i, (unsigned long)stack, PAGE_SIZE, PB_READ | PB_WRITE | PB_USER);
    // }
    

    // We need to map certain kernel portions into the user's page table. Notice
    // that the PB_USER is NOT set, but it needs to be there because we need to execute
    // the trap/start instructions while using the user's page table until we change SATP.
    void process_asm_run(void *frame_addr);
    rcb_map(&p->rcb, trampoline_thread_start, kernel_mmu_translate(trampoline_thread_start), 0x1000, PB_READ | PB_EXECUTE);
    rcb_map(&p->rcb, trampoline_trap_start, kernel_mmu_translate(trampoline_trap_start), 0x1000, PB_READ | PB_EXECUTE);
    rcb_map(&p->rcb, process_asm_run, kernel_mmu_translate(process_asm_run), 0x10000, PB_READ | PB_EXECUTE);
    rcb_map(&p->rcb, os_trap_handler, kernel_mmu_translate(os_trap_handler), 0x1000, PB_READ | PB_EXECUTE);
    rcb_map(&p->rcb, p->frame, kernel_mmu_translate(p->frame), 0x1000, PB_READ | PB_WRITE | PB_EXECUTE);

    // unsigned long trans_trampoline_start = mmu_translate(kernel_mmu_table, trampoline_thread_start);
    // unsigned long trans_trampoline_trap  = mmu_translate(kernel_mmu_table, trampoline_trap_start);
    // unsigned long trans_process_asm_run  = mmu_translate(kernel_mmu_table, process_asm_run);
    // unsigned long trans_os_trap_handler  = mmu_translate(kernel_mmu_table, os_trap_handler);
    // mmu_map_range(p->rcb.ptable, sym_start(text), sym_end(heap), sym_start(text), MMU_LEVEL_4K,
    //               PB_READ | PB_WRITE | PB_EXECUTE | PB_USER | PB_VALID);
    // PLIC
    // debugf("Mapping PLIC\n");
    // mmu_map_range(p->rcb.ptable, 0x0C000000, 0x0C2FFFFF, 0x0C000000, MMU_LEVEL_2M, PB_READ | PB_WRITE);
    // // PCIe ECAM
    // debugf("Mapping PCIe ECAM\n");
    // mmu_map_range(p->rcb.ptable, 0x30000000, 0x3FFFFFFF, 0x30000000, MMU_LEVEL_2M, PB_READ | PB_WRITE);
    // // PCIe MMIO
    // debugf("Mapping PCIe MMIO\n");
    // mmu_map_range(p->rcb.ptable, 0x40000000, 0x5FFFFFFF, 0x40000000, MMU_LEVEL_2M, PB_READ | PB_WRITE);
    // mmu_map(p->rcb.ptable, trampoline_thread_start, trans_trampoline_start, MMU_LEVEL_4K,
    //         PB_READ | PB_EXECUTE | PB_USER | PB_VALID);
    // mmu_map(p->rcb.ptable, trampoline_trap_start, trans_trampoline_trap, MMU_LEVEL_4K,
    //         PB_READ | PB_EXECUTE | PB_USER | PB_VALID);
    // // Map the trap stack
    // mmu_map_range(p->rcb.ptable,
    //             process_asm_run,
    //             process_asm_run + 0x10000,
    //             kernel_mmu_translate(process_asm_run),
    //             MMU_LEVEL_4K,
    //             PB_READ | PB_EXECUTE | PB_USER | PB_VALID);
    // mmu_map(p->rcb.ptable, os_trap_handler, trans_os_trap_handler, MMU_LEVEL_4K,
    //         PB_READ | PB_EXECUTE | PB_USER | PB_VALID);
    // Map trap frame to user's page table
    // uintptr_t trans_frame = kernel_mmu_translate((uintptr_t)&p->frame);
    // mmu_map(p->rcb.ptable, p->frame->stvec, kernel_mmu_translate(p->frame->stvec), MMU_LEVEL_4K, PB_READ | PB_WRITE | PB_EXECUTE | PB_USER);
    // mmu_map(p->rcb.ptable, &p->frame, trans_frame, MMU_LEVEL_4K, PB_READ | PB_WRITE | PB_EXECUTE | PB_USER | PB_VALID);
    // debug_page_table(p->rcb.ptable, MMU_LEVEL_4K);
    // SFENCE_ASID(p->pid);
    mmu_print_entries(p->rcb.ptable, MMU_LEVEL_4K);

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
        set_current_process(p);
        process_debug(p);
        uint64_t satp, sscratch;
        CSR_READ(satp, "satp");
        CSR_READ(sscratch, "sscratch");
        debugf("Old SATP: 0x%08x\n", satp);
        trap_frame_debug((TrapFrame*)sscratch);
        debugf("New SATP: 0x%08x\n", p->frame->satp);
        trap_frame_debug(p->frame);
        pid_harts_map_set(hart, p->pid);
        debugf("process.c (process_run): Running process %d on hart %d\n", p->pid, hart);
        // trap_frame_debug(&p->frame);
        debugf("Jumping to 0x%08lx\n", p->frame->sepc);
        process_asm_run(p->frame);
        
        fatalf("process.c (process_run): process_asm_run returned\n");
        // process_asm_run should not return, but if it does
        // something went wrong.
        return false;
    }

    return sbi_hart_start(hart, trampoline_thread_start, (unsigned long)p->frame, p->frame->satp);
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
    // pid_on_harts = map_new_with_slots(0x100);
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
