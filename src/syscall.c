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
#include <vfs.h>
#include <elf.h>
#include <virtio.h>
#include <gpu.h>
#include <uaccess.h>

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
    */
    debugf("syscall.c (sleep) Sleeping PID %d at %d until %d\n", p->pid, sbi_get_time(), sbi_get_time() + XREG(A0) * VIRT_TIMER_FREQ / 1000);
    // p->sleep_until = sbi_get_time() + XREG(A0) * VIRT_TIMER_FREQ / 1000;
    // p->state = PS_SLEEPING;
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

    debugf("syscall.c (screen_draw): Drawing to %d %d %d %d\n", rect_paddr->x, rect_paddr->y, rect_paddr->width, rect_paddr->height);

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
            Pixel p = *(Pixel*)mmu_translate(proc->rcb.ptable, &buf_vaddr[y_ * width + x_]);
            // debugf("syscall.c (screen_draw): Drawing pixel %d %d = (r=%d g=%d b=%d a=%d)\n", x_ * x_scale, y_ * y_scale, p.r, p.g, p.b, p.a);
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
    XREG(A0) = 0;

}

SYSCALL(screen_flush) {
    SYSCALL_ENTER();
    // Check to see if there's a rectangle supplied in A0
    // If there is, flush that rectangle
    // If there isn't, flush the whole screen
    Process *p = sched_get_current();
    debugf("syscall.c (screen_draw): Got process %d\n", p->pid);
    
    const Rectangle *rect_vaddr = (const Rectangle *)XREG(A0);
    debugf("syscall.c (screen_flush): Got rect vaddr %p\n", rect_vaddr);
    if (rect_vaddr) {
        debugf("syscall.c (screen_flush): Flushing rectangle\n");
        // Translate the buffer to a physical address
        const Rectangle *rect_paddr = mmu_translate(p->rcb.ptable, (uintptr_t)rect_vaddr);
        if (rect_paddr == -1UL) {
            warnf("syscall.c (screen_flush): MMU translated to null\n");
            // MMU translated to null
            XREG(A0) = -EFAULT;
            return;
        } else {
            debugf("syscall.c (screen_flush): Rect %p found\n", (char *)rect_paddr);
        }

        // Rectangle rect;
        // copy_from(&rect, p->rcb.ptable, rect_vaddr, sizeof(Rectangle));
        Rectangle rect = *rect_paddr;
        debugf("Found rect %d %d %d %d\n", rect.x, rect.y, rect.width, rect.height);
        gpu_transfer_to_host_2d(gpu_get_screen_rect(), 1, 0);
        gpu_flush(rect);
    } else {
        debugf("syscall.c (screen_flush): Flushing screen\n");
        Rectangle rect = *gpu_get_screen_rect();
        gpu_transfer_to_host_2d(&rect, 1, 0);
        gpu_flush(rect);
    }

    XREG(A0) = 0;
    // debugf("syscall.c (screen_flush): Flushing\n");
    // gpu_transfer_to_hosdt_2d(gpu_get_screen_rect(), 1, 0);
    // gpu_flush();
    // debugf("syscall.c (screen_flush): Flushed\n");
}

SYSCALL(get_keyboard_event)
{
    SYSCALL_ENTER();
    debugf("syscall.c (get_keyboard_event): Getting keyboard event\n");
    Process *p = sched_get_current();
    
    VirtioInputEvent event = keyboard_get_next_event();

    // Translate the buffer to a physical address
    const VirtioInputEvent *event_vaddr = (const VirtioInputEvent *)XREG(A0);
    const VirtioInputEvent *event_paddr = mmu_translate(p->rcb.ptable, (uintptr_t)event_vaddr);

    if (event_paddr == -1UL) {
        warnf("syscall.c (get_keyboard_event): MMU translated to null\n");
        // MMU translated to null
        XREG(A0) = -EFAULT;
        return;
    } else {
        debugf("syscall.c (get_keyboard_event): Event %p found\n", (char *)event_paddr);
    }

    memcpy((void *)event_paddr, &event, sizeof(VirtioInputEvent));

    XREG(A0) = 0;
}

