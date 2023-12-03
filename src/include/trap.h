#pragma once
// #include <asm/trampoline.S>
#include <stdint.h>


extern uint64_t trampoline_thread_start;
extern uint64_t trampoline_trap_start;

void os_trap_handler(void);

// Do NOT move or change the fields below. The
// trampoline code expects these to be in the right
// place.
typedef struct TrapFrame {
    int64_t xregs[32];
    double fregs[32];
    uint64_t sepc;
    uint64_t sstatus;
    uint64_t sie;
    uint64_t satp;
    uint64_t sscratch;
    uint64_t stvec;
    uint64_t trap_satp;
    uint64_t trap_stack;
} TrapFrame;


extern struct TrapFrame *kernel_trap_frame;