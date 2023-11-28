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
#include <lock.h>
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
    struct ListElem *e;
    // debugf("  image_pages:\n");
    // list_for_each(rcb->image_pages, e) {
    //     debugf("    %p\n", list_elem_value_ptr(e));
    // }
    // debugf("  stack_pages:\n");
    // list_for_each(rcb->stack_pages, e) {
    //     debugf("    %p\n", list_elem_value_ptr(e));
    // }
    // debugf("  heap_pages:\n");
    // list_for_each(rcb->heap_pages, e) {
    //     debugf("    %p\n", list_elem_value_ptr(e));
    // }
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
    debugf("TrapFrame (v = %p, p = %p):\n", tf, kernel_mmu_translate(tf));
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
        debugf("    x%2d: %16u (0x%p)\n", i, tf->xregs[i], tf->xregs[i]);
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
        // debugf("  text (end): %p (physical address = %p)\n", p->text_vaddr + p->text_size - 1, mmu_translate(p->rcb.ptable, (uintptr_t)p->text_vaddr + p->text_size - 1));
        for (uint64_t i = 0; i < p->text_size / PAGE_SIZE_4K; i++) {
            // Confirm the page is mapped
            debugf("  text (page %d): %p (physical address = %p)\n", i, p->text_vaddr + i * PAGE_SIZE_4K, mmu_translate(p->rcb.ptable, (uintptr_t)p->text_vaddr + i * PAGE_SIZE_4K));
        }
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

    if (p->stack_bottom && p->stack_top) {
        // debugf("  stack: %p (physical address = %p)\n", p->stack_vaddr, mmu_translate(p->rcb.ptable, (uintptr_t)p->stack_vaddr));
        // debugf("  stack_size: 0x%X (%d pages)\n", p->stack_size, ALIGN_UP_POT(p->stack_size, PAGE_SIZE_4K) / PAGE_SIZE_4K);

        debugf("  stack_bottom: %p (physical address = %p)\n", p->stack_bottom_vaddr, mmu_translate(p->rcb.ptable, (uintptr_t)p->stack_bottom_vaddr));
        debugf("  stack_top: %p (physical address = %p)\n", p->stack_top_vaddr, mmu_translate(p->rcb.ptable, (uintptr_t)p->stack_top_vaddr));
        debugf("  stack_size: 0x%X (%d pages)\n", p->stack_size, ALIGN_UP_POT(p->stack_size, PAGE_SIZE_4K) / PAGE_SIZE_4K);
    } else {
        debugf("  stack: NULL\n");
    }

    if (p->heap_bottom && p->heap_top) {
        debugf("  heap_bottom: %p (physical address = %p)\n", p->heap_bottom_vaddr, mmu_translate(p->rcb.ptable, (uintptr_t)p->heap_bottom_vaddr));
        debugf("  heap_top: %p (physical address = %p)\n", p->heap_top_vaddr, mmu_translate(p->rcb.ptable, (uintptr_t)p->heap_top_vaddr));
        debugf("  heap_size: 0x%X (%d pages)\n", p->heap_size, ALIGN_UP_POT(p->heap_size, PAGE_SIZE_4K) / PAGE_SIZE_4K);
    } else {
        debugf("  heap: NULL\n");
    }

    debugf("  entry_point: %p (physical address = %p)\n", p->entry_point, mmu_translate(p->rcb.ptable, (uintptr_t)p->entry_point));

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

