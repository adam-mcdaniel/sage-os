#include <config.h>
#include <csr.h>
#include <debug.h>
#include <errno.h>
#include <input.h>
#include <kmalloc.h>
#include <lock.h>
#include <mmu.h>
#include <process.h>
#include <sbi.h>
#include <sched.h>
#include <stdint.h>
#include <util.h>
#include <virtio.h>

// #define SYSCALL_DEBUG
#ifdef SYSCALL_DEBUG
#define debugf(...) debugf(__VA_ARGS__)
#else
#define debugf(...)
#endif

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
    infof("syscall.c (exit): Exiting process %d on hart %d\n", pid_harts_map_get(hart), hart);
    // Get the current process running on hart
    Process *p = process_map_get(pid_harts_map_get(hart));

    p->state = PS_DEAD;
    
    if (!p) {
        debugf("syscall.c (exit): Null process on hart %d", hart);
    }
    
    debugf("syscall.c (exit) Exiting PID %d\n", p->pid);

    // Free process
    // if (process_free(p)) 
    //     fatalf("syscall.c (exit): process_free failed\n");
}

SYSCALL(putchar)
{
    SYSCALL_ENTER();

    debugf("syscall.c (putchar) with args: %lx %lx %lx %lx %lx %lx\n", XREG(A0), XREG(A1), XREG(A2), XREG(A3), XREG(A4), XREG(A5));
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
    
    uint16_t pid = pid_harts_map_get(hart);
    Process *p = sched_get_current();
    if (!process_map_contains(pid)) {
        // fatalf("syscall.c (sleep): Process %d not found on hart %d\n", pid, hart);
        process_debug(p);
        fatalf("syscall.c (sleep): Process %d not found on hart %d\n", pid, hart);
    } else {
        debugf("syscall.c (sleep): Process %d found on hart %d\n", pid, hart);
    }

    if (p->pid != pid) {
        process_debug(p);
        fatalf("syscall.c (sleep): Process %d not found on hart %d\n", pid, hart);
    } else {
        debugf("syscall.c (sleep): Process %d found on hart %d\n", pid, hart);
    }
    
    if (!p) {
        fatalf("syscall.c (sleep): Null process on hart %d", p->hart);
    }
    // // Sleep the process. VIRT_TIMER_FREQ is 10MHz, divided by 1000, we get 10KHz

    /*
    // Commented out until we implement an IO buffer for the process
    // infof("syscall.c (sleep) Sleeping PID %d at %d until %d\n", p->pid, sbi_get_time(), sbi_get_time() + XREG(A0) * VIRT_TIMER_FREQ / 1000);
    p->sleep_until = sbi_get_time() + XREG(A0) * VIRT_TIMER_FREQ / 1000;
    p->state = PS_SLEEPING;
    */
}

SYSCALL(events)
{
    SYSCALL_ENTER();
    
    uint16_t pid = pid_harts_map_get(hart);
    Process *p = sched_get_current();
    
    if (!process_map_contains(pid)) {
        // fatalf("syscall.c (sleep): Process %d not found on hart %d\n", pid, hart);
        process_debug(p);
        fatalf("syscall.c (events): Process %d not found on hart %d\n", pid, hart);
    } else {
        debugf("syscall.c (events): Process %d found on hart %d\n", pid, hart);
    }

    if (p->pid != pid) {
        process_debug(p);
        fatalf("syscall.c (events): Process %d not found on hart %d\n", pid, hart);
    } else {
        debugf("syscall.c (events): Process %d found on hart %d\n", pid, hart);
    }
    
    if (!p) {
        fatalf("syscall.c (events): Null process on hart %d", p->hart);
    }

    InputDevice *keyboard = input_device_get_keyboard();
    VirtioInputEvent *event;

    mutex_spinlock(&keyboard->lock);
    
    uintptr_t event_buffer = scratch[XREG_A0];
    unsigned max_events = scratch[XREG_A1];

    uint64_t i;
    for (i = 0; i < max_events && i < keyboard->buffer_count; ++i) {
        uintptr_t phys_start = mmu_translate(p->rcb.ptable, event_buffer + i * 8);
        uintptr_t phys_end = mmu_translate(p->rcb.ptable, event_buffer + (i + 1) * 8 - 1);
        
        if (phys_start == -1UL && phys_end == -1UL) {
            debugf("syscall.c (events): MMU translated to null\n");
            break;
        }

        if (!input_device_buffer_pop(keyboard, event)) {
            debugf("syscall.c (events): Couldn't get an event from the input device buffer\n\n");
            break;
        }
        
        memcpy((void *)phys_start, event, 8);
        kfree(event);
    }

    mutex_unlock(&keyboard->lock);
    scratch[XREG_A0] = i;
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
        warnf("Invalid syscall %ld\n", XREG(A7));
        XREG(A0) = -EINVAL;
    }
    else {
        // debugf("syscall_handle: Calling syscall %ld\n", XREG(A7));
        // debugf("With args: %lx %lx %lx %lx %lx %lx\n", XREG(A0), XREG(A1), XREG(A2), XREG(A3), XREG(A4), XREG(A5));
        SYSCALL_EXEC(XREG(A7));
    }
}
