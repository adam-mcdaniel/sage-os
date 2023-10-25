// #include <asm/trampoline.S>

extern uint64_t trampoline_thread_start;
extern uint64_t trampoline_trap_start;

void os_trap_handler(void);

typedef struct Trapframe {
    uint64_t  gpregs[32]; 
    double    fpregs[32]; 
    uint64_t  sepc;     
    uint64_t  sstatus;  
    uint64_t  sie;      
    uint64_t  satp;     
    uint64_t  sscratch; 
    uint64_t  stvec;    
    uint64_t  trap_satp;
    uint64_t  trap_stack;
} Trapframe;