/**
 * @file clint.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Core Local Interrupt (CLINT) functions/utilities.
 * @version 0.1
 * @date 2022-05-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

// The MMIO base address for the clint is 0x200_0000
#define CLINT_BASE_ADDRESS      0x2000000

// The mtimecmp registers are at 0x200_4000
#define CLINT_MTIMECMP_OFFSET   0x4000

// What we're calling "infinite" to prevent MTIMECMP <= MTIME
// interrupts.
#define CLINT_MTIMECMP_INFINITE 0x7FFFFFFFFFFFFFFFUL

/**
 * @brief Set the machine software interrupt pending (MSIP)
 * for a given hart. This is how you will send an inter-processor
 * interrupts or IPIs.
 * 
 * If the given HART is listening to MSIPs, then this will cause it
 * to trap with the cause 3, which sets of a chain of events, including
 * looking at the HART start data.
 * 
 * @param hart - the hart to set the MSIP bit.
 */
void clint_set_msip(unsigned int hart);

/**
 * @brief Clear the machine software interrupt pending (MSIP)
 * for a given hart. If you don't clear this, it will keep trapping
 * the given HART. Usually the awakening HART should clear this.
 * 
 * @param hart - the hart to clear the MSIP bit.
 */
void clint_clear_msip(unsigned int hart);

/**
 * @brief Set the mtimecmp register for a given hart. Recall that
 * if MTIMECMP <= MTIME, it will generate an machine timer interrupt (MTIP).
 * 
 * @param hart - the hart to set the MTIMECMP register.
 * @param val - the value to set the MTIMECMP register. This is an absolute value.
 */
void clint_set_mtimecmp(unsigned int hart, unsigned long val);

/**
 * @brief Add a given amount of time to the mtimecmp register for a given HART.
 * 
 * This essentially reads MTIME adds val to it, and sets the MTIMECMP register to
 * that aggregate value.
 * 
 * @param hart - the hart to add time.
 * @param val - the amount of additional time (beyond MTIME) to set.
 */
void clint_add_mtimecmp(unsigned int hart, unsigned long val);

/**
 * @brief Gets the current value of the MTIME register.
 * 
 * @return unsigned long - the absolute value of the MTIME register.
 */
unsigned long clint_get_time();
