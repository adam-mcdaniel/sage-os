/**
 * @file clint.c
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Core local interrupt (CLINT) routines.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <clint.h>
#include <config.h>

void clint_set_msip(unsigned int hart)
{
    if (hart >= MAX_ALLOWABLE_HARTS)
        return;

    unsigned int *clint = (unsigned int *)CLINT_BASE_ADDRESS;
    clint[hart]         = 1;
}

void clint_clear_msip(unsigned int hart)
{
    if (hart >= MAX_ALLOWABLE_HARTS)
        return;

    unsigned int *clint = (unsigned int *)CLINT_BASE_ADDRESS;
    clint[hart]         = 0;
}

void clint_set_mtimecmp(unsigned int hart, unsigned long val)
{
    if (hart >= MAX_ALLOWABLE_HARTS)
        return;

    ((unsigned long *)(CLINT_BASE_ADDRESS + CLINT_MTIMECMP_OFFSET))[hart] = val;
}

void clint_add_mtimecmp(unsigned int hart, unsigned long val)
{
    clint_set_mtimecmp(hart, clint_get_time() + val);
}

unsigned long clint_get_time()
{
    unsigned long tm;
    __asm__ volatile("rdtime %0" : "=r"(tm));
    return tm;
}
