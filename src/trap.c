#include <config.h>
#include <csr.h>
#include <plic.h>
#include <debug.h>
#include <sbi.h>
#include <sched.h>
#include <syscall.h>
#include <compiler.h>
#include <trap.h>

// From src/syscall.c
void syscall_handle(int hart, uint64_t epc, int64_t *scratch);

// Called from asm/spawn.S: _spawn_trap
void os_trap_handler(void)
{
    unsigned long cause;
    long *scratch;
    unsigned long epc;
    unsigned long tval;
    CSR_READ(cause, "scause");
    CSR_READ(scratch, "sscratch");
    CSR_READ(epc, "sepc");
    CSR_READ(tval, "stval");
    
    int hart = sbi_whoami();
    debugf("TRAP at instruction %p\n", epc);
    // WFI_LOOP();

    if (SCAUSE_IS_ASYNC(cause)) {
        debugf("Is async!\n");
        cause = SCAUSE_NUM(cause);
        switch (cause) {
            case CAUSE_STIP:
                // Ack timer will reset the timer to INFINITE
                // In src/sbi.c
                sbi_ack_timer();
                // We typically invoke our scheduler if we get a timer
                // sched_invoke(hart);
                break;
            case CAUSE_SEIP:
                // Forward to src/plic.c
                plic_handle_irq(hart);
                CSR_WRITE("sepc", epc + 4);
                SRET();
                fatalf("Could not return from trap\n");
                break;
            default:
                debugf("Unhandled Asynchronous interrupt %ld\n", cause);
                break;
        }
    }
    else {
        debugf("Is sync!\n");
        switch (cause) {
            case CAUSE_ECALL_U_MODE:  // ECALL U-Mode
                // Forward to src/syscall.c
                syscall_handle(hart, epc, scratch);
                // We have to move beyond the ECALL instruction, which is exactly 4 bytes.
                // CSR_WRITE("sepc", epc + 4);
                SRET();
                break;
            default:
                debugf(
                    "Unhandled Synchronous interrupt %ld @ 0x%08lx [0x%08lx]. "
                    "Hanging hart %d\n",
                    cause, epc, tval, hart);
                WFI_LOOP();
                break;
        }
    }
    // debugf("Jumping to %p...\n", epc + 4);
    // CSR_WRITE("pc", epc + 4);
    // debugf("Leaving trap handler!\n");
}
