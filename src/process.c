#include <csr.h>
#include <kmalloc.h>
#include <list.h>
#include <mmu.h>
#include <page.h>
#include <process.h>
#include <sbi.h>
#include <vector.h>

extern const unsigned long trampoline_thread_start;
extern const unsigned long trampoline_trap_start;

#define STACK_PAGES 2
#define STACK_SIZE  (STACK_PAGES * PAGE_SIZE)
#define STACK_TOP   0xfffffffc0ffee000UL

struct process *process_new(process_mode mode)
{
    struct process *p       = (struct process *)kzalloc(sizeof(*p));

    p->hart                 = -1U;
    p->ptable               = mmu_table_create();
    p->state                = PS_WAITING;
    // p->pid               = fill_in_with_unique_pid

    // Set the trap frame and create all necessary structures.
    // p->frame.sepc = filled_in_by_ELF_loader
    p->frame.sstatus        = SSTATUS_SPP_BOOL(mode) | SSTATUS_FS_INITIAL | SSTATUS_SPIE;
    p->frame.sie            = SIE_SEIE | SIE_SSIE | SIE_STIE;
    p->frame.satp           = SATP(p->ptable, p->pid);
    p->frame.sscratch       = (unsigned long)&p->frame;
    p->frame.stvec          = trampoline_trap_start;
    p->frame.trap_satp      = SATP_KERNEL;
    // p->frame.trap_stack = filled_in_by_SCHEDULER

    p->fds = vector_new_with_capacity(5);
    p->pages = list_new();

    // We need to keep track of the stack itself in the kernel, so we can free it
    // later, but the user process will interact with the stack via the SP register.
    p->frame.xregs[XREG_SP] = STACK_TOP + STACK_SIZE;
    for (unsigned long i = 0; i < STACK_PAGES; i += 1) {
        void *stack = page_zalloc();
        list_add_ptr(p->pages, stack);
        mmu_map(p->ptable, STACK_TOP + PAGE_SIZE * i, (unsigned long)stack,
                MMU_LEVEL_4K, mode == PM_USER ? PB_USER : 0 | PB_READ | PB_WRITE);
    }

    // We need to map certain kernel portions into the user's page table. Notice
    // that the PB_USER is NOT set, but it needs to be there because we need to execute
    // the trap/start instructions while using the user's page table until we change SATP.
    unsigned long trans_trampoline_start = mmu_translate(kernel_mmu_table, trampoline_thread_start);
    unsigned long trans_trampoline_trap  = mmu_translate(kernel_mmu_table, trampoline_trap_start);
    mmu_map(p->ptable, trampoline_thread_start, trans_trampoline_start, MMU_LEVEL_4K,
            PB_READ | PB_EXECUTE);
    mmu_map(p->ptable, trampoline_trap_start, trans_trampoline_trap, MMU_LEVEL_4K,
            PB_READ | PB_EXECUTE);

    SFENCE_ASID(p->pid);

    return p;
}

int process_free(struct process *p)
{
    struct ListElem *e;
    unsigned int i;

    if (!p || !ON_HART_NONE(p)) {
        // Process is invalid or running somewhere, or this is stale.
        return -1;
    }

    // Free all resources allocated to the process.

    if (p->ptable) {
        mmu_free(p->ptable);
        SFENCE_ASID(p->pid);
    }

    if (p->pages) {
        list_for_each(p->pages, e) {
            page_free(list_elem_value_ptr(e));
        }
        list_free(p->pages);
    }

    if (p->fds) {
        for (i = 0;i < vector_size(p->fds);i += 1) {
            // Clean up any file descriptor stuff here.
        }
        vector_free(p->fds);
    }
    

    kfree(p);

    return 0;
}

bool process_run(struct process *p, unsigned int hart)
{
    void process_asm_run(void *frame_addr);
    unsigned int me = sbi_whoami();

    if (me == hart) {
        process_asm_run(&p->frame);
        // process_asm_run should not return, but if it does
        // something went wrong.
        return false;
    }

    return sbi_hart_start(hart, trampoline_thread_start, (unsigned long)&p->frame, p->frame.satp);
}
