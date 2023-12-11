#include <minix3.h>
#include <block.h>
#include <debug.h>
#include <stdint.h>
#include <util.h>
#include <list.h>
#include <path.h>
#include <map.h>
#include <trap.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// #define MINIX3_DEBUG

#ifdef MINIX3_DEBUG
#define debugf(...) debugf(__VA_ARGS__)
#else
#define debugf(...)
#endif

static SuperBlock sb;

// static VirtioDevice *block_device;
static VirtioDevice *loaded_block_device;
static uint8_t *inode_bitmap;
static uint8_t *zone_bitmap;

void debug_dir_entry(DirEntry entry) {
    debugf("Entry: %u `", entry.inode);
    for (uint32_t i=0; i<60; i++) {
        #ifdef minix3_DEBUG
        if (entry.name[i])
            infof("%c", entry.name[i]);
        else
            infof("\\0");
        #endif
    }
    #ifdef minix3_DEBUG
    infof("`\n");
    #endif
}

// Return the inode number from path.
// If get_parent is true, return the inode number of the parent.
// If given path /dir0/dir1file, return inode of /dir0/dir1/
uint32_t minix3_get_inode_from_path(VirtioDevice *block_device, const char *path, bool get_parent) {
    // TODO: Add support for relative path.
    debugf("Getting inode from path %s\n", path);
    List *path_items = path_split(path);

    uint32_t parent = 1; // Root inode

    uint32_t i = 0, num_items = list_size(path_items);
    debugf("num_items = %u\n", num_items);
    ListElem *elem;
    list_for_each(path_items, elem) {

        char *name = (char *)list_elem_value(elem);
        debugf("Getting inode from relative path %s, num_items = %u\n", name, num_items);
        if (strcmp(name, "/") == 0 || strcmp(name, "") == 0) {
            debugf("Skipping root\n");
            return parent;
        }
        debugf("i = %u, num_items = %u\n", i, num_items);
        if (get_parent && i == num_items - 1) {
            debugf("Returning parent inode %u\n", parent);
            return parent;
        }
        debugf("Getting child %s of inode %u\n", name, parent);
        uint32_t child = minix3_find_dir_entry(block_device, parent, name);
        debugf("Got child %u\n", child);
        
        if (child == INVALID_INODE) {
            warnf("minix3_get_inode_from_path: Couldn't find inode for %s\n", name);
            return INVALID_INODE;
        }
        parent = child;
        debugf("Got child %u\n", child);
        i++;
    }

    return parent;
}

uint32_t minix3_get_min_inode(VirtioDevice *block_device) {
    return 1;
}

uint32_t minix3_get_max_inode(VirtioDevice *block_device) {
    return minix3_get_block_size(block_device) * minix3_get_superblock(block_device).imap_blocks * 8;
}

uintptr_t minix3_get_inode_byte_offset(VirtioDevice *block_device, SuperBlock sb, uint32_t inode) {
    if (inode == INVALID_INODE) {
        fatalf("Invalid inode %u\n", inode);
        return 0;
    }
    return (FS_IMAP_IDX + sb.imap_blocks + sb.zmap_blocks) * minix3_get_block_size(block_device) + (inode - 1) * sizeof(Inode);
}
// uintptr_t minix3_get_inode_bitmap_byte_offset(SuperBlock sb, uint32_t inode) {
//     return FS_IMAP_IDX * minix3_get_block_size(block_device) + inode / 8;
// }


void minix3_superblock_init(VirtioDevice *block_device) {
    // uint64_t sector_size = block_device_get_sector_size();
    // Initialize the superblock
    // uint64_t num_sectors = block_device_get_sector_count();
    uint64_t device_bytes = block_device_get_bytes(block_device);
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
    minix3_put_superblock(block_device, superblock);
}

bool minix3_has_zone(VirtioDevice *block_device, uint32_t zone) {
    return inode_bitmap[zone / 8] & (1 << zone % 8);
}

bool minix3_take_zone(VirtioDevice *block_device, uint32_t zone) {
    minix3_load_device(block_device);
    if (minix3_has_zone(block_device, zone)) {
        warnf("minix3_take_zone: zone %u is already taken\n", zone);
        return false;
    }

    zone_bitmap[zone / 8] |= (1 << zone % 8);
    minix3_put_zone_bitmap(block_device, zone_bitmap);
    // Inode inode_data = minix3_get_zone(block_device, zone);
    // inode_data.num_links = 1;
    // minix3_put_inode(block_device, inode, inode_data);

    return true;
}

uint32_t minix3_get_next_free_zone(VirtioDevice *block_device) {
    size_t zone_bitmap_size = minix3_get_zone_size(block_device);
    for (int i = 0; i < zone_bitmap_size; i++) {
        if (zone_bitmap[i] != 0xFF) {
            for (int j = 0; j < 8; j++) {
                uint32_t zone = 8 * i + j;
                if (!minix3_has_zone(block_device, zone))
                    return zone;
            }
        }
    }
    warnf("minix3_get_next_free_zone: Couldn't find free zone\n");
    return 0;
}

void minix3_get_zone(VirtioDevice *block_device, uint32_t zone, uint8_t *data) {
    debugf("Getting zone %u (%x)\n", zone, zone);
    SuperBlock sb = minix3_get_superblock(block_device);
    debugf("Zone %u (%x) is at block %u (%x)\n", zone, zone, sb.first_data_zone + zone, sb.first_data_zone + zone);
    if (zone > sb.num_zones + sb.first_data_zone) {
        warnf("Zone %u (%x) is out of bounds\n", zone, zone);
        return;
    }

    if (zone < sb.first_data_zone) {
        warnf("Zone %u (%x) is before the first data zone %u (%x)\n", zone, zone, sb.first_data_zone, sb.first_data_zone);
        return;
    }
    
    // minix3_get_block(minix3_get_superblock().first_data_zone + zone, data);
    minix3_get_block(block_device, zone, data);
}
void minix3_put_zone(VirtioDevice *block_device, uint32_t zone, uint8_t *data) {
    SuperBlock sb = minix3_get_superblock(block_device);
    if (zone > sb.num_zones + sb.first_data_zone) {
        warnf("Zone %u (%x) is out of bounds\n", zone, zone);
        return;
    }

    if (zone < sb.first_data_zone) {
        warnf("Zone %u (%x) is before the first data zone %u (%x)\n", zone, zone, sb.first_data_zone, sb.first_data_zone);
        return;
    }

    minix3_put_block(block_device, zone, data);
}