TrapFrame *trap_frame_new(bool is_user, PageTable *page_table, uint64_t pid) {
    TrapFrame *frame = (TrapFrame *)kzalloc(sizeof(TrapFrame));
    memset(frame, 0, sizeof(TrapFrame));
    frame->sstatus = SSTATUS_FS_INITIAL | SSTATUS_SPIE;
    if (is_user) {
        frame->sstatus |= SSTATUS_SPP_USER;
    } else {
        frame->sstatus |= SSTATUS_SPP_SUPERVISOR;
    }
    debugf("Process ASID: %d\n", (pid + 5 % (0xffff - 10)) + 5);
    frame->satp = SATP(kernel_mmu_translate((uintptr_t)page_table), (pid + 5 % (0xffff - 10)) + 5);
    frame->sscratch = (uintptr_t)frame;
    frame->trap_satp = SATP_KERNEL;
    frame->stvec = kernel_trap_frame->stvec;
    frame->trap_stack = kernel_trap_frame->trap_stack;
    // frame->sie = SIE_SEIE | SIE_SSIE | SIE_STIE;
    

    uint64_t permission_bits = PB_READ | PB_EXECUTE | PB_WRITE;
    debugf("process.c (trap_frame_new): Mapping trap stack 0x%08lx:0x%08lx to 0x%08lx\n", frame->trap_stack, frame->trap_stack + 0x10000, kernel_mmu_translate(frame->trap_stack));
    mmu_map_range(page_table, 
                frame->trap_stack, 
                frame->trap_stack + 0x10000, 
                kernel_mmu_translate(frame->trap_stack), 
                MMU_LEVEL_4K,
                permission_bits);

    debugf("process.c (trap_frame_new): Mapping page table 0x%08lx:0x%08lx to 0x%08lx\n", page_table, page_table + 0x1000, kernel_mmu_translate(page_table));
    mmu_map_range(page_table,
                (uintptr_t)page_table,
                (uintptr_t)(page_table) + 0x1000,
                kernel_mmu_translate((uintptr_t)page_table),
                MMU_LEVEL_4K,
                permission_bits);

    debugf("process.c (trap_frame_new): Mapping trap frame 0x%08lx:0x%08lx to 0x%08lx\n", frame, frame + 0x1000, kernel_mmu_translate(frame));
    mmu_map_range(page_table,
                (uintptr_t)frame,
                (uintptr_t)(frame) + 0x1000,
                kernel_mmu_translate((uintptr_t)frame),
                MMU_LEVEL_4K,
                permission_bits);

    debugf("process.c (trap_frame_new): Mapping trampoline trap 0x%08lx:0x%08lx to 0x%08lx\n", trampoline_trap_start, trampoline_trap_start + 0x1000, kernel_mmu_translate(trampoline_trap_start));
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
    // size = ALIGN_UP_POT(size, PAGE_SIZE_4K);

    bits |= PB_EXECUTE | PB_READ | PB_WRITE | PB_VALID;

    uint64_t alignment = ~(PAGE_SIZE_4K - 1);
    for (uint64_t i = 0; i < size / PAGE_SIZE_4K; i++) {
        // debugf("rcb_map: Mapping 0x%08lx to 0x%08lx\n", (vaddr + i) & alignment, (paddr + i) & alignment);
        // debugf("rcb_map: Mapping 0x%08lx to 0x%08lx\n", ALIGN_UP_POT(vaddr + i, PAGE_SIZE_4K), ALIGN_UP_POT(paddr + i, PAGE_SIZE_4K));
        // mmu_map(rcb->ptable, (vaddr + i) & alignment, (paddr + i) & alignment, MMU_LEVEL_4K, bits);
        // mmu_map(rcb->ptable, ALIGN_UP_POT(vaddr + i, PAGE_SIZE_4K), kernel_mmu_translate(ALIGN_UP_POT(paddr + i, PAGE_SIZE_4K)), MMU_LEVEL_4K, bits);
        // mmu_map(rcb->ptable, vaddr + i * PAGE_SIZE_4K, kernel_mmu_translate(paddr + i * PAGE_SIZE_4K), MMU_LEVEL_4K, bits);
        // debugf("Mapped 0x%08lx to 0x%08lx\n", vaddr + i * PAGE_SIZE_4K, kernel_mmu_translate(paddr + i * PAGE_SIZE_4K));
        mmu_map(rcb->ptable, (vaddr + i) & alignment, (paddr + i) & alignment, MMU_LEVEL_4K, bits);
    }

    // for (uint64_t i = 0; i < p->heap_size / PAGE_SIZE; i++) {
    //     list_add_ptr(p->rcb.heap_pages, kernel_mmu_translate(p->heap + i * PAGE_SIZE));
    // // }
    //     // debugf("Mapping heap page\n");
    //     if (USER_HEAP_BOTTOM + i * PAGE_SIZE > USER_HEAP_TOP) {
    //         fatalf("process.c (process_new): Heap overflow\n");
    //     }
    //     rcb_map(&p->rcb, 
    //             USER_HEAP_BOTTOM + i * PAGE_SIZE, 
    //             (uint64_t)p->heap + i * PAGE_SIZE,
    //             PAGE_SIZE,
    //             permission_bits);
    //     // debugf("Heap page mapped\n");
    // }
}

