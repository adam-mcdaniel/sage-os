/**
 * @file rtc.c
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Real-time clock (RTC) routines.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <rtc.h>

// The RTC is connected to virt at MMIO address 0x10_1000
#define RTC_BASE            0x101000

// Helper macros to convert the RTC output into seconds, milliseconds, or microseconds
#define RTC_SECS(x)         ((x) / 1000000000UL)
#define RTC_MSECS(x)        ((x) / 1000000UL)
#define RTC_USECS(x)        ((x) / 1000UL)

// RTC Registers
#define RTC_TIME_LOW        0x00
#define RTC_TIME_HI         0x04
#define RTC_ALARM_LOW       0x08
#define RTC_ALARM_HI        0x0c
#define RTC_IRQ             0x10
#define RTC_CLEAR_ALARM     0x14
#define RTC_ALARM_STATUS    0x18
#define RTC_CLEAR_INTERRUPT 0x1c

unsigned long rtc_get_time(void)
{
    unsigned long low  = *(unsigned int *)(RTC_BASE + RTC_TIME_LOW);
    unsigned long high = *(unsigned int *)(RTC_BASE + RTC_TIME_HI);

    return (high << 32) | low;
}
