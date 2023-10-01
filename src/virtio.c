#include <debug.h>
#include <virtio.h>
#include <util.h>
#include <vector.h>
#include <pci.h>
#include <kmalloc.h> 
#include <vector.h>

static Vector *virtio_devices = NULL;

VirtioDevice *virtio_get_nth_saved_device(uint16_t n) {
    VirtioDevice *result;
    vector_get_ptr(virtio_devices, n, &result);
    return result;
}

void virtio_save_device(VirtioDevice device) {
    VirtioDevice *mem = (VirtioDevice *)kmalloc(sizeof(VirtioDevice));
    *mem = device;

    vector_push_ptr(virtio_devices, mem);
}

// Get the number of saved Virtio devices.
uint64_t virtio_count_saved_devices(void) {
    return vector_size(virtio_devices);
}

// Get a virtio capability for a given device by the virtio capability's type.
// If this is zero, it will get the common configuration capability. If this is
// one, it will get the notify capability. If this is two, it will get the ISR
// capability. Etc.
volatile struct VirtioCapability *virtio_get_capability(VirtioDevice *dev, uint8_t type) {
    return pci_get_virtio_capability(dev->pcidev, type);
}

/**
 * @brief Initialize the virtio system
 */


void virtio_init(void) {
    debugf("virtio_init: Initializing virtio system...\n");
    virtio_devices = vector_new();
    uint64_t num_pci_devices = pci_count_saved_devices();
    
    for (uint64_t i = 0; i < num_pci_devices; ++i) {
        PCIDevice *device = pci_get_nth_saved_device(i);
        
        if (device->ecam_header->vendor_id == 0x1AF4) { // Access through ecam_header
            VirtioDevice viodev;
            viodev.pcidev = device;
            viodev.common_cfg = pci_get_virtio_common_config(device);
            viodev.notify_cap = pci_get_virtio_notify_capability(device);
            viodev.isr = pci_get_virtio_isr_status(device);
            // Fix qsize below
            uint16_t qsize = 128;
            // Allocate contiguous physical memory for descriptor table, driver ring, and device ring
            viodev.desc = (VirtioDescriptor *)kmalloc(VIRTIO_DESCRIPTOR_TABLE_BYTES(qsize));
            viodev.driver = (VirtioDriverRing *)kmalloc(VIRTIO_DRIVER_TABLE_BYTES(qsize));
            viodev.device = (VirtioDeviceRing *)kmalloc(VIRTIO_DEVICE_TABLE_BYTES(qsize));

            // Initialize the indices
            viodev.desc_idx = 0;
            viodev.driver_idx = 0;
            viodev.device_idx = 0;
            
            // Comment until verify that this is correct
            viodev.common_cfg->queue_desc = (uint64_t)viodev.desc;
            viodev.common_cfg->queue_driver = (uint64_t)viodev.driver;
            viodev.common_cfg->queue_device = (uint64_t)viodev.device;
            viodev.common_cfg->queue_enable = 1;
            
            // Add to vector using vector_push
            virtio_save_device(viodev);
        }
    }
    debugf("virtio_init: Done initializing virtio system\n");
}

/**
 * @brief Virtio notification
 * @param viodev - virtio device to notify for
 * @param which_queue - queue number to notify
 */
void virtio_notify(VirtioDevice *viodev, uint16_t which_queue)
{
    uint16_t num_queues = viodev->common_cfg->num_queues;

    if (which_queue >= num_queues) {
        logf(LOG_ERROR, "virtio_notify: Provided queue number is too big...\n");
        return;
    }

    // Select the queue we are looking at
    viodev->common_cfg->queue_select = which_queue;

    // Determine offset for the given queue
    uint8_t bar_num = viodev->notify_cap->cap.bar;
    uint32_t offset = viodev->notify_cap->cap.offset;
    uint16_t queue_notify_off = viodev->common_cfg->queue_notify_off;
    uint32_t notify_off_multiplier = viodev->notify_cap->notify_off_multiplier;
    uint32_t bar = viodev->pcidev->ecam_header->type0.bar[bar_num];

    uint16_t *notify = bar + BAR_NOTIFY_CAP(offset, queue_notify_off, notify_off_multiplier);

    // Write the queue's number to the notify register
    *notify = which_queue; 

    return;
};
