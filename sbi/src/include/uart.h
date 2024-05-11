/**
 * @file uart.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief UART Header
 * @version 0.1
 * @date 2022-05-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

// UART is connected at 0x1000_0000
#define UART_MMIO_BASE   0x10000000

/**
 * @brief Initialize the UART device. This is a virtual device,
 * but we at least want to read and write full 8-bit characters.
 * 
 */
void uart_init(void);

/**
 * @brief Get a single character from the receiver buffer.
 * 
 * @return char - the character in the buffer or 255 (0xFF or -1) if
 * the buffer is empty.
 */
char uart_get(void);

/**
 * @brief Put a single character onto the transmitter holding register (THR)
 * 
 * @param c The character to transmit
 */
void uart_put(char val);

/**
 * @brief Handle an interrupt request (IRQ) for the UART
 * from the PLIC.
 * 
 */
void uart_handle_irq(void);

/**
 * @brief Pop the first character off the receiver ring.
 * 
 * @return char - the character or 0xff, 255, or -1 if the receiver is empty.
 */
char uart_ring_pop(void);

/**
 * @brief Push a character onto the receiver ring.
 * 
 * @param c - the character to push
 */
void uart_ring_push(char c);
