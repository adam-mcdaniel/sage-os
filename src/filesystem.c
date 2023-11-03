#include <filesystem.h>
#include <block.h>
#include <debug.h>
#include <util.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define FILESYSTEM_DEBUG

#ifdef FILESYSTEM_DEBUG
#define debugf(...) debugf(__VA_ARGS__)
#else
#define debugf(...)
#endif

static SuperBlock sb;
static uint8_t *inode_bitmap;
static uint8_t *zone_bitmap;

void debug_dir_entry(DirEntry entry) {
    debugf("Entry: %u `", entry.inode);
    for (uint8_t i=0; i<60; i++) {
        #ifdef FILESYSTEM_DEBUG
        if (entry.name[i])
            textf("%c", entry.name[i]);
        else
            textf("\\0");
        #endif
    }
    #ifdef FILESYSTEM_DEBUG
    textf("`\n");
    #endif
}

uint32_t filesystem_get_min_inode() {
    return 1;
}

uint32_t filesystem_get_max_inode() {
    return filesystem_get_block_size() * filesystem_get_superblock().imap_blocks * 8;
}

uintptr_t filesystem_get_inode_byte_offset(SuperBlock sb, uint32_t inode) {
    if (inode == INVALID_INODE) {
        fatalf("Invalid inode %u\n", inode);
        return 0;
    }
    return (FS_IMAP_IDX + sb.imap_blocks + sb.zmap_blocks) * filesystem_get_block_size() + (inode - 1) * sizeof(Inode);
}
// uintptr_t filesystem_get_inode_bitmap_byte_offset(SuperBlock sb, uint32_t inode) {
//     return FS_IMAP_IDX * filesystem_get_block_size() + inode / 8;
// }


void filesystem_superblock_init(void) {
    // uint64_t sector_size = block_device_get_sector_size();
    // Initialize the superblock
    // uint64_t num_sectors = block_device_get_sector_count();
    uint64_t device_bytes = block_device_get_bytes();
    uint64_t bytes_per_block = 1024;
    uint64_t num_blocks = device_bytes / bytes_per_block / 2;
    SuperBlock superblock;
    superblock.magic = 0x4d5a;
    superblock.log_zone_size = 0;
    superblock.max_size = num_blocks * bytes_per_block;
    superblock.num_zones = num_blocks;
    superblock.block_size = bytes_per_block;
    superblock.disk_version = 0;
    superblock.num_inodes = num_blocks / 8;
    superblock.imap_blocks = superblock.num_inodes / (bytes_per_block * 8);
    superblock.zmap_blocks = superblock.num_zones / (bytes_per_block * 8);
    superblock.first_data_zone = 0;
    filesystem_put_superblock(superblock);
}


void filesystem_get_zone(uint32_t zone, uint8_t *data) {
    SuperBlock sb = filesystem_get_superblock();
    if (zone > sb.num_zones + sb.first_data_zone) {
        fatalf("Zone %u (%x) is out of bounds\n", zone, zone);
        return;
    }

    if (zone < sb.first_data_zone) {
        fatalf("Zone %u (%x) is before the first data zone %u (%x)\n", zone, zone, sb.first_data_zone, sb.first_data_zone);
        return;
    }
    
    // filesystem_get_block(filesystem_get_superblock().first_data_zone + zone, data);
    filesystem_get_block(zone, data);
}
void filesystem_put_zone(uint32_t zone, uint8_t *data) {
    SuperBlock sb = filesystem_get_superblock();
    if (zone > sb.num_zones + sb.first_data_zone) {
        fatalf("Zone %u (%x) is out of bounds\n", zone, zone);
        return;
    }

    if (zone < sb.first_data_zone) {
        fatalf("Zone %u (%x) is before the first data zone %u (%x)\n", zone, zone, sb.first_data_zone, sb.first_data_zone);
        return;
    }
    
    // filesystem_put_block(filesystem_get_superblock().first_data_zone + zone, data);
    filesystem_put_block(zone, data);
}

uint64_t filesystem_get_file_size(uint32_t inode) {
    Inode inode_data = filesystem_get_inode(inode);
    if (S_ISREG(inode_data.mode)) {
        return inode_data.size;
    } else if (S_ISDIR(inode_data.mode)) {
        return 0;
    } else {
        fatalf("Unknown inode type %x\n", inode_data.mode);
        return 0;
    }
}

