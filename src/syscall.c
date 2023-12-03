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
#include <gpu.h>

#define SYSCALL_DEBUG
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

    // debugf("syscall.c (putchar) with args: %lx %lx %lx %lx %lx %lx\n", XREG(A0), XREG(A1), XREG(A2), XREG(A3), XREG(A4), XREG(A5));
    sbi_putchar(XREG(A0));
}

SYSCALL(getchar)
{
    SYSCALL_ENTER();
    // debugf("syscall.c (getchar) with args: %lx %lx %lx %lx %lx %lx\n", XREG(A0), XREG(A1), XREG(A2), XREG(A3), XREG(A4), XREG(A5));
    XREG(A0) = sbi_getchar();
}

SYSCALL(get_env)
{
    SYSCALL_ENTER();
    debugf("syscall.c (get_env) with args: %lx %lx %lx %lx %lx %lx\n", XREG(A0), XREG(A1), XREG(A2), XREG(A3), XREG(A4), XREG(A5));

    const char *var_vaddr = (const char *)XREG(A0);
    Process *p = sched_get_current();

    const char *var_paddr = mmu_translate(p->rcb.ptable, (uintptr_t)var_vaddr);
    if (var_paddr == -1UL) {
        XREG(A0) = -EFAULT;
        return;
    }

    debugf("syscall.c (get_env): Getting env var %s\n", (char *)var_paddr);
    
    // Get the value pointer from the process
    const char *value = process_get_env(p, (char *)var_paddr);
    if (!value) {
        debugf("syscall.c (get_env): Env var %s not found\n", (char *)var_paddr);
        XREG(A0) = -ENOENT;
        return;
    }
    debugf("syscall.c (get_env): Got env var %s\n", value);

    // Copy the value to the user address
    uintptr_t value_vaddr = XREG(A1);
    // debugf("syscall.c (get_env): Copying env var %s to %lx\n", value, value_vaddr);
    uintptr_t value_paddr = mmu_translate(p->rcb.ptable, value_vaddr);
    if (value_paddr == -1UL) {
        XREG(A0) = -EFAULT;
        return;
    }
    // Copy the value to the user address
    memcpy((void *)value_paddr, value, strlen(value) + 1);
}

SYSCALL(put_env)
{
    SYSCALL_ENTER();
    debugf("syscall.c (put_env) with args: %lx %lx %lx %lx %lx %lx\n", XREG(A0), XREG(A1), XREG(A2), XREG(A3), XREG(A4), XREG(A5));

    const char *var_vaddr = (const char *)XREG(A0);
    Process *p = sched_get_current();

    const char *var_paddr = mmu_translate(p->rcb.ptable, (uintptr_t)var_vaddr);
    if (var_paddr == -1UL) {
        debugf("syscall.c (put_env): MMU translated to null\n");
        XREG(A0) = -EFAULT;
        return;
    }

    debugf("syscall.c (get_env): Getting env var %s\n", (char *)var_paddr);
    
    // Copy the value from the user address to the process
    uintptr_t value_vaddr = XREG(A1);
    // debugf("syscall.c (get_env): Copying env var %s to %lx\n", value, value_vaddr);
    uintptr_t value_paddr = mmu_translate(p->rcb.ptable, value_vaddr);
    if (value_paddr == -1UL) {
        debugf("syscall.c (put_env): MMU translated to null\n");
        XREG(A0) = -EFAULT;
        return;
    }

    // Copy the value to the process
    process_put_env(p, (char *)var_paddr, (char *)value_paddr);
    debugf("syscall.c (put_env): Put env var %s\n", (char *)value_paddr);
}

