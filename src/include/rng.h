/*
* Random Number Generator Driver header
*/
#pragma once

#include <stdint.h>
#include <pci.h>
#include <virtio.h>
#include <stdbool.h>
#include <debug.h>
#include <mmu.h>
#include <kmalloc.h>

void rng_init(void);

//tells the number generator to fill a buffer with numbers
bool rng_fill(void *buffer, uint16_t size);
