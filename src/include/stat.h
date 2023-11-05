/**
 * @file stat.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief File system stat structures.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include <stdint.h>

#define MAX_FILE_NAME      60

#define S_FMT(x)       (S_IFMT & (x))

#define S_IFMT        0170000 /* These bits determine file type.  */

/* File types.  */
#define S_IFDIR       0040000 /* Directory.  */
#define S_IFCHR       0020000 /* Character device.  */
#define S_IFBLK       0060000 /* Block device.  */
#define S_IFREG       0100000 /* Regular file.  */
#define S_IFIFO       0010000 /* FIFO.  */
#define S_IFLNK       0120000 /* Symbolic link.  */
#define S_IFSOCK      0140000 /* Socket.  */

/* Protection bits.  */

#define S_ISUID       04000   /* Set user ID on execution.  */
#define S_ISGID       02000   /* Set group ID on execution.  */
#define S_ISVTX       01000   /* Save swapped text after use (sticky).  */
#define S_IREAD       0400    /* Read by owner.  */
#define S_IWRITE      0200    /* Write by owner.  */
#define S_IEXEC       0100    /* Execute by owner.  */

typedef struct Stat {
    uint32_t inode;    /* Inode number */
    uint16_t mode;     /* File type and mode */
    uint16_t num_link; /* Number of hard links  */
    uint32_t uid;      /* User ID of owner */ 
    uint32_t gid;      /* Group ID of owner */
    uint32_t size;     /* Total size, in bytes */
    uint32_t atime;    /* Time of last access */ 
    uint32_t mtime;    /* Time of last modification */ 
    uint32_t ctime;    /* Time of last status change */ 
} Stat;
