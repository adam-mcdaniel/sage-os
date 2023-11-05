#include <minix3.h>
#include <stat.h>
#include <stdint.h>
#include <virtio.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  8

// typedef unsigned long  size_t;
// typedef signed long    ssize_t;
typedef signed long    off_t;
typedef unsigned short mode_t;
typedef unsigned long  dev_t;

typedef struct File {
    VirtioDevice *dev;

    Inode inode_data;
    uint32_t inode;
    off_t offset;
    mode_t mode;
    size_t size;

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

File *vfs_open(const char *path, int flags, mode_t mode);
void vfs_close(File *file);

int vfs_read(File *file, void *buf, int count);
int vfs_write(File *file, const char *buf, int count);

int vfs_seek(File *file, int offset, int whence);
int vfs_tell(File *file);

int vfs_stat_path(const char *path, Stat *stat);
int vfs_stat(File *file, Stat *stat);
bool vfs_link(File *file1, File *file2);
bool vfs_link_paths(const char *path1, const char *path2);
int vfs_create(const char *path, uint16_t type);

// unlink()
// open()
// close()
// read
// write
// seek