uint64_t minix3_get_file_size(VirtioDevice *block_device, uint32_t inode) {
    Inode inode_data = minix3_get_inode(block_device, inode);
    if (S_ISREG(inode_data.mode)) {
        return inode_data.size;
    } else if (S_ISDIR(inode_data.mode)) {
        return 0;
    } else if (S_ISBLK(inode_data.mode)) {
        return 0;
    } else {
        fatalf("Unknown inode type %x\n", inode_data.mode);
        return 0;
    }
}

void debug_inode(VirtioDevice *block_device, uint32_t i) {
    Inode inode = minix3_get_inode(block_device, i);
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
    for (uint32_t j=0; j<10; j++) {
        debugf("   zones[%d] = %x (*1024 = %x)\n", j, inode.zones[j], inode.zones[j] * 1024);
    }
}


// A struct that keeps some data for this callback
typedef struct CallbackData {
    const char *mounted_path;
    uint32_t file_count;
    uint32_t dir_count;
} CallbackData;

// A callback function for counting up the files and printing them out
void minix3_init_callback(VirtioDevice *block_device, uint32_t inode, const char *path, char *name, void *data, uint32_t depth) {
    CallbackData *cb_data = (CallbackData *)data;

    for (uint32_t i=0; i<depth; i++) {
        infof("   ");
    }
    if (cb_data->mounted_path) {
        infof("%s\n", cb_data->mounted_path);
        cb_data->mounted_path = NULL;
        return;
    } else {
        infof("%s", name);
    }
    // infof("%s", name);
    // infof("name: %s", name);
    
    if (minix3_is_dir(block_device, inode)) {
        infof("/\n");
        cb_data->dir_count++;
    } else {
        infof("\n");
        cb_data->file_count++;
    }


    // for (uint32_t i=0; i<depth; i++) {
    //     infof("   ");
    // }
    // infof("path: %s\n\n", path);
}

void minix3_init(VirtioDevice *block_device, const char *path)
{
    // Initialize the filesystem
    debugf("Initializing Minix3 filesystem on device %p at %s\n", block_device, path);
    minix3_get_superblock(block_device);
    debugf("Filesystem block size: %d\n", minix3_get_block_size(block_device));
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
        warnf("Minix3 magic is not correct, initializing superblock ourselves...\n");
        minix3_superblock_init(block_device);
    }
    // for (uint16_t i=0; i<sb.imap_blocks; i++) {
    // }
    debugf("Max inode: %d\n", minix3_get_max_inode(block_device));
    // This does not work yet, broken

    // Copy the inode bitmap into memory
    // minix3_get_inode_bitmap(inode_bitmap);
    // // Copy the zone bitmap into memory
    // minix3_get_zone_bitmap(zone_bitmap);

    // inode_bitmap = (uint8_t *)kmalloc(minix3_get_inode_bitmap_size());
    // zone_bitmap = (uint8_t*)kmalloc(minix3_get_zone_bitmap_size(block_device));
    minix3_load_device(block_device);


    /*
    // Filesystem function tests
    for (uint32_t i=minix3_get_min_inode(); i<minix3_get_max_inode(); i++) {
        // debugf("Checking inode %u (%x)...\n", i, i);
        if (!minix3_has_inode(block_device, i)) {
            continue;
        }

        if (minix3_is_dir(block_device, i)) {
            debugf("Directory %u:\n", i);
            uint32_t dot = minix3_find_dir_entry(i, ".");
            uint32_t dotdot = minix3_find_dir_entry(i, "..");
            debugf("   . = %u\n", dot);
            debugf("   .. = %u\n", dotdot);
            uint32_t home;
            if (home = minix3_find_dir_entry(i, "home")) {
                debugf("   home = %u\n", home);
            }
        }


        if (!minix3_is_file(i)) {
            continue;
        }
        debug_inode(i);
        uint64_t file_size = minix3_get_file_size(i);
        debugf("File %u size: %u\n", i, file_size);
        // size_t size = min(file_size, 1024);
        size_t size = file_size;
        uint8_t buf[size];
        debugf("About to read file...\n");
        minix3_read_file(i, buf, size);
        debugf("Read file\n");
        for (uint16_t j=0; j<size; j++) {
            if (j % minix3_get_zone_size(block_device) == 0) {
                infof("[ZONE %u]", j / minix3_get_zone_size(block_device));
            }
            infof("%c", buf[j]);
        }

        char test[] = "Hello world!";
        // for (uint32_t j=0; j<minix3_get_zone_size(block_device) * 3; j+=sizeof(test)) {
        //     minix3_put_data(i, test, j, sizeof(test));
        // }
        // minix3_put_data(i, test, 5, sizeof(test));
        // debugf("Done writing first zone, now second\n");
        minix3_put_data(i, test, minix3_get_zone_size(block_device) - 5, sizeof(test));
        // minix3_put_data(i, test, minix3_get_zone_size(block_device) * 2 + 5, sizeof(test));
        minix3_read_file(i, buf, size);
        debugf("Read file\n");
        for (uint16_t j=0; j<size; j++) {
            if (j % minix3_get_zone_size(block_device) == 0) {
                infof("[ZONE %u]", j / minix3_get_zone_size(block_device));
            }
            infof("%c", buf[j]);
        }

        break;
    }
    */

    // const char *path = "/home/cosc562";

    CallbackData cb_data = {0};
    cb_data.mounted_path = path;
    // uint32_t inode = minix3_get_inode_from_path(block_device, path, false);
    // infof("%s has inode %u\n", path, inode);
    

    minix3_traverse(block_device, 1, "/", &cb_data, 0, 10, minix3_init_callback);

    // infof("Found %u files and %u directories in %s\n", cb_data2.file_count, cb_data2.dir_count, path);

    // minix3_map_files(block_device);

    // Path of the book
    // const char *book_path = "/home/cosc562/subdir1/subdir2/subdir3/subdir4/subdir5/book1.txt";
    // // Get the inode of the book
    // uint32_t book_inode = minix3_get_inode_from_path(block_device, book_path, false);
    // // Get the size of the book
    // uint64_t book_size = minix3_get_file_size(block_device, book_inode);
    // // Allocate a buffer for the book
    // uint8_t *contents = kmalloc(book_size);
    // // Read the book into the buffer
    // minix3_read_file(block_device, book_inode, contents, book_size);
    // // Print the book
    // for (uint64_t i=0; i<book_size; i++) {
    //     infof("%c", contents[i]);
    // }
    // infof("\n");
    // Free the buffer
    // kfree(contents);

    
    // infof("Files:\n");
    // CallbackData cb_data = {0};
    // minix3_traverse(block_device, 1, "/", &cb_data, 0, 10, callback);
    // infof("Found %u files and %u directories in /\n", cb_data.file_count, cb_data.dir_count);
}

