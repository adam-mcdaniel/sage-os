/*
*   Random Number Generator Driver
*/
#include <rng.h>
#include <pci.h>
#include <virtio.h>
#include <stdbool.h>
#include <debug.h>
#include <mmu.h>
#include <kmalloc.h>
#include <vector.h>

//use this like a queue
volatile static Vector *rng_active_jobs;
volatile static VirtioDevice *rng_device;

void rng_init() {
    rng_active_jobs = vector_new();
    rng_device = virtio_get_rng_device();
    debugf("RNG init done for device at %p\n", rng_device->pcidev->ecam_header);
    rng_device->ready = true;
}

//ask an rng device for numbers
void rng_get_numbers(void *buffer, uint16_t size, void* callback) {
    //select rng device 
    if (!virtio_is_rng_device(rng_device)) {
        fatalf("[RNG] Incorrect device provided\n");
    }

    //create a job
    struct RNGJob *job = kmalloc(sizeof(struct RNGJob));

    job->assigned_device = rng_device;
    job->callback = callback;
    job->output = buffer;
    
    vector_push(rng_active_jobs,job);

    //get random numbers
    if(!rng_fill(buffer ,size)) {
        logf(LOG_ERROR,"[RNG] Failed to generate random numbers\n");
    }
}

bool rng_fill(void *virtual_buffer_address, uint16_t size) {
    if (!virtio_is_rng_device(rng_device)) {
        fatalf("[RNG] Incorrect device provided\n");
    }

    if (rng_device->ready == false) {
        fatalf("RNG is not ready\n");
        return false;
    }

    uint64_t queue_size = rng_device->common_cfg->queue_size;
    debugf("Queue #%d with size 0x%x\n", rng_device->common_cfg->queue_select, rng_device->common_cfg->queue_size);
    uint64_t descriptor_index = rng_device->desc_idx;


    // All VirtIO devices deal with physical memory.
    // uint64_t virt = kzalloc(size);
    uint64_t physical_buffer_address = kernel_mmu_translate(virtual_buffer_address);

    // Fill all fields of the descriptor. Since we reuse descriptors
    // it's best to reset ALL fields.

    rng_device->desc[descriptor_index].addr = physical_buffer_address;
    rng_device->desc[descriptor_index].len = size;
    rng_device->desc[descriptor_index].flags = VIRTQ_DESC_F_WRITE;
    rng_device->desc[descriptor_index].next = 0;

    // Add the descriptor we just filled to the driver ring.
    rng_device->driver->ring[rng_device->driver->idx % queue_size] = descriptor_index;
    // As soon as we increment the index, it is "visible"
    rng_device->driver->idx++;
    rng_device->desc_idx = (rng_device->desc_idx + 1) % queue_size;
    // Write to the notify register to tell it to "do something"

    // Recall that the notification address is determined by the queue_notify_off
    // after you select the queue 0.

    // We select the notification address by multiplying the queue_notify_offset
    // by the queue notification multiplier from the notification PCI capability.
    // We then add this to the BAR offset from the capability's "offset" field.
    // Then, we add all of this to the base address.
    debugf("Notifying RNG...\n");
    virtio_notify(rng_device, descriptor_index);
    debugf("Done\n");
    return true;
}

void rng_job_done(VirtioDevice *vertdevice) {
    RNGJob *job;
    int idx;
    for(int i = 0; i < vector_size(rng_active_jobs);i++){
        vector_get_ptr(rng_active_jobs,i,job);
        if(job->assigned_device == vertdevice){
            vector_remove(rng_active_jobs, i);
            break;
        }
        else{
            job = NULL;
        }
    }

    if(job != NULL){
        //pop most recent job for the given device and send output to the callback function

        Callback callback_func = job->callback;
        callback_func(job->output);
    }
}
