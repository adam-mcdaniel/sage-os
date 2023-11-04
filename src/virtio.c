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
#include <block.h>
#include <rng.h>
#include <input.h>
#include <gpu.h>
#include <lock.h>

// #define VIRTIO_DEBUG

#ifdef VIRTIO_DEBUG
#define debugf(...) debugf(__VA_ARGS__)
#else
#define debugf(...)
#endif




static Vector *virtio_devices = NULL;

void virtio_create_job(VirtioDevice *dev, uint64_t pid_id, void (*callback)(struct VirtioDevice *device, struct Job *job)) {
    virtio_create_job_with_data(dev, pid_id, callback, NULL);
}

void virtio_create_job_with_data(VirtioDevice *dev, uint64_t pid_id, void (*callback)(struct VirtioDevice *device, struct Job *job), void *data) {
    Job job = job_create(virtio_get_next_job_id(dev), pid_id, callback);
    job.data = data;
    virtio_add_job(dev, job);
}

Job *virtio_get_job(VirtioDevice *dev, uint64_t job_id) {
    for (uint64_t i=0; i<vector_size(dev->jobs); i++) {
        Job *job = NULL;
        vector_get_ptr(dev->jobs, i, &job);
        if (job->job_id == job_id) {
            return job;
        }
    }
    debugf("No job found with ID %d\n", job_id);
    return NULL;
}

Job job_create(uint64_t job_id, uint64_t pid_id, void (*callback)(struct VirtioDevice *device, struct Job *job)) {
    return job_create_with_data(job_id, pid_id, callback, NULL);
}

Job job_create_with_data(uint64_t job_id, uint64_t pid_id, void (*callback)(struct VirtioDevice *device, struct Job *job), void *data) {
    Job job;
    job.job_id = job_id;
    job.pid_id = pid_id;
    job.callback = callback;
    job.done = false;
    job.data = data;
    return job;
}

void job_set_context(Job *job, VirtioDescriptor *desc, uint16_t num_descriptors) {
    if (job == NULL) {
        debugf("No job\n");
        return;
    }
    job->context.desc = desc;
    job->context.num_descriptors = num_descriptors;
}

void job_destroy(Job *job) {
    if (job->data != NULL) {
        debugf("About to free non-nulled Job data\n");
        kfree(job->data);
    }

    if (job != NULL) {
        kfree(job);
    }
}

void virtio_callback_and_free_job(VirtioDevice *dev, uint64_t job_id) {
    Job *job = virtio_get_job(dev, job_id);
    if (job == NULL) {
        debugf("No job\n");
        return;
    }
    job->done = true;
    if (job->callback != NULL) {
        job->callback(dev, job);
    } else {
        debugf("No callback for job\n");
    }

    job_destroy(job);
}

bool virtio_is_device_available(VirtioDevice *dev) {
    return dev->lock == MUTEX_UNLOCKED;
}

void virtio_acquire_device(VirtioDevice *dev) {
    IRQ_OFF();
    debugf("Acquiring device %p\n", dev);
    mutex_spinlock(&dev->lock);
}

void virtio_release_device(VirtioDevice *dev) {
    mutex_unlock(&dev->lock);
    debugf("Releasing device %p\n", dev);
    IRQ_ON();
}

void virtio_add_job(VirtioDevice *dev, Job job) {
    Job *mem = (Job *)kzalloc(sizeof(Job));
    memcpy(mem, &job, sizeof(Job));
    vector_push_ptr(dev->jobs, mem);
}

// Job *virtio_get_job(VirtioDevice *dev, uint64_t job_id) {
//     for (uint64_t i=0; i<vector_size(dev->jobs); i++) {
//         Job *job = NULL;
//         vector_get_ptr(dev->jobs, i, &job);
//         if (job->job_id == job_id) {
//             return job;
//         }
//     }
//     return NULL;
// }

void virtio_debug_job(VirtioDevice *dev, Job *job) {
    debugf("Device %p\n", dev);
    debugf("PCI device %p\n", dev->pcidev);
    debugf("ECAM Header %p\n", dev->pcidev->ecam_header);
    debugf("Job %d\n", job->job_id);
    debugf("PID %d\n", job->pid_id);
    debugf("Callback %p\n", job->callback);
    debugf("Done %d\n", job->done);
    debugf("Data %p\n", job->data);
    debugf("Context\n");
    debugf("  Num descriptors %d\n", job->context.num_descriptors);
    debugf("  Descriptors\n");
    for (uint16_t i=0; i<job->context.num_descriptors; i++) {
        debugf("    Descriptor %d\n", i);
        debugf("      addr: %p\n", job->context.desc[i].addr);
        debugf("      len: %d\n", job->context.desc[i].len);
        debugf("      flags: %d\n", job->context.desc[i].flags);
        debugf("      next: %d\n", job->context.desc[i].next);
    }
}