SuperBlock minix3_get_superblock(VirtioDevice *block_device) {
    // Get the superblock
    // SuperBlock superblock;
    // Superblock begins at bytes 1024
    if (sb.magic != MINIX3_MAGIC) {
        block_device_read_bytes(block_device, 1024, (uint8_t *)&sb, sizeof(SuperBlock));
    }
    return sb;
}

void minix3_put_superblock(VirtioDevice *block_device, SuperBlock superblock) {
    // Put the superblock
    block_device_write_bytes(block_device, 1024, (uint8_t *)&superblock, sizeof(SuperBlock));
}


uint16_t minix3_get_block_size(VirtioDevice *block_device) {
    SuperBlock superblock = minix3_get_superblock(block_device);
    return 1024 << superblock.log_zone_size;
}

uint16_t minix3_get_zone_size(VirtioDevice *block_device) {
    return minix3_get_block_size(block_device);
}

uint16_t minix3_sectors_per_block(VirtioDevice *block_device) {
    return minix3_get_block_size(block_device) / block_device_get_sector_size(block_device);
}

size_t minix3_get_inode_bitmap_size(VirtioDevice *block_device) {
    return minix3_get_superblock(block_device).imap_blocks * minix3_get_block_size(block_device);
}

size_t minix3_get_zone_bitmap_size(VirtioDevice *block_device) {
    return minix3_get_superblock(block_device).zmap_blocks * minix3_get_block_size(block_device);
}

// Read the inode bitmap into the given buffer
void minix3_get_inode_bitmap(VirtioDevice *block_device, uint8_t *bitmap_buf) {
    debugf("Getting inode bitmap...\n");
    SuperBlock sb = minix3_get_superblock(block_device);

    minix3_get_blocks(block_device, FS_IMAP_IDX, bitmap_buf, sb.imap_blocks);

    // uint8_t inode_byte;
    // uint64_t byte_offset = FS_IMAP_IDX * minix3_get_block_size(block_device) + inode / 8;
    // debugf("About to read inode byte at %u (%x)...\n", byte_offset, byte_offset);
    // block_device_read_bytes(byte_offset, &inode_byte, 1);
    // debugf("Inode byte: %x\n", inode_byte);
    // return inode_byte & (1 << inode % 8);
    // block_device_read_bytes(FS_IMAP_IDX * minix3_get_block_size(block_device), bitmap_buf, minix3_get_block_size(block_device) * sb.imap_blocks);
}
// Write the inode bitmap from the given buffer
void minix3_put_inode_bitmap(VirtioDevice *block_device, uint8_t *bitmap_buf) {
    debugf("Setting inode bitmap...\n");
    SuperBlock sb = minix3_get_superblock(block_device);
    minix3_put_blocks(block_device, FS_IMAP_IDX, bitmap_buf, sb.imap_blocks);
    // block_device_write_bytes(FS_IMAP_IDX * minix3_get_block_size(block_device), bitmap_buf, minix3_get_block_size(block_device) * sb.imap_blocks);
}
// Read the zone bitmap into the given buffer
void minix3_get_zone_bitmap(VirtioDevice *block_device, uint8_t *bitmap_buf) {
    debugf("Getting zone bitmap...\n");
    SuperBlock sb = minix3_get_superblock(block_device);
    minix3_get_blocks(block_device, FS_IMAP_IDX + sb.imap_blocks, bitmap_buf, sb.zmap_blocks);
    // block_device_read_bytes((FS_IMAP_IDX + sb.imap_blocks) * minix3_get_block_size(block_device), bitmap_buf, minix3_get_block_size(block_device) * sb.zmap_blocks);
}
// Write the zone bitmap from the given buffer
void minix3_put_zone_bitmap(VirtioDevice *block_device, uint8_t *bitmap_buf) {
    debugf("Setting zone bitmap...\n");
    SuperBlock sb = minix3_get_superblock(block_device);
    minix3_put_blocks(block_device, FS_IMAP_IDX + sb.imap_blocks, bitmap_buf, sb.zmap_blocks);
}

void minix3_load_device(VirtioDevice *block_device) {
    if (loaded_block_device == block_device) {
        return;
    } else {
        loaded_block_device = block_device;
    }

    if (inode_bitmap && zone_bitmap) {
        minix3_get_inode_bitmap(block_device, inode_bitmap);
        minix3_get_zone_bitmap(block_device, zone_bitmap);
    } else {
        inode_bitmap = (uint8_t *)kmalloc(minix3_get_inode_bitmap_size(block_device));
        zone_bitmap = (uint8_t *)kmalloc(minix3_get_zone_bitmap_size(block_device));
        minix3_get_inode_bitmap(block_device, inode_bitmap);
        minix3_get_zone_bitmap(block_device, zone_bitmap);
    }
}

void minix3_get_blocks(VirtioDevice *block_device, uint32_t start_block, uint8_t *data, uint16_t count) {
    // SuperBlock sb = minix3_get_superblock();
    block_device_read_bytes(block_device, start_block * minix3_get_block_size(block_device), data, minix3_get_block_size(block_device) * count);
}
void minix3_put_blocks(VirtioDevice *block_device, uint32_t start_block, uint8_t *data, uint16_t count) {
    // SuperBlock sb = minix3_get_superblock();
    block_device_write_bytes(block_device, start_block * minix3_get_block_size(block_device), data, minix3_get_block_size(block_device) * count);
}

void minix3_get_block(VirtioDevice *block_device, uint32_t block, uint8_t *data) {
    minix3_get_blocks(block_device, block, data, 1);
}

void minix3_put_block(VirtioDevice *block_device, uint32_t block, uint8_t *data) {
    minix3_put_blocks(block_device, block, data, 1);
}

bool minix3_has_inode(VirtioDevice *block_device, uint32_t inode) {
    minix3_load_device(block_device);

    if (inode == INVALID_INODE) {
        debugf("minix3_has_inode: Invalid inode %u\n", inode);
        return false;
    }
    return inode_bitmap[inode / 8] & (1 << inode % 8);
}

// Mark the inode taken in the inode map.
bool minix3_take_inode(VirtioDevice *block_device, uint32_t inode) {
    minix3_load_device(block_device);
    if (inode == INVALID_INODE) {
        debugf("minix3_has_inode: Invalid inode %u\n", inode);
        return false;
    }
    if (minix3_has_inode(block_device, inode)) {
        warnf("minix3_take_inode: Inode %u is already taken\n", inode);
        return false;
    }

    inode_bitmap[inode / 8] |= (1 << inode % 8);
    minix3_put_inode_bitmap(block_device, inode_bitmap);
    Inode inode_data = minix3_get_inode(block_device, inode);
    inode_data.num_links = 1;
    minix3_put_inode(block_device, inode, inode_data);

    return true;
}

