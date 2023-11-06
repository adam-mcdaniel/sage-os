/**
 * @file virtio.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Virtio subsystem.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include <lock.h>
#include <stdbool.h>
#include <stdint.h>
#include <pci.h>

#define VIRTIO_PCI_VENDOR_ID      0x1AF4
#define VIRTIO_PCI_DEVICE_ID(x)   (0x1040 + (x))

#define VIRTIO_PCI_DEVICE_NETWORK 1
#define VIRTIO_PCI_DEVICE_BLOCK   2
#define VIRTIO_PCI_DEVICE_CONSOLE 3
#define VIRTIO_PCI_DEVICE_ENTROPY 4
#define VIRTIO_PCI_DEVICE_WIFI    10
#define VIRTIO_PCI_DEVICE_GPU     16
#define VIRTIO_PCI_DEVICE_INPUT   18

#define VIRTIO_CAP_VENDOR         9

// Vendor specific capability for a Virtio device
typedef struct VirtioCapability {
    // Generic PCI field: PCI_CAP_ID_VNDR
    uint8_t id;
    // Generic PCI field: next ptr
    uint8_t next;
    // Generic PCI field: capability length
    uint8_t len;
    // What type of capability is this? (identifies the structure)
    // This will be one of 1, 2, 3, 4, or 5, but we only care about 1, 2, or 3
    uint8_t type;
    // Which BAR is this for? (0x0 - 0x5)
    uint8_t bar;
    // Padding to fill the next 3 bytes
    uint8_t pad[3];
    // The offset within the BAR
    uint32_t offset;
    // The length of the capability in bytes
    uint32_t length;
} VirtioCapability;

#define VIRTIO_CAP(x)             ((VirtioCapability *)(x))

// Common Configuration: Type 1 configuration
#define VIRTIO_PCI_CAP_COMMON_CFG 1
// The type 1 configuration is located in the BAR address space
// specified in the capability plus the offset.
// There, this structure is located.
typedef struct VirtioPciCommonCfg {
    uint32_t device_feature_select; /* read-write */
    uint32_t device_feature;        /* read-only for driver */
    uint32_t driver_feature_select; /* read-write */
    uint32_t driver_feature;        /* read-write */
    uint16_t msix_config;           /* read-write */
    uint16_t num_queues;            /* read-only for driver */
    uint8_t device_status;          /* read-write */
    uint8_t config_generation;      /* read-only for driver */
    /* About a specific virtqueue. */
    // Set this to select the queue to work with.
    // The rest of the fields *will update to reflect the selected queue*.
    // This must be less than num_queues.
    uint16_t queue_select;      /* read-write */
    // The negotiated queue size.
    uint16_t queue_size;        /* read-write */
    uint16_t queue_msix_vector; /* read-write */
    uint16_t queue_enable;      /* read-write */
    uint16_t queue_notify_off;  /* read-only for driver */
    // A physical memory address which points to the top
    // of the descriptor table allocated for this selected queue.
    uint64_t queue_desc;        /* read-write */
    // A physical memory address which points to the top
    // of the driver ring allocated for this selected queue.
    uint64_t queue_driver;      /* read-write */
    // A physical memory address which points to the top
    // of the device ring allocated for this selected queue.
    uint64_t queue_device;      /* read-write */
} VirtioPciCommonCfg;

// Notify Configuration: Type 2 configuration
#define VIRTIO_PCI_CAP_NOTIFY_CFG 2
// A notify register is how we tell the device to "go and do"
// what we asked it to do.
typedef struct VirtioPciNotifyCfg {
    VirtioCapability cap;
    // The actual memory address we write to notify a queue
    // is given by the **cap** portion of this structure. In there,
    // we retrieve the BAR + the offset.
    // Then, we multiply the `queue_notify_off` by the `notify_off_multiplier`.
    uint32_t notify_off_multiplier; /* Multiplier for queue_notify_off. */
} VirtioPciNotifyCfg;
#define BAR_NOTIFY_CAP(offset, queue_notify_off, notify_off_multiplier) \
    ((offset) + (queue_notify_off) * (notify_off_multiplier))

