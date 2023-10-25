#include <block.h>
#include "debug.h"
#include "mmu.h"
#include "pci.h"
#include "virtio.h"
#include "page.h"
#include "kmalloc.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

struct block_device {
    volatile VirtioDevice *vdev;
    struct VirtioBlkConfig config;
    uint8_t status;
};


struct block_device *global_blk_dev = NULL;

// Handle Block Device Operations
int virtio_setup_block_request(volatile VirtioDevice *vdev, struct BlockHeader *header, void *buffer, size_t size, int type) {
    debugf("Setting up block request: type=%d, buffer=%p, size=%lu\n", type, buffer, size);
    uint64_t queue_size = vdev->common_cfg->queue_size;
    uint64_t descriptor_index = vdev->desc_idx;

    // Convert virtual address to physical address
    uintptr_t phys_addr = mmu_translate(kernel_mmu_table, (uintptr_t)buffer);
    if (phys_addr == 0) {
        return -1; // Failed to translate address
    }

    // Setup header descriptor
    vdev->desc[descriptor_index].addr = (uint64_t)header;
    vdev->desc[descriptor_index].len = sizeof(struct BlockHeader);
    vdev->desc[descriptor_index].flags = VIRTQ_DESC_F_NEXT;
    vdev->desc[descriptor_index].next = (descriptor_index + 1) % queue_size;

    // Setup data descriptor
    descriptor_index = (descriptor_index + 1) % queue_size;
    vdev->desc[descriptor_index].addr = phys_addr;
    vdev->desc[descriptor_index].len = size;
    vdev->desc[descriptor_index].flags = VIRTQ_DESC_F_NEXT | (type == VIRTIO_BLK_T_IN ? VIRTQ_DESC_F_WRITE : 0);
    vdev->desc[descriptor_index].next = (descriptor_index + 1) % queue_size;

    // Setup status descriptor
    descriptor_index = (descriptor_index + 1) % queue_size;
    vdev->desc[descriptor_index].addr = (uint64_t)&global_blk_dev->status;
    vdev->desc[descriptor_index].len = sizeof(uint8_t);
    vdev->desc[descriptor_index].flags = VIRTQ_DESC_F_WRITE;

    // Set the descriptor index in the driver ring
    vdev->driver->ring[vdev->driver->idx % queue_size] = vdev->desc_idx;
    vdev->driver->idx += 1;

    // Update the descriptor index for next use
    vdev->desc_idx = (descriptor_index + 1) % queue_size;
    debugf("Block request setup complete: descriptor_index=%lu\n", descriptor_index);
   
    return 0;
}

int virtio_submit_and_wait(volatile VirtioDevice *vdev) {
    debugf("Submitting block request and waiting for completion\n");
    // Notify the device about the new request
    virtio_notify(vdev, vdev->common_cfg->queue_select);

    // Wait for the device to process the request
    while (vdev->device->idx == vdev->driver->idx) {
        // TODO: Spin or yield CPU
    }

    // Check the status of the completed request
    uint8_t status = global_blk_dev->status;
    vdev->device->idx++;
    debugf("Block request completed with status: %d\n", status);
    return status;  // Return the status of the completed request
}

uint8_t virtio_get_status(volatile VirtioDevice *vdev) {
    return vdev->common_cfg->device_status;
}


int block_init(void) {
    debugf("Initializing block device\n");
    PCIDevice *pdev = pci_find_saved_device(VIRTIO_PCI_VENDOR_ID, VIRTIO_PCI_DEVICE_BLOCK);
    if (!pdev) {
        return -1; // Device not found
    }

    volatile VirtioDevice *vdev = virtio_get_by_device((volatile PCIDevice *)pdev);
    if (!vdev) {
        return -1; // Failed to initialize VirtIO device
    }

    struct VirtioBlkConfig blk_config;
    volatile struct VirtioPciCommonCfg *common_cfg = pci_get_virtio_common_config(pdev);
    if (!common_cfg) {
        return -1; // Failed to get VirtIO common configuration
    }
    // Read block device configuration
    blk_config.capacity = common_cfg->device_feature;

    global_blk_dev = kmalloc(sizeof(struct block_device));
    if (!global_blk_dev) {
        return -1; // Failed to allocate memory
    }
    global_blk_dev->vdev = vdev;
    global_blk_dev->config = blk_config;
    debugf("Block device initialization complete: global_blk_dev=%p\n", global_blk_dev);
    
    return 0;
}

int block_read(uint64_t sector, void *buffer, size_t size) {
    debugf("Reading from block device: sector=%lu, buffer=%p, size=%lu\n", sector, buffer, size);
   
    if (size % SECTOR_SIZE != 0) {
        return -1; // Size must be a multiple of the sector size
    }

    // Allocate and set up the block request header
    struct BlockHeader header = {
        .type = VIRTIO_BLK_T_IN,
        .reserved = 0,
        .sector = sector
    };

    // Translate the buffer's virtual address to a physical address
    uintptr_t phys_addr = mmu_translate(kernel_mmu_table, (uintptr_t)buffer);
    if (phys_addr == 0) {
        return -1; // Failed to translate address
    }

    // Set up the VirtIO descriptors for the request
    int ret = virtio_setup_block_request(global_blk_dev->vdev, &header, (void *)phys_addr, size, VIRTIO_BLK_T_IN);
    if (ret != 0) {
        return -1; // Failed to set up block request
    }

    // Notify the device and wait for the response
    ret = virtio_submit_and_wait(global_blk_dev->vdev);
    if (ret != 0) {
        return -1; // Failed to submit request or get response
    }

    // Check the status of the request
    uint8_t status = virtio_get_status(global_blk_dev->vdev);
    if (status != VIRTIO_BLK_S_OK) {
        return -1; // Request failed
    }
    debugf("Block read %s\n", ret == 0 ? "successful" : "failed");
   
    return 0; // Success
}

int block_write(uint64_t sector, const void *buffer, size_t size) {
    debugf("Writing to block device: sector=%lu, buffer=%p, size=%lu\n", sector, buffer, size);
    
    if (size % SECTOR_SIZE != 0) {
        return -1; // Size must be a multiple of the sector size
    }

    // Allocate and set up the block request header
    struct BlockHeader header = {
        .type = VIRTIO_BLK_T_OUT,
        .reserved = 0,
        .sector = sector
    };

    // Translate the buffer's virtual address to a physical address
    uintptr_t phys_addr = mmu_translate(kernel_mmu_table, (uintptr_t)buffer);
    if (phys_addr == 0) {
        return -1; // Failed to translate address
    }

    // Set up the VirtIO descriptors for the request
    int ret = virtio_setup_block_request(global_blk_dev->vdev, &header, (void *)phys_addr, size, VIRTIO_BLK_T_OUT);
    if (ret != 0) {
        return -1; // Failed to set up block request
    }

    // Notify the device and wait for the response
    ret = virtio_submit_and_wait(global_blk_dev->vdev);
    if (ret != 0) {
        return -1; // Failed to submit request or get response
    }

    // Check the status of the request
    uint8_t status = virtio_get_status(global_blk_dev->vdev);
    if (status != VIRTIO_BLK_S_OK) {
        return -1; // Request failed
    }
    debugf("Block write %s\n", ret == 0 ? "successful" : "failed");
    
    return 0; // Success
}
