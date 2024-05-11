/**
 * @file main.c
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Contains the startup routines.
 * @version 0.1
 * @date 2022-05-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <config.h>
#include <csr.h>
#include <hart.h>
#include <lock.h>
#include <plic.h>
#include <stdint.h>
#include <printf.h>
#include <rtc.h>
#include <uart.h>

static void plic_init()
{
    #ifdef USE_OLD_PLIC
    // Enable IRQ #10 (UART) to receive on HART #0
    plic_enable(0, 10);
    // Set the priority of IRQ #10 to 7 (highest)
    plic_set_priority(10, 7);
    // Set the wall (threshold) of HART #0 to 0 (lowest)
    plic_set_threshold(0, 0);
    #endif
}

static void rtc_init()
{
    // The RTC will not give any data until we make the first
    // read to it. This awakens the device and lets us hear
    // things from it.
    // in rtc.c
    (void)rtc_get_time();
}

static void pmp_init()
{
    // Top of range address is 0xffff_ffff. Since this is pmpaddr0
    // then the bottom of the range is 0x0.
    CSR_WRITE("pmpaddr0", 0xffffffff >> 2);
    // A = 01 (TOR), X=1, W=1, and R=1
    CSR_WRITE("pmpcfg0", (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0));
}

// in asm/clearbss.S
void clear_bss();

// in asm/start.S
void park();

// in asm/trap.S
void sbi_trap_vector();

// in main.c
static uint64_t find_os_addr(void);

static struct {
    int64_t regs[32];
} SBI_GPREGS[8];

static Mutex hart0lock = MUTEX_LOCKED;

// Entry point for ALL HARTS. We can't return from
// these HARTs since we made tail calls from start.S
ATTR_NAKED_NORET
void main(int hart)
{
    if (hart != 0) {
        // Non bootstrapping (non-0) hart, park it.
        mutex_spinlock(&hart0lock);
        mutex_unlock(&hart0lock);
        pmp_init();

        // We only allow the HARTs we have space for to be awoken.
        if (hart < MAX_ALLOWABLE_HARTS) {
            sbi_hart_data[hart].status         = HS_STOPPED;
            sbi_hart_data[hart].target_address = 0;
            sbi_hart_data[hart].priv_mode      = HPM_MACHINE;
            CSR_WRITE("mscratch", SBI_GPREGS + hart);
            CSR_WRITE("sscratch", hart);
        }
        CSR_WRITE("mepc", park);
        CSR_WRITE("mideleg", 0);
        CSR_WRITE("medeleg", 0);
        CSR_WRITE("mie", MIE_MSIE);
        CSR_WRITE("mstatus", MSTATUS_FS_INITIAL | MSTATUS_MPP_MACHINE | MSTATUS_MPIE);
        CSR_WRITE("mtvec", sbi_trap_vector);
        MRET();
    }

    // If we get here, we are HART #0. The other HARTs are in the
    // if statement above, and they all MRET to the park() function, which is
    // just a WFI loop with software interrupts enabled.
    uint64_t os_addr;
    clear_bss();
    uart_init();
    // Determine the next address to go to based on the target mode. These
    // can be manipulated in src/include/config.h
    switch (OS_TARGET_MODE) {
        case OS_TARGET_MODE_MAGIC: {
            os_addr = find_os_addr();
            if (os_addr == 0) {
                printf("[SBI]: Unable to find OS boot address.\n");
                WFI_LOOP();
            }
        }
        break;
        case OS_TARGET_MODE_JUMP:
            os_addr = OS_TARGET_JUMP_ADDR;
        break;
        default:
            printf("[SBI]: Undefined OS_TARGET_MODE (%d)...hanging.\n", OS_TARGET_MODE);
            WFI_LOOP();
        break;
    }
    // Let the other HARTs go
    mutex_unlock(&hart0lock);

    // in plic.c
    plic_init();
    // in main.c
    pmp_init();
    rtc_init();

    sbi_hart_data[0].status         = HS_STARTED;
    sbi_hart_data[0].target_address = 0;
    sbi_hart_data[0].priv_mode      = HPM_MACHINE;
    // Store trap frame into mscratch register
    CSR_WRITE("mscratch", SBI_GPREGS + hart);
    // Store hart # in sscratch so the OS can hear it.
    CSR_WRITE("sscratch", hart);

    // Set up the control registers to get ready to jump to the OS.
    CSR_WRITE("mepc", os_addr);
    CSR_WRITE("mtvec", sbi_trap_vector);
    CSR_WRITE("mie", MIE_MEIE | MIE_MTIE | MIE_MSIE);
    CSR_WRITE("mideleg", SIP_SEIP | SIP_STIP | SIP_SSIP);
    CSR_WRITE("medeleg", MEDELEG_ALL);
    CSR_WRITE("mstatus", MSTATUS_FS_INITIAL | MSTATUS_MPP_SUPERVISOR | MSTATUS_MPIE);
    MRET();
}

/**
 * @brief Search for the OS target address by first finding the MAGIC.
 *
 * @return uint64_t - the OS's boot address or 0 if the OS couldn't be found.
 */
static uint64_t find_os_addr(void)
{
    uint64_t s;
    struct os_target *t;
    // Search for the OS_TARGET_MAGIC. When we find it, the target address
    // is right after it.
    for (s = OS_TARGET_START; s <= OS_TARGET_END; s += OS_TARGET_ALIGNMENT) {
        t = (struct os_target *)s;
        if (t->magic == OS_TARGET_MAGIC) {
            return t->target_address;
        }
    }
    return 0;
}