SYSCALL(get_tablet_event)
{
    SYSCALL_ENTER();
    debugf("syscall.c (get_tablet_event): Getting tablet event\n");
    Process *p = sched_get_current();
    
    VirtioInputEvent event = tablet_get_next_event();

    // Translate the buffer to a physical address
    const VirtioInputEvent *event_vaddr = (const VirtioInputEvent *)XREG(A0);
    const VirtioInputEvent *event_paddr = mmu_translate(p->rcb.ptable, (uintptr_t)event_vaddr);

    if (event_paddr == -1UL) {
        warnf("syscall.c (get_tablet_event): MMU translated to null\n");
        // MMU translated to null
        XREG(A0) = -EFAULT;
        return;
    } else {
        debugf("syscall.c (get_tablet_event): Event %p found\n", (char *)event_paddr);
    }

    memcpy((void *)event_paddr, &event, sizeof(VirtioInputEvent));

    XREG(A0) = 0;
}

SYSCALL(get_time)
{
    SYSCALL_ENTER();
    debugf("syscall.c (get_time): Getting time\n");
    XREG(A0) = sbi_get_time();
}

SYSCALL(path_exists)
{
    SYSCALL_ENTER();
    debugf("syscall.c (path_exists): Checking if path exists\n");
    Process *p = sched_get_current();

    const char *path_vaddr = (const char *)XREG(A0);
    if (!path_vaddr) {
        XREG(A0) = -EINVAL;
        return;
    }

    const char *path_paddr = mmu_translate(p->rcb.ptable, (uintptr_t)path_vaddr);
    if (path_paddr == -1UL) {
        XREG(A0) = -EFAULT;
        return;
    }

    debugf("syscall.c (path_exists): Checking if path %s is dir\n", path_paddr);
    XREG(A0) = vfs_exists(path_paddr);
}


SYSCALL(path_is_dir)
{
    SYSCALL_ENTER();
    debugf("syscall.c (path_is_dir): Checking if path is dir\n");
    Process *p = sched_get_current();

    const char *path_vaddr = (const char *)XREG(A0);
    if (!path_vaddr) {
        XREG(A0) = -EINVAL;
        return;
    }

    const char *path_paddr = mmu_translate(p->rcb.ptable, (uintptr_t)path_vaddr);
    if (path_paddr == -1UL) {
        XREG(A0) = -EFAULT;
        return;
    }

    debugf("syscall.c (path_is_dir): Checking if path %s is dir\n", path_paddr);
    XREG(A0) = vfs_is_dir(path_paddr);
}

SYSCALL(path_is_file)
{
    SYSCALL_ENTER();
    debugf("syscall.c (path_is_file): Checking if path is file\n");
    Process *p = sched_get_current();

    const char *path_vaddr = (const char *)XREG(A0);
    if (!path_vaddr) {
        XREG(A0) = -EINVAL;
        return;
    }

    const char *path_paddr = mmu_translate(p->rcb.ptable, (uintptr_t)path_vaddr);
    if (path_paddr == -1UL) {
        XREG(A0) = -EFAULT;
        return;
    }

    debugf("syscall.c (path_is_file): Checking if path %s is file\n", path_paddr);
    XREG(A0) = vfs_is_file(path_paddr);
}

SYSCALL(path_list_dir)
{
    SYSCALL_ENTER();
    debugf("syscall.c (path_list_dir): Listing dir\n");
    Process *p = sched_get_current();

    const char *path_vaddr = (const char *)XREG(A0);
    char *entries_buffer_vaddr = (char *)XREG(A1);
    uint64_t entries_buffer_size = XREG(A2);
    bool return_full_path = XREG(A3);

    if (!path_vaddr) {
        warnf("syscall.c (path_list_dir): Path is null\n");
        XREG(A0) = -EINVAL;
        return;
    }

    if (!entries_buffer_vaddr) {
        warnf("syscall.c (path_list_dir): Entries buffer is null\n");
        XREG(A0) = -EINVAL;
        return;
    }

    const char *path_paddr = mmu_translate(p->rcb.ptable, (uintptr_t)path_vaddr);

    if (path_paddr == -1UL) {
        XREG(A0) = -EFAULT;
        return;
    }
    debugf("syscall.c (path_list_dir): Got path %s\n", path_paddr);
    
    // if (!vfs_is_dir(path_paddr)) {
    //     XREG(A0) = -ENOTDIR;
    //     warnf("syscall.c (path_list_dir): Path %s is not a directory\n", path_paddr);
    //     return;
    // } else {
    //     debugf("syscall.c (path_list_dir): Path %s is a directory\n", path_paddr);
    // }


    char *entries_buffer_paddr = mmu_translate(p->rcb.ptable, (uintptr_t)entries_buffer_vaddr);
    if (entries_buffer_paddr == -1UL) {
        XREG(A0) = -EFAULT;
        return;
    }
    // debugf("syscall.c (path_list_dir): Got entries buffer %p\n", entries_buffer_paddr);

    // debugf("syscall.c (path_list_dir): Listing dir %s\n", path_paddr);

    CSR_WRITE("sscratch", p->frame->sscratch);
    // char buf[2048] = {0};
    vfs_list_dir(path_paddr, entries_buffer_paddr, entries_buffer_size, return_full_path);
    // debugf("syscall.c (path_list_dir): Listed dir %s\n", buf);
    // copy_to(entries_buffer_vaddr, p->rcb.ptable, buf, entries_buffer_size);
    // memcpy(entries_buffer_paddr, buf, entries_buffer_size);
    // vfs_list_dir(path_paddr, entries_buffer_paddr, entries_buffer_size, return_full_path);
    debugf("syscall.c (path_list_dir): Listed dir %s\n", path_paddr);
    XREG(A0) = 0;
}

