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

void debug_file(File *file) {
    debugf("File {\n");
    debugf("    path: %p\n", file->path);
    debugf("    dev: %p\n", file->dev);
    debugf("    inode: %u\n", file->inode);
    debugf("    inode_data: {\n");
    debugf("        mode: %x\n", file->inode_data.mode);
    debugf("        num_links: %u\n", file->inode_data.num_links);
    debugf("        uid: %u\n", file->inode_data.uid);
    debugf("        gid: %u\n", file->inode_data.gid);
    debugf("        size: %u\n", file->inode_data.size);
    debugf("        atime: %u\n", file->inode_data.atime);
    debugf("        mtime: %u\n", file->inode_data.mtime);
    debugf("        ctime: %u\n", file->inode_data.ctime);
    debugf("    }\n");
    debugf("    size: %u\n", file->size);
    debugf("    offset: %u\n", file->offset);
    debugf("    flags: %x\n", file->flags);
    debugf("    mode: %x\n", file->mode);
    debugf("    type: %x\n", file->type);
    debugf("    is_file: %u\n", file->is_file);
    debugf("    is_dir: %u\n", file->is_dir);
    debugf("    is_block_device: %u\n", file->is_block_device);
    debugf("}\n");
}

int vfs_read(File *file, void *buf, int count) {
    if (file->offset + count > file->size) {
        count = file->size - file->offset;
    }

    switch (file->type) {
    case VFS_TYPE_FILE:
        debugf("vfs_read: reading from file\n");
        minix3_get_data(file->dev, file->inode, buf, file->offset, count);
        file->offset += count;
        return count;
    case VFS_TYPE_BLOCK:
        debugf("vfs_read: reading from block device\n");
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
int vfs_write(File *file, const char *buf, int count) {
    if (file->offset + count > file->size) {
        count = file->size - file->offset;
    }

    switch (file->type) {
    case VFS_TYPE_FILE:
        minix3_put_data(file->dev, file->inode, buf, file->offset, count);
        file->offset += count;
        return count;
    case VFS_TYPE_BLOCK:
        block_device_write_bytes(file->dev, file->offset, buf, (uint64_t)count);
        file->offset += count;
        return count;
    case VFS_TYPE_DIR:
        debugf("vfs_write: writing to directory not supported\n");
        return -1;
    default:
        debugf("vfs_write: unsupported file type %d\n", file->type);
        return -1;
    }
}


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

static Map *open_files;
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

    List *keys = map_get_keys(mounted_devices);
    list_sort(keys, list_sort_string_comparator_ascending);
    ListElem *key = NULL;
    size_t count = 0;
    infof("Printing mounted drives:\n");
    list_for_each(keys, key) {
        VirtioDevice *block_device;
        map_get(mounted_devices, list_elem_value(key), &block_device);
        infof("    %s at disk #%u\n", list_elem_value(key), count);
        count++;
    }
    map_free_get_keys(keys);

    if (count == 0) {
        warnf("There are no mounted devices\n");
    } else {
        infof("There are %u mounted drives\n", count);
    }
}

const char *vfs_path_from_mounted_device(VirtioDevice *mounted_device) {
    if (mounted_devices == NULL) {
        mounted_devices = map_new();
    }

    List *keys = map_get_keys(mounted_devices);
    list_sort(keys, list_sort_string_comparator_ascending);
    ListElem *key = NULL;
    size_t count = 1;

    VirtioDevice *result;
    list_for_each(keys, key) {
        VirtioDevice *block_device;
        map_get(mounted_devices, list_elem_value(key), &block_device);
        if (block_device == mounted_device) {
            result = list_elem_value(key);
        }
        count++;
    }
    map_free_get_keys(keys);

    return result;
}

void vfs_print_open_files() {
    if (open_files == NULL) {
        open_files = map_new();
    }

    infof("Printing open files:\n");
    List *keys = map_get_keys(open_files);
    ListElem *key = NULL;
    size_t count = 0;
    list_for_each(keys, key) {
        File *file;
        map_get(open_files, list_elem_value(key), &file);
        const char *device_name = vfs_path_from_mounted_device(file->dev);
        infof("   %s on device %s\n", list_elem_value(key), device_name);
        count++;
    }
    map_free_get_keys(keys);

    if (count == 0) {
        infof("There are no open files\n");
    } else {
        infof("There are %u open files\n", count);
    }
}

void vfs_mount_callback(VirtioDevice *block_device, uint32_t inode, const char *path, char *name, void *data, uint32_t depth) {
    if (strcmp(path, "/") == 0) {
        debugf("vfs_mount_callback: skipping /\n");
        return;
    }

    // Check if this is a block device that needs to be mounted
    Inode inode_data = minix3_get_inode(block_device, inode);
    if (S_ISBLK(inode_data.mode)) {
        debugf("vfs_mount_callback: found block device %s\n", path);
        // Mount the block device
        block_device = virtio_get_block_device(mounted_device_count);
        if (block_device == NULL) {
            debugf("vfs_mount_callback: could not find block device %u\n", mounted_device_count);
            return;
        }
        infof("Mounting block device at %s\n", path);
        vfs_mount(block_device, path);
    }
}

void vfs_init(void) {
    mounted_devices = map_new();
    mounted_device_count = 0;
    VirtioDevice *block_device = virtio_get_block_device(0);
    vfs_print_mounted_devices();
    vfs_mount(block_device, "/");
    
    minix3_traverse(block_device, 1, "/", NULL, 0, 10, vfs_mount_callback);
    vfs_print_mounted_devices();
    vfs_print_open_files();
    infof("vfs_init: mounted %u devices\n", mounted_device_count);
}

char *get_parent_path(const char *path) {
    char *parent_path = kmalloc(strlen(path) + 1);
    strcpy(parent_path, path);
    char *filename = path_file_name(parent_path);
    if (&filename[-1] == parent_path) {
        // We are at root
        filename[0] = '\0';
        return parent_path;
    }

    filename[-1] = '\0';
    return parent_path;
}

void vfs_mount(VirtioDevice *block_device, const char *path) {
    minix3_init(block_device, path);

    if (mounted_devices == NULL) {
        mounted_devices = map_new();
    }

    char *block_device_path = kmalloc(strlen(path) + 1);
    strcpy(block_device_path, path);
    
    map_set(mounted_devices, block_device_path, (uint64_t)block_device);
    debugf("vfs_mount: mounted (%p) %s at %s\n", block_device, block_device_path, path);
    mounted_device_count += 1;

}

VirtioDevice *vfs_get_mounted_device(const char *path) {
    static int i = 0;
    if (path == NULL) {
        debugf("vfs_get_mounted_device: path is NULL\n");
        return NULL;
    }

    if (strlen(path) == 0 || strcmp(path, "") == 0) {
        debugf("vfs_get_mounted_device: path is empty\n");
        return NULL;
    }

    if (strcmp(path, ".") == 0 || strcmp(path, "..") == 0) {
        debugf("vfs_get_mounted_device: path is . or ..\n");
        return NULL;
    }

    if (mounted_devices == NULL) {
        mounted_devices = map_new();
    }

    VirtioDevice *block_device = NULL;
    map_get(mounted_devices, path, &block_device);
    // map_get_int(mounted_devices, 0, &block_device);

    if (block_device == NULL) {
        if (i++ > 20) {
            i = 0;
            debugf("vfs_get_mounted_device: too many iterations\n");
            return NULL;
        }
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
        debugf("vfs_get_mounted_device: getting parent device for %s\n", path);
        block_device = vfs_get_mounted_device(parent_path);
        kfree(parent_path);
    } else {
        debugf("vfs_get_mounted_device: found device for %s\n", path);
    }

    debugf("vfs_get_mounted_device: returning %p\n", block_device);
    i = 0;
    return block_device;
}

bool is_mounted_device(const char *path) {
    return map_contains(mounted_devices, path);
}

char *get_path_relative_to_mount_point(const char *path) {
    // vfs_get_mounted_device(path);
    if (path == NULL) {
        debugf("get_path_relative_to_mount_point: path is NULL\n");
        return NULL;
    }

    if (strlen(path) == 0 || strcmp(path, "") == 0) {
        debugf("get_path_relative_to_mount_point: path is empty\n");
        return NULL;
    }

    char *result = kmalloc(strlen(path) + 1);
    if (strcmp(path, "/") == 0) {
        debugf("get_path_relative_to_mount_point: path is /\n");
        strcpy(result, "/");
        return result;
    }


    if (is_mounted_device(path)) {
        debugf("get_path_relative_to_mount_point: path is a mounted device\n");
        strcpy(result, "/");
        return result;
    } else {
        debugf("get_path_relative_to_mount_point: path %s is not a mounted device\n", path);
        char *parent_path = get_parent_path(path);
        int i=0;
        while (!is_mounted_device(parent_path)) {
            if (i++ > 100) {
                debugf("get_path_relative_to_mount_point: too many iterations\n");
                return NULL;
            }

            debugf("get_path_relative_to_mount_point: %s is not a mounted device\n", parent_path);
            char *parent_parent_path = get_parent_path(parent_path);
            kfree(parent_path);
            parent_path = parent_parent_path;

            if (strcmp(parent_path, "/") == 0 || strcmp(parent_path, "") == 0) {
                debugf("get_path_relative_to_mount_point: parent path is /\n");
            }
        }

        debugf("get_path_relative_to_mount_point: %s is a mounted device\n", parent_path);
        if (strcmp(parent_path, "/") == 0 || strcmp(parent_path, "") == 0) {
            debugf("get_path_relative_to_mount_point: parent path is /\n");
            strcpy(result, path);
        } else {
            strcpy(result, path + strlen(parent_path));
        }
        kfree(parent_path);
        return result;
    }
}

bool vfs_should_create_if_doesnt_exist(flags_t flags, mode_t mode, type_t type) {
    return flags & O_CREAT;
}

bool vfs_should_truncate(flags_t flags, mode_t mode, type_t type) {
    return flags & O_TRUNC;
}

File *vfs_get_open_file(const char *path) {
    if (open_files == NULL) {
        open_files = map_new();
    }
    if (!map_contains(open_files, path)) {
        debugf("vfs_get_open_file: file is not open\n");
        return NULL;
    }

    File *file = NULL;
    map_get(open_files, path, &file);
    debugf("vfs_get_open_file: %s is %p\n", path, file);
    return file;
}

bool vfs_is_open(const char *path) {
    if (open_files == NULL) {
        open_files = map_new();
    }
    debugf("vfs_is_open: %s\n", path);

    return map_contains(open_files, path);
}

File *vfs_open(const char *path, flags_t flags, mode_t mode, type_t type) {
    debugf("vfs_open: opening %s\n", path);

    if (is_mounted_device(path)) {
        debugf("vfs_open: path is a mounted device\n");
        if (!vfs_is_open(path)) {
            debugf("vfs_open: mounting device\n");
            VirtioDevice *block_device = vfs_get_mounted_device(path);
            File *file = kmalloc(sizeof(File));
            memset(file, 0, sizeof(File));
            file->path = kmalloc(strlen(path) + 1);
            strcpy(file->path, path);
            file->flags = flags;
            file->mode = mode;
            file->type = type;
            file->dev = block_device;
            file->inode = 1;
            file->inode_data = minix3_get_inode(block_device, file->inode);
            file->size = file->inode_data.size;
            file->is_dir = true;
            file->is_file = false;
            file->is_symlink = false;
            file->is_hardlink = false;
            file->is_block_device = false;
            file->is_char_device = false;
            file->major = 0;
            file->minor = 0;
            map_set(open_files, path, file);
            return file; 
        }
        return NULL;
    }

    if (path == NULL) {
        debugf("vfs_open: path is NULL\n");
        return NULL;
    }

    if (strlen(path) == 0 || strcmp(path, "") == 0) {
        debugf("vfs_open: path is empty\n");
        return NULL;
    }

    if (open_files == NULL) {
        open_files = map_new();
    }

    if (map_contains(open_files, path)) {
        debugf("vfs_open: file is already open\n");
        File *file;
        map_get(open_files, path, &file);
        return file;
    }


    File *file = kmalloc(sizeof(File));
    memset(file, 0, sizeof(File));
    file->path = kmalloc(strlen(path) + 1);
    strcpy(file->path, path);
    file->flags = flags;
    file->mode = mode;
    file->type = type;
    file->dev = vfs_get_mounted_device(path);

    char *path_relative_to_mount_point;
    // debugf("vfs_open: %s relative to mount point is %s\n", path, path_relative_to_mount_point);
    VirtioDevice *parent_device = vfs_get_mounted_device(path);
    debugf("vfs_open: parent device is %p\n", parent_device);
    char *block_device_name = path_file_name(path);
    uint32_t block_device_num = mounted_device_count;
    char *parent_path = get_parent_path(path);
    debugf("vfs_open: parent path is %s\n", parent_path);
    DirEntry dir_entry;
    size_t i = 0;
    uint32_t free_dir_entry = 0;
    uint32_t parent_inode = 0;

    bool is_parent_open = vfs_is_open(parent_path);
    debugf("vfs_open: parent is open: %u\n", is_parent_open);
    switch (type) {
    case VFS_TYPE_INFER:
        // Infer the type from the minix3 type
        file->dev = vfs_get_mounted_device(path);
        file->inode = minix3_get_inode_from_path(file->dev, file->path, false);
        // file->inode_data = minix3_get_inode(file->dev, file->inode);
        // file->size = file->inode_data.size;

        // kfree(path_relative_to_mount_point);
        kfree(parent_path);
        if (minix3_is_file(file->dev, file->inode)) {
            debugf("vfs_open: %s is a file\n", path);
            type = VFS_TYPE_FILE;
        } else if (minix3_is_dir(file->dev, file->inode)) {
            debugf("vfs_open: %s is a dir\n", path);
            type = VFS_TYPE_DIR;
        } else if (minix3_is_block_device(file->dev, file->inode)) {
            debugf("vfs_open: %s is a block device\n", path);
            type = VFS_TYPE_BLOCK;
        } else {
            debugf("vfs_open: could not infer type\n");
            kfree(file);
            return NULL;
        }
        return vfs_open(path, flags, mode, type);
        break;

    case VFS_TYPE_FILE:
        debugf("vfs_open: opening file\n");
        if (vfs_is_open(path)) {
            debugf("vfs_open: file is already open\n");
            return vfs_get_open_file(path);
        }
        if (!is_parent_open) {
            debugf("vfs_open: parent is not open\n");
            vfs_open(parent_path, flags, mode, VFS_TYPE_INFER);
            // if (!vfs_is_open(parent_path)) {
            //     debugf("vfs_open: could not open parent %s\n", parent_path);
            //     return NULL;
            // }
        }

        file->dev = vfs_get_mounted_device(path);
        path_relative_to_mount_point = get_path_relative_to_mount_point(path);
        debugf("vfs_open: device is %p\n", file->dev);
        file->inode = minix3_get_inode_from_path(file->dev, path_relative_to_mount_point, false);
        file->inode_data = minix3_get_inode(file->dev, file->inode);
        file->size = file->inode_data.size;
        file->is_file = true;

        if (!is_parent_open) {
            debugf("vfs_open: closing parent %s\n", parent_path);
            vfs_close(vfs_get_open_file(parent_path));
        }

        break;

    case VFS_TYPE_DIR:
        file->dev = vfs_get_mounted_device(path);
        path_relative_to_mount_point = get_path_relative_to_mount_point(path);
        file->inode = minix3_get_inode_from_path(file->dev, path_relative_to_mount_point, false);
        file->inode_data = minix3_get_inode(file->dev, file->inode);
        file->size = file->inode_data.size;
        file->is_dir = true;
        break;
    
    case VFS_TYPE_BLOCK:
        // Read the filename
        debugf("vfs_open: block device name is %s\n", block_device_name);
        path_relative_to_mount_point = get_path_relative_to_mount_point(path);

        // Get the number of the block device
        debugf("vfs_open: block device number is %u\n", block_device_num);
        file->inode = minix3_get_inode_from_path(parent_device, path_relative_to_mount_point, false);
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

            parent_inode = minix3_get_inode_from_path(parent_device, path_relative_to_mount_point, true);
            debugf("vfs_open: parent inode is %u\n", parent_inode);

            // Create the directory entry
            DirEntry dir_entry;
            memset(&dir_entry, 0, sizeof(dir_entry));
            dir_entry.inode = file->inode;
            strcpy(dir_entry.name, block_device_name);
            debugf("vfs_open: created dir entry with inode %u and name %s\n", dir_entry.inode, dir_entry.name);

            // Find the next free directory entry
            free_dir_entry = minix3_find_next_free_dir_entry(parent_device, parent_inode);
            debugf("vfs_open: found free dir entry %u\n", free_dir_entry);

            // Put the directory entry
            minix3_put_dir_entry(parent_device, parent_inode, free_dir_entry, dir_entry);
            debugf("vfs_open: put dir entry\n");

            // Create the inode
            Inode inode_data;
            memset(&inode_data, 0, sizeof(inode_data));
            inode_data.mode = S_IFBLK;
            inode_data.num_links = 1;
            inode_data.uid = 0;
            inode_data.gid = 0;
            inode_data.size = 0;
            inode_data.atime = 0;
            inode_data.mtime = 0;
            inode_data.ctime = 0;
            minix3_put_inode(parent_device, file->inode, inode_data);
            debugf("vfs_open: put inode %u\n", file->inode);
        } else {
            debugf("vfs_open: not creating block device\n");
        }

        // Get the block device
        file->dev = virtio_get_block_device(block_device_num);
        
        if (file->dev == NULL) {
            debugf("vfs_open: could not find block device\n");
            kfree(path_relative_to_mount_point);
            kfree(parent_path);
            return NULL;
        } else {
            debugf("vfs_open: found block device\n");
        }
        debugf("vfs_open: block device is %p\n", file->dev);
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
    kfree(path_relative_to_mount_point);
    kfree(parent_path);

    // Insert the file into the open files map
    map_set(open_files, path, file);
    if (!map_contains(open_files, path)) {
        debugf("vfs_open: could not insert file into open files map\n");
        return NULL;
    }

    // debugf("vfs_open: %d files open\n", map_size(open_files));
    debug_file(file);
    vfs_print_open_files();
    return file;
}

void vfs_close(File *file) {
    debugf("vfs_close: closing %s\n", file->path);
    if (open_files == NULL) {
        open_files = map_new();
    }
    if (!map_contains(open_files, file->path)) {
        debugf("vfs_close: file is not open\n");
        return;
    }
    // Remove the file from the open files map
    map_remove(open_files, file->path);
    // debug_file(file);
    vfs_print_open_files();
    kfree(file->path);
    kfree(file);
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
    minix3_put_dir_entry(block_device, dir->inode, free_dir_entry, data);

    Inode inode1_data = minix3_get_inode(block_device, file->inode);
    inode1_data.num_links += 1;
    minix3_put_inode(block_device, file->inode, inode1_data);
    return true;
}

// bool vfs_link(File *file1, File *file2) {
//     return vfs_link_paths(file1->path, file2->path);
// }


bool vfs_exists(const char *path) {
    // file->dev = vfs_get_mounted_device(path);
    // path_relative_to_mount_point = get_path_relative_to_mount_point(path);
    // file->inode = minix3_get_inode_from_path(file->dev, path_relative_to_mount_point, false);

    VirtioDevice *block_device = vfs_get_mounted_device(path);
    if (block_device == NULL) {
        debugf("vfs_exists: could not find block device\n");
        return false;
    }
    char *path_relative_to_mount_point = get_path_relative_to_mount_point(path);
    uint32_t inode = minix3_get_inode_from_path(block_device, path_relative_to_mount_point, false);
    if (inode == INVALID_INODE) {
        debugf("vfs_exists: could not find inode\n");
        return false;
    }
    return true;
}

bool vfs_is_dir(const char *path) {
    debugf("vfs_is_dir: %s\n", path);
    VirtioDevice *block_device = vfs_get_mounted_device(path);
    debugf("vfs_is_dir: block device is %p\n", block_device);
    if (block_device == NULL) {
        debugf("vfs_exists: could not find block device\n");
        return false;
    }
    char *path_relative_to_mount_point = get_path_relative_to_mount_point(path);
    debugf("vfs_is_dir: path relative to mount point is %s\n", path_relative_to_mount_point);
    uint32_t inode = minix3_get_inode_from_path(block_device, path_relative_to_mount_point, false);
    debugf("vfs_is_dir: inode is %u\n", inode);
    if (inode == INVALID_INODE) {
        debugf("vfs_exists: could not find inode\n");
        return false;
    }
    debugf("vfs_is_dir: inode is %u\n", inode);
    return minix3_is_dir(block_device, inode);
}

bool vfs_is_file(const char *path) {
    VirtioDevice *block_device = vfs_get_mounted_device(path);
    if (block_device == NULL) {
        debugf("vfs_exists: could not find block device\n");
        return false;
    }
    char *path_relative_to_mount_point = get_path_relative_to_mount_point(path);
    uint32_t inode = minix3_get_inode_from_path(block_device, path_relative_to_mount_point, false);
    if (inode == INVALID_INODE) {
        debugf("vfs_exists: could not find inode\n");
        return false;
    }
    return minix3_is_file(block_device, inode);
}

// List a directory separated by newlines to a buffer
void vfs_list_dir(const char *path, char *buf, size_t buf_size, bool return_full_path) {
    strcpy(buf, "");
    VirtioDevice *block_device = vfs_get_mounted_device(path);
    if (block_device == NULL) {
        debugf("vfs_exists: could not find block device\n");
        return false;
    }
    char *path_relative_to_mount_point = get_path_relative_to_mount_point(path);
    debugf("vfs_is_dir: path relative to mount point is %s\n", path_relative_to_mount_point);
    uint32_t inode = minix3_get_inode_from_path(block_device, path_relative_to_mount_point, false);
    debugf("vfs_is_dir: inode is %u\n", inode);

    // if (minix3_is_dir(block_device, inode)) {
    //     debugf("Is a directory\n");
    // } else {
    //     debugf("Not a directory\n");
    //     strcpy(buf, "");
    //     return;
    // }

    // Get the number of entries in the directory
    uint32_t max_entries = minix3_get_zone_size(block_device) / sizeof(DirEntry);
    DirEntry entries[max_entries];
    uint32_t num_entries = minix3_list_dir(block_device, inode, entries, max_entries);

    // List the entries
    char entry_name[64] = {0};
    char *entry_path = kmalloc(1024);
    for (size_t i = 0; i < num_entries; i++) {
        if (entries[i].inode == INVALID_INODE) {
            continue;
        }
        strncpy(entry_name, entries[i].name, sizeof(entry_name));
        if (strlen(entry_name) == 0) {
            break;
        }
        // for (j=0; j<sizeof(entries[0].name) && j<sizeof(name); j++) {
        //     if (entries[i].name[j] == 0) {
        //         name[j] = 0;
        //         break;
        //     }
        //     name[j] = entries[i].name[j];
        // }
        // if (strcmp(entry_name, ".") == 0 || strcmp(name, "..") == 0) {
        //     continue;
        // }

        strcpy(entry_path, path);
        if (strcmp(entry_path, "/") != 0) {
            strcat(entry_path, "/");
        }

        strcat(entry_path, entry_name);

        // if (!return_full_path && strlen(buf) + sizeof(entry_name) >= buf_size) {
        //     debugf("Buffer is full\n");
        //     break;
        // } else if (return_full_path && strlen(buf) + sizeof(entry_path) + sizeof(entry_name) >= buf_size) {
        //     debugf("Buffer is full\n");
        //     break;
        // }

        if (return_full_path) {
            if (strlen(buf) + strlen(entry_path) >= buf_size) {
                debugf("Buffer is full\n");
                break;
            }
            strcat(buf, entry_path);
        } else {
            if (strlen(buf) + strlen(entry_name) >= buf_size) {
                debugf("Buffer is full\n");
                break;
            }
            strcat(buf, entry_name);
        }
        strcat(buf, "\n");
    }
    infof("Listed directory\n");
}