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
#include <sched.h>

// #define TRAP_DEBUG

#ifdef TRAP_DEBUG
#define debugf(...) debugf(__VA_ARGS__)
#else
#define debugf(...)
#endif

// From src/syscall.c
void syscall_handle(int hart, uint64_t epc, int64_t *scratch);

// Called from asm/spawn.S: _spawn_kthread
void os_trap_handler(void)
{
    debugf("Entering OS trap handler\n");
    unsigned long cause;
    long *scratch;
    unsigned long epc;
    unsigned long tval;
    unsigned long sie;
    CSR_READ(cause, "scause");
    CSR_READ(scratch, "sscratch");
    CSR_READ(epc, "sepc");
    CSR_READ(tval, "stval");
    CSR_READ(sie, "sie");

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
    // CSR_WRITE("sscratch", hart);
    // __asm__ volatile ("li t1, 2\n"
    //                 "csrc sip, t2\n");
    // __asm__ volatile ("li t1,2\n"
    //                 "csrs sstatus, t1\n"
    //                 "csrs sie, t1\n");


    debugf("Is async: %d\n", SCAUSE_IS_ASYNC(cause));

    if (SCAUSE_IS_ASYNC(cause)) {
        debugf("os_trap_handler: Is async!\n");
        cause = SCAUSE_NUM(cause);
        switch (cause) {
            case CAUSE_SSIP:
                debugf("os_trap_handerl: Supervisor software interrupt!\n");
                // TODO
                break;
            case CAUSE_STIP:
                // Ack timer will reset the timer to INFINITE
                // In src/sbi.c
                debugf("os_trap_handler: Supervisor timer interrupt!\n");
                // CSR_CLEAR("sip");
                sbi_ack_timer();
                // We typically invoke our scheduler if we get a timer
                sched_handle_timer_interrupt(hart);
                break;
            case CAUSE_SEIP:
                debugf("os_trap_handler: Supervisor external interrupt!\n");
                // Forward to src/plic.c
                plic_handle_irq(hart);
                break;
            default:
                fatalf("os_trap_handler: Unhandled Asynchronous interrupt %ld\n", cause);
                WFI_LOOP();
                break;
        }
    } else {
        debugf("Is sync!\n");
        switch (cause) {
            case CAUSE_ILLEGAL_INSTRUCTION:
                fatalf("Illegal instruction \"%x\" at %p\n", *((uint64_t*)epc), epc);
                CSR_WRITE("sepc", epc + 4);
                break;
            case CAUSE_INSTRUCTION_ACCESS_FAULT:
                fatalf("Couldn't access instruction=%p at instruction %p\n", tval, epc);
                break;
            case CAUSE_INSTRUCTION_PAGE_FAULT:
                fatalf("Instruction page fault at instruction %p accessing address %p\n", epc, tval);
                break;
            case CAUSE_LOAD_PAGE_FAULT:
                fatalf("Load page fault at %p = %p", epc, tval);
                break;
            case CAUSE_ECALL_U_MODE:  // ECALL U-Mode
                // Forward to src/syscall.c
                debugf("Handling syscall\n");
                syscall_handle(hart, epc, scratch);
                // We have to move beyond the ECALL instruction, which is exactly 4 bytes.
                break;
            case CAUSE_ECALL_S_MODE:  // ECALL U-Mode
                // Forward to src/syscall.c
                debugf("Handling syscall\n");
                syscall_handle(hart, epc, scratch);
                // We have to move beyond the ECALL instruction, which is exactly 4 bytes.
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
    // debugf("Jumping to %p...\n", epc + 4);
    // CSR_WRITE("pc", epc + 4);
    // CSR_WRITE("sscratch", scratch);
    // CSR_WRITE("sie", sie);
    // Swap t0 into sscratch
    // __asm__ volatile ("csrr t0, sscratch");
    // __asm__ volatile ("savefp");
    // __asm__ volatile ("savegp");

    // SRET();
    debugf("Leaving OS trap handler\n");
    // fatalf("Could not return from trap\n");
}