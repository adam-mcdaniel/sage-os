/**
 * @file ctrap.c
 * @author Stephen Marz (sgm@utk.edu)
 * @brief High-level (C) Trap Routines
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <clint.h>
#include <csr.h>
#include <hart.h>
#include <plic.h>
#include <printf.h>
#include <stdbool.h>
#include <svcall.h>

/**
 * @brief The trap handler for the SBI only handles a few things
 * at the machine level. Everything else should be delegated to the OS.
 *
 */
void c_trap_handler(void)
{
    unsigned long mcause;
    unsigned int mhartid;
    long *mscratch;
    unsigned long mepc;
    unsigned long sip;

    // Read the control and status registers
    CSR_READ(mcause, "mcause");
    CSR_READ(mhartid, "mhartid");
    CSR_READ(mscratch, "mscratch");
    CSR_READ(mepc, "mepc");

    if (MCAUSE_IS_ASYNC(mcause)) {
        // If we get here, the trap is an interrupt (asynchronous)
        switch (MCAUSE_NUM(mcause)) {
            case CAUSE_MSIP:
                // machine software interrupt pending (MSIP)
                hart_handle_msip(mhartid);
                break;
            case CAUSE_MTIP: {  // MTIP
                // Delegate timer interrupt pending (TIP) to supervisor mode (STIP)
                CSR_READ(sip, "mip");
                CSR_WRITE("mip", SIP_STIP);
                clint_set_mtimecmp(mhartid, CLINT_MTIMECMP_INFINITE);
            } break;
            case CAUSE_MEIP:
                // machine external interrupt (MEI)
                // The PLIC told us an external device interrupted, so go out
                // and check it.
                #ifdef USE_OLD_PLIC
                plic_handle_irq(mhartid);
                #endif
                break;
            default:
                // This is not a cause we can handle.
                printf("[SBI]: Unhandled Asynchronous interrupt %ld\n", mcause);
                break;
        }
    }
    else { 
        // If we get here, the trap is an exception (synchronous)
        switch (MCAUSE_NUM(mcause)) {
            case CAUSE_ECALL_S_MODE:
                // ECALL from S-mode (from the OS)
                svcall_handle(mhartid);
                // We have to move beyond the ECALL instruction, which is exactly 4 bytes.
                CSR_WRITE("mepc", mepc + 4);
                break;
            default:
                printf("[SBI]: Unhandled Synchronous interrupt %ld. Hanging hart %d\n", mcause,
                       mhartid);
                WFI_LOOP();
                break;
        }
    }
}