void debug_inode(uint32_t i) {
    Inode inode = filesystem_get_inode(i);
    debugf("Inode %u (%x):\n", i, i);
    debugf("   mode: %x\n", inode.mode);
    debugf("   num_links: %d\n", inode.num_links);
    debugf("   uid: %d\n", inode.uid);
    debugf("   gid: %d\n", inode.gid);
    debugf("   size: %d\n", inode.size);
    debugf("   atime: %d\n", inode.atime);
    debugf("   mtime: %d\n", inode.mtime);
    debugf("   ctime: %d\n", inode.ctime);
    debugf("   zones[]:\n");
    for (uint8_t j=0; j<10; j++) {
        if (!inode.zones[j]) {
            continue;
        }
        debugf("   zones[%d] = %x (*1024 = %x)\n", j, inode.zones[j], inode.zones[j] * 1024);
    }
}

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

    if (sb.magic != MINIX3_MAGIC) {
        // We need to initialize the superblock
        debugf("Minix3 magic is not correct, initializing superblock ourselves...\n");
        filesystem_superblock_init();
    }
    // for (uint16_t i=0; i<sb.imap_blocks; i++) {
    // }
    debugf("Max inode: %d\n", filesystem_get_max_inode());
    // This does not work yet, broken

    // Copy the inode bitmap into memory
    inode_bitmap = (uint8_t *)kmalloc(filesystem_get_inode_bitmap_size());
    filesystem_get_inode_bitmap(inode_bitmap);
    // Copy the zone bitmap into memory
    zone_bitmap = (uint8_t*)kmalloc(filesystem_get_zone_bitmap_size());
    filesystem_get_zone_bitmap(zone_bitmap);

    // for (uint32_t i=0; i<sb.num_zones; i++) {
    //     if (zone_bitmap[i / 8] & (1 << i % 8)) {
    //         debugf("Zone %u (%x) is taken\n", i, i);
    //         uint8_t data[filesystem_get_block_size()];

    //         filesystem_get_zone(i, data);

    //         bool any_nonzero = false;
    //         for (uint16_t j=0; j<filesystem_get_block_size(); j++) {
    //             if (data[j] != 0) {
    //                 textf("%c", data[j]);
    //                 any_nonzero = true;
    //             }
    //         }
    //         if (!any_nonzero) {
    //             // debugf("Zone %u (%x) is empty\n", i, i);
    //         } else {
    //             debugf("\n");
    //         }
    //     } else {
    //         debugf("Zone %u (%x) is free\n", i, i);
    //     }
    // }

    // for (uint32_t i=0; i<filesystem_get_max_inode(); i++) {
    //     if (filesystem_has_inode(i)) {
    //         Inode inode = filesystem_get_inode(i);
    //         if (inode.num_links > 0) {
    //             debugf("Inode %u (%x):\n", i, i);
    //             debugf("   mode: %x\n", inode.mode);
    //             debugf("   num_links: %d\n", inode.num_links);
    //             debugf("   uid: %d\n", inode.uid);
    //             debugf("   gid: %d\n", inode.gid);
    //             debugf("   size: %d\n", inode.size);
    //             debugf("   atime: %d\n", inode.atime);
    //             debugf("   mtime: %d\n", inode.mtime);
    //             debugf("   ctime: %d\n", inode.ctime);
    //             debugf("   zones[]:\n");
    //             for (uint8_t j=0; j<10; j++) {
    //                 if (!inode.zones[j]) {
    //                     continue;
    //                 }
    //                 debugf("   zones[%d] = %x (*1024 = %x)\n", j, inode.zones[j], inode.zones[j] * 1024);
    //             }
    //         }
    //     }
    // }

    for (uint32_t i=filesystem_get_min_inode(); i<filesystem_get_max_inode(); i++) {
        // debugf("Checking inode %u (%x)...\n", i, i);
        if (!filesystem_has_inode(i)) {
            continue;
        }

        if (filesystem_is_dir(i)) {
            debugf("Directory %u:\n", i);

            DirEntry files[64];
            uint32_t num_entries = filesystem_list_dir(i, files, 64);
            for (uint32_t j=0; j<num_entries; j++) {
                textf("   "); debug_dir_entry(files[j]);
            }

        }


        if (!filesystem_is_file(i)) {
            continue;
        }
        debug_inode(i);
        uint64_t file_size = filesystem_get_file_size(i);
        debugf("File %u size: %u\n", i, file_size);
        // size_t size = min(file_size, 1024);
        size_t size = file_size;
        uint8_t buf[size];
        debugf("About to read file...\n");
        filesystem_read_file(i, buf, size);
        debugf("Read file\n");
        for (uint16_t j=0; j<size; j++) {
            textf("%c", buf[j]);
        }

        char test[] = "Hello world!";
        filesystem_put_data(i, test, 128, sizeof(test));
        break;
    }
}

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
    block_device_write_bytes(1024, (uint8_t *)&superblock, sizeof(SuperBlock));
}


