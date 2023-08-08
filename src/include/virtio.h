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

typedef struct VirtioCapability {
    uint8_t id;
    uint8_t next;
    uint8_t len;
    uint8_t type;
    uint8_t bar;
    uint8_t pad[3];
    uint32_t offset;
    uint32_t length;
} VirtioCapability;

#define VIRTIO_CAP(x)             ((VirtioCapability *)(x))

// Type 1 configuration
#define VIRTIO_PCI_CAP_COMMON_CFG 1
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
    uint16_t queue_select;      /* read-write */
    uint16_t queue_size;        /* read-write */
    uint16_t queue_msix_vector; /* read-write */
    uint16_t queue_enable;      /* read-write */
    uint16_t queue_notify_off;  /* read-only for driver */
    uint64_t queue_desc;        /* read-write */
    uint64_t queue_driver;      /* read-write */
    uint64_t queue_device;      /* read-write */
} VirtioPciCommonCfg;

// Type 2 configuration
#define VIRTIO_PCI_CAP_NOTIFY_CFG 2
typedef struct VirtioPciNotifyCap {
    VirtioCapability cap;
    uint32_t notify_off_multiplier; /* Multiplier for queue_notify_off. */
} VirtioPciNotifyCap;
#define BAR_NOTIFY_CAP(offset, queue_notify_off, notify_off_multiplier) \
    ((offset) + (queue_notify_off) * (notify_off_multiplier))

// Type 3 configuration
#define VIRTIO_PCI_CAP_ISR_CFG 3
typedef struct VirtioPciIsrCap {
    union {
        struct {
            unsigned queue_interrupt : 1;
            unsigned device_cfg_interrupt : 1;
            unsigned reserved : 30;
        };
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

struct PciDevice;
struct List;
typedef struct VirtioDevice {
    struct PciDevice *pcidev;
    volatile VirtioPciCommonCfg *common_cfg;
    volatile char *notify;
    volatile VirtioPciIsrCap *isr;

    volatile VirtioDescriptor *desc;
    volatile VirtioDriverRing *driver;
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
