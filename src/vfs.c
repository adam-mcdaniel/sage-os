#include <stdint.h>
#include <string.h>
#include <vfs.h>
#include <debug.h>
#include <path.h>
#include <map.h>

#define VFS_DEBUG
#ifdef VFS_DEBUG
#define debugf(...) debugf(__VA_ARGS__)
#else
#define debugf(...)
#endif

int vfs_read(File *file, void *buf, int count) {
    if (file->offset + count > file->size) {
        count = file->size - file->offset;
    }

    switch (file->type) {
    case VFS_TYPE_FILE:
        minix3_get_data(file->dev, file->inode, buf, file->offset, count);
        file->offset += count;
        return count;
    case VFS_TYPE_BLOCK:
        block_device_read_bytes(file->dev, file->offset, buf, (uint64_t)count);
        file->offset += count;
        return count;
    case VFS_TYPE_DIR:
        debugf("vfs_read: reading from directory not supported\n");
        return -1;
    default:
        debugf("vfs_read: unsupported file type %d\n", file->type);
        return -1;
    }
}
int vfs_write(File *file, const char *buf, int count);


// Populate stat from a path.
// Used by stat(2).
// int vfs_stat_path(const char *path, Stat *stat) {
//     uint32_t inode = minix3_path_to_inode(path);
//     Inode data = minix3_get_inode(block_device, inode);
//     stat->inode = inode;
//     stat->mode = data.mode;
//     stat->num_link = data.num_links;
//     stat->uid = data.uid;
//     stat->gid = data.gid;
//     stat->size = data.size;
//     stat->atime = data.atime;
//     stat->mtime = data.mtime;
//     stat->ctime = data.ctime;
//     return 0;
// }

static Map *mounted_devices;

// // A map of file paths (absolute path strings) to inodes
// static Map *mapped_paths;

// // A map of inodes to file paths (absolute path strings)
// static Map *mapped_inodes;

void vfs_mount(VirtioDevice *block_device, const char *path) {
    if (mounted_devices == NULL) {
        mounted_devices = map_new();
    }

    map_set(mounted_devices, path, block_device);
}

VirtioDevice *vfs_get_mounted_device(const char *path) {
    if (mounted_devices == NULL) {
        mounted_devices = map_new();
    }

    VirtioDevice *block_device;
    map_get(mounted_devices, path, &block_device);
    if (block_device == NULL) {
        // Get the parent path
        char *parent_path = kmalloc(strlen(path) + 1);
        strcpy(parent_path, path);
        char *filename = path_file_name(parent_path);
        filename[-1] = '\0';

        // Get the parent device
        debugf("vfs_get_mounted_device: getting parent device for %s\n", parent_path);
        block_device = vfs_get_mounted_device(parent_path);
    } else {
        debugf("vfs_get_mounted_device: found device for %s\n", path);
    }

    return block_device;
}


File *vfs_open(const char *path, flags_t flags, mode_t mode) {
    File *file = kmalloc(sizeof(File));
    memset(file, 0, sizeof(File));
    file->path = kmalloc(strlen(path) + 1);
    strcpy(file->path, path);
    file->flags = flags;
    file->mode = mode;


}

// This creates the `mapped_paths` and `mapped_inodes` maps
// for caching the paths of files and their inodes
// void minix3_map_files(VirtioDevice *block_device, const char *mounted_path) {
//     mapped_block_device = block_device;

//     infof("Mapping files...\n");
//     mapped_paths = map_new();
//     mapped_inodes = map_new();

//     minix3_traverse(block_device, 1, "/", NULL, 0, 10, map_callback);
// }

// const char *minix3_inode_to_path(VirtioDevice *block_device, uint32_t inode) {
//     if (mapped_block_device != block_device) {
//         minix3_map_files(block_device);
//     }

//     uint64_t path;
//     map_get_int(mapped_inodes, inode, &path);
//     return (const char *)path;
// }

// uint32_t minix3_path_to_inode(VirtioDevice *block_device, const char *path) {
//     if (mapped_block_device != block_device) {
//         minix3_map_files(block_device);
//     }

//     uint64_t inode;
//     map_get(mapped_paths, path, &inode);
//     return inode;
// }

// void map_callback(VirtioDevice *block_device, uint32_t inode, const char *path, char *name, void *data, uint32_t depth) {
//     infof("Mapping %s to %u\n", path, inode);
//     map_set(mapped_paths, path, inode);
//     map_set_int(mapped_inodes, inode, (uintptr_t)path);
// }

int vfs_stat(File *file, Stat *stat) {
    VirtioDevice *block_device = file->dev;
    Inode data = minix3_get_inode(block_device, file->inode);
    stat->inode = file->inode;
    stat->mode = data.mode;
    stat->num_link = data.num_links;
    stat->uid = data.uid;
    stat->gid = data.gid;
    stat->size = data.size;
    stat->atime = data.atime;
    stat->mtime = data.mtime;
    stat->ctime = data.ctime;
    return 0;
}

bool vfs_link(File *dir, File *file) {
    // uint32_t inode1 = minix3_get_inode_from_path(file1->path1, 0);
    // uint32_t inode2 = minix3_get_inode_from_path(path2, 1);

    VirtioDevice *block_device = dir->dev;

    // path1 must exist
    if (file->inode == INVALID_INODE) {
        debugf("vfs_link: file doesn't exist\n");
        return false;
    }

    // path1 must not be a directory
    if (minix3_is_dir(block_device, file->inode)) {
        debugf("vfs_link: file is a directory\n");
        return false;
    }

    // Parent of path2 must exist
    if (dir->inode == INVALID_INODE) {
        debugf("vfs_link: parent of dir doesn't exist\n");
        return false;
    }

    if (!minix3_is_dir(block_device, dir->inode)) {
        debugf("vfs_link: parent of dir is not a directory\n");
        return false;
    }

    DirEntry data;
    memset(&data, 0, sizeof(data));
    data.inode = file->inode;
    strcpy(data.name, path_file_name(dir->path));
    uint32_t free_dir_entry = minix3_find_next_free_dir_entry(block_device, dir->inode);
    minix3_put_dir_entry(block_device, dir->inode, free_dir_entry, &data);

    Inode inode1_data = minix3_get_inode(block_device, file->inode);
    inode1_data.num_links += 1;
    minix3_put_inode(block_device, file->inode, inode1_data);
    return true;
}

// bool vfs_link(File *file1, File *file2) {
//     return vfs_link_paths(file1->path, file2->path);
// }