uint16_t filesystem_get_block_size(void) {
    SuperBlock superblock = filesystem_get_superblock();
    return 1024 << superblock.log_zone_size;
}

uint16_t filesystem_get_zone_size(void) {
    return filesystem_get_block_size();
}

uint16_t filesystem_sectors_per_block(void) {
    return filesystem_get_block_size() / block_device_get_sector_size();
}

size_t filesystem_get_inode_bitmap_size(void) {
    return filesystem_get_superblock().imap_blocks * filesystem_get_block_size();
}

size_t filesystem_get_zone_bitmap_size(void) {
    return filesystem_get_superblock().zmap_blocks * filesystem_get_block_size();
}

// Read the inode bitmap into the given buffer
void filesystem_get_inode_bitmap(uint8_t *bitmap_buf) {
    debugf("Getting inode bitmap...\n");
    SuperBlock sb = filesystem_get_superblock();

    filesystem_get_blocks(FS_IMAP_IDX, bitmap_buf, sb.imap_blocks);

    // uint8_t inode_byte;
    // uint64_t byte_offset = FS_IMAP_IDX * filesystem_get_block_size() + inode / 8;
    // debugf("About to read inode byte at %u (%x)...\n", byte_offset, byte_offset);
    // block_device_read_bytes(byte_offset, &inode_byte, 1);
    // debugf("Inode byte: %x\n", inode_byte);
    // return inode_byte & (1 << inode % 8);
    // block_device_read_bytes(FS_IMAP_IDX * filesystem_get_block_size(), bitmap_buf, filesystem_get_block_size() * sb.imap_blocks);
}
// Write the inode bitmap from the given buffer
void filesystem_put_inode_bitmap(uint8_t *bitmap_buf) {
    debugf("Setting inode bitmap...\n");
    SuperBlock sb = filesystem_get_superblock();
    filesystem_put_blocks(FS_IMAP_IDX, bitmap_buf, sb.imap_blocks);
    // block_device_write_bytes(FS_IMAP_IDX * filesystem_get_block_size(), bitmap_buf, filesystem_get_block_size() * sb.imap_blocks);
}
// Read the zone bitmap into the given buffer
void filesystem_get_zone_bitmap(uint8_t *bitmap_buf) {
    debugf("Getting zone bitmap...\n");
    SuperBlock sb = filesystem_get_superblock();
    filesystem_get_blocks(FS_IMAP_IDX + sb.imap_blocks, bitmap_buf, sb.zmap_blocks);
    // block_device_read_bytes((FS_IMAP_IDX + sb.imap_blocks) * filesystem_get_block_size(), bitmap_buf, filesystem_get_block_size() * sb.zmap_blocks);
}
// Write the zone bitmap from the given buffer
void filesystem_put_zone_bitmap(uint8_t *bitmap_buf) {
    debugf("Setting zone bitmap...\n");
    SuperBlock sb = filesystem_get_superblock();
    filesystem_put_blocks(FS_IMAP_IDX + sb.imap_blocks, bitmap_buf, sb.zmap_blocks);
}

void filesystem_get_blocks(uint32_t start_block, uint8_t *data, uint16_t count) {
    // SuperBlock sb = filesystem_get_superblock();
    block_device_read_bytes(start_block * filesystem_get_block_size(), data, filesystem_get_block_size() * count);
}
void filesystem_put_blocks(uint32_t start_block, uint8_t *data, uint16_t count) {
    // SuperBlock sb = filesystem_get_superblock();
    block_device_write_bytes(start_block * filesystem_get_block_size(), data, filesystem_get_block_size() * count);
}

void filesystem_get_block(uint32_t block, uint8_t *data) {
    filesystem_get_blocks(block, data, 1);
}

void filesystem_put_block(uint32_t block, uint8_t *data) {
    filesystem_put_blocks(block, data, 1);
}

uint32_t filesystem_alloc_zone(void);
void filesystem_free_zone(uint32_t zone);

