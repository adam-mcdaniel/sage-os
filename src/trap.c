#include <config.h>
#include <csr.h>
#include <plic.h>
#include <debug.h>
#include <sbi.h>
#include <sched.h>
#include <syscall.h>
#include <compiler.h>
#include <trap.h>
#include <sbi.h>
#include <process.h>
#include <sched.h>

// #define TRAP_DEBUG
#ifdef TRAP_DEBUG
#define debugf(...) debugf(__VA_ARGS__)
#else
#define debugf(...)
#endif

// From src/syscall.c
void syscall_handle(int hart, uint64_t epc, int64_t *scratch);


static uint64_t scheduler_time = 0;

// Called from asm/spawn.S: _spawn_kthread
void os_trap_handler(void)
{
    // debugf("Entering OS trap handler\n");
    // SFENCE_ALL();

    unsigned long cause;
    long *scratch;
    unsigned long epc;
    unsigned long tval;
    unsigned long sie;
    unsigned long sstatus;
    CSR_READ(cause, "scause");
    CSR_READ(scratch, "sscratch");
    CSR_READ(epc, "sepc");
    CSR_READ(tval, "stval");
    CSR_READ(sie, "sie");
    CSR_READ(sstatus, "sstatus");
    CSR_CLEAR("sie");
    CSR_CLEAR("sip");

    Process *p;
    TrapFrame *frame = (TrapFrame*)scratch;


    // debugf("os_trap_handler: Trap frame @ %p\n", frame);
    // debugf("os_trap_handler: old sepc: %p\n", frame->sepc);
    // debugf("sstatus: %lx\n", sstatus);
    // // Process *p = sched_get_current();
    // // debugf("os_trap_handler: current process: %d\n", p->pid);
    

    // if (sstatus & frame->sstatus & SSTATUS_SPP_SUPERVISOR) {
    //     // debugf("sstatus=%lx, frame->sstatus=%lx\n", sstatus, frame->sstatus);
    //     frame->sepc = epc;
    // } else if (~sstatus & ~frame->sstatus & SSTATUS_SPP_SUPERVISOR) {
    //     // If the bit is not set, we are in user mode
    //     // debugf("sstatus=%lx, frame->sstatus=%lx\n", sstatus, frame->sstatus);
    //     frame->sepc = epc;
    // }
    // debugf("os_trap_handler: new sepc: %p\n", frame->sepc);


    // CSR_WRITE("sscratch", kernel_trap_frame);

    // __asm__ volatile ("savegp");
    // __asm__ volatile ("savefp");
    // __asm__ volatile ("csrw sscratch, t0");

    // unsigned long status;
    // CSR_READ(status, "sstatus");
    // status |= SSTATUS_SPP_BIT;
    // CSR_WRITE("sstatus", status);
    // CSR_CLEAR("sie");
    // CSR_CLEAR("sip");


    // debugf("SPP: %lx\n", status & SSTATUS_SPP_BIT);
    // debugf("Scause: %lx\n", cause);
    // debugf("Sscratch: %lx\n", scratch);

    int hart = sbi_whoami();
    // IRQ_ON();
    // CSR_WRITE("sscratch", hart);
    // __asm__ volatile ("li t1, 2\n"
    //                 "csrc sip, t2\n");
    // __asm__ volatile ("li t1,2\n"
    //                 "csrs sstatus, t1\n"
    //                 "csrs sie, t1\n");

    // debugf("Is async: %d\n", SCAUSE_IS_ASYNC(cause));
    uint64_t now = sbi_get_time();

    if (SCAUSE_IS_ASYNC(cause)) {
        // debugf("os_trap_handler: Is async!\n");
        cause = SCAUSE_NUM(cause);
        frame->sepc = epc;
        TrapFrame save = *frame;
        switch (cause) {
            case CAUSE_SSIP:
                infof("os_trap_handerl: Supervisor software interrupt!\n");
                // TODO
                break;
            case CAUSE_STIP:
                // Ack timer will reset the timer to INFINITE
                // In src/sbi.c
                debugf("os_trap_handler: Supervisor timer interrupt!\n");
                sbi_ack_timer();
                debugf("Timer: old sepc: %p\n", frame->sepc);
                debugf("Timer: new sepc: %p\n", frame->sepc);
                // We typically invoke our scheduler if we get a timer
                CSR_WRITE("sscratch", &save);
                // CSR_WRITE("sscratch", kernel_trap_frame);
                // sched_handle_timer_interrupt(hart);
                if (now - scheduler_time > CONTEXT_SWITCH_TIMER) {
                    debugf("Process %d is running. Resuming process\n", p->pid);
                    process_run(sched_get_current(), hart);
                } else {
                    debugf("Process %d is running. Scheduling next process\n", p->pid);
                    scheduler_time = now;
                    sched_handle_timer_interrupt(hart);
                }
                break;
            case CAUSE_SEIP:
                debugf("os_trap_handler: Supervisor external interrupt!\n");
                // Forward to src/plic.c
                // if (sstatus & SSTATUS_SPP_SUPERVISOR) {
                //     debugf("os_trap_handler: SPP is set\n");
                // } else {
                //     debugf("os_trap_handler: SPP is not set\n");
                //     frame->sepc = epc;
                // }
                CSR_WRITE("sscratch", &save);
                IRQ_OFF();
                plic_handle_irq(hart);
                IRQ_OFF();

                // CSR_WRITE("sscratch", frame);


                if (!(frame->sstatus & SSTATUS_SPP_SUPERVISOR) && sched_get_current() != NULL) {
                    process_run(sched_get_current(), hart);
                    // if (now - scheduler_time < CONTEXT_SWITCH_TIMER) {
                    //     debugf("Process %d is running. Resuming process\n", p->pid);
                    //     process_run(p, hart);
                    // } else {
                    //     scheduler_time = now;
                    //     sched_handle_timer_interrupt(hart);
                    // }
                }
                // p = sched_get_current();
                // // If there is a current process, we should schedule it
                // if (p != NULL) {
                //     debugf("Resuming process %d\n", p->pid);
                //     process_debug(p);
                //     process_run(p, hart);
                // // } else {
                //     // debugf("No current process. Scheduling next process\n");
                //     // sched_handle_timer_interrupt(hart);
                // } else {
                //     debugf("No current process. Scheduling next process\n");
                // }
                // WFI();
                // sched_handle_timer_interrupt(hart);
                break;
            default:
                debugf("ERROR!!!\n");
                trap_frame_debug(scratch);
                fatalf("os_trap_handler: Unhandled Asynchronous interrupt %ld\n", cause);
                WFI_LOOP();
                break;
        }
    } else {
        // debugf("Is sync!\n");
        if (cause != CAUSE_ECALL_S_MODE && cause != CAUSE_ECALL_U_MODE) {
            debugf("ERROR!!!\n");
            trap_frame_debug(scratch);
        }
        // if (sstatus & SSTATUS_SPP_SUPERVISOR) {
        //     debugf("os_trap_handler: SPP is set\n");
        // } else {
        //     debugf("os_trap_handler: SPP is not set\n");
        // }
        frame->sepc = epc + 4;
        TrapFrame save = *kernel_trap_frame;
        switch (cause) {
            case CAUSE_ECALL_U_MODE:  // ECALL U-Mode
                // Forward to src/syscall.c
                // debugf("Handling syscall\n");
                // trap_frame_debug(scratch);

                CSR_WRITE("sscratch", &save);
                IRQ_OFF();
                syscall_handle(hart, epc, scratch);
                // Get the process
                p = sched_get_current();
                // if (now - time_since_scheduler > CONTEXT_SWITCH_TIMER || p == sched_get_idle_process() || p->state != PS_RUNNING) {
                //     debugf("Context switch timer expired. Scheduling next process\n");
                //     sched_handle_timer_interrupt(hart);
                // } else {
                //     debugf("Context switch timer not expired. Resuming process\n");
                //     process_run(p, hart);
                // }
                // return;

                switch (p->state) {
                case PS_RUNNING:
                    // sched_handle_timer_interrupt(hart);
                    if (p == sched_get_idle_process()) {
                        debugf("Process %d is idle. Scheduling next process\n", p->pid);
                        scheduler_time = now;
                        sched_handle_timer_interrupt(hart);
                    } else if (now - scheduler_time > CONTEXT_SWITCH_TIMER) {
                        debugf("Process %d is running. Resuming process\n", p->pid);
                        process_run(p, hart);
                    } else {
                        debugf("Process %d is running. Scheduling next process\n", p->pid);
                        scheduler_time = now;
                        sched_handle_timer_interrupt(hart);
                    }
                    return;
                case PS_SLEEPING:
                    debugf("Process %d is sleeping. Scheduling next process\n", p->pid);
                    sched_handle_timer_interrupt(hart);
                case PS_WAITING:
                    debugf("Process %d is waiting. Scheduling next process\n", p->pid);
                    sched_handle_timer_interrupt(hart);
                case PS_DEAD:
                    debugf("Process %d is dead. Scheduling next process\n", p->pid);
                    sched_handle_timer_interrupt(hart);
                default:
                    fatalf("Unknown process state %d\n", p->state);
                }

                // We have to move beyond the ECALL instruction, which is exactly 4 bytes.
                break;
            case CAUSE_ECALL_S_MODE:  // ECALL U-Mode
                // Forward to src/syscall.c
                // debugf("Handling supervisor syscall\n");
                syscall_handle(hart, epc, scratch);
                // We have to move beyond the ECALL instruction, which is exactly 4 bytes.
                break;
            case CAUSE_ILLEGAL_INSTRUCTION:
                fatalf("Illegal instruction \"%x\" at %p\n", *((uint64_t*)epc), epc);
                // CSR_WRITE("sepc", epc + 4);
                break;
            case CAUSE_INSTRUCTION_ACCESS_FAULT:
                fatalf("Couldn't access instruction=%p at instruction %p\n", tval, epc);
                break;
            case CAUSE_INSTRUCTION_PAGE_FAULT:
                fatalf("Instruction page fault at instruction %p accessing address %p\n", epc, tval);
                break;
            case CAUSE_STORE_AMO_PAGE_FAULT:
                fatalf("Instruction store page fault at instruction %p accessing address %p\n", epc, tval);
                break;
            case CAUSE_LOAD_PAGE_FAULT:
                fatalf("Load page fault at %p = %p", epc, tval);
                break;
            default:
                fatalf(
                    "Unhandled Synchronous interrupt %ld @ 0x%08lx [0x%08lx]. "
                    "Hanging hart %d\n",
                    cause, epc, tval, hart);
                WFI_LOOP();
                break;
        }
        // CSR_WRITE("sepc", epc + 4);
    }
    // CSR_WRITE("sepc", frame->sepc);
    CSR_WRITE("sie", sie);

    // debugf("Jumping to %p...\n", epc + 4);
    // CSR_WRITE("pc", epc + 4);
    // CSR_WRITE("sscratch", scratch);
    // CSR_WRITE("sie", sie);
    // Swap t0 into sscratch
    // __asm__ volatile ("csrr t0, sscratch");
    // __asm__ volatile ("savefp");
    // __asm__ volatile ("savegp");

    // SRET();
    // debugf("Leaving OS trap handler\n");
    // fatalf("Could not return from trap\n");
}