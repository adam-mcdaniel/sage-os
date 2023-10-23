#include <debug.h>
#include <virtio.h>
#include <util.h>
#include <mmu.h>
#include <pci.h>
#include <kmalloc.h> 
#include <vector.h>
#include <config.h>
#include <csr.h>
#include <plic.h>
#include <sbi.h>
#include <sched.h>
#include <syscall.h>
#include <compiler.h>

#include "include/virtio.h"

static Vector *virtio_devices = NULL;

bool virtio_is_rng_device(VirtioDevice *dev) {
    return dev->pcidev->ecam_header->device_id == VIRTIO_PCI_DEVICE_ID(VIRTIO_PCI_DEVICE_ENTROPY);
}

VirtioDevice *virtio_get_rng_device() {
    for (uint16_t i=0; i<virtio_count_saved_devices(); i++) {
        VirtioDevice *dev = virtio_get_nth_saved_device(i);
        if (virtio_is_rng_device(dev)) {
            return dev;
        }
    }

    debugf("No RNG device could be found");
    return NULL;
}

VirtioDevice *virtio_get_nth_saved_device(uint16_t n) {
    VirtioDevice *result;
    vector_get_ptr(virtio_devices, n, &result);
    return result;
}

void virtio_save_device(VirtioDevice device) {
    VirtioDevice *mem = (VirtioDevice *)kzalloc(sizeof(VirtioDevice));
    memcpy(mem, &device, sizeof(VirtioDevice));
    vector_push_ptr(virtio_devices, mem);
}

volatile VirtioDevice *virtio_get_by_device(volatile PCIDevice *pcidevice) {
    for(int i = 0; i < vector_size(virtio_devices);i++){
        VirtioDevice *curr_virt_device;
        vector_get_ptr(virtio_devices,i,curr_virt_device);
        if(curr_virt_device->pcidev == pcidevice){
            return curr_virt_device;
        }
    }
    return NULL;
}

// Get the number of saved Virtio devices.
uint64_t virtio_count_saved_devices(void) {
    return vector_size(virtio_devices);
}

// Get a virtio capability for a given device by the virtio capability's type.
// If this is zero, it will get the common configuration capability. If this is
// one, it will get the notify capability. If this is two, it will get the ISR
// capability. Etc.
volatile struct VirtioCapability *virtio_get_capability(volatile VirtioDevice *dev, uint8_t type) {
    return pci_get_virtio_capability(dev->pcidev, type);
}

/**
 * @brief Initialize the virtio system
 */


