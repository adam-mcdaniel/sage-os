#include <minix3.h>
#include <block.h>
#include <debug.h>
#include <stdint.h>
#include <util.h>
#include <list.h>
#include <path.h>
#include <map.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// #define MINIX3_DEBUG

#ifdef MINIX3_DEBUG
#define debugf(...) debugf(__VA_ARGS__)
#else
#define debugf(...)
#endif

static SuperBlock sb;

// A map of file paths (absolute path strings) to inodes
static Map *mapped_paths;

// A map of inodes to file paths (absolute path strings)
static Map *mapped_inodes;

const char *minix3_inode_to_path(uint32_t inode) {
    uint64_t path;
    map_get_int(mapped_inodes, inode, &path);
    return (const char *)path;
}

uint32_t minix3_path_to_inode(const char *path) {
    uint64_t inode;
    map_get(mapped_paths, path, &inode);
    return inode;
}

void map_callback(uint32_t inode, const char *path, char *name, void *data, uint32_t depth) {
    infof("Mapping %s to %u\n", path, inode);
    map_set(mapped_paths, path, inode);
    map_set_int(mapped_inodes, inode, (uintptr_t)path);
}

// This creates the `mapped_paths` and `mapped_inodes` maps
// for caching the paths of files and their inodes
void minix3_map_files(void) {
    infof("Mapping files...\n");
    mapped_paths = map_new();
    mapped_inodes = map_new();

    minix3_traverse(1, "/", NULL, 0, 10, map_callback);
}

static uint8_t *inode_bitmap;
static uint8_t *zone_bitmap;