// Interrupt Service Routine: Type 3 configuration
#define VIRTIO_PCI_CAP_ISR_CFG 3
// The ISR notifies us that this particular device caused an interrupt.
// The ISR has this layout.
typedef struct VirtioPciIsrCfg {
    union {
        struct {
            // If we read `queue_interrupt` and it is 1, then
            // we know that the interrupt was caused by the device.
            unsigned queue_interrupt : 1;
            // If we read `device_cfg_interrupt` and it is 1, then
            // we know that the device has changed its configuration,
            // and we should reinitialize the device.
            unsigned device_cfg_interrupt : 1;
            // The rest of the bits are reserved.
            unsigned reserved : 30;
        };
        // We can also read the entire register as a 32-bit unsigned
        // integer.
        unsigned int isr_cap;
    };
} VirtioPciIsrCfg;

#define VIRTIO_PCI_CAP_DEVICE_CFG 4
#define VIRTIO_PCI_CAP_PCI_CFG    5

typedef struct VirtioDescriptor {
    uint64_t    addr;
    uint32_t    len;
#define VIRTQ_DESC_F_NEXT 1
#define VIRTQ_DESC_F_WRITE 2
#define VIRTQ_DESC_F_INDIRECT 4
    uint16_t    flags;
    uint16_t    next;
} VirtioDescriptor;

typedef struct VirtioDriverRing {
#define VIRTQ_DRIVER_F_NO_INTERRUPT 1
    uint16_t    flags;
    uint16_t    idx;
    uint16_t    ring[];
    // uint16_t     used_event;
} VirtioDriverRing;

typedef struct VirtioDeviceRingElem {
    uint32_t    id;
    uint32_t    len;
} VirtioDeviceRingElem;

typedef struct VirtioDeviceRing {
#define VIRTQ_DEVICE_F_NO_NOTIFY 1
    uint16_t    flags;
    uint16_t    idx;
    VirtioDeviceRingElem ring[];
    // uint16_t     avail_event;
} VirtioDeviceRing;

struct VirtioDevice;

typedef struct Job {
    uint64_t job_id;
    uint64_t pid_id;

    bool done;
    struct Context {
        VirtioDescriptor *desc;
        uint16_t num_descriptors;
    } context;

    void (*callback)(struct VirtioDevice *device, struct Job *job);
    void *data;
} Job;

Job job_create(uint64_t job_id, uint64_t pid_id, void (*callback)(struct VirtioDevice *device, struct Job *job));
Job job_create_with_data(uint64_t job_id, uint64_t pid_id, void (*callback)(struct VirtioDevice *device, struct Job *job), void *data);
// void job_set_context(Job *job, VirtioDescriptor *desc, uint16_t num_descriptors);
void job_destroy(Job *job);

// This is the actual Virtio device structure that the OS will
// keep track of for each device. It contains the data for the OS
// to quickly access vital information for the device.
typedef struct VirtioDevice {
    // The name of the device, if any.
    char name[64];

    // A pointer the PCIDevice bookkeeping structure.
    // This is used by the OS to track useful information about
    // the PCIDevice, and as a useful interface for configuring
    // the device.
    struct PCIDevice *pcidev;
    // The common configuration for the device.
    volatile struct VirtioPciCommonCfg *common_cfg;
    // The notify configuration for the device.
    volatile struct VirtioPciNotifyCfg *notify_cap;
    // volatile uint16_t *notify;
    volatile struct VirtioPciIsrCfg *isr;

    // The descriptor ring for the device.
    volatile VirtioDescriptor *desc;
    // The driver ring for the device.
    volatile VirtioDriverRing *driver;
    // The device ring for the device.
    volatile VirtioDeviceRing *device;

    void *priv;
    struct Vector *jobs;

    uint16_t desc_idx;
    uint16_t driver_idx;
    uint16_t device_idx;

    bool ready;
    Mutex lock;

    // Input device identification
    struct {
        unsigned is_keyboard : 1;
        unsigned is_tablet : 1;
    };
} VirtioDevice;

// Read the queue size from the common configuration.
uint16_t virtio_get_queue_size(VirtioDevice *dev);

