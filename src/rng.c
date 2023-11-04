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
#include <csr.h>

//use this like a queue
static Vector *rng_active_jobs;
static VirtioDevice *rng_device;

void rng_device_init() {
    rng_active_jobs = vector_new();
    rng_device = virtio_get_rng_device();
    debugf("RNG init done for device at %p\n", rng_device->pcidev->ecam_header);
    virtio_set_device_name(rng_device, "RNG Device");
    rng_device->ready = true;
}


void rng_fill(void *virtual_buffer_address, uint16_t size) {
    if (!virtio_is_rng_device(rng_device)) {
        fatalf("[RNG] Incorrect device provided\n");
    }

    if (!rng_device->ready) {
        fatalf("RNG is not ready\n");
        return;
    }

    VirtioDescriptor desc;
    desc.addr = kernel_mmu_translate((uintptr_t)virtual_buffer_address);
    desc.len = size;
    desc.flags = VIRTQ_DESC_F_WRITE;
    desc.next = 0;

    virtio_send_one_descriptor(rng_device, 0, desc, true);
}