uint32_t minix3_get_next_free_inode(VirtioDevice *block_device) {
    debugf("Getting next free inode...\n");
    minix3_load_device(block_device);
    size_t inode_bitmap_size = minix3_get_inode_bitmap_size(block_device);

    for (size_t i = 0; i < inode_bitmap_size; i++) {
        if (inode_bitmap[i] != 0xFF) {
            for (size_t j = 0; j < 8; j++) {
                uint32_t inode = 8 * i + j;
                if (!minix3_has_inode(block_device, inode)) {
                    return inode;
                }
            }
        }
    }

    warnf("minix3_get_next_free_inode: Couldn't find free inode\n");
    return 0;
}

static uint32_t last_inode = 0; // Last inode number we looked up
static Inode last_inode_data; // Data of the last inode

Inode minix3_get_inode(VirtioDevice *block_device, uint32_t inode) {
    minix3_load_device(block_device);
    if (inode == INVALID_INODE) {
        warnf("minix3_get_inode: Invalid inode %u\n", inode);
        return (Inode){0};
    }
    else if (inode == last_inode) {
        return last_inode_data;
    }
    SuperBlock sb = minix3_get_superblock(block_device);
    Inode data;
    uint64_t offset = minix3_get_inode_byte_offset(block_device, sb, inode);

    // block_device_read_bytes(offset, (uint8_t*)&data, sizeof(Inode));
    block_device_read_bytes(block_device, offset, (uint8_t*)&data, sizeof(Inode));
    last_inode_data = data;
    last_inode = inode;
    return data;
}

void minix3_put_inode(VirtioDevice *block_device, uint32_t inode, Inode data) {
    minix3_load_device(block_device);
    if (inode == INVALID_INODE) {
        warnf("minix3_put_inode: Invalid inode %u\n", inode);
        return;
    }
    SuperBlock sb = minix3_get_superblock(block_device);
    uint64_t offset = minix3_get_inode_byte_offset(block_device, sb, inode);

    // debugf("Putting inode %u at offset %u (%x)...\n", inode, offset, offset);
    block_device_write_bytes(block_device, offset, (uint8_t*)&data, sizeof(Inode));
}

// Allocate a free inode.
// Return the allocated zero'd inode. 
uint32_t minix3_alloc_inode(VirtioDevice *block_device) {
    uint32_t free_inode = minix3_get_next_free_inode(block_device);
    if (!free_inode) {
        warnf("minix3_alloc_inode: Couldn't find free inode\n");
        return 0;
    } else {
        debugf("minix3_alloc_inode: Found free inode %u\n", free_inode);
    }
    if (!minix3_take_inode(block_device, free_inode)) {
        warnf("minix3_alloc_inode: Couldn't take inode %u\n", free_inode);
        return 0;
    }

    Inode data;
    memset(&data, 0, sizeof(data));
    data.num_links = 1;
    minix3_put_inode(block_device, free_inode, data);
    // infof("minix3_alloc_inode %p\n", minix3_get_inode_byte_offset(sb, free_inode)); // TODO: REMOVE
    return free_inode;
}

uint32_t minix3_alloc_zone(VirtioDevice *block_device) {
    uint32_t free_zone = minix3_get_next_free_zone(block_device);
    if (!free_zone) {
        warnf("minix3_alloc_zone: Couldn't find free zone\n");
        return 0;
    } else {
        debugf("minix3_alloc_zone: Found free zone %u\n", free_zone);
    }
    if (!minix3_take_zone(block_device, free_zone)) {
        warnf("minix3_alloc_zone: Couldn't take zone %u\n", free_zone);
        return 0;
    }
    return free_zone;
}

bool minix3_is_dir(VirtioDevice *block_device, uint32_t inode) {
    Inode inode_data = minix3_get_inode(block_device, inode);
    if (inode_data.num_links == 0) {
        warnf("minix3_is_dir: Inode %u has no links\n", inode);
    }
    return S_ISDIR(inode_data.mode) && inode_data.num_links > 0;
}

bool minix3_is_file(VirtioDevice *block_device, uint32_t inode) {
    Inode inode_data = minix3_get_inode(block_device, inode);
    if (inode_data.num_links == 0) {
        warnf("minix3_is_file: Inode %u has no links\n", inode);
    }
    return S_ISREG(inode_data.mode) && inode_data.num_links > 0;
}

bool minix3_is_block_device(VirtioDevice *block_device, uint32_t inode) {
    Inode inode_data = minix3_get_inode(block_device, inode);
    if (inode_data.num_links == 0) {
        warnf("minix3_is_block_device: Inode %u has no links\n", inode);
    }
    return S_ISBLK(inode_data.mode) && inode_data.num_links > 0;
}

void minix3_read_file(VirtioDevice *block_device, uint32_t inode, uint8_t *data, uint32_t count) {
    minix3_get_data(block_device, inode, data, 0, count);
}

