#include "rtfs.h"

struct open_file openfiles[MAX_OPEN_FILES];
struct rtfs_config rtfs_conf;

struct fuse_operations rt_oper = {
        .open    = rt_open,
        .read    = rt_read,
        .write   = rt_write,
        .release = rt_release,
};

int rt_open(const char *path, struct fuse_file_info *fi) {
    printf("rt_open: %s\n", path);

    char remote_path[PATH_MAX];
    snprintf(remote_path, sizeof(remote_path), "%s%s", REMOTE_BASE, path);

    char local_path[PATH_MAX];
    snprintf(local_path, sizeof(local_path), "%s%s", LOCAL_BASE, path);

    char cmd[PATH_MAX * 3];
    snprintf(cmd, sizeof(cmd),
             "mkdir -p \"%s\" && scp %s@%s:\"%s\" \"%s\"",
             dirname(strdup(local_path)), REMOTE_USER, REMOTE_HOST,
             remote_path, local_path);

    printf("running: %s\n", cmd);
    int ret = system(cmd);

    if (ret != 0) {
        perror("scp fetch failed");
        return -EIO;
    }

    int fd = open(local_path, O_RDWR);
    if (fd == -1) {
        perror("open local copy failed");
        return -errno;
    }

    fi->fh = fd;   // store local fd directly for simplicity
    return 0;
}

int rt_release(const char *path, struct fuse_file_info *fi) {
    printf("rt_release: %s\n", path);

    char remote_path[PATH_MAX];
    snprintf(remote_path, sizeof(remote_path), "%s%s", REMOTE_BASE, path);

    char local_path[PATH_MAX];
    snprintf(local_path, sizeof(local_path), "%s%s", LOCAL_BASE, path);

    char cmd[PATH_MAX * 3];
    snprintf(cmd, sizeof(cmd),
             "scp \"%s\" %s@%s:\"%s\"",
             local_path, REMOTE_USER, REMOTE_HOST, remote_path);

    printf("running: %s\n", cmd);
    int ret = system(cmd);

    if (ret != 0) {
        perror("scp push failed");
        return -EIO;
    }

    int res = close(fi->fh);
    if (res == -1) {
        perror("close local copy failed");
        return -errno;
    }
    return 0;
}

int rt_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd = fi->fh;  // local file descriptor set in rt_open
    ssize_t res = pread(fd, buf, size, offset);

    if (res == -1) {
        int err = -errno;
        perror("pread failed");
        return err;
    }

    printf("rt_read(path=\"%s\", size=%zu, offset=%lld) -> %zd bytes\n",
           path, size, (long long)offset, res);

    return (int)res;
}

int rt_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd = fi->fh;  // local file descriptor set in rt_open
    ssize_t res = pwrite(fd, buf, size, offset);

    if (res == -1) {
        int err = -errno;
        perror("pwrite failed");
        return err;
    }

    printf("rt_write(path=\"%s\", size=%zu, offset=%lld) -> %zd bytes\n",
           path, size, (long long)offset, res);

    return (int)res;
}


int main(int argc, char** argv) {
    if (argc < 5) {
        fprintf(stderr,
                "Usage: %s <remote_user> <remote_host> <remote_base> <mountpoint> [FUSE options]\n",
                argv[0]);
        exit(1);
    }
    rtfs_conf.remote_user = argv[1];
    rtfs_conf.remote_host = argv[2];
    rtfs_conf.remote_base = realpath(argv[3], NULL); // canonical remote dir string
    argv[1] = argv[4];
    for (int i = 2; i < argc - 3; i++) {
        argv[i] = argv[i + 3];
    }
    argc -= 3;
    return fuse_main(argc, argv, &rt_oper);
}