SYSCALL(pid_get_env)
{
    // Take a PID argument, a variable name, and a buffer to write the value to
    SYSCALL_ENTER();
    debugf("syscall.c (pid_get_env): Got args: %lx %lx %lx %lx %lx %lx\n", XREG(A0), XREG(A1), XREG(A2), XREG(A3), XREG(A4), XREG(A5));
    // Get the first argument, the PID
    int pid = XREG(A0);
    Process *parent = sched_get_current();

    // Get the second argument, the variable name
    const char *var_vaddr = (const char *)XREG(A1);
    // Get the third argument, the buffer to write the value to
    const char *value_vaddr = (const char *)XREG(A2);
    debugf("syscall.c (pid_get_env): Got PID %d\n", pid);
    pid %= PID_LIMIT;
    debugf("syscall.c (pid_get_env): Got var vaddr %p\n", var_vaddr);
    debugf("syscall.c (pid_get_env): Got value vaddr %p\n", value_vaddr);

    // Get the process from the PID
    Process *p = process_map_get(pid);
    if (!p) {
        warnf("syscall.c (pid_get_env): Process %d not found\n", pid);
        // Process not found
        XREG(A0) = -ENOENT;
        return;
    } else {
        debugf("syscall.c (pid_get_env): Process %d found\n", pid);
    }

    // Translate the variable name to a physical address
    const char *var_paddr = mmu_translate(parent->rcb.ptable, (uintptr_t)var_vaddr);
    if (var_paddr == -1UL) {
        warnf("syscall.c (pid_get_env): MMU translated to null\n");
        // MMU translated to null
        XREG(A0) = -EFAULT;
        return;
    } else {
        debugf("syscall.c (pid_get_env): Var %s found\n", (char *)var_paddr);
    }
    
    const char *value_paddr = mmu_translate(parent->rcb.ptable, (uintptr_t)value_vaddr);
    if (value_paddr == -1UL) {
        warnf("syscall.c (pid_get_env): MMU translated to null\n");
        // MMU translated to null
        XREG(A0) = -EFAULT;
        return;
    } else {
        debugf("syscall.c (pid_get_env): Value %s found\n", (char *)value_paddr);
    }

    // Get the value pointer from the process
    const char *value = process_get_env(p, (char *)var_paddr);
    if (!value) {
        warnf("syscall.c (pid_get_env): Env var %s not found\n", (char *)var_paddr);
        // Env var not found
        XREG(A0) = -ENOENT;
        return;
    } else {
        debugf("syscall.c (pid_get_env): Got env var %s\n", value);
    }
    memcpy((void *)value_paddr, value, strlen(value) + 1);
}

SYSCALL(pid_put_env)
{
    // Take a PID argument, a variable name, and a buffer to read the value from
    SYSCALL_ENTER();
    debugf("syscall.c (pid_put_env): Got args: %lx %lx %lx %lx %lx %lx\n", XREG(A0), XREG(A1), XREG(A2), XREG(A3), XREG(A4), XREG(A5));

    // Get the first argument, the PID
    int pid = XREG(A0);
    Process *parent = sched_get_current();

    // Get the second argument, the variable name
    const char *var_vaddr = (const char *)XREG(A1);
    // Get the third argument, the buffer to write the value to
    const char *value_vaddr = (const char *)XREG(A2);
    debugf("syscall.c (pid_put_env): Got PID %d\n", pid);
    pid %= PID_LIMIT;
    debugf("syscall.c (pid_put_env): Got var vaddr %p\n", var_vaddr);
    debugf("syscall.c (pid_put_env): Got value vaddr %p\n", value_vaddr);

    // Get the process from the PID
    Process *p = process_map_get(pid);

    if (!p) {
        warnf("syscall.c (pid_put_env): Process %d not found\n", pid);
        // Process not found
        XREG(A0) = -ENOENT;
        return;
    } else {
        debugf("syscall.c (pid_put_env): Process %d found\n", pid);
    }

    // Translate the variable name to a physical address
    const char *var_paddr = mmu_translate(parent->rcb.ptable, (uintptr_t)var_vaddr);
    if (var_paddr == -1UL) {
        warnf("syscall.c (pid_put_env): MMU translated to null\n");
        // MMU translated to null
        XREG(A0) = -EFAULT;
        return;
    } else {
        debugf("syscall.c (pid_put_env): Var %s found\n", (char *)var_paddr);
    }

    const char *value_paddr = mmu_translate(parent->rcb.ptable, (uintptr_t)value_vaddr);
    if (value_paddr == -1UL) {
        warnf("syscall.c (pid_put_env): MMU translated to null\n");
        // MMU translated to null
        XREG(A0) = -EFAULT;
        return;
    } else {
        debugf("syscall.c (pid_put_env): Value %s found\n", (char *)value_paddr);
    }

    // Copy the value to the process
    process_put_env(p, (char *)var_paddr, (char *)value_paddr);
}


