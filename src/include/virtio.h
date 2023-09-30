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
    // Which BAR is this for?
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
typedef struct VirtioPciNotifyCap {
    // The actual memory address we write to notify a queue
    // is given by the **cap** portion of this structure. In there,
    // we retrieve the BAR + the offset.
    VirtioCapability cap;
    // Then, we multiply the `queue_notify_off` by the `notify_off_multiplier`.
    uint32_t notify_off_multiplier; /* Multiplier for queue_notify_off. */
} VirtioPciNotifyCap;
#define BAR_NOTIFY_CAP(offset, queue_notify_off, notify_off_multiplier) \
    ((offset) + (queue_notify_off) * (notify_off_multiplier))

// Interrupt Service Routine: Type 3 configuration
#define VIRTIO_PCI_CAP_ISR_CFG 3
// The ISR notifies us that this particular device caused an interrupt.
// The ISR has this layout.
typedef struct VirtioPciIsrCap {
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
} VirtioPciIsrCap;

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

struct List;

// This is the actual Virtio device structure that the OS will
// keep track of for each device. It contains the data for the OS
// to quickly access vital information for the device.
typedef struct VirtioDevice {
    // A pointer the PCI device structure for this Virtio device.
    // Right now this just points to the ECAM for the device.
    struct PCIDevice *pcidev;
    // The common configuration for the device.
    volatile VirtioPciCommonCfg *common_cfg;
    // The notify register for the device. This is not the notify
    // structure; we go ahead and do the address calculation before
    // storing the pointer here.
    volatile char *notify;
    // The ISR capability for the device.
    volatile VirtioPciIsrCap *isr;

    // The descriptor ring for the device.
    volatile VirtioDescriptor *desc;
    // The driver ring for the device.
    volatile VirtioDriverRing *driver;
    // The device ring for the device.
    volatile VirtioDeviceRing *device;

    void *priv;
    struct List *jobs;

    uint16_t desc_idx;
    uint16_t driver_idx;
    uint16_t device_idx;

    uint16_t notifymult;
    bool     ready;
} VirtioDevice;

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