void minix3_get_data(VirtioDevice *block_device, uint32_t inode, uint8_t *data, uint32_t offset, uint32_t count) {
    debugf("minix3_get_data: Getting data from inode %u, offset %u, count %u\n", inode, offset, count);
    // First, get the inode
    Inode inode_data = minix3_get_inode(block_device, inode);
    debugf("minix3_get_data: Got inode %u\n", inode);
    

    uint8_t zone_data[minix3_get_zone_size(block_device)];

    // The cursor is the current position in the data buffer
    uint32_t buffer_cursor = 0;
    uint32_t file_cursor = 0;

    // Now, get the data
    // The first 7 zones are direct zones
    for (uint32_t direct_zone=0; direct_zone<7; direct_zone++) {
        uint32_t zone = inode_data.zones[direct_zone];
        if (zone == 0) {
            debugf("No direct zone #%d = %d\n", direct_zone, zone);
            continue;
        } else {
            debugf("Direct zone #%d = %d\n", direct_zone, zone);
        }
        memset(zone_data, 0, minix3_get_zone_size(block_device));

        if (file_cursor + minix3_get_zone_size(block_device) < offset) {
            // We're not at the offset yet
            file_cursor += minix3_get_zone_size(block_device);
            continue;
        } else if (file_cursor < offset) {
            // We're in the middle of the offset
            // Read the zone into the buffer
            debugf("Reading first direct zone %d\n", zone);
            minix3_get_zone(block_device, zone, zone_data);
            debugf("Read first direct zone %d\n", zone);
            // Copy the remaining data into the buffer
            size_t remaining = min(count, minix3_get_zone_size(block_device) - (offset - file_cursor));
            memcpy(data, zone_data + offset - file_cursor, remaining);
            buffer_cursor += remaining;
            file_cursor = offset + remaining;
            continue;
        }
        debugf("file_cursor = %u, offset = %u\n", file_cursor, offset);

        // Read the zone into the buffer
        minix3_get_zone(block_device, zone, zone_data);

        // If the cursor is past the amount of data we want, we're done
        if (buffer_cursor + minix3_get_zone_size(block_device) > count) {
            debugf("Reading last direct zone %d\n", zone);
            // Copy the remaining data into the buffer
            memcpy(data + buffer_cursor, zone_data, count - buffer_cursor);
            // We're done
            return;
        } else {
            debugf("Reading direct zone %d\n", zone);
            // Copy the entire zone into the buffer
            memcpy(data + buffer_cursor, zone_data, minix3_get_zone_size(block_device));
        }
        buffer_cursor += minix3_get_zone_size(block_device);
    }

    debugf("Done with direct zones\n");
    // The next zone is an indirect zone
    if (inode_data.zones[7] != 0) {
        debugf("Reading indirect zone %d\n", inode_data.zones[7]);
        uint32_t indirect_zones[minix3_get_zone_size(block_device) / sizeof(uint32_t)];
        minix3_get_zone(block_device, inode_data.zones[7], (uint8_t*)indirect_zones);
        

        for (uint32_t indirect_zone=0; indirect_zone<minix3_get_zone_size(block_device) / sizeof(uint32_t); indirect_zone++) {
            uint32_t zone = indirect_zones[indirect_zone];
            debugf("Zone #%d\n", indirect_zone);
            if (zone == 0) {
                debugf("No indirect zone %d\n", indirect_zone);
            } else {
                debugf("Reading indirect zone %d\n", zone);
            }
            

            if (file_cursor + minix3_get_zone_size(block_device) < offset) {
                // We're not at the offset yet
                file_cursor += minix3_get_zone_size(block_device);
                continue;
            } else if (file_cursor < offset) {
                // We're in the middle of the offset
                // Read the zone into the buffer
                minix3_get_zone(block_device, zone, zone_data);
                // Copy the remaining data into the buffer
                size_t remaining = min(count, minix3_get_zone_size(block_device) - (offset - file_cursor));
                memcpy(data, zone_data + offset - file_cursor, remaining);
                buffer_cursor += remaining;
                file_cursor = offset + remaining;
                continue;
            }
            
            memset(zone_data, 0, minix3_get_zone_size(block_device));

            // Read the zone into the buffer
            minix3_get_zone(block_device, zone, zone_data);
            // If the cursor is past the amount of data we want, we're done
            if (buffer_cursor + minix3_get_zone_size(block_device) > count) {
                // Copy the remaining data into the buffer
                memcpy(data + buffer_cursor, zone_data, count - buffer_cursor);
                // We're done
                return;
            } else {
                // Copy the entire zone into the buffer
                memcpy(data + buffer_cursor, zone_data, minix3_get_zone_size(block_device));
            }
            buffer_cursor += minix3_get_zone_size(block_device);
        }
    } else {
        debugf("No indirect zone\n");
    }

    // The next zone is a double indirect zone
    if (inode_data.zones[8] != 0) {
        debugf("Reading double indirect zone %d\n", inode_data.zones[8]);
        uint32_t double_indirect_zones[minix3_get_zone_size(block_device) / sizeof(uint32_t)];
        // We're done
        minix3_get_zone(block_device, inode_data.zones[8], (uint8_t*)double_indirect_zones);

        for (uint32_t double_indirect_zone=0; double_indirect_zone<minix3_get_zone_size(block_device) / sizeof(uint32_t); double_indirect_zone++) {
            uint32_t indirect_zone = double_indirect_zones[double_indirect_zone];
            if (indirect_zone == 0) continue;

            uint32_t indirect_zones[minix3_get_zone_size(block_device) / sizeof(uint32_t)];
            minix3_get_zone(block_device, indirect_zone, (uint8_t*)indirect_zones);

            for (uint32_t indirect_zone=0; indirect_zone<minix3_get_zone_size(block_device) / sizeof(uint32_t); indirect_zone++) {
                uint32_t zone = indirect_zones[indirect_zone];
                if (zone == 0) {
                    debugf("No double indirect zone %d\n", indirect_zone);
                    continue;
                }
                debugf("Reading double indirect zone %d\n", zone);

                if (file_cursor + minix3_get_zone_size(block_device) < offset) {
                    // We're not at the offset yet
                    file_cursor += minix3_get_zone_size(block_device);
                    continue;
                } else if (file_cursor < offset) {
                    // We're in the middle of the offset
                    // Read the zone into the buffer
                    minix3_get_zone(block_device, zone, zone_data);
                    // Copy the remaining data into the buffer
                    size_t remaining = min(count, minix3_get_zone_size(block_device) - (offset - file_cursor));
                    memcpy(data, zone_data + offset - file_cursor, remaining);
                    buffer_cursor += remaining;
                    file_cursor = offset + remaining;
                    continue;
                }
                
                // Read the zone into the buffer
                memset(zone_data, 0, minix3_get_zone_size(block_device));
                minix3_get_zone(block_device, zone, zone_data);

                // If the cursor is past the amount of data we want, we're done
                if (buffer_cursor + minix3_get_zone_size(block_device) > count) {
                    // Copy the remaining data into the buffer
                    memcpy(data + buffer_cursor, zone_data, count - buffer_cursor);
                    // We're done
                    return;
                } else {
                    // Copy the entire zone into the buffer
                    memcpy(data + buffer_cursor, zone_data, minix3_get_zone_size(block_device));
                }
                buffer_cursor += minix3_get_zone_size(block_device);
            }
        }
    } else {
        debugf("No double indirect zone\n");
    }

    // The next zone is a triple indirect zone
    if (inode_data.zones[9] != 0) {
        uint32_t triple_indirect_zones[minix3_get_zone_size(block_device) / sizeof(uint32_t)];
        minix3_get_zone(block_device, inode_data.zones[9], (uint8_t*)triple_indirect_zones);

        for (uint32_t triple_indirect_zone=0; triple_indirect_zone<minix3_get_zone_size(block_device) / sizeof(uint32_t); triple_indirect_zone++) {
            uint32_t double_indirect_zone = triple_indirect_zones[triple_indirect_zone];
            if (double_indirect_zone == 0) continue;
            uint32_t double_indirect_zones[minix3_get_zone_size(block_device) / sizeof(uint32_t)];
            minix3_get_zone(block_device, double_indirect_zone, (uint8_t*)double_indirect_zones);

            for (uint32_t double_indirect_zone=0; double_indirect_zone<minix3_get_zone_size(block_device) / sizeof(uint32_t); double_indirect_zone++) {
                uint32_t indirect_zone = double_indirect_zones[double_indirect_zone];
                if (indirect_zone == 0) continue;
                uint32_t indirect_zones[minix3_get_zone_size(block_device) / sizeof(uint32_t)];
                minix3_get_zone(block_device, indirect_zone, (uint8_t*)indirect_zones);

                for (uint32_t indirect_zone=0; indirect_zone<minix3_get_zone_size(block_device) / sizeof(uint32_t); indirect_zone++) {
                    uint32_t zone = indirect_zones[indirect_zone];
                    if (zone == 0) continue;
                    debugf("Reading triple indirect zone %d\n", zone);
                    
                    if (file_cursor + minix3_get_zone_size(block_device) < offset) {
                        // We're not at the offset yet
                        file_cursor += minix3_get_zone_size(block_device);
                        continue;
                    } else if (file_cursor < offset) {
                        // We're in the middle of the offset
                        // Read the zone into the buffer
                        minix3_get_zone(block_device, zone, zone_data);
                        // Copy the remaining data into the buffer
                        size_t remaining = min(count, minix3_get_zone_size(block_device) - (offset - file_cursor));
                        memcpy(data, zone_data + offset - file_cursor, remaining);
                        buffer_cursor += remaining;
                        file_cursor = offset + remaining;
                        continue;
                    }
                
                    
                    // Read the zone into the buffer
                    memset(zone_data, 0, minix3_get_zone_size(block_device));
                    minix3_get_zone(block_device, zone, zone_data);

                    // If the cursor is past the amount of data we want, we're done
                    if (buffer_cursor + minix3_get_zone_size(block_device) > count) {
                        // Copy the remaining data into the buffer
                        memcpy(data + buffer_cursor, zone_data, count - buffer_cursor);
                        // We're done
                        return;
                    } else {
                        // Copy the entire zone into the buffer
                        memcpy(data + buffer_cursor, zone_data, minix3_get_zone_size(block_device));
                    }
                    buffer_cursor += minix3_get_zone_size(block_device);
                }
            }
        }
    } else {
        debugf("No triple indirect zone\n");
    }

    // If we get here, we've read all the data we can
    return;
}
void minix3_put_data(VirtioDevice *block_device, uint32_t inode, uint8_t *data, uint32_t offset, uint32_t count) {
    // First, get the inode
    Inode inode_data = minix3_get_inode(block_device, inode);

    uint8_t zone_data[minix3_get_zone_size(block_device)];

    // The cursor is the current position in the data buffer
    uint32_t buffer_cursor = 0;
    // The file cursor is the current position in the file
    uint32_t file_cursor = 0;

    // Now, get the data
    // The first 7 zones are direct zones
    for (uint32_t direct_zone=0; direct_zone<7; direct_zone++) {
        uint32_t zone = inode_data.zones[direct_zone];
        if (zone == 0) {
            debugf("No direct zone %d\n", zone);
            continue;
        }
        if (file_cursor + minix3_get_zone_size(block_device) < offset) {
            // We're not at the offset yet
            file_cursor += minix3_get_zone_size(block_device);
            continue;
        } else if (file_cursor < offset) {
            // We're in the middle of the offset
            // Read the zone into the buffer
            debugf("Writing first direct zone %d\n", zone);
            minix3_get_zone(block_device, zone, zone_data);

            // Copy the remaining data into the buffer
            size_t remaining = min(count, minix3_get_zone_size(block_device) - (offset - file_cursor));
            memcpy(zone_data + offset - file_cursor, data, remaining);
            debugf("Wrote %u bytes to zone %u at offset %u (0x%x)\n", remaining, zone, offset, offset);
            minix3_put_zone(block_device, zone, zone_data);

            buffer_cursor += remaining;
            file_cursor = offset + remaining;
            if (buffer_cursor >= count) {
                // We're done
                return;
            }

            continue;
        }

        memset(zone_data, 0, minix3_get_zone_size(block_device));

        // Write the buffer into the zone
        if (buffer_cursor + minix3_get_zone_size(block_device) > count) {
            minix3_get_zone(block_device, zone, zone_data);
            debugf("Writing last direct zone %d\n", zone);
            // Copy the remaining data into the buffer
            memcpy(zone_data, data + buffer_cursor, count - buffer_cursor);
            // We're done
            minix3_put_zone(block_device, zone, zone_data);
            return;
        } else {
            debugf("Writing direct zone %d\n", zone);
            // Copy the entire zone into the buffer
            memcpy(zone_data, data + buffer_cursor, minix3_get_zone_size(block_device));
            minix3_put_zone(block_device, zone, zone_data);
        }
        buffer_cursor += minix3_get_zone_size(block_device);
    }

    debugf("Done with direct zones\n");
    // The next zone is an indirect zone
    if (inode_data.zones[7] != 0) {
        uint32_t indirect_zones[minix3_get_zone_size(block_device) / sizeof(uint32_t)];
        minix3_get_zone(block_device, inode_data.zones[7], (uint8_t*)indirect_zones);

        for (uint32_t indirect_zone=0; indirect_zone<minix3_get_zone_size(block_device) / sizeof(uint32_t); indirect_zone++) {
            uint32_t zone = indirect_zones[indirect_zone];
            if (zone == 0) continue;
            debugf("Writing indirect zone %d\n", zone);

            if (file_cursor + minix3_get_zone_size(block_device) < offset) {
                // We're not at the offset yet
                file_cursor += minix3_get_zone_size(block_device);
                continue;
            } else if (file_cursor < offset) {
                // We're in the middle of the offset
                // Read the zone into the buffer
                minix3_get_zone(block_device, zone, zone_data);
                // Copy the remaining data into the buffer
                size_t remaining = min(count, minix3_get_zone_size(block_device) - (offset - file_cursor));
                memcpy(zone_data + offset - file_cursor, data, remaining);
                minix3_put_zone(block_device, zone, zone_data);

                buffer_cursor += remaining;
                file_cursor = offset + remaining;
                if (buffer_cursor >= count) {
                    // We're done
                    return;
                }
                continue;
            }

            memset(zone_data, 0, minix3_get_zone_size(block_device));

            if (buffer_cursor + minix3_get_zone_size(block_device) > count) {
                minix3_get_zone(block_device, zone, zone_data);
                // Copy the remaining data into the buffer
                memcpy(zone_data, data + buffer_cursor, count - buffer_cursor);
                // We're done
                minix3_put_zone(block_device, zone, zone_data);
                return;
            } else {
                // Copy the entire zone into the buffer
                memcpy(zone_data, data + buffer_cursor, minix3_get_zone_size(block_device));
                minix3_put_zone(block_device, zone, zone_data);
            }
            buffer_cursor += minix3_get_zone_size(block_device);
        }
    } else {
        debugf("No indirect zone\n");
    }

    // The next zone is a double indirect zone
    // The next zone is a double indirect zone
    if (inode_data.zones[8] != 0) {
        uint32_t double_indirect_zones[minix3_get_zone_size(block_device) / sizeof(uint32_t)];
        // We're done
        minix3_get_zone(block_device, inode_data.zones[8], (uint8_t*)double_indirect_zones);

        for (uint32_t double_indirect_zone=0; double_indirect_zone<minix3_get_zone_size(block_device) / sizeof(uint32_t); double_indirect_zone++) {
            uint32_t indirect_zone = double_indirect_zones[double_indirect_zone];
            if (indirect_zone == 0) continue;

            uint32_t indirect_zones[minix3_get_zone_size(block_device) / sizeof(uint32_t)];
            minix3_get_zone(block_device, indirect_zone, (uint8_t*)indirect_zones);

            for (uint32_t indirect_zone=0; indirect_zone<minix3_get_zone_size(block_device) / sizeof(uint32_t); indirect_zone++) {
                uint32_t zone = indirect_zones[indirect_zone];
                if (zone == 0) continue;
                debugf("Writing double indirect zone %d\n", zone);

                if (file_cursor + minix3_get_zone_size(block_device) < offset) {
                    // We're not at the offset yet
                    file_cursor += minix3_get_zone_size(block_device);
                    continue;
                } else if (file_cursor < offset) {
                    // We're in the middle of the offset
                    // Read the zone into the buffer
                    minix3_get_zone(block_device, zone, zone_data);
                    // Copy the remaining data into the buffer
                    size_t remaining = min(count, minix3_get_zone_size(block_device) - (offset - file_cursor));
                    memcpy(zone_data + offset - file_cursor, data, remaining);
                    minix3_put_zone(block_device, zone, zone_data);

                    buffer_cursor += remaining;
                    file_cursor = offset + remaining;
                    if (buffer_cursor >= count) {
                        // We're done
                        return;
                    }
                    continue;
                }

                memset(zone_data, 0, minix3_get_zone_size(block_device));
                if (buffer_cursor + minix3_get_zone_size(block_device) > count) {
                    minix3_get_zone(block_device, zone, zone_data);
                    // Copy the remaining data into the buffer
                    memcpy(zone_data, data + buffer_cursor, count - buffer_cursor);
                    // We're done
                    minix3_put_zone(block_device, zone, zone_data);
                    return;
                } else {
                    // Copy the entire zone into the buffer
                    memcpy(zone_data, data + buffer_cursor, minix3_get_zone_size(block_device));
                    minix3_put_zone(block_device, zone, zone_data);
                }

                buffer_cursor += minix3_get_zone_size(block_device);
            }
        }
    } else {
        debugf("No double indirect zone\n");
    }

    // The next zone is a triple indirect zone
    if (inode_data.zones[9] != 0) {
        uint32_t triple_indirect_zones[minix3_get_zone_size(block_device) / sizeof(uint32_t)];
        minix3_get_zone(block_device, inode_data.zones[9], (uint8_t*)triple_indirect_zones);

        for (uint32_t triple_indirect_zone=0; triple_indirect_zone<minix3_get_zone_size(block_device) / sizeof(uint32_t); triple_indirect_zone++) {
            uint32_t double_indirect_zone = triple_indirect_zones[triple_indirect_zone];
            if (double_indirect_zone == 0) continue;
            uint32_t double_indirect_zones[minix3_get_zone_size(block_device) / sizeof(uint32_t)];
            minix3_get_zone(block_device, double_indirect_zone, (uint8_t*)double_indirect_zones);

            for (uint32_t double_indirect_zone=0; double_indirect_zone<minix3_get_zone_size(block_device) / sizeof(uint32_t); double_indirect_zone++) {
                uint32_t indirect_zone = double_indirect_zones[double_indirect_zone];
                if (indirect_zone == 0) continue;
                uint32_t indirect_zones[minix3_get_zone_size(block_device) / sizeof(uint32_t)];
                minix3_get_zone(block_device, indirect_zone, (uint8_t*)indirect_zones);

                for (uint32_t indirect_zone=0; indirect_zone<minix3_get_zone_size(block_device) / sizeof(uint32_t); indirect_zone++) {
                    uint32_t zone = indirect_zones[indirect_zone];
                    if (zone == 0) continue;
                    debugf("Writing triple indirect zone %d\n", zone);

                    if (file_cursor + minix3_get_zone_size(block_device) < offset) {
                        // We're not at the offset yet
                        file_cursor += minix3_get_zone_size(block_device);
                        continue;
                    } else if (file_cursor < offset) {
                        // We're in the middle of the offset
                        // Read the zone into the buffer
                        minix3_get_zone(block_device, zone, zone_data);
                        // Copy the remaining data into the buffer
                        size_t remaining = min(count, minix3_get_zone_size(block_device) - (offset - file_cursor));
                        memcpy(zone_data + offset - file_cursor, data, remaining);
                        minix3_put_zone(block_device, zone, zone_data);

                        buffer_cursor += remaining;
                        file_cursor = offset + remaining;
                        if (buffer_cursor >= count) {
                            // We're done
                            return;
                        }
                        continue;
                    }

                    memset(zone_data, 0, minix3_get_zone_size(block_device));

                    if (buffer_cursor + minix3_get_zone_size(block_device) > count) {
                        minix3_get_zone(block_device, zone, zone_data);
                        // Copy the remaining data into the buffer
                        memcpy(zone_data, data + buffer_cursor, count - buffer_cursor);
                        // We're done
                        minix3_put_zone(block_device, zone, zone_data);
                        return;
                    } else {
                        // Copy the entire zone into the buffer
                        memcpy(zone_data, data + buffer_cursor, minix3_get_zone_size(block_device));
                        minix3_put_zone(block_device, zone, zone_data);
                    }
                    buffer_cursor += minix3_get_zone_size(block_device);
                }
            }
        }
    } else {
        debugf("No triple indirect zone\n");
    }


    // If we get here, we've read all the data we can
    return;
}

