#include <config.h>
#include <csr.h>
#include <plic.h>
#include <debug.h>
#include <sbi.h>
#include <sched.h>
#include <syscall.h>
#include <compiler.h>

// From src/syscall.c
void syscall_handle(int hart, uint64_t epc, int64_t *scratch);

// Called from asm/spawn.S: _spawn_trap
void c_trap_handler(void)
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

    if (SCAUSE_IS_ASYNC(cause)) {
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
                break;
            default:
                debugf("Unhandled Asynchronous interrupt %ld\n", cause);
                break;
        }
    }
    else {
        switch (cause) {
            case CAUSE_ECALL_U_MODE:  // ECALL U-Mode
                // Forward to src/syscall.c
                syscall_handle(hart, epc, scratch);
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
}