SYSCALL(get_pid)
{
    SYSCALL_ENTER();
    Process *p = sched_get_current();
    debugf("syscall.c (get_pid): Got PID %d\n", p->pid);
    XREG(A0) = p->pid;
}

SYSCALL(next_pid)
{
    SYSCALL_ENTER();
    debugf("syscall.c (next_pid): Got args: %lx %lx %lx %lx %lx %lx\n", XREG(A0), XREG(A1), XREG(A2), XREG(A3), XREG(A4), XREG(A5));
    trap_frame_debug(scratch);
    // Get a PID from the argument
    int pid = XREG(A0);
    debugf("syscall.c (next_pid): Got PID %d\n", pid);
    pid %= PID_LIMIT;

    // Get the next highest PID
    for (uint16_t i = pid + 1; i < PID_LIMIT; ++i) {
        pid = i % PID_LIMIT;
        // debugf("syscall.c (next_pid): Checking PID %x\n", pid);
        if (process_map_contains(pid)) {
            XREG(A0) = pid;
            return;
        }
    }

    // No PIDs found
    XREG(A0) = -1;
}

SYSCALL(yield)
{
    debugf("syscall.c (yield): Got args: %lx %lx %lx %lx %lx %lx\n", XREG(A0), XREG(A1), XREG(A2), XREG(A3), XREG(A4), XREG(A5));
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
    }

    if (p->pid != pid) {
        process_debug(p);
        fatalf("syscall.c (sleep): Process %d not found on hart %d\n", pid, hart);
    }
    
    if (!p) {
        fatalf("syscall.c (sleep): Null process on hart %d", p->hart);
    }
    // // Sleep the process. VIRT_TIMER_FREQ is 10MHz, divided by 1000, we get 10KHz

    /*
    // Commented out until we implement an IO buffer for the process
    // debugf("syscall.c (sleep) Sleeping PID %d at %d until %d\n", p->pid, sbi_get_time(), sbi_get_time() + XREG(A0) * VIRT_TIMER_FREQ / 1000);
    // p->sleep_until = sbi_get_time() + XREG(A0) * VIRT_TIMER_FREQ / 1000;
    // p->state = PS_SLEEPING;
    */
}

