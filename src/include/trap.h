// #include <asm/trampoline.S>

extern uint64_t spawn_start;
extern uint64_t spawn_end;

extern uint64_t _spawn_kthread;
extern uint64_t _trap_end;

void os_trap_handler(void);
