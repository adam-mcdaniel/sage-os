#include <csr.h>
#include <errno.h>
#include <sbi.h>
#include <stdint.h>
#include <util.h>
#include <debug.h>
#include <process.h>

#define XREG(x)             (scratch[XREG_##x])
#define SYSCALL_RETURN_TYPE void
#define SYSCALL_PARAM_LIST  int hart, uint64_t epc, int64_t *scratch
#define SYSCALL(t)          static SYSCALL_RETURN_TYPE syscall_##t(SYSCALL_PARAM_LIST)
#define SYSCALL_PTR(t)      syscall_##t
#define SYSCALL_EXEC(x)     SYSCALLS[(x)](hart, epc, scratch)
#define SYSCALL_ENTER() \
    (void)hart;         \
    (void)epc;          \
    (void)scratch

SYSCALL(exit)
{
    SYSCALL_ENTER();
    // Kill the current process on this HART and schedule the next
    // one.
    debugf("HELLO\n");
    
    // Get the current process running on hart
    Process *p = process_map_get(pid_harts_map_get(hart));
    
    // Kill process
    // ...

    // Free process
    if (process_free(p)) 
        fatalf("syscall.c (exit): process_free failed\n");
}

SYSCALL(putchar)
{
    SYSCALL_ENTER();
    sbi_putchar(XREG(A0));
}

SYSCALL(getchar)
{
    SYSCALL_ENTER();
    XREG(A0) = sbi_getchar();
}

SYSCALL(yield)
{
    SYSCALL_ENTER();
    // sched_invoke(hart);
}

SYSCALL(sleep)
{
    SYSCALL_ENTER();
    // Sleep the process. VIRT_TIMER_FREQ is 10MHz, divided by 1000, we get 10KHz
    //     p->sleep_until = sbi_get_time() + XREG(A0) * VIRT_TIMER_FREQ / 1000;
    //     p->state = PS_SLEEPING;
}

SYSCALL(events)
{
    SYSCALL_ENTER();
    
}

/**
    SYS_EXIT = 0,
    SYS_PUTCHAR,
    SYS_GETCHAR,
    SYS_YIELD,
    SYS_SLEEP,
    SYS_OLD_GET_EVENTS,
    SYS_OLD_GET_FB,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_READ,
    SYS_WRITE,
    SYS_STAT,
    SYS_SEEK,
    SYS_SBRK
*/
// These syscall numbers MUST match the user/libc numbers!
static SYSCALL_RETURN_TYPE (*const SYSCALLS[])(SYSCALL_PARAM_LIST) = {
    SYSCALL_PTR(exit),    /* 0 */
    SYSCALL_PTR(putchar), /* 1 */
    SYSCALL_PTR(getchar), /* 2 */
    SYSCALL_PTR(yield),   /* 3 */
    SYSCALL_PTR(sleep),   /* 4 */
    SYSCALL_PTR(events),  /* 5 */
};

static const int NUM_SYSCALLS = sizeof(SYSCALLS) / sizeof(SYSCALLS[0]);

// We get here from the trap.c if this is an ECALL from U-MODE
void syscall_handle(int hart, uint64_t epc, int64_t *scratch)
{
    // Sched invoke will save sepc, so we want it to resume
    // 4 bytes ahead, which will be the next instruction.
    CSR_WRITE("sepc", epc + 4);

    if (XREG(A7) >= NUM_SYSCALLS || SYSCALLS[XREG(A7)] == NULL) {
        // Invalid syscall
        XREG(A0) = -EINVAL;
    }
    else {
        SYSCALL_EXEC(XREG(A7));
    }
}
