/*
* Random Number Generator Driver header
*/
#pragma once

#include <stdint.h>
#include <pci.h>
#include <virtio.h>
#include <stdbool.h>
#include <debug.h>
#include <mmu.h>
#include <kmalloc.h>

#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1
#define VIRTIO_BLK_T_FLUSH 4
#define VIRTIO_BLK_T_DISCARD 11
#define VIRTIO_BLK_T_WRITE_ZEROES 13

#define VIRTIO_BLK_S_OK 0
#define VIRTIO_BLK_S_IOERR 1
#define VIRTIO_BLK_S_UNSUPP 2

void block_device_init(void);

typedef struct BlockRequestPacket {
    // First descriptor
    uint32_t type;      // IN/OUT
    uint32_t reserved;
    uint64_t sector;    // start sector (LBA / cfg->blk_size)
    // Second descriptor
    // Multiple of cfg->blk_size
    // which will be 512.
    uint8_t *data;
    uint8_t sector_count; // Number of sectors to read/write
    // Third descriptor
    uint8_t status;
} BlockRequestPacket;

void block_device_send_request(BlockRequestPacket *packet);

// THE DATA MUST BE PHYSICALLY CONTIGUOUS!
void block_device_read_sector(uint64_t sector, uint8_t *data);

// THE DATA MUST BE PHYSICALLY CONTIGUOUS!
void block_device_write_sector(uint64_t sector, uint8_t *data);

// THE DATA MUST BE PHYSICALLY CONTIGUOUS!
void block_device_read_sectors(uint64_t sector, uint8_t *data, uint64_t count);

// THE DATA MUST BE PHYSICALLY CONTIGUOUS!
void block_device_write_sectors(uint64_t sector, uint8_t *data, uint64_t count);

uint64_t block_device_get_sector_size(void);
uint64_t block_device_get_sector_count(void);
uint64_t block_device_get_bytes(void);

// Data does not need to be physically contiguous.
void block_device_read_bytes(uint64_t byte, uint8_t *data, uint64_t count);
// Data does not need to be physically contiguous.
void block_device_write_bytes(uint64_t byte, uint8_t *data, uint64_t count);