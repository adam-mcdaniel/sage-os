#include <filesystem.h>
#include <block.h>


static SuperBlock sb;

void filesystem_init(void)
{
    // Initialize the filesystem
    filesystem_get_superblock();
    debugf("Filesystem block size: %d\n", filesystem_get_block_size());
    debugf("Superblock:\n");
    debugf("   num_inodes: %d\n", sb.num_inodes);
    debugf("   imap_blocks: %d\n", sb.imap_blocks);
    debugf("   zmap_blocks: %d\n", sb.zmap_blocks);
    debugf("   first_data_zone: %d\n", sb.first_data_zone);
    debugf("   log_zone_size: %d\n", sb.log_zone_size);
    debugf("   max_size: 0x%x (%d)\n", sb.max_size, sb.max_size);
    debugf("   num_zones: 0x%x (%d)\n", sb.num_zones, sb.num_zones);
    debugf("   magic: 0x%x\n", sb.magic);
    debugf("   block_size: %d\n", sb.block_size);
    debugf("   disk_version: %d\n", sb.disk_version);
    for (uint16_t i=0; i<sb.imap_blocks; i++) {
        
    }
}

// void filesystem_superblock_init(void) {
//     // Initialize the superblock
//     uint64_t sector_size = block_device_get_sector_size();
//     uint64_t num_sectors = block_device_get_sector_count();
//     uint64_t device_bytes = block_device_get_bytes();
//     uint64_t bytes_per_block = 1024;
//     uint64_t num_blocks = device_bytes / bytes_per_block;
//     SuperBlock superblock;
//     superblock.magic = 0x4d5a;
//     superblock.log_zone_size = 0;
// }

SuperBlock filesystem_get_superblock() {
    // Get the superblock
    // SuperBlock superblock;
    // Superblock begins at bytes 1024
    if (sb.magic != MINIX3_MAGIC) {
        block_device_read_bytes(1024, (uint8_t *)&sb, sizeof(SuperBlock));
    }
    return sb;
}

void filesystem_put_superblock(SuperBlock superblock) {
    // Put the superblock
    block_device_write_bytes(1024, (uint8_t *)&sb, sizeof(SuperBlock));
}


uint16_t filesystem_get_block_size(void) {
    SuperBlock superblock = filesystem_get_superblock();
    return 1024 << superblock.log_zone_size;
}

uint16_t filesystem_sectors_per_block(void) {
    return filesystem_get_block_size() / block_device_get_sector_size();
}

// Read the inode bitmap into the given buffer
void filesystem_get_inode_bitmap(uint8_t *bitmap_buf) {
    SuperBlock sb = filesystem_get_superblock();
    block_device_read_bytes(FS_IMAP_IDX * filesystem_get_block_size(), bitmap_buf, filesystem_get_block_size() * sb.imap_blocks);
}
// Write the inode bitmap from the given buffer
void filesystem_put_inode_bitmap(uint8_t *bitmap_buf) {
    SuperBlock sb = filesystem_get_superblock();
    block_device_write_bytes(FS_IMAP_IDX * filesystem_get_block_size(), bitmap_buf, filesystem_get_block_size() * sb.imap_blocks);
}
// Read the zone bitmap into the given buffer
void filesystem_get_zone_bitmap(uint8_t *bitmap_buf) {
    SuperBlock sb = filesystem_get_superblock();
    block_device_read_bytes((FS_IMAP_IDX + sb.imap_blocks) * filesystem_get_block_size(), bitmap_buf, filesystem_get_block_size() * sb.zmap_blocks);
}
// Write the zone bitmap from the given buffer
void filesystem_put_zone_bitmap(uint8_t *bitmap_buf) {
    SuperBlock sb = filesystem_get_superblock();
    block_device_write_bytes((FS_IMAP_IDX + sb.imap_blocks) * filesystem_get_block_size(), bitmap_buf, filesystem_get_block_size() * sb.zmap_blocks);
}

void filesystem_get_block(uint32_t block, uint8_t *data);
void filesystem_put_block(uint32_t block, uint8_t *data);

uint32_t filesystem_alloc_zone(void);
void filesystem_free_zone(uint32_t zone);

void filesystem_get_inode(uint32_t inode, Inode *data);
void filesystem_put_inode(uint32_t inode, Inode *data);
uint32_t filesystem_alloc_inode(Inode *inode);
void filesystem_free_inode(uint32_t inode);

void filesystem_get_dir_entry(uint32_t inode, uint32_t entry, DirEntry *data);
void filesystem_put_dir_entry(uint32_t inode, uint32_t entry, DirEntry *data);
