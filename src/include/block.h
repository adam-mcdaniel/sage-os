#include <stdint.h>

struct VirtioBlkConfig {
    uint64_t capacity;
    uint32_t size_max;
    uint32_t seg_max;
    struct VirtioBlkGeometry {
        uint16_t cylinders;
        uint8_t  heads;
        uint8_t  sectors;
    } geometry;
    uint32_t blk_size;
    struct VirtioBlkTopology {
        // # of logical blocks per physical block (log2)
        uint8_t  physical_block_exp;
        // offset of first aligned logical block
        uint8_t  alignment_offset;
        // suggested minimum I/O size in blocks
        uint16_t min_io_size;
        // optimal (suggested maximum) I/O size in blocks
        uint32_t opt_io_size;
    } topology;
    uint8_t  writeback;
    uint8_t  unused0[3];
    uint32_t max_discard_sectors;
    uint32_t max_discard_seg;
    uint32_t discard_sector_alignment;
    uint32_t max_write_zeroes_sectors;
    uint32_t max_write_zeroes_seg;
    uint8_t  write_zeroes_may_unmap;
    uint8_t  unused1[3];
};

#define SECTOR_SIZE        512

#define VIRTIO_BLK_T_IN    0
#define VIRTIO_BLK_T_OUT   1
#define VIRTIO_BLK_T_FLUSH 4

struct BlockHeader {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
};

#define VIRTIO_BLK_S_OK      0
#define VIRTIO_BLK_S_IOERR   1
#define VIRTIO_BLK_S_UNSUPP  2