// TODO: Figure out a better way to report error than return -1 since it can 
// be a valid dir entry number.
uint32_t minix3_find_next_free_dir_entry(VirtioDevice *block_device, uint32_t inode) {
    if (!minix3_is_dir(block_device, inode)) {
        warnf("minix3_find_next_free_dir_entry: Inode %u (0x%x) not a directory\n", inode, inode);
        return -1;
    }

    Inode data = minix3_get_inode(block_device, inode);
    DirEntry entry;
    for (size_t i = 0; i < minix3_get_zone_size(block_device) / sizeof(DirEntry); i++) {
        minix3_get_dir_entry(block_device, inode, i, &entry);

        if (entry.inode == 0) {
            debugf("minix3_find_next_free_dir_entry: Found free entry %u\n", i);
            return i;
        } else {
            debugf("minix3_find_next_free_dir_entry: Entry %u is %s\n", i, entry.name);
        }
    }
    warnf("minix3_find_next_free_dir_entry: Couldn't find a free directory entry\n");
    return -1;
}

bool minix3_get_dir_entry(VirtioDevice *block_device, uint32_t inode, uint32_t entry, DirEntry *data) {
    if (!minix3_is_dir(block_device, inode)) {
        warnf("minix3_get_dir_entry: Inode %u (%x) is not a directory\n", inode, inode);
        return false;
    }
    debugf("minix3_get_dir_entry: Getting entry %u from inode %u\n", entry, inode);
    Inode inode_data = minix3_get_inode(block_device, inode);
    debugf("minix3_get_dir_entry: Getting entry %u from inode %u\n", entry, inode);
    DirEntry tmp;

    uint32_t offset = entry * sizeof(DirEntry);
    debugf("minix3_get_dir_entry: Getting entry %u from inode %u at offset %u\n", entry, inode, offset);
    minix3_get_data(block_device, inode, (uint8_t*)&tmp, offset, sizeof(DirEntry));
    debugf("minix3_get_dir_entry: Got entry %s at offset %u from inode %u\n", tmp.name, offset, inode);
    memcpy(data, &tmp, sizeof(DirEntry));
    debugf("minix3_get_dir_entry: Got entry %s at offset %u from inode %u\n", tmp.name, offset, inode);
    if (tmp.inode == 0) {
        return false;
    }
    if (tmp.inode == INVALID_INODE) {
        return false;
    }
    debugf("minix3_get_dir_entry: Got entry %s at inode %u\n", tmp.name, tmp.inode);

    return true;
}

