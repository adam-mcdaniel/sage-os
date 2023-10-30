#include <filesystem.h>
#include <block.h>


void filesystem_init(void)
{
    // Initialize the filesystem
    
}

void filesystem_superblock_init(void) {
    // Initialize the superblock
    uint64_t sector_size = block_device_get_sector_size();
    uint64_t num_sectors = block_device_get_sector_count();
    uint64_t device_bytes = block_device_get_bytes();

    uint64_t bytes_per_block = 1024;

    uint64_t num_blocks = device_bytes / bytes_per_block;

    SuperBlock superblock;
    superblock.magic = 0x4d5a;
    superblock.log_zone_size = 0;
}

SuperBlock filesystem_get_superblock() {
    // Get the superblock
    SuperBlock superblock;
    // Superblock begins at bytes 1024
    block_device_read_bytes(1024, (uint8_t *)&superblock, sizeof(SuperBlock));
    return superblock;
}

void filesystem_put_superblock(SuperBlock superblock) {
    // Put the superblock
    block_device_write_bytes(1024, (uint8_t *)&superblock, sizeof(SuperBlock));
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
