#pragma once

typedef unsigned long size_t;
typedef signed long ssize_t;
typedef signed long off_t;
typedef unsigned short mode_t;
typedef unsigned long dev_t;

struct stat;

void    exit   (void);
void    yield  (void);
void    sleep  (int secs);
int     fstat  (const char *path, struct stat *stat);
void   *sbrk   (int amount);
int     open   (const char *pathname, int flags, mode_t mode);
int     close  (int fd);
ssize_t read   (int fd, void *buf, size_t count);
ssize_t write  (int fd, const void *buf, size_t count);
off_t   lseek  (int fd, off_t offset, int whence);
int     unlink (const char *path);
int     chmod  (const char *path, mode_t mode);
int     mkdir  (const char *path, mode_t mode);
int     rmdir  (const char *path);
int     chdir  (const char *path);
size_t  getcwd (char *buf, size_t bufsize);
int     mknod  (const char *path, mode_t mode, dev_t dev);
int     fork   (void);
int     exec   (const char *path, const char *argv[]);
int     wait   (int pid);
int     kill   (int pid);

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  0100
#define O_TRUNC  01000