SYSCALL(get_file_size)
{
    SYSCALL_ENTER();
    debugf("syscall.c (get_file_size): Getting file size\n");
    Process *p = sched_get_current();

    const char *path_vaddr = (const char *)XREG(A0);
    if (!path_vaddr) {
        warnf("syscall.c (get_file_size): Path is null\n");
        XREG(A0) = -EINVAL;
        return;
    }

    const char *path_paddr = mmu_translate(p->rcb.ptable, (uintptr_t)path_vaddr);

    if (path_paddr == -1UL) {
        XREG(A0) = -EFAULT;
        return;
    }
    debugf("syscall.c (get_file_size): Got path %s\n", path_paddr);

    // debugf("syscall.c (get_file_size): Getting file size of %s\n", path_paddr);
    // char buf[2048] = {0};
    // vfs_read(path_paddr, path_paddr, buffer_size);
    File *file = vfs_open(path_paddr, 0, O_RDONLY, VFS_TYPE_FILE);
    Stat stat;
    vfs_stat(file, &stat);
    vfs_close(file);
    debugf("syscall.c (get_file_size): File %s has size %d\n", path_paddr, stat.size);
    XREG(A0) = stat.size;
}

SYSCALL(read_file)
{
    SYSCALL_ENTER();
    debugf("syscall.c (read_file): Reading file\n");
    Process *p = sched_get_current();

    const char *path_vaddr = (const char *)XREG(A0);
    char *buffer_vaddr = (char *)XREG(A1);
    uint64_t buffer_size = XREG(A2);

    if (!path_vaddr) {
        warnf("syscall.c (read_file): Path is null\n");
        XREG(A0) = -EINVAL;
        return;
    }

    if (!buffer_vaddr) {
        warnf("syscall.c (read_file): Buffer is null\n");
        XREG(A0) = -EINVAL;
        return;
    }

    const char *path_paddr = mmu_translate(p->rcb.ptable, (uintptr_t)path_vaddr);

    if (path_paddr == -1UL) {
        XREG(A0) = -EFAULT;
        return;
    }
    debugf("syscall.c (read_file): Got path %s\n", path_paddr);

    char *buffer_paddr = mmu_translate(p->rcb.ptable, (uintptr_t)buffer_vaddr);
    if (buffer_paddr == -1UL) {
        XREG(A0) = -EFAULT;
        return;
    }
    debugf("syscall.c (read_file): Got buffer %p\n", buffer_paddr);

    // debugf("syscall.c (read_file): Reading file %s\n", path_paddr);
    // char buf[2048] = {0};
    // vfs_read(path_paddr, path_paddr, buffer_size);
    File *file = vfs_open(path_paddr, 0, O_RDONLY, VFS_TYPE_FILE);
    Stat stat;
    vfs_stat(file, &stat);
    

    uint64_t file_size = stat.size;
    debugf("syscall.c (read_file): File %s has size %d\n", path_paddr, file_size);
    #define min(a, b) ((a) < (b) ? (a) : (b))
    #define max(a, b) ((a) > (b) ? (a) : (b))
    char *tmp_buffer = (char*)kzalloc(max(file_size, buffer_size));

    uint64_t read_bytes = vfs_read(file, tmp_buffer, min(file_size, buffer_size));
    // uint64_t read_bytes = 0;
    if (read_bytes < 0) {
        warnf("syscall.c (read_file): Failed to read file %s\n", path_paddr);
        XREG(A0) = -EIO;
        vfs_close(file);
        return;
    }
    vfs_close(file);
    debugf("syscall.c (read_file): Read %d bytes\n", read_bytes);

    for (uint64_t i = 0; i < read_bytes; ++i) {
        *(uint8_t*)mmu_translate(p->rcb.ptable, (uintptr_t)&buffer_vaddr[i]) = tmp_buffer[i];
    }
    kfree(tmp_buffer);

    XREG(A0) = read_bytes;
}


