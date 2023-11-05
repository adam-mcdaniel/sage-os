#include <minix3.h>
#include <stat.h>
#include <stdint.h>
#include <virtio.h>

typedef struct File {
    VirtioDevice *dev;

    uint32_t inode;
    uint32_t size;
    const char *path;
    bool is_dir;
    bool is_file;
    bool is_symlink;
    bool is_hardlink;
    bool is_block_device;
    bool is_char_device;
    uint16_t major;
    uint16_t minor;
} File;

void vfs_stat(const char *path, Stat *stat);
bool vfs_link(const char *path1, const char *path2);
int vfs_create(const char *path, uint16_t type);
// unlink()
// open()
// close()
// read
// write
// seek