void trap_frame_set_stack_pointer(TrapFrame *frame, uint64_t stack_pointer) {
    frame->xregs[2] = stack_pointer;
}

void trap_frame_set_heap_pointer(TrapFrame *frame, uint64_t heap_pointer) {
    frame->xregs[3] = heap_pointer;
}

Process *process_new(ProcessMode mode)
{
    Process *p = (Process *)kzalloc(sizeof(Process));
    debugf("process.c (process_new): Process address: 0x%08x\n", p);
    p->lock = MUTEX_UNLOCKED;
    mutex_spinlock(&p->lock);
    p->pid = generate_unique_pid();
    p->hart = sbi_whoami();
    p->mode = mode;
    p->state = PS_WAITING;
    p->quantum = 10;
    p->priority = 10; 

    // Initialize the Resource Control Block
    rcb_init(&p->rcb);
    debugf("process.c (process_new): RCB address: 0x%08x\n", &p->rcb);

    // Set the trap frame and create all necessary structures.
    // p->frame->sepc = filled_in_by_ELF_loader
    // p->frame->sstatus = SSTATUS_SPP_BOOL(mode) | SSTATUS_FS_INITIAL | SSTATUS_SPIE;
    // p->frame->satp = SATP(p->rcb.ptable, p->pid);
    // p->frame->sscratch = (unsigned long)&p->frame;
    // p->frame->stvec = trampoline_trap_start;
    // p->frame->trap_satp = SATP_KERNEL;

    p->frame = trap_frame_new(mode == PM_USER, p->rcb.ptable, p->pid);


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
    rcb_map(&p->rcb, (uintptr_t)process_asm_run, kernel_mmu_translate((uintptr_t)process_asm_run), 0x10000, PB_READ | PB_EXECUTE);
    rcb_map(&p->rcb, (uintptr_t)os_trap_handler, kernel_mmu_translate((uintptr_t)os_trap_handler), 0x1000, PB_READ | PB_EXECUTE);
    rcb_map(&p->rcb, (uintptr_t)p->frame, kernel_mmu_translate((uintptr_t)p->frame), 0x1000, PB_READ | PB_WRITE | PB_EXECUTE);

    uint64_t permission_bits = PB_READ | PB_WRITE | PB_EXECUTE;
    if (mode == PM_USER) {
        permission_bits |= PB_USER;
    }

    p->heap_size = USER_HEAP_SIZE;
    p->stack_size = USER_STACK_SIZE;
    p->stack_top = NULL;
    p->stack_bottom = NULL;
    do {
        debugf("Allocating %d pages for the stack\n", p->stack_size / PAGE_SIZE);
        p->stack_size = ALIGN_UP_TO_PAGE(p->stack_size + 2);
        p->stack_bottom = page_znalloc(p->stack_size / PAGE_SIZE + 2);
        if (!p->stack_bottom) {
            p->stack_size /= 2;
            debugf("Stack allocation failed, trying with %d pages\n", p->stack_size / PAGE_SIZE);
        } else {
            debugf("Stack allocated %d pages\n", p->stack_size / PAGE_SIZE);
            p->stack_top = p->stack_bottom + p->stack_size;
        }

        if (p->stack_size < PAGE_SIZE * 16) {
            fatalf("process.c (process_new): Stack size too small\n");
        }
    } while (!p->stack_top || !p->stack_bottom);
    
    do {
        debugf("Allocating %d pages for the heap\n", p->heap_size / PAGE_SIZE);
        p->heap_size = ALIGN_UP_TO_PAGE(p->heap_size + 2);
        p->heap_bottom = page_znalloc(p->heap_size / PAGE_SIZE + 2);
        if (!p->heap_bottom) {
            p->heap_size /= 2;
            debugf("Heap allocation failed, trying with %d pages\n", p->heap_size / PAGE_SIZE);
        } else {
            debugf("Heap allocated %d pages\n", p->heap_size / PAGE_SIZE);
            p->heap_top = p->heap_bottom + p->heap_size;
        }

        if (p->heap_size < PAGE_SIZE * 16) {
            fatalf("process.c (process_new): Heap size too small\n");
        }
    } while (!p->heap_top || !p->heap_bottom);

    p->stack_top_vaddr = USER_STACK_TOP;
    p->stack_bottom_vaddr = p->stack_top_vaddr - p->stack_size;

    p->heap_bottom_vaddr = USER_HEAP_BOTTOM;
    p->heap_top_vaddr = p->heap_bottom_vaddr + p->heap_size;
    // do {
    //     debugf("Allocating %d pages for the heap\n", p->heap_size / PAGE_SIZE);
    //     p->heap = page_znalloc(ALIGN_UP_TO_PAGE(p->heap_size) / PAGE_SIZE);
    //     debugf("Heap address: %p\n", p->heap);
    //     if (!p->heap) {
    //         p->heap_size /= 2;
    //         p->heap_size = ALIGN_UP_TO_PAGE(p->heap_size);
    //         debugf("Heap allocation failed, trying with %d pages\n", p->heap_size / PAGE_SIZE);
    //     }

    //     if (p->heap_size < PAGE_SIZE * 16) {
    //         fatalf("process.c (process_new): Heap size too small\n");
    //     }
    // } while (!p->heap);

    if (p->stack_bottom == NULL || p->stack_top == NULL) {
        fatalf("process.c (process_new): Stack allocation failed\n");
    }
    if (p->heap_bottom == NULL || p->heap_top == NULL) {
        fatalf("process.c (process_new): Heap allocation failed\n");
    }

    // debugf("Stack: %p\n", p->stack);
    // debugf("Heap: %p\n", p->heap);
    trap_frame_set_stack_pointer(p->frame, p->stack_top_vaddr);
    trap_frame_set_heap_pointer(p->frame, p->heap_bottom_vaddr);

    debugf("Mapping stack\n");
    // rcb_map(&p->rcb, p->stack_bottom_vaddr, (uint64_t)p->stack_bottom, p->stack_size + 1, permission_bits);
    debugf("Mapping heap\n");
    // rcb_map(&p->rcb, p->heap_bottom_vaddr, (uint64_t)p->heap_bottom, p->heap_size + 1, permission_bits);
    // p->heap_vaddr = USER_HEAP_START;


    for (uint64_t i = 0; i < p->heap_size / PAGE_SIZE; i++) {
        list_add_ptr(p->rcb.heap_pages, kernel_mmu_translate(p->heap_bottom + i * PAGE_SIZE));
    // }
        // debugf("Mapping heap page\n");
        if (USER_HEAP_BOTTOM + i * PAGE_SIZE > USER_HEAP_TOP) {
            fatalf("process.c (process_new): Heap overflow\n");
        }
        rcb_map(&p->rcb, 
                (uint64_t)p->heap_bottom_vaddr + i * PAGE_SIZE, 
                (uint64_t)p->heap_bottom + i * PAGE_SIZE,
                PAGE_SIZE,
                permission_bits);
        // debugf("Heap page mapped\n");
    }
    for (uint64_t i = 0; i < p->stack_size / PAGE_SIZE; i++) {
        // list_add_ptr(p->rcb.stack_pages, p->stack + i * PAGE_SIZE);
        list_add_ptr(p->rcb.stack_pages, kernel_mmu_translate(p->stack_bottom + i * PAGE_SIZE));
    // }
        // debugf("Mapping stack page\n");
        if (p->stack_bottom_vaddr + i * PAGE_SIZE > USER_STACK_TOP) {
            fatalf("process.c (process_new): Stack overflow\n");
        }
        
        rcb_map(&p->rcb,
                (uint64_t)p->stack_bottom_vaddr + i * PAGE_SIZE, 
                (uint64_t)p->stack_bottom + i * PAGE_SIZE,
                PAGE_SIZE,
                permission_bits);
        // debugf("Stack page mapped\n");
    }
    // debugf("Stack: %p\n", p->stack);
    // debugf("Heap: %p\n", p->heap);
    #ifdef DEBUG_PROCESS
    mmu_print_entries(p->rcb.ptable, MMU_LEVEL_4K);
    #endif

    mutex_unlock(&p->lock);
    process_map_set(p);
    debugf("process.c (process_new): Process created\n");

    process_debug(p);
    return p;
}

