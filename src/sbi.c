#include <compiler.h>
#include <config.h>
#include <sbi.h>

void sbi_putchar(char c)
{
    asm volatile("mv a7, %0\nmv a0, %1\necall" ::"r"(SBI_SVCALL_PUTCHAR), "r"(c) : "a7", "a0");
}

char sbi_getchar(void)
{
    char c;
    asm volatile("mv a7, %1\necall\nmv %0, a0\n" : "=r"(c) : "r"(SBI_SVCALL_GETCHAR) : "a7", "a0");
    return c;
}

int sbi_hart_get_status(unsigned int hart)
{
    int stat;
    asm volatile("mv a7, %1\nmv a0, %2\necall\nmv %0, a0\n"
                 : "=r"(stat)
                 : "r"(SBI_SVCALL_HART_STATUS), "r"(hart)
                 : "a0", "a7");
    return stat;
}

int sbi_hart_start(unsigned int hart, unsigned long target, unsigned long scratch, unsigned long satp)
{
    int stat;
    asm volatile("mv a7, %1\nmv a0, %2\nmv a1, %3\nmv a2, %4\nmv a3, %5\necall\nmv %0, a0\n"
                 : "=r"(stat)
                 : "r"(SBI_SVCALL_HART_START), "r"(hart), "r"(target), "r"(scratch), "r"(satp)
                 : "a0", "a1", "a2", "a7");
    return stat;
}

void sbi_hart_stop(void)
{
    asm volatile("mv a7, %0\necall\nwfi" : : "r"(SBI_SVCALL_HART_STOP) : "a0", "a7");
}

void sbi_poweroff(void)
{
    asm volatile("mv a7, %0\necall" : : "r"(SBI_SVCALL_POWEROFF) : "a0", "a7");
}

unsigned long sbi_get_time(void)
{
    unsigned long ret;
    asm volatile("mv a7, %1\necall\nmv %0, a0" : "=r"(ret) : "r"(SBI_SVCALL_GET_TIME) : "a0", "a7");
    return ret;
}

void sbi_set_timer(unsigned int hart, unsigned long val)
{
    asm volatile("mv a7, %0\nmv a0, %1\nmv a1, %2\necall" ::"r"(SBI_SVCALL_SET_TIMECMP), "r"(hart),
                 "r"(val)
                 : "a0", "a1", "a7");
}

void sbi_add_timer(unsigned int hart, unsigned long val)
{
    asm volatile("mv a7, %0\nmv a0, %1\nmv a1, %2\necall" ::"r"(SBI_SVCALL_ADD_TIMECMP), "r"(hart),
                 "r"(val)
                 : "a0", "a1", "a7");
}

void sbi_ack_timer(void)
{
    asm volatile("mv a7, %0\necall" ::"r"(SBI_SVCALL_ACK_TIMER) : "a7");
}

unsigned long sbi_rtc_get_time(void)
{
    unsigned long ret;
    asm volatile("mv a7, %1\necall\nmv %0, a0"
                 : "=r"(ret)
                 : "r"(SBI_SVCALL_RTC_GET_TIME)
                 : "a0", "a7");
    return ret;
}

int sbi_whoami(void)
{
    int ret;
    asm volatile("mv a7, %1\necall\nmv %0, a0" : "=r"(ret) : "r"(SBI_SVCALL_WHOAMI) : "a0", "a7");
    return ret;
}

int sbi_num_harts(void)
{
    unsigned int i;
    int num_harts = 0;
    for (i = 0; i < MAX_ALLOWABLE_HARTS; i++) {
        if (sbi_hart_get_status(i) != 0) {
            num_harts += 1;
        }
    }
    return num_harts;
}
