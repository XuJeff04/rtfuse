#ifndef RTFS_H
#define RTFS_H
#define MAX_OPEN_FILES 1024

// Remote server configuration
#define REMOTE_USER     "user"
#define REMOTE_HOST     "remote"
#define REMOTE_BASE     "/temp/fuse"

// Local mountpoint (must match where you mount the FUSE FS)
#define LOCAL_BASE      "/mnt/rtfs"
#include <stdio.h>
#include <fuse.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <time.h>
#include <fcntl.h>

int rt_open(const char *path, struct fuse_file_info *fi);
int rt_release(const char *path, struct fuse_file_info *fi);
int rt_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int rt_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

struct open_file {
    char remote_path[PATH_MAX];
    char local_path[PATH_MAX];
    int local_fd;
};

extern struct rtfs_config rtfs_conf;

struct rtfs_config {
    char *remote_user;
    char *remote_host;
    char *remote_base;
    char *local_base;   // usually the FUSE mountpoint
};

#endif