bool filesystem_has_inode(uint32_t inode) {
    if (inode == INVALID_INODE) {
        debugf("filesystem_has_inode: Invalid inode %u\n", inode);
        return false;
    }
    return inode_bitmap[inode / 8] & (1 << inode % 8);
}
Inode filesystem_get_inode(uint32_t inode) {
    if (inode == INVALID_INODE) {
        fatalf("filesystem_get_inode: Invalid inode %u\n", inode);
        return (Inode){0};
    }
    SuperBlock sb = filesystem_get_superblock();
    Inode data;
    uint64_t offset = filesystem_get_inode_byte_offset(sb, inode);

    block_device_read_bytes(offset, (uint8_t*)&data, sizeof(Inode));
    return data;
}
void filesystem_put_inode(uint32_t inode, Inode data) {
    if (inode == INVALID_INODE) {
        fatalf("filesystem_put_inode: Invalid inode %u\n", inode);
        return;
    }
    SuperBlock sb = filesystem_get_superblock();
    uint64_t offset = filesystem_get_inode_byte_offset(sb, inode);

    // debugf("Putting inode %u at offset %u (%x)...\n", inode, offset, offset);
    block_device_write_bytes(offset, (uint8_t*)&data, sizeof(Inode));
}

bool filesystem_is_dir(uint32_t inode) {
    Inode inode_data = filesystem_get_inode(inode);
    return S_ISDIR(inode_data.mode) && inode_data.num_links > 0;
}

bool filesystem_is_file(uint32_t inode) {
    Inode inode_data = filesystem_get_inode(inode);
    return S_ISREG(inode_data.mode) && inode_data.num_links > 0;
}

void filesystem_read_file(uint32_t inode, uint8_t *data, uint32_t count) {
    filesystem_get_data(inode, data, 0, count);
}

