/**
 * @file uart.c
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Universal Asynchronous Receiver/Transmitter (UART) routines.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <lock.h>
#include <uart.h>

#define UART_MAX_BUFFER_SIZE 128

static Mutex uart_mutex;

// The following three fields are needed for uart_ring_push and uart_ring_pop.
static char uart_buffer[UART_MAX_BUFFER_SIZE];
static int uart_buffer_size     = 0;
static int uart_buffer_start    = 0;

static volatile char *UART_MMIO = (char *)UART_MMIO_BASE;

void uart_init(void)
{
    UART_MMIO[3] = 3;  // Word length select bits set to 11 (3)
    UART_MMIO[2] = 1;  // FIFO enabled
    UART_MMIO[1] = 1;  // Interrupts enabled
}

char uart_get(void)
{
	// Check if the transmitter can take more data.
	// UART_MMIO[5] is the line status register, and bit index
	// 0 is the transmitter empty (TEMT) bit.
    if (UART_MMIO[5] & 1) {
		// Register UART_MMIO[0] is the transmitter holding register (THR)
		// only when we read from it.
        return UART_MMIO[0];
    }
    return 255;
}

void uart_put(char c)
{
	// Busy loop waiting for data
	// UART_MMIO[5] is the line status register, and bit index
	// 6 is the Data Ready bit.
    while (!(UART_MMIO[5] & (1 << 6)))
        ;
	// Register UART_MMIO[0] is the receiver buffer register RBR when
	// we write to it.
    UART_MMIO[0] = c;
}

// UART Ring Buffer Operations

char uart_ring_pop(void)
{
    char ret = 0xff;
    mutex_spinlock(&uart_mutex);
    if (uart_buffer_size > 0) {
        ret               = uart_buffer[uart_buffer_start];
        uart_buffer_start = (uart_buffer_start + 1) % UART_MAX_BUFFER_SIZE;
        uart_buffer_size -= 1;
    }
    mutex_unlock(&uart_mutex);
    return ret;
}

void uart_ring_push(char c)
{
    mutex_spinlock(&uart_mutex);
    if (uart_buffer_size < UART_MAX_BUFFER_SIZE) {
        uart_buffer[(uart_buffer_start + uart_buffer_size) % UART_MAX_BUFFER_SIZE] = c;
        uart_buffer_size += 1;
    }
    mutex_unlock(&uart_mutex);
}

void uart_handle_irq(void)
{
    char c;
    // We use a while loop to drain the FIFO before returning. Otherwise,
    // we would get interrupted for every element in the FIFO.
    while ((c = uart_get()) != 0xff) {
        uart_ring_push(c);
    }
}