SYSCALL(spawn_process)
{
    SYSCALL_ENTER();
    // Add a new process to the scheduler using the file path in A0,
    // and return the PID in A0
    debugf("syscall.c (spawn_process): Spawning process\n");
    Process *p = sched_get_current();

    const char *path_vaddr = (const char *)XREG(A0);
    if (!path_vaddr) {
        warnf("syscall.c (spawn_process): Path is null\n");
        XREG(A0) = -EINVAL;
        return;
    }

    const char *path_paddr = mmu_translate(p->rcb.ptable, (uintptr_t)path_vaddr);

    if (path_paddr == -1UL) {
        warnf("syscall.c (spawn_process): MMU translated to null\n");
        XREG(A0) = -EFAULT;
        return;
    } else {
        debugf("syscall.c (spawn_process): Got path %s\n", path_paddr);
    }

    // debugf("syscall.c (spawn_process): Got path %s\n", path_paddr);

    // Check to see if the path exists
    if (!vfs_exists(path_paddr)) {
        warnf("syscall.c (spawn_process): Path %s does not exist\n", path_paddr);
        XREG(A0) = -ENOENT;
        return;
    } else {
        debugf("syscall.c (spawn_process): Path %s exists\n", path_paddr);
    }

    // Check to see if the path is a file
    if (!vfs_is_file(path_paddr)) {
        warnf("syscall.c (spawn_process): Path %s is not a file\n", path_paddr);
        XREG(A0) = -ENOTDIR;
        return;
    } else {
        debugf("syscall.c (spawn_process): Path %s is a file\n", path_paddr);
    }

    // Read the file
    debugf("syscall.c (spawn_process): Opening file %s\n", path_paddr);
    File *elf_file = vfs_open(path_paddr, 0, O_RDONLY, VFS_TYPE_FILE);
    Stat stat;
    vfs_stat(elf_file, &stat);
    debugf("syscall.c (spawn_process): File %s has size %d\n", path_paddr, stat.size);

    uint64_t file_size = stat.size;

    uint8_t *file_buffer = (char*)kmalloc(file_size);
    if (vfs_read(elf_file, file_buffer, file_size) < 0) {
        warnf("syscall.c (spawn_process): Failed to read file %s\n", path_paddr);
        XREG(A0) = -EIO;
        return;
    }
    vfs_close(elf_file);

    // Check to see if the file is an ELF
    if (!elf_is_valid(file_buffer)) {
        warnf("syscall.c (spawn_process): File %s is not a valid ELF\n", path_paddr);
        XREG(A0) = -ENOEXEC;
        return;
    }

    // Create a new process
    debugf("syscall.c (spawn_process): Creating new process\n");
    Process *new_process = process_new(PM_USER);
    if (elf_create_process(new_process, file_buffer)) {
        warnf("syscall.c (spawn_process): Failed to create process\n");
        XREG(A0) = -ENOEXEC;
        return;
    }
    debugf("syscall.c (spawn_process): Created new process\n");
    new_process->state = PS_RUNNING;
    new_process->hart = sbi_whoami();
    debugf("syscall.c (spawn_process): Scheduling new process\n");
    // sched_add(new_process);
    debugf("syscall.c (spawn_process): Scheduled new process\n");
    // Store the PID in A0
    debugf("syscall.c (spawn_process): Child process has PID %d\n", new_process->pid);
    XREG(A0) = new_process->pid;

    sched_add(new_process);
    process_run(new_process, new_process->hart);

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
    SYSCALL_PTR(get_keyboard_event), /* 16 */
    SYSCALL_PTR(get_tablet_event), /* 17 */
    SYSCALL_PTR(path_exists), /* 18 */
    SYSCALL_PTR(path_is_dir), /* 19 */
    SYSCALL_PTR(path_is_file), /* 20 */
    SYSCALL_PTR(path_list_dir), /* 21 */
    SYSCALL_PTR(spawn_process), /* 22 */
    SYSCALL_PTR(read_file), /* 23 */
    SYSCALL_PTR(get_file_size), /* 24 */
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