void filesystem_get_data(uint32_t inode, uint8_t *data, uint32_t offset, uint32_t count) {
    // First, get the inode
    Inode inode_data = filesystem_get_inode(inode);
    

    uint8_t zone_data[filesystem_get_zone_size()];

    // The cursor is the current position in the data buffer
    uint32_t buffer_cursor = 0;
    uint32_t file_cursor = 0;

    // Now, get the data
    // The first 7 zones are direct zones
    for (uint8_t direct_zone=0; direct_zone<7; direct_zone++) {
        uint32_t zone = inode_data.zones[direct_zone];
        if (zone == 0) {
            // debugf("No direct zone %d\n", zone);
            continue;
        }
        memset(zone_data, 0, filesystem_get_zone_size());

        if (file_cursor + filesystem_get_zone_size() < offset) {
            // We're not at the offset yet
            file_cursor += filesystem_get_zone_size();
            continue;
        } else if (file_cursor < offset) {
            // We're in the middle of the offset
            // Read the zone into the buffer
            debugf("Reading direct zone %d\n", zone);
            filesystem_get_zone(zone, zone_data);
            // Copy the remaining data into the buffer
            size_t remaining = min(count, filesystem_get_zone_size() - (offset - file_cursor));
            memcpy(data, zone_data + offset - file_cursor, remaining);
            buffer_cursor += remaining;
            file_cursor += remaining;
            continue;
        }

        debugf("Reading direct zone %d\n", zone);
        // Read the zone into the buffer
        filesystem_get_zone(zone, zone_data);
        // If the cursor is past the amount of data we want, we're done

        
        if (buffer_cursor + filesystem_get_zone_size() > count) {
            // Copy the remaining data into the buffer
            memcpy(data + buffer_cursor, zone_data, count - buffer_cursor);
            // We're done
            return;
        } else {
            // Copy the entire zone into the buffer
            memcpy(data + buffer_cursor, zone_data, filesystem_get_zone_size());
        }
        buffer_cursor += filesystem_get_zone_size();
    }

    debugf("Done with direct zones\n");
    // The next zone is an indirect zone
    if (inode_data.zones[7] != 0) {
        uint32_t indirect_zones[filesystem_get_zone_size() / sizeof(uint32_t)];
        filesystem_get_zone(inode_data.zones[7], (uint8_t)*indirect_zones);

        for (uint8_t indirect_zone=0; indirect_zone<filesystem_get_zone_size() / sizeof(uint32_t); indirect_zone++) {
            uint32_t zone = indirect_zones[indirect_zone];
            if (zone == 0) continue;
            

            if (file_cursor + filesystem_get_zone_size() < offset) {
                // We're not at the offset yet
                file_cursor += filesystem_get_zone_size();
                continue;
            } else if (file_cursor < offset) {
                // We're in the middle of the offset
                // Read the zone into the buffer
                filesystem_get_zone(zone, zone_data);
                // Copy the remaining data into the buffer
                size_t remaining = min(count, filesystem_get_zone_size() - (offset - file_cursor));
                memcpy(data, zone_data + offset - file_cursor, remaining);
                buffer_cursor += remaining;
                file_cursor += remaining;
                continue;
            }
            
            memset(zone_data, 0, filesystem_get_zone_size());

            // Read the zone into the buffer
            filesystem_get_zone(zone, zone_data);
            // If the cursor is past the amount of data we want, we're done
            if (buffer_cursor + filesystem_get_zone_size() > count) {
                // Copy the remaining data into the buffer
                memcpy(data + buffer_cursor, zone_data, count - buffer_cursor);
                // We're done
                return;
            } else {
                // Copy the entire zone into the buffer
                memcpy(data + buffer_cursor, zone_data, filesystem_get_zone_size());
            }
            buffer_cursor += filesystem_get_zone_size();
        }
    } else {
        debugf("No indirect zone\n");
    }

    // The next zone is a double indirect zone
    if (inode_data.zones[8] != 0) {
        uint32_t double_indirect_zones[filesystem_get_zone_size() / sizeof(uint32_t)];
        // We're done
        filesystem_get_zone(inode_data.zones[8], (uint8_t)*double_indirect_zones);

        for (uint8_t double_indirect_zone=0; double_indirect_zone<filesystem_get_zone_size() / sizeof(uint32_t); double_indirect_zone++) {
            uint32_t indirect_zone = double_indirect_zones[double_indirect_zone];
            if (indirect_zone == 0) continue;

            uint32_t indirect_zones[filesystem_get_zone_size() / sizeof(uint32_t)];
            filesystem_get_zone(indirect_zone, (uint8_t)*indirect_zones);

            for (uint8_t indirect_zone=0; indirect_zone<filesystem_get_zone_size() / sizeof(uint32_t); indirect_zone++) {
                uint32_t zone = indirect_zones[indirect_zone];
                if (zone == 0) continue;

                if (file_cursor + filesystem_get_zone_size() < offset) {
                    // We're not at the offset yet
                    file_cursor += filesystem_get_zone_size();
                    continue;
                } else if (file_cursor < offset) {
                    // We're in the middle of the offset
                    // Read the zone into the buffer
                    filesystem_get_zone(zone, zone_data);
                    // Copy the remaining data into the buffer
                    size_t remaining = min(count, filesystem_get_zone_size() - (offset - file_cursor));
                    memcpy(data, zone_data + offset - file_cursor, remaining);
                    buffer_cursor += remaining;
                    file_cursor += remaining;
                    continue;
                }
                
                // Read the zone into the buffer
                memset(zone_data, 0, filesystem_get_zone_size());
                filesystem_get_zone(zone, zone_data);

                // If the cursor is past the amount of data we want, we're done
                if (buffer_cursor + filesystem_get_zone_size() > count) {
                    // Copy the remaining data into the buffer
                    memcpy(data + buffer_cursor, zone_data, count - buffer_cursor);
                    // We're done
                    return;
                } else {
                    // Copy the entire zone into the buffer
                    memcpy(data + buffer_cursor, zone_data, filesystem_get_zone_size());
                }
                buffer_cursor += filesystem_get_zone_size();
            }
        }
    } else {
        debugf("No double indirect zone\n");
    }

    // The next zone is a triple indirect zone
    if (inode_data.zones[9] != 0) {
        uint32_t triple_indirect_zones[filesystem_get_zone_size() / sizeof(uint32_t)];
        filesystem_get_zone(inode_data.zones[9], (uint8_t)*triple_indirect_zones);

        for (uint8_t triple_indirect_zone=0; triple_indirect_zone<filesystem_get_zone_size() / sizeof(uint32_t); triple_indirect_zone++) {
            uint32_t double_indirect_zone = triple_indirect_zones[triple_indirect_zone];
            if (double_indirect_zone == 0) continue;
            uint32_t double_indirect_zones[filesystem_get_zone_size() / sizeof(uint32_t)];
            filesystem_get_zone(double_indirect_zone, (uint8_t)*double_indirect_zones);

            for (uint8_t double_indirect_zone=0; double_indirect_zone<filesystem_get_zone_size() / sizeof(uint32_t); double_indirect_zone++) {
                uint32_t indirect_zone = double_indirect_zones[double_indirect_zone];
                if (indirect_zone == 0) continue;
                uint32_t indirect_zones[filesystem_get_zone_size() / sizeof(uint32_t)];
                filesystem_get_zone(indirect_zone, (uint8_t)*indirect_zones);

                for (uint8_t indirect_zone=0; indirect_zone<filesystem_get_zone_size() / sizeof(uint32_t); indirect_zone++) {
                    uint32_t zone = indirect_zones[indirect_zone];
                    if (zone == 0) continue;
                    
                    if (file_cursor + filesystem_get_zone_size() < offset) {
                        // We're not at the offset yet
                        file_cursor += filesystem_get_zone_size();
                        continue;
                    }
                    
                    // Read the zone into the buffer
                    memset(zone_data, 0, filesystem_get_zone_size());
                    filesystem_get_zone(zone, zone_data);

                    // If the cursor is past the amount of data we want, we're done
                    if (buffer_cursor + filesystem_get_zone_size() > count) {
                        // Copy the remaining data into the buffer
                        memcpy(data + buffer_cursor, zone_data, count - buffer_cursor);
                        // We're done
                        return;
                    } else {
                        // Copy the entire zone into the buffer
                        memcpy(data + buffer_cursor, zone_data, filesystem_get_zone_size());
                    }
                    buffer_cursor += filesystem_get_zone_size();
                }
            }
        }
    } else {
        debugf("No triple indirect zone\n");
    }

    // If we get here, we've read all the data we can
    return;
}
void filesystem_put_data(uint32_t inode, uint8_t *data, uint32_t offset, uint32_t count) {
    // First, get the inode
    Inode inode_data = filesystem_get_inode(inode);

    uint8_t zone_data[filesystem_get_zone_size()];

    // The cursor is the current position in the data buffer
    uint32_t buffer_cursor = 0;
    // The file cursor is the current position in the file
    uint32_t file_cursor = 0;

    // Now, get the data
    // The first 7 zones are direct zones
    for (uint8_t direct_zone=0; direct_zone<7; direct_zone++) {
        uint32_t zone = inode_data.zones[direct_zone];
        if (zone == 0) {
            // debugf("No direct zone %d\n", zone);
            continue;
        }

        if (file_cursor + filesystem_get_zone_size() < offset) {
            // We're not at the offset yet
            file_cursor += filesystem_get_zone_size();
            continue;
        } else if (file_cursor < offset) {
            // We're in the middle of the offset
            // Read the zone into the buffer
            debugf("Writing first direct zone %d\n", zone);
            filesystem_get_zone(zone, zone_data);
            // Copy the remaining data into the buffer
            size_t remaining = min(count, filesystem_get_zone_size() - (offset - file_cursor));
            debugf("Copying %d bytes into zone %d\n", remaining, zone);
            memcpy(zone_data + offset - file_cursor, data, remaining);
            filesystem_put_zone(zone, zone_data);

            buffer_cursor += remaining;
            file_cursor += remaining;
            if (buffer_cursor >= count) {
                // We're done
                return;
            }

            continue;
        }

        memset(zone_data, 0, filesystem_get_zone_size());
        debugf("Writing direct zone %d\n", zone);

        // Write the buffer into the zone
        if (buffer_cursor + filesystem_get_zone_size() > count) {
            // Copy the remaining data into the buffer
            memcpy(zone_data, data + buffer_cursor, count - buffer_cursor);
            // We're done
            filesystem_put_zone(zone, zone_data);
            return;
        } else {
            // Copy the entire zone into the buffer
            memcpy(zone_data, data + buffer_cursor, filesystem_get_zone_size());
            filesystem_put_zone(zone, zone_data);
        }
        buffer_cursor += filesystem_get_zone_size();
    }

    debugf("Done with direct zones\n");
    // The next zone is an indirect zone
    if (inode_data.zones[7] != 0) {
        uint32_t indirect_zones[filesystem_get_zone_size() / sizeof(uint32_t)];
        filesystem_get_zone(inode_data.zones[7], (uint8_t)*indirect_zones);

        for (uint8_t indirect_zone=0; indirect_zone<filesystem_get_zone_size() / sizeof(uint32_t); indirect_zone++) {
            uint32_t zone = indirect_zones[indirect_zone];
            if (zone == 0) continue;

            if (file_cursor + filesystem_get_zone_size() < offset) {
                // We're not at the offset yet
                file_cursor += filesystem_get_zone_size();
                continue;
            } else if (file_cursor < offset) {
                // We're in the middle of the offset
                // Read the zone into the buffer
                filesystem_get_zone(zone, zone_data);
                // Copy the remaining data into the buffer
                size_t remaining = min(count, filesystem_get_zone_size() - (offset - file_cursor));
                memcpy(zone_data + offset - file_cursor, data, remaining);
                filesystem_put_zone(zone, zone_data);

                buffer_cursor += remaining;
                file_cursor += remaining;
                if (buffer_cursor >= count) {
                    // We're done
                    return;
                }
                continue;
            }

            memset(zone_data, 0, filesystem_get_zone_size());

            if (buffer_cursor + filesystem_get_zone_size() > count) {
                // Copy the remaining data into the buffer
                memcpy(zone_data, data + buffer_cursor, count - buffer_cursor);
                // We're done
                filesystem_put_zone(zone, zone_data);
                return;
            } else {
                // Copy the entire zone into the buffer
                memcpy(zone_data, data + buffer_cursor, filesystem_get_zone_size());
                filesystem_put_zone(zone, zone_data);
            }
            buffer_cursor += filesystem_get_zone_size();
        }
    } else {
        debugf("No indirect zone\n");
    }

    // The next zone is a double indirect zone
    // The next zone is a double indirect zone
    if (inode_data.zones[8] != 0) {
        uint32_t double_indirect_zones[filesystem_get_zone_size() / sizeof(uint32_t)];
        // We're done
        filesystem_get_zone(inode_data.zones[8], (uint8_t)*double_indirect_zones);

        for (uint8_t double_indirect_zone=0; double_indirect_zone<filesystem_get_zone_size() / sizeof(uint32_t); double_indirect_zone++) {
            uint32_t indirect_zone = double_indirect_zones[double_indirect_zone];
            if (indirect_zone == 0) continue;

            uint32_t indirect_zones[filesystem_get_zone_size() / sizeof(uint32_t)];
            filesystem_get_zone(indirect_zone, (uint8_t)*indirect_zones);

            for (uint8_t indirect_zone=0; indirect_zone<filesystem_get_zone_size() / sizeof(uint32_t); indirect_zone++) {
                uint32_t zone = indirect_zones[indirect_zone];
                if (zone == 0) continue;

                if (file_cursor + filesystem_get_zone_size() < offset) {
                    // We're not at the offset yet
                    file_cursor += filesystem_get_zone_size();
                    continue;
                } else if (file_cursor < offset) {
                    // We're in the middle of the offset
                    // Read the zone into the buffer
                    filesystem_get_zone(zone, zone_data);
                    // Copy the remaining data into the buffer
                    size_t remaining = min(count, filesystem_get_zone_size() - (offset - file_cursor));
                    memcpy(zone_data + offset - file_cursor, data, remaining);
                    filesystem_put_zone(zone, zone_data);

                    buffer_cursor += remaining;
                    file_cursor += remaining;
                    if (buffer_cursor >= count) {
                        // We're done
                        return;
                    }
                    continue;
                }

                memset(zone_data, 0, filesystem_get_zone_size());
                if (buffer_cursor + filesystem_get_zone_size() > count) {
                    // Copy the remaining data into the buffer
                    memcpy(zone_data, data + buffer_cursor, count - buffer_cursor);
                    // We're done
                    filesystem_put_zone(zone, zone_data);
                    return;
                } else {
                    // Copy the entire zone into the buffer
                    memcpy(zone_data, data + buffer_cursor, filesystem_get_zone_size());
                    filesystem_put_zone(zone, zone_data);
                }

                buffer_cursor += filesystem_get_zone_size();
            }
        }
    } else {
        debugf("No double indirect zone\n");
    }

    // The next zone is a triple indirect zone
    if (inode_data.zones[9] != 0) {
        uint32_t triple_indirect_zones[filesystem_get_zone_size() / sizeof(uint32_t)];
        filesystem_get_zone(inode_data.zones[9], (uint8_t)*triple_indirect_zones);

        for (uint8_t triple_indirect_zone=0; triple_indirect_zone<filesystem_get_zone_size() / sizeof(uint32_t); triple_indirect_zone++) {
            uint32_t double_indirect_zone = triple_indirect_zones[triple_indirect_zone];
            if (double_indirect_zone == 0) continue;
            uint32_t double_indirect_zones[filesystem_get_zone_size() / sizeof(uint32_t)];
            filesystem_get_zone(double_indirect_zone, (uint8_t)*double_indirect_zones);

            for (uint8_t double_indirect_zone=0; double_indirect_zone<filesystem_get_zone_size() / sizeof(uint32_t); double_indirect_zone++) {
                uint32_t indirect_zone = double_indirect_zones[double_indirect_zone];
                if (indirect_zone == 0) continue;
                uint32_t indirect_zones[filesystem_get_zone_size() / sizeof(uint32_t)];
                filesystem_get_zone(indirect_zone, (uint8_t)*indirect_zones);

                for (uint8_t indirect_zone=0; indirect_zone<filesystem_get_zone_size() / sizeof(uint32_t); indirect_zone++) {
                    uint32_t zone = indirect_zones[indirect_zone];
                    if (zone == 0) continue;

                    if (file_cursor + filesystem_get_zone_size() < offset) {
                        // We're not at the offset yet
                        file_cursor += filesystem_get_zone_size();
                        continue;
                    } else if (file_cursor < offset) {
                        // We're in the middle of the offset
                        // Read the zone into the buffer
                        filesystem_get_zone(zone, zone_data);
                        // Copy the remaining data into the buffer
                        size_t remaining = min(count, filesystem_get_zone_size() - (offset - file_cursor));
                        memcpy(zone_data + offset - file_cursor, data, remaining);
                        filesystem_put_zone(zone, zone_data);

                        buffer_cursor += remaining;
                        file_cursor += remaining;
                        if (buffer_cursor >= count) {
                            // We're done
                            return;
                        }
                        continue;
                    }

                    memset(zone_data, 0, filesystem_get_zone_size());

                    if (buffer_cursor + filesystem_get_zone_size() > count) {
                        // Copy the remaining data into the buffer
                        memcpy(zone_data, data + buffer_cursor, count - buffer_cursor);
                        // We're done
                        filesystem_put_zone(zone, zone_data);
                        return;
                    } else {
                        // Copy the entire zone into the buffer
                        memcpy(zone_data, data + buffer_cursor, filesystem_get_zone_size());
                        filesystem_put_zone(zone, zone_data);
                    }
                    buffer_cursor += filesystem_get_zone_size();
                }
            }
        }
    } else {
        debugf("No triple indirect zone\n");
    }


    // If we get here, we've read all the data we can
    return;
}