void virtio_handle_interrupt(VirtioDevice *dev, VirtioDescriptor desc[], uint16_t num_descriptors) {
    uint64_t job_id = virtio_which_job_from_interrupt(dev);
    if (job_id == -1ULL) {
        warnf("No job found matching interrupt\n");
        return;
    }

    job_set_context(virtio_get_job(dev, job_id), desc, num_descriptors);

    virtio_complete_job(dev, job_id);
}

uint64_t virtio_which_job_from_interrupt(VirtioDevice *dev) {
    // Get the ID of the job from the descriptor
    return (uint64_t)dev->device_idx - 1;
}

uint64_t virtio_get_job_id_by_index(VirtioDevice *dev, uint64_t index) {
    Job *job = NULL;
    if (index >= vector_size(dev->jobs)) {
        return -1ULL;
    }

    vector_get_ptr(dev->jobs, index, &job);
    return job->job_id;
}

uint64_t virtio_get_next_job_id(VirtioDevice *dev) {
    return (uint64_t)dev->device_idx;
}

void virtio_complete_job(VirtioDevice *dev, uint64_t job_id) {
    virtio_acquire_device(dev);
    Job *job = virtio_get_job(dev, job_id);
    if (job == NULL) {
        warnf("No job found with ID %d\n", job_id);
        virtio_release_device(dev);
        return;
    }

    if (job->done) {
        warnf("Job %d already done\n", job_id);
        virtio_release_device(dev);
        return;
    }
    virtio_callback_and_free_job(dev, job_id);
    if (job->done) {
        debugf("Job %d done\n", job_id);
        vector_remove_val_ptr(dev->jobs, job);
    } else {
        debugf("Job %d not done\n", job_id);
    }

    virtio_release_device(dev);
}

volatile struct VirtioBlockConfig *virtio_get_block_config(VirtioDevice *device) {
    return (volatile struct VirtioBlockConfig *)pci_get_device_specific_config(device->pcidev);
}

volatile struct VirtioInputConfig *virtio_get_input_config(VirtioDevice *device) {
    return (volatile struct VirtioInputConfig *)pci_get_device_specific_config(device->pcidev);
}

volatile struct VirtioGpuConfig *virtio_get_gpu_config(VirtioDevice *device) {
    return (volatile struct VirtioGpuConfig *)pci_get_device_specific_config(device->pcidev);
}

uint16_t virtio_get_device_id(VirtioDevice *dev) {
    return dev->pcidev->ecam_header->device_id;
}

bool virtio_is_rng_device(VirtioDevice *dev) {
    return virtio_get_device_id(dev) == VIRTIO_PCI_DEVICE_ID(VIRTIO_PCI_DEVICE_ENTROPY);
}

bool virtio_is_block_device(VirtioDevice *dev) {
    return virtio_get_device_id(dev) == VIRTIO_PCI_DEVICE_ID(VIRTIO_PCI_DEVICE_BLOCK);
}

bool virtio_is_input_device(VirtioDevice *dev) {
    return virtio_get_device_id(dev) == VIRTIO_PCI_DEVICE_ID(VIRTIO_PCI_DEVICE_INPUT);
}

bool virtio_is_gpu_device(VirtioDevice *dev) {
    return virtio_get_device_id(dev) == VIRTIO_PCI_DEVICE_ID(VIRTIO_PCI_DEVICE_GPU);
}

uint16_t virtio_get_queue_size(VirtioDevice *dev) {
    return dev->common_cfg->queue_size;
}

VirtioDevice *virtio_get_device(uint16_t device_type) {
    for (uint16_t i=0; i<virtio_count_saved_devices(); i++) {
        VirtioDevice *dev = virtio_get_nth_saved_device(i);
        if (virtio_get_device_id(dev) == VIRTIO_PCI_DEVICE_ID(device_type)) {
            return dev;
        }
    }

    warnf("No device could be found");
    return NULL;
}

VirtioDevice *virtio_get_rng_device(void) {
    return virtio_get_device(VIRTIO_PCI_DEVICE_ENTROPY);
}

VirtioDevice *virtio_get_block_device(void) {
    return virtio_get_device(VIRTIO_PCI_DEVICE_BLOCK);
}

