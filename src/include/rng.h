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

//tells the number generator to fill a buffer with numbers
void rng_get_numbers(struct VirtioDevice *viodev, void *buffer, uint16_t size, void* callback);

bool rng_fill(void *buffer, uint16_t size, struct VirtioDevice *rng_device);

//TODO print values
void rng_print();

void rng_job_done(VirtioDevice *vertdevice);

typedef void (*Callback)(void*);

typedef struct RNGJob{
    VirtioDevice *assigned_device;   //virtio device performing job
    Callback *callback;                 //pointer to callback function
    void *output;                   //location of written bytes
}RNGJob;