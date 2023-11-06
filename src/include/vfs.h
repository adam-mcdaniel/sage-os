#pragma once

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
#define O_TRUNC  16

#define VFS_TYPE_INFER 0
#define VFS_TYPE_FILE 1
#define VFS_TYPE_DIR  2
#define VFS_TYPE_LINK 4
#define VFS_TYPE_CHAR 8
#define VFS_TYPE_BLOCK 16
#define VFS_TYPE_PIPE 32
#define VFS_TYPE_SOCKET 64

typedef uint16_t type_t;

// typedef unsigned long  size_t;
// typedef signed long    ssize_t;
typedef signed long    off_t;
typedef unsigned short mode_t;
typedef unsigned long flags_t;

bool vfs_should_create_if_doesnt_exist(flags_t flags, mode_t mode, type_t type);
bool vfs_should_truncate(flags_t flags, mode_t mode, type_t type);

typedef struct File {
    VirtioDevice *dev;

    type_t type;

    Inode inode_data;
    uint32_t inode;
    off_t offset;
    mode_t mode;
    size_t size;
    flags_t flags;

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

void vfs_init(void);
void vfs_print_mounted_devices(void);
void vfs_print_open_files(void);
void vfs_mount(VirtioDevice *block_device, const char *path);
File *vfs_open(const char *path, flags_t flags, mode_t mode, type_t type);
void vfs_close(File *file);

int vfs_read(File *file, void *buf, int count);
int vfs_write(File *file, const char *buf, int count);

int vfs_seek(File *file, int offset, int whence);
int vfs_tell(File *file);


int vfs_create(const char *path, type_t type);
int vfs_stat_path(const char *path, Stat *stat);
int vfs_stat(File *file, Stat *stat);
bool vfs_link(File *file1, File *file2);
bool vfs_link_paths(const char *path1, const char *path2);

// unlink()
// open()
// close()
// read
// write
// seek