VirtioDevice *virtio_get_input_device(void) {
    return virtio_get_device(VIRTIO_PCI_DEVICE_INPUT);
}

VirtioDevice *virtio_get_gpu_device(void) {
    return virtio_get_device(VIRTIO_PCI_DEVICE_GPU);
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

VirtioDevice *virtio_from_pci_device(PCIDevice *pcidevice) {
    for(uint32_t i = 0; i < vector_size(virtio_devices);i++){
        VirtioDevice *curr_virt_device = NULL;
        vector_get_ptr(virtio_devices, i, &curr_virt_device);
        if(curr_virt_device->pcidev == pcidevice) {
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
volatile struct VirtioCapability *virtio_get_capability(VirtioDevice *dev, uint8_t type) {
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
        PCIDevice *pcidevice = pci_get_nth_saved_device(i);
        
        // Is this a virtio device?
        if (pci_is_virtio_device(pcidevice)) { // Access through ecam_header

            // Create a new bookkeeping structure for the virtio device
            VirtioDevice viodev;

            // Add the PCI device to the bookkeeping structure
            viodev.pcidev = pcidevice;
            // Add the common configuration, notify capability, and ISR to the bookkeeping structure
            viodev.common_cfg = pci_get_virtio_common_config(pcidevice);
            viodev.notify_cap = pci_get_virtio_notify_capability(pcidevice);
            viodev.isr = pci_get_virtio_isr_status(pcidevice);
            if (virtio_is_rng_device(&viodev)) {
                debugf("Setting up RNG device\n");
            } else if (virtio_is_block_device(&viodev)) {
                debugf("Setting up block device\n");
            } else if (virtio_is_input_device(&viodev)) {
                debugf("Setting up input device\n");
            }

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
            if (viodev.common_cfg->device_feature & VIRTIO_F_IN_ORDER) {
                debugf("Device supports in-order\n");
                viodev.common_cfg->driver_feature |= VIRTIO_F_IN_ORDER;
            } else {
                debugf("Device does not support in-order\n");
            }
            viodev.common_cfg->device_status |= VIRTIO_F_FEATURES_OK;
            if (!(viodev.common_cfg->device_status & VIRTIO_F_FEATURES_OK)) {
                warnf("Device does not accept features\n");
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
            uint64_t phys_desc = kernel_mmu_translate((uint64_t)viodev.desc),
                phys_driver = kernel_mmu_translate((uint64_t)viodev.driver),
                phys_device = kernel_mmu_translate((uint64_t)viodev.device);
            viodev.common_cfg->queue_desc = phys_desc;
            viodev.common_cfg->queue_driver = phys_driver;
            viodev.common_cfg->queue_device = phys_device;
            debugf("virtio_init: queue_desc = 0x%08lx physical (0x%08lx virtual)\n", phys_desc, viodev.desc);
            debugf("virtio_init: queue_driver = 0x%08lx physical (0x%08lx virtual)\n", phys_driver, viodev.driver);
            debugf("virtio_init: queue_device = 0x%08lx physical (0x%08lx virtual)\n", phys_device, viodev.device);
            if (viodev.common_cfg->queue_desc != phys_desc) {
                warnf("Device does not reflect physical desc ring  @0x%08x (wrote %x but read %x)\n", &viodev.common_cfg->queue_desc, phys_desc, viodev.common_cfg->queue_desc);
            }
            if (viodev.common_cfg->queue_driver != phys_driver) {
                warnf("Device does not reflect physical driver ring@0x%08x (wrote %x but read %x)\n", &viodev.common_cfg->queue_driver, phys_driver, viodev.common_cfg->queue_driver);
            }
            if (viodev.common_cfg->queue_device != phys_device){
                warnf("Device does not reflect physical device ring@0x%08x (wrote %x but read %x)\n", &viodev.common_cfg->queue_device, phys_device, viodev.common_cfg->queue_device);
            }
            debugf("Set up tables for virtio device\n");
            viodev.common_cfg->queue_enable = 1;
            viodev.common_cfg->device_status |= VIRTIO_F_DRIVER_OK;
            viodev.device->flags = 0;
            viodev.lock = MUTEX_UNLOCKED;
            viodev.jobs = vector_new();
            virtio_set_device_name(&viodev, "Unknown Virtio Device");
            // Add to vector using vector_push
            virtio_save_device(viodev);
        }
    }
    rng_device_init();
    block_device_init();
    gpu_device_init();
    /*
    loop over every virtio device and initialize based on type
    */

    for (uint16_t i=0; i<virtio_count_saved_devices(); i++) {
        VirtioDevice *dev = virtio_get_nth_saved_device(i);
        if(virtio_get_device_id(dev) == VIRTIO_PCI_DEVICE_ID(VIRTIO_PCI_DEVICE_INPUT)){
            input_device_init(dev);
        }
    }
    for (uint16_t i=0; i<virtio_count_saved_devices(); i++) {
        VirtioDevice *dev = virtio_get_nth_saved_device(i);
        infof("Found device #%u: \"%s\"\n", i, virtio_get_device_name(dev));
    }
    debugf("virtio_init: Done initializing virtio system\n");
}

void virtio_set_device_name(VirtioDevice *dev, const char *name) {
    strncpy(dev->name, name, sizeof(dev->name));
}

const char *virtio_get_device_name(VirtioDevice *dev) {
    return dev->name;
}

// Get the notify capability for the given virtio device.
volatile uint16_t *virtio_notify_register(VirtioDevice *device) {
    // struct VirtioCapability *vio_cap = pci_get_virtio_capability(device->pcidev, VIRTIO_PCI_CAP_NOTIFY_CFG);
    // volatile VirtioPciNotifyCfg *notify_cap = pci_get_virtio_notify_capability(device->pcidev);
    uint8_t bar_num = device->notify_cap->cap.bar;
    uint64_t offset = device->notify_cap->cap.offset;
    uint16_t queue_notify_off = device->common_cfg->queue_notify_off;
    uint32_t notify_off_multiplier = device->notify_cap->notify_off_multiplier;
    uint64_t bar = (uint64_t)pci_get_device_bar(device->pcidev, bar_num);
    // debugf("Notify cap bar=%d offset=%x, (len=%d)\n", bar_num, offset, device->notify_cap->cap.length);
    // debugf("BAR at %x, offset=%x, queue_notify_off=%x, notify_off_mult=%x\n", bar, offset, queue_notify_off, notify_off_multiplier);

    uint16_t *notify = (uint16_t*)(bar + BAR_NOTIFY_CAP(offset, queue_notify_off, notify_off_multiplier));
    return notify;
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
        warnf("virtio_notify: Provided queue number %d is too big (num_queues=%d) for device %s\n", which_queue, num_queues, viodev->name);
        return;
    }

    // Select the queue we are looking at
    viodev->common_cfg->queue_select = which_queue;

    volatile uint16_t *notify_register = virtio_notify_register(viodev);
    // debugf("Notifying at 0x%p on instruction...\n", notify_register);
    *notify_register = which_queue;
    // debugf("Notified device\n\n");
}

// Select the queue and get its size
uint64_t virtio_set_queue_and_get_size(VirtioDevice *device, uint16_t which_queue) {
    if (device->common_cfg->queue_select != which_queue) {
        device->common_cfg->queue_select = which_queue;
    }
    uint16_t num_queues = device->common_cfg->num_queues;

    if (which_queue >= num_queues) {
        warnf("virtio_notify: Provided queue number %d is too big (num_queues=%d)...\n", which_queue, num_queues);
        return -1ULL;
    }

    return device->common_cfg->queue_size;
}

void virtio_send_one_descriptor(VirtioDevice *device, uint16_t which_queue, VirtioDescriptor descriptor, bool notify_device_when_done) {
    virtio_send_descriptor_chain(device, which_queue, &descriptor, 1, notify_device_when_done);
}


void virtio_send_descriptor_chain(VirtioDevice *device, uint16_t which_queue, VirtioDescriptor *descriptors, uint16_t num_descriptors, bool notify_device_when_done) {
    // Confirm the device is ready
    if (!device->ready) {
        fatalf("device is not ready\n");
        return;
    }

    virtio_acquire_device(device);

    // Select the queue we're using
    if (which_queue >= device->common_cfg->num_queues) {
        fatalf("queue number %d is too big (num_queues=%d)\n", which_queue, device->common_cfg->num_queues);
        return;
    }

    // The size of the queue we're using
    uint64_t queue_size = virtio_set_queue_and_get_size(device, which_queue);
    /*
    uint64_t    addr;
    uint32_t    len;
#define VIRTQ_DESC_F_NEXT 1
#define VIRTQ_DESC_F_WRITE 2
#define VIRTQ_DESC_F_INDIRECT 4
    uint16_t    flags;
    uint16_t    next;*/
    device->driver_idx = device->driver->idx;
    uint64_t head_descriptor_index = device->desc_idx;
    for (int i=0; i<num_descriptors; i++) {
        uint64_t descriptor_index = (device->desc_idx + i) % queue_size;
        // debugf("Writing descriptor %d to queue %d\n", descriptor_index, which_queue);
        VirtioDescriptor descriptor = descriptors[i];
        descriptor.next = (descriptor_index + 1) % queue_size;
        if (i < num_descriptors - 1) {
            descriptor.flags |= VIRTQ_DESC_F_NEXT;
        } else {
            descriptor.flags &= ~VIRTQ_DESC_F_NEXT;
        }
        // debugf("Descriptor addr: %p\n", descriptor.addr);
        // debugf("Descriptor len: 0x%x = %d\n", descriptor.len, descriptor.len);
        // debugf("Descriptor flags: 0x%x = %d\n", descriptor.flags, descriptor.flags);
        // debugf("Descriptor next: 0x%x = %d\n", descriptor.next, descriptor.next);
        // Put the descriptor in the descriptor table
        device->desc[descriptor_index] = descriptor;
    }
    // Put the descriptor into the driver ring
    device->driver->ring[device->driver->idx % queue_size] = head_descriptor_index;
    // Increment the index to make it "visible" to the device
    device->driver->idx++;
    // Update the descriptor index for our bookkeeping
    device->desc_idx = (device->desc_idx + num_descriptors) % queue_size;

    // debugf("Driver index: %d\n", device->driver->idx);
    // debugf("Descriptor index: %d\n", device->desc_idx);
    
    virtio_release_device(device);

    // Notify the device if we're ready to do so
    if (notify_device_when_done) {
        virtio_notify(device, which_queue);
    }
}


uint16_t virtio_receive_descriptor_chain(VirtioDevice *device, uint16_t which_queue, VirtioDescriptor *received, uint16_t max_descriptors, bool wait_for_descriptor) {
    uint64_t queue_size = virtio_set_queue_and_get_size(device, which_queue);
    if (wait_for_descriptor) {
        virtio_wait_for_descriptor(device, which_queue);
    }

    if (!virtio_has_received_descriptor(device, which_queue)) {
        warnf("No descriptor received\n");
        return 0;
    }
    

    // Get the descriptor index from the device ring
    uint64_t descriptor_index = device->device->ring[device->device_idx % queue_size].id;
    // Get the length of the descriptor
    volatile VirtioDescriptor *descriptor = (volatile VirtioDescriptor*)&device->desc[descriptor_index];

    uint16_t i = 0;
    while (descriptor->flags & VIRTQ_DESC_F_NEXT) {
        received[i++] = *descriptor;
        // debugf("Reading descriptor %d from queue %d\n", descriptor_index, which_queue);
        // debugf("Descriptor addr: %p\n", descriptor->addr);
        // debugf("Descriptor len: 0x%x = %d\n", descriptor->len, descriptor->len);
        // debugf("Descriptor flags: 0x%x = %d\n", descriptor->flags, descriptor->flags);
        // debugf("Descriptor next: 0x%x = %d\n", descriptor->next, descriptor->next);
        descriptor_index = descriptor->next;
        descriptor = (volatile VirtioDescriptor*)&device->desc[descriptor_index];
    }

    received[i] = *descriptor;
    // debugf("Reading descriptor %d from queue %d\n", i, which_queue);
    // debugf("Descriptor addr: %p\n", descriptor->addr);
    // debugf("Descriptor len: 0x%x = %d\n", descriptor->len, descriptor->len);
    // debugf("Descriptor flags: 0x%x = %d\n", descriptor->flags, descriptor->flags);
    // debugf("Descriptor next: 0x%x = %d\n", descriptor->next, descriptor->next);
    i++;
    device->device_idx = device->device->idx;
    if (i > max_descriptors) {
        warnf("Received %d descriptors, but expected %d or fewer\n", i, max_descriptors);
    }
    return i;
}

VirtioDescriptor virtio_receive_one_descriptor(VirtioDevice *device, uint16_t which_queue, bool wait_for_descriptor) {
    VirtioDescriptor received;
    received.addr = 0;
    received.flags = 0;
    received.len = 0;
    received.next = 0;
    virtio_receive_descriptor_chain(device, which_queue, &received, 1, wait_for_descriptor);
    return received;
}

bool virtio_has_received_descriptor(VirtioDevice *device, uint16_t which_queue) {
    virtio_set_queue_and_get_size(device, which_queue);
    if (device->device_idx == device->device->idx) {
        return false;
    }
    return true;
}

void virtio_wait_for_descriptor(VirtioDevice *device, uint16_t which_queue) {
    while (!virtio_has_received_descriptor(device, which_queue)) {
        debugf("Blocking on descriptor\n");
        // Do nothing
    }
}
