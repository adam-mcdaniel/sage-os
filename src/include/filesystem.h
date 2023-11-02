#include <block.h>
#include <stdint.h>

#define MINIX3_MAGIC 0x4d5a

#define INVALID_INODE 0

#define FS_BOOT_BLOCK_IDX 0
#define FS_SUPER_BLOCK_IDX 1
#define FS_IMAP_IDX 2

#define S_IFMT  00170000
#define S_IFSOCK 0140000
#define S_IFLNK  0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000
#define S_ISLNK(m)    (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)    (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)    (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)    (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)    (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)    (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)    (((m) & S_IFMT) == S_IFSOCK)
#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100
#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010
#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

typedef struct SuperBlock {
    // The number of inodes in the filesystem.
    // This is both allocated and unallocated.
    // Used to bounds check the inode array and inode bitmap.
    uint32_t num_inodes;
    uint16_t pad0;
    // The number of blocksize-byte blocks that the inode map takes.
    uint16_t imap_blocks;
    // The number of blocksize-byte blocks that the zone map takes.
    uint16_t zmap_blocks;
    // The zone number of the first data zone. This is generally not used.
    uint16_t first_data_zone;
    // 1024 << log_zone_size (left shift) tells us the block size. This is usually
    // the same as the block size field. For 1024-byte blocks, this will be 0. For 2048 blocks,
    // this will be 1, and so fourth.
    uint16_t log_zone_size;
    uint16_t pad1;
    // The maximum file size. This is generally not used because we can calculate the maximum file size based on the zone pointers.
    uint32_t max_size;
    // The number of blocksize-byte zones. This is the number of blocks after the boot block, super block, bitmaps, and inode table.
    uint32_t num_zones;
    // This identifies the file system. ***This must be the value 0x4d5a (characters ZM)***. If it is not,
    // we cannot guarantee that this is a minix 3 filesystem, and an error should occur.
    uint16_t magic;
    uint16_t pad2;
    // **This field is invalid for the minix 3 file systems. Do not read from it.**
    uint16_t block_size;
    // Unused but usually 0
    uint8_t disk_version;
} SuperBlock;


typedef struct Inode {
    uint16_t mode;
    uint16_t num_links;
    uint16_t uid;
    uint16_t gid;
    uint32_t size;
    uint32_t atime;
    uint32_t mtime;
    uint32_t ctime;
    uint32_t zones[10];
} Inode;


typedef struct DirEntry {
   uint32_t inode;
   char name[60];
} DirEntry;

void filesystem_init(void);

SuperBlock filesystem_get_superblock(void);
void filesystem_put_superblock(SuperBlock superblock);

// Get the block size for the file system.
uint16_t filesystem_get_block_size(void);
// Get the zone size for the file system.
uint16_t filesystem_get_zone_size(void);

size_t filesystem_get_inode_bitmap_size(void);
size_t filesystem_get_zone_bitmap_size(void);

// Read the inode bitmap into the given buffer
void filesystem_get_inode_bitmap(uint8_t *bitmap_buf);
// Write the inode bitmap from the given buffer
void filesystem_put_inode_bitmap(uint8_t *bitmap_buf);
// Read the zone bitmap into the given buffer
void filesystem_get_zone_bitmap(uint8_t *bitmap_buf);
// Write the zone bitmap from the given buffer
void filesystem_put_zone_bitmap(uint8_t *bitmap_buf);

void filesystem_get_blocks(uint32_t block, uint8_t *data, uint16_t count);
void filesystem_put_blocks(uint32_t block, uint8_t *data, uint16_t count);

void filesystem_get_block(uint32_t block, uint8_t *data);
void filesystem_put_block(uint32_t block, uint8_t *data);

uint32_t filesystem_alloc_zone();
void filesystem_free_zone(uint32_t zone);

bool filesystem_has_inode(uint32_t inode);
Inode filesystem_get_inode(uint32_t inode);
void filesystem_put_inode(uint32_t inode, Inode data);

void filesystem_get_zone(uint32_t zone, uint8_t *data);
void filesystem_put_zone(uint32_t zone, uint8_t *data);

void filesystem_get_data(uint32_t inode, uint8_t *data, uint32_t offset, uint32_t count);
void filesystem_put_data(uint32_t inode, uint8_t *data, uint32_t offset, uint32_t count);
uint64_t filesystem_get_file_size(uint32_t inode);
void filesystem_read_file(uint32_t inode, uint8_t *data, uint32_t count);

bool filesystem_is_file(uint32_t inode);
bool filesystem_is_dir(uint32_t inode);
bool filesystem_has_dir_entry(uint32_t inode, uint32_t entry);
void filesystem_get_dir_entry(uint32_t inode, uint32_t entry, DirEntry *data);
void filesystem_put_dir_entry(uint32_t inode, uint32_t entry, DirEntry *data);
