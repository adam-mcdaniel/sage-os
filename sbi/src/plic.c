/**
 * @file plic.c
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Platform-level Interrupt Controller (PLIC) routines.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <plic.h>

#ifdef USE_OLD_PLIC

#include <printf.h>
#include <uart.h>

// For S-mode, use PLIC_MODE_SUPERVISOR instead of PLIC_MODE_MACHINE

void plic_set_priority(int interrupt_id, char priority)
{
    uint32_t *base = (uint32_t *)PLIC_PRIORITY(interrupt_id);
    *base          = priority & 0x7;
}

void plic_set_threshold(int hart, char priority)
{
    uint32_t *base = (uint32_t *)PLIC_THRESHOLD(hart, PLIC_MODE_MACHINE);
    *base          = priority & 0x7;
}

void plic_enable(int hart, int interrupt_id)
{
    uint32_t *base = (uint32_t *)PLIC_ENABLE(hart, PLIC_MODE_MACHINE);
    base[interrupt_id / 32] |= 1UL << (interrupt_id % 32);
}

void plic_disable(int hart, int interrupt_id)
{
    uint32_t *base = (uint32_t *)PLIC_ENABLE(hart, PLIC_MODE_MACHINE);
    base[interrupt_id / 32] &= ~(1UL << (interrupt_id % 32));
}

uint32_t plic_claim(int hart)
{
    uint32_t *base = (uint32_t *)PLIC_CLAIM(hart, PLIC_MODE_MACHINE);
    return *base;
}

void plic_complete(int hart, int id)
{
    uint32_t *base = (uint32_t *)PLIC_CLAIM(hart, PLIC_MODE_MACHINE);
    *base          = id;
}

void plic_handle_irq(int hart)
{
    int irq = plic_claim(hart);
    // The only IRQ we really care about is 10, which comes from UART
    if (irq == 10) {
        uart_handle_irq();
    }
    plic_complete(hart, irq);
}
#endif