void debug_dir_entry(DirEntry entry) {
    debugf("Entry: %u `", entry.inode);
    for (uint8_t i=0; i<60; i++) {
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

uint32_t minix3_get_inode_from_path(const char *path) {
    List *path_items = path_split(path);

    uint32_t parent = 1; // Root inode

    uint32_t i = 0, num_items = list_size(path_items);
    ListElem *elem;
    list_for_each(path_items, elem) {
        char *name = (char *)list_elem_value(elem);
        infof("Getting inode from relative path %s, num_items = %u\n", name, num_items);
        if (strcmp(name, "/") == 0 || strcmp(name, "") == 0) {
            return parent;
        }
        uint32_t child = minix3_find_dir_entry(parent, name);
        infof("Got child %u\n", child);
        parent = child;
        i++;
    }

    return parent;
}


uint32_t minix3_get_min_inode() {
    return 1;
}

uint32_t minix3_get_max_inode() {
    return minix3_get_block_size() * minix3_get_superblock().imap_blocks * 8;
}

uintptr_t minix3_get_inode_byte_offset(SuperBlock sb, uint32_t inode) {
    if (inode == INVALID_INODE) {
        fatalf("Invalid inode %u\n", inode);
        return 0;
    }
    return (FS_IMAP_IDX + sb.imap_blocks + sb.zmap_blocks) * minix3_get_block_size() + (inode - 1) * sizeof(Inode);
}
// uintptr_t minix3_get_inode_bitmap_byte_offset(SuperBlock sb, uint32_t inode) {
//     return FS_IMAP_IDX * minix3_get_block_size() + inode / 8;
// }


void minix3_superblock_init(void) {
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
    minix3_put_superblock(superblock);
}

bool minix3_has_zone(uint32_t zone) {
    return inode_bitmap[zone / 8] & (1 << zone % 8);
}

bool minix3_take_zone(uint32_t zone) {
    return inode_bitmap[zone / 8] |= (1 << zone % 8);
}

uint32_t minix3_get_next_free_zone() {
    size_t zone_bitmap_size = minix3_get_zone_size();
    for (int i = 0; i < zone_bitmap_size; i++) {
        if (zone_bitmap[i] != 0xFF) {
            for (int j = 0; j < 8; j++) {
                uint32_t zone = 8 * i + j;
                if (!minix3_has_zone(zone))
                    return zone;
            }
        }
    }
    warnf("minix3_get_next_free_zone: Couldn't find free zone\n");
    return 0;
}

void minix3_get_zone(uint32_t zone, uint8_t *data) {
    SuperBlock sb = minix3_get_superblock();
    if (zone > sb.num_zones + sb.first_data_zone) {
        fatalf("Zone %u (%x) is out of bounds\n", zone, zone);
        return;
    }

    if (zone < sb.first_data_zone) {
        fatalf("Zone %u (%x) is before the first data zone %u (%x)\n", zone, zone, sb.first_data_zone, sb.first_data_zone);
        return;
    }
    
    // minix3_get_block(minix3_get_superblock().first_data_zone + zone, data);
    minix3_get_block(zone, data);
}
void minix3_put_zone(uint32_t zone, uint8_t *data) {
    SuperBlock sb = minix3_get_superblock();
    if (zone > sb.num_zones + sb.first_data_zone) {
        fatalf("Zone %u (%x) is out of bounds\n", zone, zone);
        return;
    }

    if (zone < sb.first_data_zone) {
        fatalf("Zone %u (%x) is before the first data zone %u (%x)\n", zone, zone, sb.first_data_zone, sb.first_data_zone);
        return;
    }

    minix3_put_block(zone, data);
}

uint64_t minix3_get_file_size(uint32_t inode) {
    Inode inode_data = minix3_get_inode(inode);
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
    Inode inode = minix3_get_inode(i);
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
        debugf("   zones[%d] = %x (*1024 = %x)\n", j, inode.zones[j], inode.zones[j] * 1024);
    }
}


// A struct that keeps some data for this callback
typedef struct CallbackData {
    uint32_t file_count;
    uint32_t dir_count;
} CallbackData;

// A callback function for counting up the files and printing them out
void callback(uint32_t inode, const char *path, char *name, void *data, uint32_t depth) {
    CallbackData *cb_data = (CallbackData *)data;

    for (uint32_t i=0; i<depth; i++) {
        infof("   ");
    }
    infof("%s", name);
    // infof("name: %s", name);
    
    if (minix3_is_dir(inode)) {
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

void minix3_init(void)
{
    // Initialize the filesystem
    minix3_get_superblock();
    debugf("Filesystem block size: %d\n", minix3_get_block_size());
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
        minix3_superblock_init();
    }
    // for (uint16_t i=0; i<sb.imap_blocks; i++) {
    // }
    debugf("Max inode: %d\n", minix3_get_max_inode());
    // This does not work yet, broken

    // Copy the inode bitmap into memory
    inode_bitmap = (uint8_t *)kmalloc(minix3_get_inode_bitmap_size());
    minix3_get_inode_bitmap(inode_bitmap);
    // Copy the zone bitmap into memory
    zone_bitmap = (uint8_t*)kmalloc(minix3_get_zone_bitmap_size());
    minix3_get_zone_bitmap(zone_bitmap);


    /*
    // Filesystem function tests
    for (uint32_t i=minix3_get_min_inode(); i<minix3_get_max_inode(); i++) {
        // debugf("Checking inode %u (%x)...\n", i, i);
        if (!minix3_has_inode(i)) {
            continue;
        }

        if (minix3_is_dir(i)) {
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
            if (j % minix3_get_zone_size() == 0) {
                infof("[ZONE %u]", j / minix3_get_zone_size());
            }
            infof("%c", buf[j]);
        }

        char test[] = "Hello world!";
        // for (uint32_t j=0; j<minix3_get_zone_size() * 3; j+=sizeof(test)) {
        //     minix3_put_data(i, test, j, sizeof(test));
        // }
        // minix3_put_data(i, test, 5, sizeof(test));
        // debugf("Done writing first zone, now second\n");
        minix3_put_data(i, test, minix3_get_zone_size() - 5, sizeof(test));
        // minix3_put_data(i, test, minix3_get_zone_size() * 2 + 5, sizeof(test));
        minix3_read_file(i, buf, size);
        debugf("Read file\n");
        for (uint16_t j=0; j<size; j++) {
            if (j % minix3_get_zone_size() == 0) {
                infof("[ZONE %u]", j / minix3_get_zone_size());
            }
            infof("%c", buf[j]);
        }

        break;
    }
    */

    const char *path = "/home/cosc562";

    CallbackData cb_data2 = {0};
    uint32_t inode = minix3_get_inode_from_path(path);
    infof("%s has inode %u\n", path, inode);
    minix3_traverse(inode, path, &cb_data2, 0, 10, callback);

    infof("Found %u files and %u directories in %s\n", cb_data2.file_count, cb_data2.dir_count, path);

    minix3_map_files();

    // Path of the book
    const char *book_path = "/home/cosc562/subdir1/subdir2/subdir3/subdir4/subdir5/book1.txt";
    // Get the inode of the book
    uint32_t book_inode = minix3_path_to_inode(book_path);
    // Get the size of the book
    uint64_t book_size = minix3_get_file_size(book_inode);
    // Allocate a buffer for the book
    uint8_t *contents = kmalloc(book_size);
    // Read the book into the buffer
    minix3_read_file(book_inode, contents, book_size);
    // Print the book
    for (uint64_t i=0; i<book_size; i++) {
        infof("%c", contents[i]);
    }
    infof("\n");
    // Free the buffer
    kfree(contents);

    
    infof("Files:\n");
    CallbackData cb_data = {0};
    minix3_traverse(1, "/", &cb_data, 0, 10, callback);
    infof("Found %u files and %u directories in /\n", cb_data.file_count, cb_data.dir_count);
}

SuperBlock minix3_get_superblock() {
    // Get the superblock
    // SuperBlock superblock;
    // Superblock begins at bytes 1024
    if (sb.magic != MINIX3_MAGIC) {
        block_device_read_bytes(1024, (uint8_t *)&sb, sizeof(SuperBlock));
    }
    return sb;
}

void minix3_put_superblock(SuperBlock superblock) {
    // Put the superblock
    block_device_write_bytes(1024, (uint8_t *)&superblock, sizeof(SuperBlock));
}


uint16_t minix3_get_block_size(void) {
    SuperBlock superblock = minix3_get_superblock();
    return 1024 << superblock.log_zone_size;
}

uint16_t minix3_get_zone_size(void) {
    return minix3_get_block_size();
}

uint16_t minix3_sectors_per_block(void) {
    return minix3_get_block_size() / block_device_get_sector_size();
}

size_t minix3_get_inode_bitmap_size(void) {
    return minix3_get_superblock().imap_blocks * minix3_get_block_size();
}

size_t minix3_get_zone_bitmap_size(void) {
    return minix3_get_superblock().zmap_blocks * minix3_get_block_size();
}

// Read the inode bitmap into the given buffer
void minix3_get_inode_bitmap(uint8_t *bitmap_buf) {
    debugf("Getting inode bitmap...\n");
    SuperBlock sb = minix3_get_superblock();

    minix3_get_blocks(FS_IMAP_IDX, bitmap_buf, sb.imap_blocks);

    // uint8_t inode_byte;
    // uint64_t byte_offset = FS_IMAP_IDX * minix3_get_block_size() + inode / 8;
    // debugf("About to read inode byte at %u (%x)...\n", byte_offset, byte_offset);
    // block_device_read_bytes(byte_offset, &inode_byte, 1);
    // debugf("Inode byte: %x\n", inode_byte);
    // return inode_byte & (1 << inode % 8);
    // block_device_read_bytes(FS_IMAP_IDX * minix3_get_block_size(), bitmap_buf, minix3_get_block_size() * sb.imap_blocks);
}
// Write the inode bitmap from the given buffer
void minix3_put_inode_bitmap(uint8_t *bitmap_buf) {
    debugf("Setting inode bitmap...\n");
    SuperBlock sb = minix3_get_superblock();
    minix3_put_blocks(FS_IMAP_IDX, bitmap_buf, sb.imap_blocks);
    // block_device_write_bytes(FS_IMAP_IDX * minix3_get_block_size(), bitmap_buf, minix3_get_block_size() * sb.imap_blocks);
}
// Read the zone bitmap into the given buffer
void minix3_get_zone_bitmap(uint8_t *bitmap_buf) {
    debugf("Getting zone bitmap...\n");
    SuperBlock sb = minix3_get_superblock();
    minix3_get_blocks(FS_IMAP_IDX + sb.imap_blocks, bitmap_buf, sb.zmap_blocks);
    // block_device_read_bytes((FS_IMAP_IDX + sb.imap_blocks) * minix3_get_block_size(), bitmap_buf, minix3_get_block_size() * sb.zmap_blocks);
}
// Write the zone bitmap from the given buffer
void minix3_put_zone_bitmap(uint8_t *bitmap_buf) {
    debugf("Setting zone bitmap...\n");
    SuperBlock sb = minix3_get_superblock();
    minix3_put_blocks(FS_IMAP_IDX + sb.imap_blocks, bitmap_buf, sb.zmap_blocks);
}

void minix3_get_blocks(uint32_t start_block, uint8_t *data, uint16_t count) {
    // SuperBlock sb = minix3_get_superblock();
    block_device_read_bytes(start_block * minix3_get_block_size(), data, minix3_get_block_size() * count);
}
void minix3_put_blocks(uint32_t start_block, uint8_t *data, uint16_t count) {
    // SuperBlock sb = minix3_get_superblock();
    block_device_write_bytes(start_block * minix3_get_block_size(), data, minix3_get_block_size() * count);
}

void minix3_get_block(uint32_t block, uint8_t *data) {
    minix3_get_blocks(block, data, 1);
}

void minix3_put_block(uint32_t block, uint8_t *data) {
    minix3_put_blocks(block, data, 1);
}

bool minix3_has_inode(uint32_t inode) {
    if (inode == INVALID_INODE) {
        debugf("minix3_has_inode: Invalid inode %u\n", inode);
        return false;
    }
    return inode_bitmap[inode / 8] & (1 << inode % 8);
}

// Mark the inode taken in the inode map.
bool minix3_take_inode(uint32_t inode) {
    if (inode == INVALID_INODE) {
        debugf("minix3_has_inode: Invalid inode %u\n", inode);
        return false;
    }
    inode_bitmap[inode / 8] |= (1 << inode % 8);
    return true;
}

uint32_t minix3_get_next_free_inode() {
    size_t inode_bitmap_size = minix3_get_inode_bitmap_size();

    for (int i = 0; i < inode_bitmap_size; i++) {
        if (inode_bitmap[i] != 0xFF) {
            for (int j = 0; j < 8; j++) {
                uint32_t inode = 8 * i + j;
                if (!minix3_has_inode(inode)) {
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

Inode minix3_get_inode(uint32_t inode) {
    if (inode == INVALID_INODE) {
        warnf("minix3_get_inode: Invalid inode %u\n", inode);
        return (Inode){0};
    }
    else if (inode == last_inode) {
        return last_inode_data;
    }
    SuperBlock sb = minix3_get_superblock();
    Inode data;
    uint64_t offset = minix3_get_inode_byte_offset(sb, inode);

    // block_device_read_bytes(offset, (uint8_t*)&data, sizeof(Inode));
    block_device_read_bytes(offset, (uint8_t*)&data, sizeof(Inode));
    last_inode_data = data;
    last_inode = inode;
    return data;
}

void minix3_put_inode(uint32_t inode, Inode data) {
    if (inode == INVALID_INODE) {
        warnf("minix3_put_inode: Invalid inode %u\n", inode);
        return;
    }
    SuperBlock sb = minix3_get_superblock();
    uint64_t offset = minix3_get_inode_byte_offset(sb, inode);

    // debugf("Putting inode %u at offset %u (%x)...\n", inode, offset, offset);
    block_device_write_bytes(offset, (uint8_t*)&data, sizeof(Inode));
}

// Allocate a free inode.
// Return the allocated zero'd inode. 
Inode *minix3_alloc_inode() {
    uint32_t free_inode = minix3_get_next_free_inode();
    if (!free_inode) {
        warnf("minix3_alloc_inode: Couldn't find free inode\n");
        return 0;
    }
    minix3_take_inode(free_inode);
    Inode data;
    memset(&data, 0, sizeof(data));
    minix3_put_inode(free_inode, data);
    // infof("minix3_alloc_inode %p\n", minix3_get_inode_byte_offset(sb, free_inode)); // TODO: REMOVE
    return (Inode *)minix3_get_inode_byte_offset(sb, free_inode);
}

bool minix3_is_dir(uint32_t inode) {
    Inode inode_data = minix3_get_inode(inode);
    if (inode_data.num_links == 0) {
        warnf("minix3_is_dir: Inode %u has no links\n", inode);
    }
    return S_ISDIR(inode_data.mode) && inode_data.num_links > 0;
}

bool minix3_is_file(uint32_t inode) {
    Inode inode_data = minix3_get_inode(inode);
    if (inode_data.num_links == 0) {
        warnf("minix3_is_file: Inode %u has no links\n", inode);
    }
    return S_ISREG(inode_data.mode) && inode_data.num_links > 0;
}

void minix3_read_file(uint32_t inode, uint8_t *data, uint32_t count) {
    minix3_get_data(inode, data, 0, count);
}

void minix3_get_data(uint32_t inode, uint8_t *data, uint32_t offset, uint32_t count) {
    // First, get the inode
    Inode inode_data = minix3_get_inode(inode);
    

    uint8_t zone_data[minix3_get_zone_size()];

    // The cursor is the current position in the data buffer
    uint32_t buffer_cursor = 0;
    uint32_t file_cursor = 0;

    // Now, get the data
    // The first 7 zones are direct zones
    for (uint8_t direct_zone=0; direct_zone<7; direct_zone++) {
        uint32_t zone = inode_data.zones[direct_zone];
        if (zone == 0) {
            debugf("No direct zone %d\n", zone);
            continue;
        }
        memset(zone_data, 0, minix3_get_zone_size());

        if (file_cursor + minix3_get_zone_size() < offset) {
            // We're not at the offset yet
            file_cursor += minix3_get_zone_size();
            continue;
        } else if (file_cursor < offset) {
            // We're in the middle of the offset
            // Read the zone into the buffer
            debugf("Reading first direct zone %d\n", zone);
            minix3_get_zone(zone, zone_data);
            // Copy the remaining data into the buffer
            size_t remaining = min(count, minix3_get_zone_size() - (offset - file_cursor));
            memcpy(data, zone_data + offset - file_cursor, remaining);
            buffer_cursor += remaining;
            file_cursor = offset + remaining;
            continue;
        }

        // Read the zone into the buffer
        minix3_get_zone(zone, zone_data);

        // If the cursor is past the amount of data we want, we're done
        if (buffer_cursor + minix3_get_zone_size() > count) {
            debugf("Reading last direct zone %d\n", zone);
            // Copy the remaining data into the buffer
            memcpy(data + buffer_cursor, zone_data, count - buffer_cursor);
            // We're done
            return;
        } else {
            debugf("Reading direct zone %d\n", zone);
            // Copy the entire zone into the buffer
            memcpy(data + buffer_cursor, zone_data, minix3_get_zone_size());
        }
        buffer_cursor += minix3_get_zone_size();
    }

    debugf("Done with direct zones\n");
    // The next zone is an indirect zone
    if (inode_data.zones[7] != 0) {
        debugf("Reading indirect zone %d\n", inode_data.zones[7]);
        uint32_t indirect_zones[minix3_get_zone_size() / sizeof(uint32_t)];
        minix3_get_zone(inode_data.zones[7], (uint8_t*)indirect_zones);
        

        for (uint8_t indirect_zone=0; indirect_zone<minix3_get_zone_size() / sizeof(uint32_t); indirect_zone++) {
            uint32_t zone = indirect_zones[indirect_zone];
            debugf("Reading indirect zone %d\n", zone);
            if (zone == 0) continue;
            

            if (file_cursor + minix3_get_zone_size() < offset) {
                // We're not at the offset yet
                file_cursor += minix3_get_zone_size();
                continue;
            } else if (file_cursor < offset) {
                // We're in the middle of the offset
                // Read the zone into the buffer
                minix3_get_zone(zone, zone_data);
                // Copy the remaining data into the buffer
                size_t remaining = min(count, minix3_get_zone_size() - (offset - file_cursor));
                memcpy(data, zone_data + offset - file_cursor, remaining);
                buffer_cursor += remaining;
                file_cursor = offset + remaining;
                continue;
            }
            
            memset(zone_data, 0, minix3_get_zone_size());

            // Read the zone into the buffer
            minix3_get_zone(zone, zone_data);
            // If the cursor is past the amount of data we want, we're done
            if (buffer_cursor + minix3_get_zone_size() > count) {
                // Copy the remaining data into the buffer
                memcpy(data + buffer_cursor, zone_data, count - buffer_cursor);
                // We're done
                return;
            } else {
                // Copy the entire zone into the buffer
                memcpy(data + buffer_cursor, zone_data, minix3_get_zone_size());
            }
            buffer_cursor += minix3_get_zone_size();
        }
    } else {
        debugf("No indirect zone\n");
    }

    // The next zone is a double indirect zone
    if (inode_data.zones[8] != 0) {
        uint32_t double_indirect_zones[minix3_get_zone_size() / sizeof(uint32_t)];
        // We're done
        minix3_get_zone(inode_data.zones[8], (uint8_t*)double_indirect_zones);

        for (uint8_t double_indirect_zone=0; double_indirect_zone<minix3_get_zone_size() / sizeof(uint32_t); double_indirect_zone++) {
            uint32_t indirect_zone = double_indirect_zones[double_indirect_zone];
            if (indirect_zone == 0) continue;

            uint32_t indirect_zones[minix3_get_zone_size() / sizeof(uint32_t)];
            minix3_get_zone(indirect_zone, (uint8_t*)indirect_zones);

            for (uint8_t indirect_zone=0; indirect_zone<minix3_get_zone_size() / sizeof(uint32_t); indirect_zone++) {
                uint32_t zone = indirect_zones[indirect_zone];
                if (zone == 0) continue;
                debugf("Reading double indirect zone %d\n", zone);

                if (file_cursor + minix3_get_zone_size() < offset) {
                    // We're not at the offset yet
                    file_cursor += minix3_get_zone_size();
                    continue;
                } else if (file_cursor < offset) {
                    // We're in the middle of the offset
                    // Read the zone into the buffer
                    minix3_get_zone(zone, zone_data);
                    // Copy the remaining data into the buffer
                    size_t remaining = min(count, minix3_get_zone_size() - (offset - file_cursor));
                    memcpy(data, zone_data + offset - file_cursor, remaining);
                    buffer_cursor += remaining;
                    file_cursor = offset + remaining;
                    continue;
                }
                
                // Read the zone into the buffer
                memset(zone_data, 0, minix3_get_zone_size());
                minix3_get_zone(zone, zone_data);

                // If the cursor is past the amount of data we want, we're done
                if (buffer_cursor + minix3_get_zone_size() > count) {
                    // Copy the remaining data into the buffer
                    memcpy(data + buffer_cursor, zone_data, count - buffer_cursor);
                    // We're done
                    return;
                } else {
                    // Copy the entire zone into the buffer
                    memcpy(data + buffer_cursor, zone_data, minix3_get_zone_size());
                }
                buffer_cursor += minix3_get_zone_size();
            }
        }
    } else {
        debugf("No double indirect zone\n");
    }

    // The next zone is a triple indirect zone
    if (inode_data.zones[9] != 0) {
        uint32_t triple_indirect_zones[minix3_get_zone_size() / sizeof(uint32_t)];
        minix3_get_zone(inode_data.zones[9], (uint8_t*)triple_indirect_zones);

        for (uint8_t triple_indirect_zone=0; triple_indirect_zone<minix3_get_zone_size() / sizeof(uint32_t); triple_indirect_zone++) {
            uint32_t double_indirect_zone = triple_indirect_zones[triple_indirect_zone];
            if (double_indirect_zone == 0) continue;
            uint32_t double_indirect_zones[minix3_get_zone_size() / sizeof(uint32_t)];
            minix3_get_zone(double_indirect_zone, (uint8_t*)double_indirect_zones);

            for (uint8_t double_indirect_zone=0; double_indirect_zone<minix3_get_zone_size() / sizeof(uint32_t); double_indirect_zone++) {
                uint32_t indirect_zone = double_indirect_zones[double_indirect_zone];
                if (indirect_zone == 0) continue;
                uint32_t indirect_zones[minix3_get_zone_size() / sizeof(uint32_t)];
                minix3_get_zone(indirect_zone, (uint8_t*)indirect_zones);

                for (uint8_t indirect_zone=0; indirect_zone<minix3_get_zone_size() / sizeof(uint32_t); indirect_zone++) {
                    uint32_t zone = indirect_zones[indirect_zone];
                    if (zone == 0) continue;
                    debugf("Reading triple indirect zone %d\n", zone);
                    
                    if (file_cursor + minix3_get_zone_size() < offset) {
                        // We're not at the offset yet
                        file_cursor += minix3_get_zone_size();
                        continue;
                    } else if (file_cursor < offset) {
                        // We're in the middle of the offset
                        // Read the zone into the buffer
                        minix3_get_zone(zone, zone_data);
                        // Copy the remaining data into the buffer
                        size_t remaining = min(count, minix3_get_zone_size() - (offset - file_cursor));
                        memcpy(data, zone_data + offset - file_cursor, remaining);
                        buffer_cursor += remaining;
                        file_cursor = offset + remaining;
                        continue;
                    }
                
                    
                    // Read the zone into the buffer
                    memset(zone_data, 0, minix3_get_zone_size());
                    minix3_get_zone(zone, zone_data);

                    // If the cursor is past the amount of data we want, we're done
                    if (buffer_cursor + minix3_get_zone_size() > count) {
                        // Copy the remaining data into the buffer
                        memcpy(data + buffer_cursor, zone_data, count - buffer_cursor);
                        // We're done
                        return;
                    } else {
                        // Copy the entire zone into the buffer
                        memcpy(data + buffer_cursor, zone_data, minix3_get_zone_size());
                    }
                    buffer_cursor += minix3_get_zone_size();
                }
            }
        }
    } else {
        debugf("No triple indirect zone\n");
    }

    // If we get here, we've read all the data we can
    return;
}
void minix3_put_data(uint32_t inode, uint8_t *data, uint32_t offset, uint32_t count) {
    // First, get the inode
    Inode inode_data = minix3_get_inode(inode);

    uint8_t zone_data[minix3_get_zone_size()];

    // The cursor is the current position in the data buffer
    uint32_t buffer_cursor = 0;
    // The file cursor is the current position in the file
    uint32_t file_cursor = 0;

    // Now, get the data
    // The first 7 zones are direct zones
    for (uint8_t direct_zone=0; direct_zone<7; direct_zone++) {
        uint32_t zone = inode_data.zones[direct_zone];
        if (zone == 0) {
            debugf("No direct zone %d\n", zone);
            continue;
        }
        if (file_cursor + minix3_get_zone_size() < offset) {
            // We're not at the offset yet
            file_cursor += minix3_get_zone_size();
            continue;
        } else if (file_cursor < offset) {
            // We're in the middle of the offset
            // Read the zone into the buffer
            debugf("Writing first direct zone %d\n", zone);
            minix3_get_zone(zone, zone_data);

            // Copy the remaining data into the buffer
            size_t remaining = min(count, minix3_get_zone_size() - (offset - file_cursor));
            memcpy(zone_data + offset - file_cursor, data, remaining);
            minix3_put_zone(zone, zone_data);

            buffer_cursor += remaining;
            file_cursor = offset + remaining;
            if (buffer_cursor >= count) {
                // We're done
                return;
            }

            continue;
        }

        memset(zone_data, 0, minix3_get_zone_size());

        // Write the buffer into the zone
        if (buffer_cursor + minix3_get_zone_size() > count) {
            minix3_get_zone(zone, zone_data);
            debugf("Writing last direct zone %d\n", zone);
            // Copy the remaining data into the buffer
            memcpy(zone_data, data + buffer_cursor, count - buffer_cursor);
            // We're done
            minix3_put_zone(zone, zone_data);
            return;
        } else {
            debugf("Writing direct zone %d\n", zone);
            // Copy the entire zone into the buffer
            memcpy(zone_data, data + buffer_cursor, minix3_get_zone_size());
            minix3_put_zone(zone, zone_data);
        }
        buffer_cursor += minix3_get_zone_size();
    }

    debugf("Done with direct zones\n");
    // The next zone is an indirect zone
    if (inode_data.zones[7] != 0) {
        uint32_t indirect_zones[minix3_get_zone_size() / sizeof(uint32_t)];
        minix3_get_zone(inode_data.zones[7], (uint8_t*)indirect_zones);

        for (uint8_t indirect_zone=0; indirect_zone<minix3_get_zone_size() / sizeof(uint32_t); indirect_zone++) {
            uint32_t zone = indirect_zones[indirect_zone];
            if (zone == 0) continue;
            debugf("Writing indirect zone %d\n", zone);

            if (file_cursor + minix3_get_zone_size() < offset) {
                // We're not at the offset yet
                file_cursor += minix3_get_zone_size();
                continue;
            } else if (file_cursor < offset) {
                // We're in the middle of the offset
                // Read the zone into the buffer
                minix3_get_zone(zone, zone_data);
                // Copy the remaining data into the buffer
                size_t remaining = min(count, minix3_get_zone_size() - (offset - file_cursor));
                memcpy(zone_data + offset - file_cursor, data, remaining);
                minix3_put_zone(zone, zone_data);

                buffer_cursor += remaining;
                file_cursor = offset + remaining;
                if (buffer_cursor >= count) {
                    // We're done
                    return;
                }
                continue;
            }

            memset(zone_data, 0, minix3_get_zone_size());

            if (buffer_cursor + minix3_get_zone_size() > count) {
                minix3_get_zone(zone, zone_data);
                // Copy the remaining data into the buffer
                memcpy(zone_data, data + buffer_cursor, count - buffer_cursor);
                // We're done
                minix3_put_zone(zone, zone_data);
                return;
            } else {
                // Copy the entire zone into the buffer
                memcpy(zone_data, data + buffer_cursor, minix3_get_zone_size());
                minix3_put_zone(zone, zone_data);
            }
            buffer_cursor += minix3_get_zone_size();
        }
    } else {
        debugf("No indirect zone\n");
    }

    // The next zone is a double indirect zone
    // The next zone is a double indirect zone
    if (inode_data.zones[8] != 0) {
        uint32_t double_indirect_zones[minix3_get_zone_size() / sizeof(uint32_t)];
        // We're done
        minix3_get_zone(inode_data.zones[8], (uint8_t*)double_indirect_zones);

        for (uint8_t double_indirect_zone=0; double_indirect_zone<minix3_get_zone_size() / sizeof(uint32_t); double_indirect_zone++) {
            uint32_t indirect_zone = double_indirect_zones[double_indirect_zone];
            if (indirect_zone == 0) continue;

            uint32_t indirect_zones[minix3_get_zone_size() / sizeof(uint32_t)];
            minix3_get_zone(indirect_zone, (uint8_t*)indirect_zones);

            for (uint8_t indirect_zone=0; indirect_zone<minix3_get_zone_size() / sizeof(uint32_t); indirect_zone++) {
                uint32_t zone = indirect_zones[indirect_zone];
                if (zone == 0) continue;
                debugf("Writing double indirect zone %d\n", zone);

                if (file_cursor + minix3_get_zone_size() < offset) {
                    // We're not at the offset yet
                    file_cursor += minix3_get_zone_size();
                    continue;
                } else if (file_cursor < offset) {
                    // We're in the middle of the offset
                    // Read the zone into the buffer
                    minix3_get_zone(zone, zone_data);
                    // Copy the remaining data into the buffer
                    size_t remaining = min(count, minix3_get_zone_size() - (offset - file_cursor));
                    memcpy(zone_data + offset - file_cursor, data, remaining);
                    minix3_put_zone(zone, zone_data);

                    buffer_cursor += remaining;
                    file_cursor = offset + remaining;
                    if (buffer_cursor >= count) {
                        // We're done
                        return;
                    }
                    continue;
                }

                memset(zone_data, 0, minix3_get_zone_size());
                if (buffer_cursor + minix3_get_zone_size() > count) {
                    minix3_get_zone(zone, zone_data);
                    // Copy the remaining data into the buffer
                    memcpy(zone_data, data + buffer_cursor, count - buffer_cursor);
                    // We're done
                    minix3_put_zone(zone, zone_data);
                    return;
                } else {
                    // Copy the entire zone into the buffer
                    memcpy(zone_data, data + buffer_cursor, minix3_get_zone_size());
                    minix3_put_zone(zone, zone_data);
                }

                buffer_cursor += minix3_get_zone_size();
            }
        }
    } else {
        debugf("No double indirect zone\n");
    }

    // The next zone is a triple indirect zone
    if (inode_data.zones[9] != 0) {
        uint32_t triple_indirect_zones[minix3_get_zone_size() / sizeof(uint32_t)];
        minix3_get_zone(inode_data.zones[9], (uint8_t*)triple_indirect_zones);

        for (uint8_t triple_indirect_zone=0; triple_indirect_zone<minix3_get_zone_size() / sizeof(uint32_t); triple_indirect_zone++) {
            uint32_t double_indirect_zone = triple_indirect_zones[triple_indirect_zone];
            if (double_indirect_zone == 0) continue;
            uint32_t double_indirect_zones[minix3_get_zone_size() / sizeof(uint32_t)];
            minix3_get_zone(double_indirect_zone, (uint8_t*)double_indirect_zones);

            for (uint8_t double_indirect_zone=0; double_indirect_zone<minix3_get_zone_size() / sizeof(uint32_t); double_indirect_zone++) {
                uint32_t indirect_zone = double_indirect_zones[double_indirect_zone];
                if (indirect_zone == 0) continue;
                uint32_t indirect_zones[minix3_get_zone_size() / sizeof(uint32_t)];
                minix3_get_zone(indirect_zone, (uint8_t*)indirect_zones);

                for (uint8_t indirect_zone=0; indirect_zone<minix3_get_zone_size() / sizeof(uint32_t); indirect_zone++) {
                    uint32_t zone = indirect_zones[indirect_zone];
                    if (zone == 0) continue;
                    debugf("Writing triple indirect zone %d\n", zone);

                    if (file_cursor + minix3_get_zone_size() < offset) {
                        // We're not at the offset yet
                        file_cursor += minix3_get_zone_size();
                        continue;
                    } else if (file_cursor < offset) {
                        // We're in the middle of the offset
                        // Read the zone into the buffer
                        minix3_get_zone(zone, zone_data);
                        // Copy the remaining data into the buffer
                        size_t remaining = min(count, minix3_get_zone_size() - (offset - file_cursor));
                        memcpy(zone_data + offset - file_cursor, data, remaining);
                        minix3_put_zone(zone, zone_data);

                        buffer_cursor += remaining;
                        file_cursor = offset + remaining;
                        if (buffer_cursor >= count) {
                            // We're done
                            return;
                        }
                        continue;
                    }

                    memset(zone_data, 0, minix3_get_zone_size());

                    if (buffer_cursor + minix3_get_zone_size() > count) {
                        minix3_get_zone(zone, zone_data);
                        // Copy the remaining data into the buffer
                        memcpy(zone_data, data + buffer_cursor, count - buffer_cursor);
                        // We're done
                        minix3_put_zone(zone, zone_data);
                        return;
                    } else {
                        // Copy the entire zone into the buffer
                        memcpy(zone_data, data + buffer_cursor, minix3_get_zone_size());
                        minix3_put_zone(zone, zone_data);
                    }
                    buffer_cursor += minix3_get_zone_size();
                }
            }
        }
    } else {
        debugf("No triple indirect zone\n");
    }


    // If we get here, we've read all the data we can
    return;
}


bool minix3_get_dir_entry(uint32_t inode, uint32_t entry, DirEntry *data) {
    if (!minix3_is_dir(inode)) {
        warnf("Inode %u (%x) is not a directory\n", inode, inode);
        return false;
    }
    Inode inode_data = minix3_get_inode(inode);
    debugf("Getting entry %u from inode %u\n", entry, inode);
    DirEntry tmp;
    minix3_get_data(inode, &tmp, entry * sizeof(DirEntry), sizeof(DirEntry));
    debugf("Got entry %s at inode %u\n", tmp.name, tmp.inode);
    if (tmp.inode == 0) {
        return false;
    }

    memcpy(data, &tmp, sizeof(DirEntry));
    return true;
}

void minix3_put_dir_entry(uint32_t inode, uint32_t entry, DirEntry *data) {
    if (!minix3_is_dir(inode)) {
        warnf("Inode %u (%x) is not a directory\n", inode, inode);
        return;
    }
    Inode inode_data = minix3_get_inode(inode);
    minix3_put_data(inode, data, entry * sizeof(DirEntry), sizeof(DirEntry));
}


// List all of the entries in the given directory to the given buffer.
uint32_t minix3_list_dir(uint32_t inode, DirEntry *entries, uint32_t max_entries) {
    if (!minix3_is_dir(inode)) {
        warnf("Inode %u (%x) is not a directory\n", inode, inode);
        return 0;
    }
    Inode inode_data = minix3_get_inode(inode);
    debugf("Listing directory %u\n", inode);
    uint32_t entry = 0;
    DirEntry tmp;
    while (minix3_get_dir_entry(inode, entry, &tmp)) {
        debugf("Found entry %s at inode %u\n", tmp.name, tmp.inode);
        memcpy(entries + entry, &tmp, sizeof(DirEntry));
        entry++;
        if (entry >= max_entries) {
            warnf("Too many entries in directory %u\n", inode);
            break;
        }
    }
    return entry;
}
// Returns the inode number of the file with the given name in the given directory.
// If the file does not exist, return INVALID_INODE.
uint32_t minix3_find_dir_entry(uint32_t inode, char *name) {
    DirEntry entries[128];
    uint32_t num_entries = minix3_list_dir(inode, entries, 128);

    for (uint32_t i=0; i<num_entries; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            debugf("Found entry %s at inode %u\n", name, entries[i].inode);
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

void minix3_traverse(uint32_t inode, char *root_path, void *data, uint32_t current_depth, uint32_t max_depth, void (*callback)(uint32_t inode, const char *path, char *name, void *data, uint32_t depth)) {
    debugf("Traversing %s: inode %u at depth %d\n", root_path, inode, current_depth);
    if (current_depth > max_depth) {
        return;
    }
    if (!minix3_has_inode(inode)) {
        warnf("Inode %u does not exist\n", inode);
        return;
    }
    debugf("Traversing inode %u at depth %d\n", inode, current_depth);
    char name[128];
    strncpy(name, path_file_name(root_path), sizeof(name));
    if (!minix3_is_dir(inode)) {
        debugf("Not a directory\n");
        callback(inode, root_path, name, data, current_depth);
        return;
    } else {
        debugf("Is a directory\n");
    }

    Inode inode_data = minix3_get_inode(inode);
    debug_inode(inode);
    uint32_t max_entries = minix3_get_zone_size() / sizeof(DirEntry);
    debugf("Max entries: %u\n", max_entries);
    DirEntry entries[max_entries];
    debugf("Allocated entries\n");
    uint32_t num_entries = minix3_list_dir(inode, entries, max_entries);
    debugf("Found %u entries\n", num_entries);

    char *path = kmalloc(1024);
    callback(inode, root_path, name, data, current_depth);
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

        minix3_traverse(entries[i].inode, path, data, current_depth + 1, max_depth, callback);
    }

    kfree(path);
    return;
}
