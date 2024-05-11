/**
 * @file svcall.c
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Supervisor call handler
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <clint.h>
#include <csr.h>
#include <hart.h>
#include <printf.h>
#include <rtc.h>
#include <svcall.h>
#include <uart.h>

// Called from asm/trap.S
void svcall_handle(int hart)
{
    long *mscratch;
    CSR_READ(mscratch, "mscratch");
    // The A7 register contains the system call numbers
    // Parameters are in: A0, A1, A2, A3, A4, A5, and A6
    // Return register is A0
    switch (mscratch[XREG_A7]) {
        case SBI_SVCALL_HART_STATUS:
            mscratch[XREG_A0] = hart_get_status(mscratch[XREG_A0]);
            break;
        case SBI_SVCALL_HART_START:
            mscratch[XREG_A0] = hart_start(mscratch[XREG_A0], mscratch[XREG_A1], mscratch[XREG_A2], mscratch[XREG_A3]);
            break;
        case SBI_SVCALL_HART_STOP:
            mscratch[XREG_A0] = hart_stop(hart);
            break;
        case SBI_SVCALL_GET_TIME:
            mscratch[XREG_A0] = clint_get_time();
            break;
        case SBI_SVCALL_SET_TIMECMP:
            clint_set_mtimecmp(mscratch[XREG_A0], mscratch[XREG_A1]);
            break;
        case SBI_SVCALL_ADD_TIMECMP:
            clint_add_mtimecmp(mscratch[XREG_A0], mscratch[XREG_A1]);
            break;
        case SBI_SVCALL_ACK_TIMER: {
            unsigned long mip;
            CSR_READ(mip, "mip");
            CSR_WRITE("mip", mip & ~SIP_STIP);
        } break;
        case SBI_SVCALL_RTC_GET_TIME:
            mscratch[XREG_A0] = rtc_get_time();
            break;
        case SBI_SVCALL_PUTCHAR:
            uart_put(mscratch[XREG_A0]);
            break;
        case SBI_SVCALL_WHOAMI:
            mscratch[XREG_A0] = hart;
            break;
        case SBI_SVCALL_GETCHAR:
            // Since the UART interrupts us, we push all of the UART data onto a ring.
            mscratch[XREG_A0] = uart_ring_pop();
            break;
        case SBI_SVCALL_POWEROFF:
            // Turns off QEMU. This is an exit() call in QEMU, so this should not really be used.
            // All we do is write the magic key 0x5555 into the test device at 0x10_0000.
            *((unsigned short *)0x100000) = 0x5555;
            break;
        default:
            printf("[SBI]: Unknown supervisor call '%d' on hart %d\n", mscratch[XREG_A7], hart);
            break;
    }
}