// Set the name of the device
void virtio_set_device_name(VirtioDevice *dev, const char *name);
// Get the name of the device
const char *virtio_get_device_name(VirtioDevice *dev);

void virtio_debug_job(VirtioDevice *dev, Job *job);
// Use this to create a job with no state saved
void virtio_create_job(VirtioDevice *dev, uint64_t pid_id, void (*callback)(struct VirtioDevice *device, struct Job *job));
// Use this to create a job with state that will be saved for when the job is called
void virtio_create_job_with_data(VirtioDevice *dev, uint64_t pid_id, void (*callback)(struct VirtioDevice *device, struct Job *job), void *data);

// Performed by virtio_handle_interrupt
void virtio_callback_and_free_job(VirtioDevice *dev, uint64_t job_id);

// Is the device free to be acquired?
bool virtio_is_device_available(VirtioDevice *dev);
// Performed by virtio_handle_interrupt
void virtio_acquire_device(VirtioDevice *dev);
// Performed by virtio_handle_interrupt
void virtio_release_device(VirtioDevice *dev);

// Get the next scheduled job ID
uint64_t virtio_get_next_job_id(VirtioDevice *dev);
// Get a job's ID from its index in the scheduled jobs
uint64_t virtio_get_job_id_by_index(VirtioDevice *dev, uint64_t index);

// Add a job to the list of jobs in the virtio device (done with virtio_job_create)
void virtio_add_job(VirtioDevice *dev, Job job);
// Get a job from its ID
Job *virtio_get_job(VirtioDevice *dev, uint64_t job_id);
// Call the device's callback and destroy the job if it is done
void virtio_complete_job(VirtioDevice *dev, uint64_t job_id);
// Which job ID does this interrupt correspond to
uint64_t virtio_which_job_from_interrupt(VirtioDevice *dev);

// Handle the job from an interrupt
void virtio_handle_interrupt(VirtioDevice *dev, VirtioDescriptor desc[], uint16_t num_descriptors);

#define VIRTIO_F_RESET         0
#define VIRTIO_F_ACKNOWLEDGE  (1 << 0)
#define VIRTIO_F_DRIVER       (1 << 1)
#define VIRTIO_F_DRIVER_OK    (1 << 2)
#define VIRTIO_F_FEATURES_OK  (1 << 3)

#define VIRTIO_F_RING_PACKED  34
#define VIRTIO_F_IN_ORDER     35


#define VIRTIO_DESCRIPTOR_TABLE_BYTES(qsize)   (16 * (qsize))
#define VIRTIO_DRIVER_TABLE_BYTES(qsize)       (6 + 2 * (qsize))
#define VIRTIO_DEVICE_TABLE_BYTES(qsize)       (6 + 8 * (qsize))

void virtio_init(void);
void virtio_notify(VirtioDevice *viodev, uint16_t which_queue);

// Find a saved device by its index.
VirtioDevice *virtio_get_nth_saved_device(uint16_t n);

// Get the RNG device from the list of virtio devices.
VirtioDevice *virtio_get_rng_device();
// Get the Block device from the list of virtio devices.
VirtioDevice *virtio_get_block_device(uint16_t n);
// Get the Input device from the list of virtio devices.
VirtioDevice *virtio_get_input_device(uint16_t n);
// Get the GPU device from the list of virtio devices.
VirtioDevice *virtio_get_gpu_device();

// Get the device ID from the given VirtioDevice
uint16_t virtio_get_device_id(VirtioDevice *dev);
// Is this an RNG device?
bool virtio_is_rng_device(VirtioDevice *dev);
// Is this an block device?
bool virtio_is_block_device(VirtioDevice *dev);
// Is this an INPUT device?
bool virtio_is_input_device(VirtioDevice *dev);
// Is this an GPU device?
bool virtio_is_gpu_device(VirtioDevice *dev);

// Save the Virtio device for later use.
void virtio_save_device(VirtioDevice device);

// Get the number of saved Virtio devices.
uint64_t virtio_count_saved_devices(void);

// Get a virtio capability for a given device by the virtio capability's type.
// If this is zero, it will get the common configuration capability. If this is
// one, it will get the notify capability. If this is two, it will get the ISR
// capability. Etc.
volatile VirtioCapability *virtio_get_capability(VirtioDevice *dev, uint8_t type);