void virtio_init(void) {
    debugf("virtio_init: Initializing virtio system...\n");
    // Initialize the vector of virtio devices
    virtio_devices = vector_new();

    // Get the number of PCI devices saved
    // This will allow us to iterate through all of them and find the virtio devices
    uint64_t num_pci_devices = pci_count_saved_devices();
    
    for (uint64_t i = 0; i < num_pci_devices; ++i) {
        // Get the PCI device
        volatile PCIDevice *pcidevice = pci_get_nth_saved_device(i);
        
        // Is this a virtio device?
        if (pci_is_virtio_device(pcidevice)) { // Access through ecam_header
            // Create a new bookkeeping structure for the virtio device
            volatile VirtioDevice viodev;
            // Add the PCI device to the bookkeeping structure
            viodev.pcidev = pcidevice;
            // Add the common configuration, notify capability, and ISR to the bookkeeping structure
            viodev.common_cfg = pci_get_virtio_common_config(pcidevice);
            viodev.notify_cap = pci_get_virtio_notify_capability(pcidevice);
            viodev.isr = pci_get_virtio_isr_status(pcidevice);

            debugf("Common config at 0x%08x\n", viodev.common_cfg);
            debugf("Notify config at 0x%08x\n", viodev.notify_cap);
            debugf("ISR config at 0x%08x\n", viodev.isr);

            debugf("Status: %x\n", viodev.common_cfg->device_status);
            viodev.common_cfg->device_status = VIRTIO_F_RESET;
            debugf("Status: %x\n", viodev.common_cfg->device_status);
            viodev.common_cfg->device_status = VIRTIO_F_ACKNOWLEDGE;
            debugf("Status: %x\n", viodev.common_cfg->device_status);
            viodev.common_cfg->device_status |= VIRTIO_F_DRIVER;
            debugf("Status: %x\n", viodev.common_cfg->device_status);
            viodev.common_cfg->device_status |= VIRTIO_F_FEATURES_OK;
            if (!(viodev.common_cfg->device_status & VIRTIO_F_FEATURES_OK)) {
                debugf("Device does not accept features\n");
            }
            
            // Fix qsize below
            viodev.common_cfg->queue_select = 0;
            uint16_t qsize = viodev.common_cfg->queue_size;
            debugf("Virtio device has queue size %d\n", qsize);

            // Allocate contiguous physical memory for descriptor table, driver ring, and device ring
            // These are virtual memory pointers that we will use in the OS side.
            viodev.desc = (VirtioDescriptor *)kzalloc(VIRTIO_DESCRIPTOR_TABLE_BYTES(qsize));
            viodev.driver = (VirtioDriverRing *)kzalloc(VIRTIO_DRIVER_TABLE_BYTES(qsize));
            viodev.device = (VirtioDeviceRing *)kzalloc(VIRTIO_DEVICE_TABLE_BYTES(qsize));
            debugf("Descriptor ring size: %d\n", VIRTIO_DESCRIPTOR_TABLE_BYTES(qsize));
            debugf("Driver ring size: %d\n", VIRTIO_DRIVER_TABLE_BYTES(qsize));
            debugf("Device ring size: %d\n", VIRTIO_DEVICE_TABLE_BYTES(qsize));

            // Initialize the indices
            viodev.desc_idx = 0;
            viodev.driver_idx = 0;
            viodev.device_idx = 0;

            // Add the physical addresses for the descriptor table, driver ring, and device ring to the common configuration
            // We translate the virtual addresses so the devices can actuall access the memory.
            void *phys_desc = kernel_mmu_translate(viodev.desc),
                *phys_driver = kernel_mmu_translate(viodev.driver),
                *phys_device = kernel_mmu_translate(viodev.device);
            viodev.common_cfg->queue_desc = phys_desc;
            viodev.common_cfg->queue_driver = phys_driver;
            viodev.common_cfg->queue_device = phys_device;
            debugf("virtio_init: queue_desc = 0x%08lx physical (0x%08lx virtual)\n", phys_desc, viodev.desc);
            debugf("virtio_init: queue_driver = 0x%08lx physical (0x%08lx virtual)\n", phys_driver, viodev.driver);
            debugf("virtio_init: queue_device = 0x%08lx physical (0x%08lx virtual)\n", phys_device, viodev.device);
            if (viodev.common_cfg->queue_desc != phys_desc) {
                debugf("Device does not reflect physical desc ring  @0x%08x (wrote %x but read %x)\n", &viodev.common_cfg->queue_desc, phys_desc, viodev.common_cfg->queue_desc);
            }
            if (viodev.common_cfg->queue_driver != phys_driver) {
                debugf("Device does not reflect physical driver ring@0x%08x (wrote %x but read %x)\n", &viodev.common_cfg->queue_driver, phys_driver, viodev.common_cfg->queue_driver);
            }
            if (viodev.common_cfg->queue_device != phys_device){
                debugf("Device does not reflect physical device ring@0x%08x (wrote %x but read %x)\n", &viodev.common_cfg->queue_device, phys_device, viodev.common_cfg->queue_device);
            }
            debugf("Set up tables for virtio device\n");
            viodev.common_cfg->queue_enable = 1;
            viodev.common_cfg->device_status |= VIRTIO_F_DRIVER_OK;
            // Add to vector using vector_push
            virtio_save_device(viodev);
        }
    }
    rng_init();
    debugf("virtio_init: Done initializing virtio system\n");
}


// Get the notify capability for the given virtio device.
volatile uint32_t *virtio_notify_register(volatile VirtioDevice *device) {
    // struct VirtioCapability *vio_cap = pci_get_virtio_capability(device->pcidev, VIRTIO_PCI_CAP_NOTIFY_CFG);
    // volatile VirtioPciNotifyCfg *notify_cap = pci_get_virtio_notify_capability(device->pcidev);
    uint8_t bar_num = device->notify_cap->cap.bar;
    uint64_t offset = device->notify_cap->cap.offset;
    uint16_t queue_notify_off = device->common_cfg->queue_notify_off;
    uint32_t notify_off_multiplier = device->notify_cap->notify_off_multiplier;
    uint64_t bar = (uint64_t)pci_get_device_bar(device->pcidev, bar_num);
    debugf("Notify cap bar=%d offset=%x, (len=%d)\n", bar_num, offset, device->notify_cap->cap.length);
    debugf("BAR at %x, offset=%x, queue_notify_off=%x, notify_off_mult=%x\n", bar, offset, queue_notify_off, notify_off_multiplier);

    uint32_t *notify = bar + BAR_NOTIFY_CAP(offset, queue_notify_off, notify_off_multiplier);
    return notify;
}

/**
 * @brief Virtio notification
 * @param viodev - virtio device to notify for
 * @param which_queue - queue number to notify
 */
void virtio_notify(volatile VirtioDevice *viodev, uint16_t which_queue)
{
    uint16_t num_queues = viodev->common_cfg->num_queues;

    if (which_queue >= num_queues) {
        logf(LOG_ERROR, "virtio_notify: Provided queue number is too big...\n");
        return;
    }

    // Select the queue we are looking at
    viodev->common_cfg->queue_select = which_queue;

    volatile uint16_t *notify_register = virtio_notify_register(viodev);
    debugf("Notifying at 0x%p...\n", notify_register);
    *notify_register = which_queue;
    debugf("Notified device\n\n");
}
