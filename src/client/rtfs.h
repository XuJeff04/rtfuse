#ifndef RTFS_H
#define RTFS_H
// Remote server configuration
#define REMOTE_USER     "user"
#define REMOTE_HOST     "remote"
#define REMOTE_BASE     "/temp/fuse"

// Local mountpoint (must match where you mount the FUSE FS)
#define LOCAL_BASE      "/mnt/rtfs"

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


