#include <debug.h>
#include <virtio.h>
#include <util.h>
#include <vector.h>
#include <pci.h>
#include <kmalloc.h> 

static Vector *virtio_devices = NULL;

/**
 * @brief Initialize the virtio system
 */


void virtio_init(void) {
    virtio_devices = vector_new();
    uint64_t num_pci_devices = pci_count_saved_devices();
    
    for (uint64_t i = 0; i < num_pci_devices; ++i) {
        PCIDevice *device = pci_get_nth_saved_device(i);
        
        if (device->ecam_header->vendor_id == 0x1AF4) { // Access through ecam_header
            VirtioDevice *viodev = kmalloc(sizeof(VirtioDevice));
            viodev->pcidev = device;
            viodev->common_cfg = pci_get_virtio_common_config(device);
            viodev->notify_cap = pci_get_virtio_notify_capability(device);
            viodev->isr = pci_get_virtio_isr_status(device);
            // Fix qsize below
            uint16_t qsize = 128;
            // Allocate contiguous physical memory for descriptor table, driver ring, and device ring
            viodev->desc = (VirtioDescriptor *)kmalloc(VIRTIO_DESCRIPTOR_TABLE_BYTES(qsize));
            viodev->driver = (VirtioDriverRing *)kmalloc(VIRTIO_DRIVER_TABLE_BYTES(qsize));
            viodev->device = (VirtioDeviceRing *)kmalloc(VIRTIO_DEVICE_TABLE_BYTES(qsize));

            // Initialize the indices
            viodev->desc_idx = 0;
            viodev->driver_idx = 0;
            viodev->device_idx = 0;
            
            // Enable the queue
            viodev->common_cfg->queue_enable = 1;
            
            // Add to vector using vector_push
            vector_push(virtio_devices, (uint64_t)viodev);
        }
    }
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