//get a virtio device by using a pcidevice pointer
VirtioDevice *virtio_from_pci_device(PCIDevice *pcidevice);

// Get the pointer to the virtio-device's notify-register
volatile uint16_t *virtio_notify_register(VirtioDevice *device);

uint16_t virtio_set_queue_and_get_size(VirtioDevice *device, uint16_t which_queue);

// Send exactly one descriptor to the given device's queue, and optionally notify the device when done.
// The descriptor must contain a physical address.
void virtio_send_one_descriptor(VirtioDevice *device, uint16_t which_queue, VirtioDescriptor descriptor, bool notify_device_when_done);

// Receive exactly one descriptor from the virtio-device's queue, and optionally block for it.
// If we do not block, this function will bail out and all of the descriptor data will be zero'd.
// The descriptor will contain a physical address.
VirtioDescriptor virtio_receive_one_descriptor(VirtioDevice *device, uint16_t which_queue, bool wait_for_descriptor);

// This checks if a device has sent us a descriptor on a given queue, ready to be received.
bool virtio_has_received_descriptor(VirtioDevice *device, uint16_t which_queue);

// Receive a chain of descriptors from the virtio-device's queue. This will write to the `descriptors` parameter.
// This gives you back physical addresses in the descriptors.
uint16_t virtio_receive_descriptor_chain(VirtioDevice *device, uint16_t which_queue, VirtioDescriptor *descriptors, uint16_t num_descriptors, bool wait_for_descriptor);

// Send an array of descriptors to a given virtio-device's queue, and optionally notify it when finished. This will automatically set the `next`
// and VIRTIO_F_NEXT field of the `flag` bits of the descriptors to setup the chain. These descriptors must use physical addresses.
void virtio_send_descriptor_chain(VirtioDevice *device, uint16_t which_queue, VirtioDescriptor *descriptors, uint16_t num_descriptors, bool notify_device_when_done);

// Wait for the given device's queue to update with a descriptor.
void virtio_wait_for_descriptor(VirtioDevice *device, uint16_t which_queue);

typedef struct VirtioBlockConfig {
   uint64_t capacity;
   uint32_t size_max;
   uint32_t seg_max;
   struct VirtioBlockGeometry {
      uint16_t cylinders;
      uint8_t heads;
      uint8_t sectors;
   } geometry;
   uint32_t blk_size; // the size of a sector, usually 512
   struct VirtioBlockTopology {
      // # of logical blocks per physical block (log2)
      uint8_t physical_block_exp;
      // offset of first aligned logical block
      uint8_t alignment_offset;
      // suggested minimum I/O size in blocks
      uint16_t min_io_size;
      // optimal (suggested maximum) I/O size in blocks
      uint32_t opt_io_size;
   } topology;
   uint8_t writeback;
   uint8_t unused0[3];
   uint32_t max_discard_sectors;
   uint32_t max_discard_seg;
   uint32_t discard_sector_alignment;
   uint32_t max_write_zeroes_sectors;
   uint32_t max_write_zeroes_seg;
   uint8_t write_zeroes_may_unmap;
   uint8_t unused1[3];
} VirtioBlockConfig;

typedef struct virtio_input_absinfo {
    uint32_t min;
    uint32_t max;
    uint32_t fuzz;
    uint32_t flat;
    uint32_t res;
} InputAbsInfo;

typedef struct virtio_input_devids {
    uint16_t bustype;
    uint16_t vendor;
    uint16_t product;
    uint16_t version;
} InputDevIds;

typedef struct VirtioInputConfig {
    uint8_t select;
    uint8_t subsel;
    uint8_t size;
    uint8_t reserved[5];
    union {
        char string[128];
        uint8_t bitmap[128];
        struct virtio_input_absinfo abs;
        struct virtio_input_devids ids;
    };
} VirtioInputConfig;

volatile struct VirtioBlockConfig *virtio_get_block_config(VirtioDevice *device);
volatile struct VirtioGpuConfig *virtio_get_gpu_config(VirtioDevice *device);
volatile struct VirtioInputConfig *virtio_get_input_config(VirtioDevice *device);