bool filesystem_get_dir_entry(uint32_t inode, uint32_t entry, DirEntry *data) {
    if (!filesystem_is_dir(inode)) {
        fatalf("Inode %u (%x) is not a directory\n", inode, inode);
        return false;
    }
    Inode inode_data = filesystem_get_inode(inode);
    DirEntry tmp;
    filesystem_get_data(inode, &tmp, entry * sizeof(DirEntry), sizeof(DirEntry));
    if (tmp.inode == 0) {
        return false;
    }

    memcpy(data, &tmp, sizeof(DirEntry));
    return true;
}

void filesystem_put_dir_entry(uint32_t inode, uint32_t entry, DirEntry *data) {
    if (!filesystem_is_dir(inode)) {
        fatalf("Inode %u (%x) is not a directory\n", inode, inode);
        return;
    }
    Inode inode_data = filesystem_get_inode(inode);
    filesystem_put_data(inode, data, entry * sizeof(DirEntry), sizeof(DirEntry));
}


// List all of the entries in the given directory to the given buffer.
uint32_t filesystem_list_dir(uint32_t inode, DirEntry *entries, uint32_t max_entries) {
    if (!filesystem_is_dir(inode)) {
        fatalf("Inode %u (%x) is not a directory\n", inode, inode);
        return 0;
    }
    Inode inode_data = filesystem_get_inode(inode);
    uint32_t entry = 0;
    DirEntry tmp;
    while (filesystem_get_dir_entry(inode, entry, &tmp)) {
        memcpy(entries + entry, &tmp, sizeof(DirEntry));
        entry++;
        if (entry >= max_entries) {
            break;
        }
    }
    return entry;
}
// Returns the inode number of the file with the given name in the given directory.
// If the file does not exist, return INVALID_INODE.
uint32_t filesystem_find_dir_entry(uint32_t inode, char *name);