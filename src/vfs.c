#include <stdint.h>
#include <string.h>
#include <vfs.h>
#include <debug.h>
#include <path.h>
#include <map.h>
#include <list.h>

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

static size_t mounted_device_count = 0;
static Map *mounted_devices;

// // A map of file paths (absolute path strings) to inodes
// static Map *mapped_paths;

// // A map of inodes to file paths (absolute path strings)
// static Map *mapped_inodes;

void vfs_print_mounted_devices() {
    if (mounted_devices == NULL) {
        mounted_devices = map_new();
    }

    if (map_size(mounted_devices) == 0) {
        debugf("vfs_print_mounted_devices: no mounted devices\n");
        return;
    }

    List *keys = map_get_keys(mounted_devices);
    ListElem *key;
    list_for_each(keys, key) {
        VirtioDevice *block_device;
        map_get(mounted_devices, key, &block_device);
        debugf("vfs_print_mounted_devices: %s -> %p\n", list_elem_value(key), block_device);
    }

    map_free_get_keys(keys);
}

void vfs_init(void) {
    mounted_devices = map_new();
    VirtioDevice *block_device = virtio_get_block_device(0);
    vfs_print_mounted_devices();
    vfs_mount(block_device, "/");
    vfs_print_mounted_devices();

    vfs_open("/dev/sda", O_CREAT, 0, VFS_TYPE_BLOCK);

    infof("vfs_init: mounted %u devices\n", mounted_device_count);
}

void vfs_mount(VirtioDevice *block_device, const char *path) {
    if (mounted_devices == NULL) {
        mounted_devices = map_new();
    }

    char *block_device_path = kmalloc(strlen(path) + 1);
    strcpy(block_device_path, path);
    
    map_set(mounted_devices, block_device_path, block_device);
    mounted_device_count += 1;
}

VirtioDevice *vfs_get_mounted_device(const char *path) {
    if (path == NULL) {
        debugf("vfs_get_mounted_device: path is NULL\n");
        return NULL;
    }

    if (strlen(path) == 0 || strcmp(path, "") == 0) {
        debugf("vfs_get_mounted_device: path is empty\n");
        return NULL;
    }

    if (mounted_devices == NULL) {
        mounted_devices = map_new();
    }

    VirtioDevice *block_device = NULL;
    map_get(mounted_devices, path, &block_device);
    // map_get_int(mounted_devices, 0, &block_device);

    if (block_device == NULL) {
        debugf("vfs_get_mounted_device: no device found for %s\n", path);
        if (strcmp(path, "/") == 0) {
            debugf("vfs_get_mounted_device: no device found for /\n");
            return NULL;
        }

        // Get the parent path
        char *parent_path = kmalloc(strlen(path) + 1);
        strcpy(parent_path, path);
        char *filename = path_file_name(parent_path);
        if (&filename[-1] == parent_path) {
            // We are at root
            debugf("vfs_get_mounted_device: we are at root\n");
            return vfs_get_mounted_device("/");
        }

        filename[-1] = '\0';

        // Get the parent device
        debugf("vfs_get_mounted_device: getting parent device for %s\n", parent_path);
        block_device = vfs_get_mounted_device(parent_path);
    } else {
        debugf("vfs_get_mounted_device: found device for %s\n", path);
    }

    return block_device;
}

bool vfs_should_create_if_doesnt_exist(flags_t flags, mode_t mode, type_t type) {
    return flags & O_CREAT;
}

bool vfs_should_truncate(flags_t flags, mode_t mode, type_t type) {
    return flags & O_TRUNC;
}

