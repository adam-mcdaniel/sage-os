/**
 * @file sbi.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief SBI calling routines.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include <compiler.h>

#define SBI_SVCALL_HART_STATUS  (1)
#define SBI_SVCALL_HART_START   (2)
#define SBI_SVCALL_HART_STOP    (3)

#define SBI_SVCALL_GET_TIME     (4)
#define SBI_SVCALL_SET_TIMECMP  (5)
#define SBI_SVCALL_ADD_TIMECMP  (6)
#define SBI_SVCALL_ACK_TIMER    (7)
#define SBI_SVCALL_RTC_GET_TIME (8)

#define SBI_SVCALL_PUTCHAR      (9)
#define SBI_SVCALL_GETCHAR      (10)

#define SBI_SVCALL_WHOAMI       (11)

#define SBI_SVCALL_POWEROFF     (12)
// The following calls are helpers to make the ECALL to the SBI.

/**
 * @brief Put a single character to the console via SBI (UART).
 *
 * @param c the character to put.
 */
void sbi_putchar(char c);

/**
 * @brief Get a single character from the console via SBI (UART).
 *
 * @return the character retrieved or -1 (0xFF) if no character is in the buffer.
 */
char sbi_getchar(void);

/**
 * @brief Get the status of a HART.
 * Status codes:
 * `0 - invalid status`
 * `1 - HART is parked`
 * `2 - HART has awoken and is starting`
 * `3 - HART is running`
 *
 * @param hart the HART to get the status.
 * @return HS_INVALID if the HART is invalid, or the status code.
 */
int sbi_hart_get_status(unsigned int hart);

/**
 * @brief Start a parked HART in supervisor mode. If you want to go to user mode,
 * create a veneer to target.
 *
 * @param hart the HART to start.
 * @param target the physical target address to load.
 * @param scratch any data that gets loaded into sscratch.
 * @param satp any data that gets loaded into satp (may be 0)
 * @return 0 (false) on error or 1 (true) if successful.
 */
int sbi_hart_start(unsigned int hart, unsigned long target, unsigned long scratch, unsigned long satp);

/**
 * @brief Stop the current HART. This function should not return as the HART running it
 * will go park.
 *
 * @return false (0) on error. This function does not return on success.
 */
void sbi_hart_stop(void);

/**
 * @brief Immediately exit out of QEMU.
 *
 */
void sbi_poweroff(void);

/**
 * @brief Get the value of mtime in the CLINT.
 *
 * @return the value of mtime.
 */
unsigned long sbi_get_time(void);

/**
 * @brief Acknowledge a delegated timer interrupt. Clears the MTIP bit.
 *
 */
void sbi_ack_timer(void);

/**
 * @brief Set MTIMECMP of the given HART to the given value.
 *
 * @param hart the hart to set
 * @param val the value to set MTIMECMP.
 */
void sbi_set_timer(unsigned int hart, unsigned long val);

/**
 * @brief Set MTIMECMP of the given HART to the given value + MTIME.
 *
 * @param hart the hart to add
 * @param val the value to add to MTIME to set into MTIMECMP
 */
void sbi_add_timer(unsigned int hart, unsigned long val);

/**
 * @brief Get the real UNIX time in nanoseconds.
 *
 * @return the value of the real time clock (RTC).
 */
unsigned long sbi_rtc_get_time(void);

/**
 * @brief Get the MHARTID number of the currently executing HART.
 *
 * @return the MHARTID value
 */
int sbi_whoami(void);

/**
 * @brief Get the total number of HARTs on the system (maximum of MAX_ALLOWABLE_HARTS).
 *
 * @return the number of HARTs.
 */
int sbi_num_harts(void);