void minix3_put_dir_entry(VirtioDevice *block_device, uint32_t inode, uint32_t entry, DirEntry data) {
    if (!minix3_is_dir(block_device, inode)) {
        warnf("Inode %u (%x) is not a directory\n", inode, inode);
        return;
    }
    // Inode inode_data = minix3_get_inode(block_device, inode);
    debugf("Putting entry %u to inode %u\n", entry, inode);

    minix3_put_data(block_device, inode, (uint8_t*)&data, entry * sizeof(DirEntry), sizeof(DirEntry));
}


// List all of the entries in the given directory to the given buffer.
uint32_t minix3_list_dir(VirtioDevice *block_device, uint32_t inode, DirEntry *entries, uint32_t max_entries) {
    debugf("minix3_list_dir: Listing directory %u\n", inode);
    if (!minix3_is_dir(block_device, inode)) {
        warnf("minix3_list_dir: Inode %u (%x) is not a directory\n", inode, inode);
        return 0;
    }
    Inode inode_data = minix3_get_inode(block_device, inode);
    debugf("minix3_list_dir: Got inode %u\n", inode);
    uint32_t entry = 0;
    DirEntry tmp;
    while (minix3_get_dir_entry(block_device, inode, entry, &tmp)) {
        debugf("minix3_list_dir: Found entry %s at inode %u\n", tmp.name, tmp.inode);
        memcpy(entries + entry, &tmp, sizeof(DirEntry));
        entry++;
        if (entry >= max_entries) {
            warnf("minix3_list_dir: Too many entries in directory %u\n", inode);
            break;
        }
    }
    debugf("minix3_list_dir: Listed directory %u\n", inode);
    return entry;
}
// Returns the inode number of the file with the given name in the given directory.
// If the file does not exist, return INVALID_INODE.
uint32_t minix3_find_dir_entry(VirtioDevice *block_device, uint32_t inode, const char *name) {
    minix3_load_device(block_device);
    debugf("minix3_find_dir_entry: Finding entry %s in inode %u\n", name, inode);
    
    DirEntry entries[128];
    uint32_t num_entries = minix3_list_dir(block_device, inode, entries, 128);

    for (uint32_t i=0; i<num_entries; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            debugf("minix3_find_dir_entry: Found entry %s at inode %u\n", name, entries[i].inode);
            trap_frame_debug(kernel_trap_frame);
            return entries[i].inode;
        }
    }
    warnf("Could not find entry %s in inode %u\n", name, inode);
    return INVALID_INODE;
}

