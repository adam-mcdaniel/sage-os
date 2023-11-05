#include <stdint.h>
#include <string.h>
#include <vfs.h>
#include <debug.h>
#include "path.h"


int vfs_read(File *file, void *buf, int count);
int vfs_write(File *file, const char *buf, int count);


// Populate stat from a path.
// Used by stat(2).
int vfs_stat_path(const char *path, Stat *stat) {
    uint32_t inode = minix3_path_to_inode(path);
    Inode data = minix3_get_inode(inode);
    stat->inode = inode;
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

int vfs_stat(File *file, Stat *stat) {
    return vfs_stat_path(file->path, stat);
}

bool vfs_link_paths(const char *path1, const char *path2) {
    uint32_t inode1 = minix3_get_inode_from_path(path1, 0);
    uint32_t inode2 = minix3_get_inode_from_path(path2, 1);

    // path1 must exist
    if (inode1 == INVALID_INODE) {
        debugf("vfs_link: path1 doesn't exist\n");
        return false;
    }

    // path1 must not be a directory
    if (minix3_is_dir(inode1)) {
        debugf("vfs_link: path1 is a directory\n");
        return false;
    }

    // Parent of path2 must exist
    if (inode2 == INVALID_INODE) {
        debugf("vfs_link: parent of path2 doesn't exist\n");
        return false;
    }

    DirEntry data;
    memset(&data, 0, sizeof(data));
    data.inode = inode1;
    strcpy(data.name, path_file_name(path2));
    uint32_t free_dir_entry = minix3_find_next_free_dir_entry(inode2);
    minix3_put_dir_entry(inode2, free_dir_entry, &data);

    Inode inode1_data = minix3_get_inode(inode1);
    inode1_data.num_links += 1;
    minix3_put_inode(inode1, inode1_data);
    return true;
}

bool vfs_link(File *file1, File *file2) {
    return vfs_link_paths(file1->path, file2->path);
}

