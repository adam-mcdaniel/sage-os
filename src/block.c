/*
*   Block Device Driver
*/
#include <pci.h>
#include <virtio.h>
#include <stdbool.h>
#include <debug.h>
#include <mmu.h>
#include <kmalloc.h>
#include <vector.h>
#include <csr.h>
#include <block.h>

//use this like a queue
static Vector *device_active_jobs;
static VirtioDevice *block_device;

void block_device_init() {
    device_active_jobs = vector_new();
    block_device = virtio_get_block_device();
    debugf("Block device init done for device at %p\n", block_device->pcidev->ecam_header);
    block_device->ready = true;

    volatile VirtioBlockConfig *config = virtio_get_block_config(block_device);

    debugf("Block device has %d segments\n", config->seg_max);
    debugf("Block device has %d max size\n", config->size_max);
    debugf("Block device has block size of %d\n", config->blk_size);
    debugf("Block device has %d capacity\n", config->capacity);
    debugf("Block device has %d cylinders\n", config->geometry.cylinders);
    debugf("Block device has %d heads\n", config->geometry.heads);
}

void block_device_send_request(BlockRequestPacket *packet) {
    debugf("Sending block request\n");
    // First descriptor is the header
    VirtioDescriptor header;
    header.addr = kernel_mmu_translate((uint64_t)packet);
    header.flags = VIRTQ_DESC_F_NEXT;
    header.len = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t);

    // Second descriptor is the data
    VirtioDescriptor data;
    data.addr = kernel_mmu_translate((uint64_t)packet->data);
    if (packet->type == VIRTIO_BLK_T_IN)
        data.flags = VIRTQ_DESC_F_WRITE;
    data.flags |= VIRTQ_DESC_F_NEXT;
    data.len = packet->sector_count * 512;

    // The third descriptor is the status
    VirtioDescriptor status;
    status.addr = kernel_mmu_translate((uint64_t)&packet->status);
    status.flags = VIRTQ_DESC_F_WRITE;
    status.len = sizeof(packet->status);


    VirtioDescriptor chain[3];
    chain[0] = header;
    chain[1] = data;
    chain[2] = status;

    // Create the chain
    debugf("Status before: %d\n", packet->status);
    virtio_send_descriptor_chain(block_device, 0, chain, 3, true);

    // Check the status
    debugf("Status after: %d\n", packet->status);
}

void block_device_read_sector(uint64_t sector, uint8_t *data) {
    debugf("Reading sector %d\n", sector);
    BlockRequestPacket packet;
    packet.type = VIRTIO_BLK_T_IN;
    packet.sector = sector;
    packet.data = data;
    packet.sector_count = 1;
    packet.status = 0xf;

    block_device_send_request(&packet);
}

void block_device_write_sector(uint64_t sector, uint8_t *data) {
    debugf("Writing sector %d\n", sector);
    BlockRequestPacket packet;
    packet.type = VIRTIO_BLK_T_OUT;
    packet.sector = sector;
    packet.data = data;
    packet.sector_count = 1;
    packet.status = 0xf;

    block_device_send_request(&packet);
}

void block_device_read_sectors(uint64_t sector, uint8_t *data, uint64_t count);

void block_device_write_sectors(uint64_t sector, uint8_t *data, uint64_t count);
