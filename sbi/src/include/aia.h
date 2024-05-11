/**
 * @file aia.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Advanced Interrupt Architecture Routines
 * @version 0.1
 * @date 2023-03-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>


typedef enum AplicModes {
    APLIC_M_MODE = 0x0c000000,
    APLIC_S_MODE = 0x0d000000
} AplicModes;


/**
 * @brief Set the threshold of a given HART. The threshold is
 * a wall x, where any interrupt priority at or below x will be
 * stopped, and thus not heard by the HART. A threshold of 7
 * will mask all interrupts, and they will not be heard by
 * the HART. A threshold of 0 hears all interrupts except
 * those at priority 0.
 * 
 * @param hart the hart to set the threshold.
 * @param tresh the wall height [0..=7]
 */
void plic_set_threshold(int hart, char tresh);

/**
 * @brief Enable a given interrupt id.
 * 
 * @param hart the hart to listen to a given interrupt
 * @param interrupt_id the interrupt to listen to
 */
void plic_enable(int hart, int interrupt_id);

/**
 * @brief Disable a given interrupt id.
 * 
 * @param hart the hart to ignore a given interrupt
 * @param interrupt_id the interrupt to ignore
 */
void plic_disable(int hart, int interrupt_id);

/**
 * @brief Claim an external interrupt
 * 
 * @param hart the HART that is making the claim
 * @return uint32_t the IRQ identifier of the top priority IRQ
 */
uint32_t plic_claim(int hart);

/**
 * @brief Tell the PLIC we completed a claimed IRQ.
 * 
 * @param hart the HART that completed the IRQ
 * @param id the ID of the given IRQ. This should be the same
 * value as given by plic_claim().
 */
void plic_complete(int hart, int id);

/**
 * @brief Entry routine for ctrap when hearing a MEIP.
 * 
 * @param hart the hart that heard the external interrupt
 */
void plic_handle_irq(int hart);

typedef volatile void Aplic;