int process_free(Process *p)
{
    mutex_spinlock(&p->lock);
    struct ListElem *e;

    if (!p) {
        warnf("process.c (process_free): Process is NULL\n");
        return -1;
    }

    if (ON_HART_NONE(p)) {
        warnf("process.c (process_free): Process is not running on any hart\n");
        return -1;
    }

    // Free all resources allocated to the process.
    rcb_free(&p->rcb);

    kfree(p);
    return 0;
}

bool process_run(Process *p, unsigned int hart)
{
    void process_asm_run(void *frame_addr);
    unsigned int me = sbi_whoami();

    if (me == hart) {
        if (p->state == PS_DEAD) {
            warnf("process.c (process_run): Process is dead, running idle instead\n");
            process_run(sched_get_idle_process(), hart);
        }

        if (!process_map_contains(p->pid)) {
            fatalf("process.c (process_run): Process %d not found on process map\n", p->pid);
        }

        set_current_process(p);
        // process_debug(p);
        // uint64_t satp, sscratch;
        // CSR_READ(satp, "satp");
        // CSR_READ(sscratch, "sscratch");
        // debugf("Old SATP: 0x%08x\n", satp);
        // trap_frame_debug((TrapFrame*)sscratch);
        // debugf("New SATP: 0x%08x\n", p->frame->satp);
        // trap_frame_debug(p->frame);
        debugf("Process %d running on hart %d\n", p->pid, hart);
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
    map_clear(processes);
}

// Store a process on a map as its PID as the key.
void process_map_set(Process *p)
{
    mutex_spinlock(&p->lock);
    infof("process.c (process_map_set): Setting PID %d\n", p->pid);
    map_set_int(processes, p->pid, (MapValue)p);
    mutex_unlock(&p->lock);
}

// void process_map_remove(Process *p)
// {
//     map_remove_int(processes, p->pid);
// }

// Get process stored on the process map using the PID as the key.
Process *process_map_get(uint16_t pid) 
{
    if (!process_map_contains(pid)) {
        return NULL;
    }
    MapValue val;
    map_get_int(processes, pid, &val);
    return (Process *)val;
}

bool process_map_contains(uint16_t pid) 
{
    return map_contains_int(processes, pid);
}

void process_map_remove(uint16_t pid)
{
    map_remove_int(processes, pid);
}

// Keep track of the PIDs running on each hart.
static Map *pid_on_harts;

// Initialize the PID on harts map, needs to be called before creating the
// first process.
void pid_harts_map_init()
{
    // pid_on_harts = map_new_with_slots(0x100);
    pid_on_harts = map_new();
    map_clear(pid_on_harts);
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