void strcat(char *dest, char *src) {
    uint32_t i, j;
    for (i=0; dest[i] != 0; i++) {
    }
    for (j=0; src[j] != 0; j++) {
        dest[i + j] = src[j];
    }
    dest[i + j] = 0;
}

void minix3_traverse(VirtioDevice *block_device, uint32_t inode, char *root_path, void *data, uint32_t current_depth, uint32_t max_depth, void (*callback)(VirtioDevice *block_device, uint32_t inode, const char *path, char *name, void *data, uint32_t depth)) {
    debugf("Traversing %s: inode %u at depth %d\n", root_path, inode, current_depth);
    if (current_depth > max_depth) {
        return;
    }
    if (!minix3_has_inode(block_device, inode)) {
        warnf("Inode %u does not exist\n", inode);
        return;
    }
    debugf("Traversing inode %u at depth %d\n", inode, current_depth);
    char name[128];
    strncpy(name, path_file_name(root_path), sizeof(name));
    if (minix3_is_dir(block_device, inode)) {
        debugf("Is a directory\n");
    } else {
        debugf("Not a directory\n");
        callback(block_device, inode, root_path, name, data, current_depth);
        return;
    }

    Inode inode_data = minix3_get_inode(block_device, inode);
    debug_inode(block_device, inode);
    uint32_t max_entries = minix3_get_zone_size(block_device) / sizeof(DirEntry);
    debugf("Max entries: %u\n", max_entries);
    DirEntry entries[max_entries];
    debugf("Allocated entries\n");
    uint32_t num_entries = minix3_list_dir(block_device, inode, entries, max_entries);
    debugf("Found %u entries\n", num_entries);

    char *path = kmalloc(1024);
    callback(block_device, inode, root_path, name, data, current_depth);
    for (uint32_t i=0; i<num_entries; i++) {
        if (entries[i].inode == INVALID_INODE) {
            continue;
        }
        strncpy(name, entries[i].name, sizeof(name));
        // for (j=0; j<sizeof(entries[0].name) && j<sizeof(name); j++) {
        //     if (entries[i].name[j] == 0) {
        //         name[j] = 0;
        //         break;
        //     }
        //     name[j] = entries[i].name[j];
        // }
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        strcpy(path, root_path);
        if (strcmp(path, "/") != 0) {
            strcat(path, "/");
        }
        strcat(path, name);

        minix3_traverse(block_device, entries[i].inode, path, data, current_depth + 1, max_depth, callback);
    }

    kfree(path);
    return;
}