File *vfs_open(const char *path, flags_t flags, mode_t mode, type_t type) {
    File *file = kmalloc(sizeof(File));
    memset(file, 0, sizeof(File));
    file->path = kmalloc(strlen(path) + 1);
    strcpy(file->path, path);
    file->flags = flags;
    file->mode = mode;
    file->type = type;

    VirtioDevice *parent_device = vfs_get_mounted_device(path);
    debugf("vfs_open: parent device is %p\n", parent_device);
    debugf("vfs_open: parent path is %s\n", path);
    char *block_device_name = path_file_name(path);
    uint32_t block_device_num = mounted_device_count;
            
    DirEntry dir_entry;
    size_t i = 0;
    uint32_t free_dir_entry = 0;
    uint32_t parent_inode = 0;
    switch (type) {
    case VFS_TYPE_FILE:
        file->dev = vfs_get_mounted_device(path);
        file->inode = minix3_get_inode_from_path(file->dev, file->path, false);
        file->inode_data = minix3_get_inode(file->dev, file->inode);
        file->size = file->inode_data.size;
        file->is_file = true;
        break;

    case VFS_TYPE_DIR:
        file->dev = vfs_get_mounted_device(path);
        file->inode = minix3_get_inode_from_path(file->dev, file->path, false);
        file->inode_data = minix3_get_inode(file->dev, file->inode);
        file->size = file->inode_data.size;
        file->is_dir = true;
        break;
    
    case VFS_TYPE_BLOCK:
        // Read the filename
        debugf("vfs_open: block device name is %s\n", block_device_name);

        // Get the number of the block device
        debugf("vfs_open: block device number is %u\n", block_device_num);


        file->inode = minix3_get_inode_from_path(parent_device, file->path, false);
        debugf("vfs_open: inode is %u\n", file->inode);

        if (file->inode == 0 && vfs_should_create_if_doesnt_exist(flags, mode, type)) {
            debugf("vfs_open: creating block device\n");
            debugf("vfs_open: parent device is %p\n", parent_device);
            // Create the block device
            file->inode = minix3_alloc_inode(parent_device);
            debugf("vfs_open: created block device with inode %u\n", file->inode);
            file->inode_data = minix3_get_inode(parent_device, file->inode);
            file->inode_data.mode = S_IFBLK;
            file->inode_data.num_links = 1;
            file->inode_data.uid = 0;
            file->inode_data.gid = 0;
            file->inode_data.size = 0;
            file->inode_data.atime = 0;
            file->inode_data.mtime = 0;
            file->inode_data.ctime = 0;
            minix3_put_inode(parent_device, file->inode, file->inode_data);
            debugf("vfs_open: put inode\n");

            parent_inode = minix3_get_inode_from_path(parent_device, file->path, true);
            debugf("vfs_open: parent inode is %u\n", parent_inode);

            // Create the directory entry
            DirEntry dir_entry;
            memset(&dir_entry, 0, sizeof(dir_entry));
            dir_entry.inode = file->inode;
            strcpy(dir_entry.name, block_device_name);
            debugf("vfs_open: created dir entry\n");

            // Find the next free directory entry
            free_dir_entry = minix3_find_next_free_dir_entry(parent_device, parent_inode);
            debugf("vfs_open: found free dir entry %u\n", free_dir_entry);

            // Put the directory entry
            minix3_put_dir_entry(parent_device, parent_inode, free_dir_entry, &dir_entry);
            debugf("vfs_open: put dir entry\n");

            // Create the inode
            Inode inode_data;
            memset(&inode_data, 0, sizeof(inode_data));
            inode_data.mode = mode;
            inode_data.num_links = 1;
            inode_data.uid = 0;
            inode_data.gid = 0;
            inode_data.size = 0;
            inode_data.atime = 0;
            inode_data.mtime = 0;
            inode_data.ctime = 0;
            minix3_put_inode(parent_device, file->inode, inode_data);
            debugf("vfs_open: put inode\n");
        } else {
            debugf("vfs_open: not creating block device\n");
        }

        // Get the block device
        file->dev = virtio_get_block_device(block_device_num);
        if (file->dev == NULL) {
            debugf("vfs_open: could not find block device\n");
            return NULL;
        } else {
            debugf("vfs_open: found block device\n");
        }
        debugf("vfs_open: block device is %p\n", file->dev);
        minix3_init(file->dev, path);
        // if (vfs_should_create_if_doesnt_exist(flags, mode, type)) {
        //     debugf("vfs_open: creating block device\n");
        // } else {
        //     debugf("vfs_open: not creating block device\n");
        // }

        


        // if (file->inode_data.mode)
        // if (minix3_is_block_device(file->dev, file->inode)) {
        //     debugf("vfs_open: is a block device\n");
        //     file->is_block_device = true;
        // } else {
        //     debugf("vfs_open: not a block device\n");
        //     debugf("vfs_open: mode is %x\n", file->inode_data.mode);
        //     return NULL;
        // }

        file->size = block_device_get_bytes(file->dev);
        file->is_block_device = true;

        vfs_mount(file->dev, path);
        break;
    }
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