SYSCALL(events)
{
    SYSCALL_ENTER();
    debugf("syscall.c (events): Got args: %lx %lx %lx %lx %lx %lx\n", XREG(A0), XREG(A1), XREG(A2), XREG(A3), XREG(A4), XREG(A5));
    
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

SYSCALL(screen_draw)
{
    SYSCALL_ENTER();
    debugf("syscall.c (screen_draw): Drawing\n");

    Process *proc = sched_get_current();
    // p->state = PS_WAITING;

    // p->sleep_until = sbi_get_time() + VIRT_TIMER_FREQ / 1000000;
    // p->state = PS_SLEEPING;
    debugf("syscall.c (screen_draw): Got process %d\n", proc->pid);

    // Get the first argument, the buffer to draw
    const Pixel *buf_vaddr = (const Pixel *)XREG(A0);
    debugf("syscall.c (screen_draw): Got buf vaddr %p\n", buf_vaddr);
    // Get the second argument, the rectangle to draw to
    const Rectangle *rect_vaddr = (const Rectangle *)XREG(A1);
    debugf("syscall.c (screen_draw): Got rect vaddr %p\n", rect_vaddr);

    // Get the scale factor
    uint64_t x_scale = XREG(A2);
    debugf("syscall.c (screen_draw): Got scale %d\n", x_scale);
    // Get the scale factor
    uint64_t y_scale = XREG(A3);
    debugf("syscall.c (screen_draw): Got scale %d\n", y_scale);

    // Translate the buffer to a physical address
    const Pixel *buf_paddr = mmu_translate(proc->rcb.ptable, (uintptr_t)buf_vaddr);
    if (buf_paddr == -1UL) {
        warnf("syscall.c (screen_draw): MMU translated to null\n");
        // MMU translated to null
        XREG(A0) = -EFAULT;
        return;
    } else {
        debugf("syscall.c (screen_draw): Buffer %p found\n", (char *)buf_paddr);
    }

    const Rectangle *rect_paddr = mmu_translate(proc->rcb.ptable, (uintptr_t)rect_vaddr);
    if (rect_paddr == -1UL) {
        warnf("syscall.c (screen_draw): MMU translated to null\n");
        // MMU translated to null
        XREG(A0) = -EFAULT;
        return;
    } else {
        debugf("syscall.c (screen_draw): Rect %p found\n", (char *)rect_paddr);
    }

    Console *console = gpu_get_console();
    debugf("Console at %p\n", console);
    Pixel *frame_buf = gpu_get_frame_buf();
    Rectangle *screen_rect = gpu_get_screen_rect();
    uint32_t screen_width = screen_rect->width;
    uint32_t screen_height = screen_rect->height;
    debugf("Screen width %d\n", screen_width);
    debugf("Screen height %d\n", screen_height);

    uint32_t base_x = rect_paddr->x;
    uint32_t base_y = rect_paddr->y;
    uint32_t width = rect_paddr->width;
    uint32_t height = rect_paddr->height;

    // Draw the buffer
    // for (unsigned y_ = 0; y_ < height; ++y_) {
    //     for (unsigned x_ = 0; x_ < width; ++x_) {
    //         if (base_x + x_ >= screen_width || base_y + y_ >= screen_height) {
    //             continue;
    //         }
    //         // debugf("syscall.c (screen_draw): Drawing pixel %d %d\n", x + x_, y + y_);
    //         frame_buf[(base_y + y_) * screen_width + (base_x + x_)] = buf_paddr[y_ * width + x_];
    //     }
    // }
    XREG(A0) = 0;
    // for (uint32_t y=0; y < height * y_scale; y += y_scale) {
    //     for (uint32_t x=0; x < width * x_scale; x += x_scale) {
    //         uint32_t frame_buf_x = base_x + x;
    //         uint32_t frame_buf_y = base_y + y;
    //         uint32_t frame_buff_offset = frame_buf_y * screen_width + frame_buf_x;
    //         if (frame_buf_x >= screen_width || frame_buf_y >= screen_height) {
    //             XREG(A0) = -EFAULT;
    //             continue;
    //         }
    //         uint32_t user_buf_x = x / x_scale;
    //         uint32_t user_buf_y = y / y_scale;
    //         uint32_t user_buf_offset = user_buf_y * width + user_buf_x;
    //         Pixel p = buf_paddr[user_buf_offset];
    //         debugf("syscall.c (screen_draw): Drawing pixel r=%d g=%d b=%d a=%d at (%d,%d)\n", p.r, p.g, p.b, p.a, frame_buf_x, frame_buf_y);
    //         for (uint32_t sy = 0; sy < y_scale; ++sy) {
    //             for (uint32_t sx = 0; sx < x_scale; ++sx) {
    //                 uint32_t frame_buf_x = base_x + x + sx;
    //                 uint32_t frame_buf_y = base_y + y + sy;
    //                 uint32_t frame_buff_offset = frame_buf_y * screen_width + frame_buf_x;
    //                 if (frame_buf_x >= screen_width || frame_buf_y >= screen_height) {
    //                     XREG(A0) = -EFAULT;
    //                     continue;
    //                 }
    //                 // If the pixel is non-zero, print it
    //                 if (p.r || p.g || p.b || p.a) {
    //                     // debugf("syscall.c (screen_draw): Drawing pixel r=%d g=%d b=%d a=%d at (%d,%d)\n", p.r, p.g, p.b, p.a, frame_buf_x, frame_buf_y);
    //                 }
    //                 frame_buf[frame_buff_offset] = p;
    //             }
    //         }

    //     }
    // }

    // Draw the buffer scaled by `scale`
    for (unsigned y_ = 0; y_ < height; ++y_) {
        for (unsigned x_ = 0; x_ < width; ++x_) {
            // if (x + x_ * x_scale >= screen_width || y + y_ * y_scale >= screen_height) {
            //     XREG(A0) = -EFAULT;
            //     continue;
            // }
            // debugf("syscall.c (screen_draw): Drawing pixel %d %d\n", x + x_, y + y_);
            Pixel p = *(Pixel*)mmu_translate(proc->rcb.ptable, &buf_vaddr[y_ * width + x_]);
            for (unsigned sy = 0; sy < y_scale; ++sy) {
                for (unsigned sx = 0; sx < x_scale; ++sx) {
                    uint64_t x_pos = base_x + x_ * x_scale + sx;
                    uint64_t y_pos = base_y + y_ * y_scale + sy;

                    if (x_pos >= screen_width || y_pos >= screen_height) {
                        XREG(A0) = -EFAULT;
                        continue;
                    }

                    frame_buf[y_pos * screen_width + x_pos] = p;
                }
            }
        }
    }
    debugf("syscall.c (screen_draw): Drawn\n");
    
}

SYSCALL(screen_get_dims)
{
    SYSCALL_ENTER();
    debugf("syscall.c (screen_get_dims): Writing dims to rectangle in userspace\n");
    Process *p = sched_get_current();
    // p->state = PS_WAITING;
    debugf("syscall.c (screen_draw): Got process %d\n", p->pid);

    // Get the first argument, the buffer to draw
    // const Pixel *buf_vaddr = (const Pixel *)XREG(A0);
    // debugf("syscall.c (screen_draw): Got buf vaddr %p\n", buf_vaddr);
    // Get the second argument, the rectangle to draw to
    const Rectangle *rect_vaddr = (const Rectangle *)XREG(A0);
    debugf("syscall.c (screen_draw): Got rect vaddr %p\n", rect_vaddr);

    // Translate the buffer to a physical address
    const Pixel *rect_paddr = mmu_translate(p->rcb.ptable, (uintptr_t)rect_vaddr);

    if (rect_paddr == -1UL) {
        warnf("syscall.c (screen_draw): MMU translated to null\n");
        // MMU translated to null
        XREG(A0) = -EFAULT;
        return;
    } else {
        debugf("syscall.c (screen_draw): Rect %p found\n", (char *)rect_paddr);
    }
    memcpy((void *)rect_paddr, gpu_get_screen_rect(), sizeof(Rectangle));

}

SYSCALL(screen_flush) {
    SYSCALL_ENTER();
    debugf("syscall.c (screen_flush): Flushing\n");
    gpu_transfer_to_host_2d(gpu_get_screen_rect(), 1, 0);
    gpu_flush();
    debugf("syscall.c (screen_flush): Flushed\n");
}

SYSCALL(get_time)
{
    SYSCALL_ENTER();
    debugf("syscall.c (get_time): Getting time\n");
    XREG(A0) = sbi_get_time();
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
    SYSCALL_PTR(exit),     /* 0 */
    SYSCALL_PTR(putchar),  /* 1 */
    SYSCALL_PTR(getchar),  /* 2 */
    SYSCALL_PTR(yield),    /* 3 */
    SYSCALL_PTR(sleep),    /* 4 */
    SYSCALL_PTR(events),   /* 5 */
    SYSCALL_PTR(get_env),  /* 6 */
    SYSCALL_PTR(put_env),  /* 7 */
    SYSCALL_PTR(get_pid),  /* 8 */
    SYSCALL_PTR(next_pid), /* 9 */
    SYSCALL_PTR(pid_get_env),  /* 10 */
    SYSCALL_PTR(pid_put_env),  /* 11 */
    SYSCALL_PTR(screen_draw),  /* 12 */
    SYSCALL_PTR(screen_get_dims), /* 13 */
    SYSCALL_PTR(get_time),   /* 14 */
    SYSCALL_PTR(screen_flush), /* 15 */
};

static const int NUM_SYSCALLS = sizeof(SYSCALLS) / sizeof(SYSCALLS[0]);

// We get here from the trap.c if this is an ECALL from U-MODE
void syscall_handle(int hart, uint64_t epc, int64_t *scratch)
{
    // Sched invoke will save sepc, so we want it to resume
    // 4 bytes ahead, which will be the next instruction.
    // CSR_WRITE("sepc", epc + 4);
    // TrapFrame *tf = (TrapFrame *)scratch;
    // tf->sepc = epc + 4;

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
