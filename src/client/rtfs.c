#include "rtfs.h"

struct open_file openfiles[MAX_OPEN_FILES];
struct rtfs_config rtfs_conf;

struct fuse_operations rt_oper = {
    .getattr	= rt_getattr,
    .readdir	= rt_readdir,
    .open       = rt_open,
    .read       = rt_read,
    .write      = rt_write,
    .release    = rt_release,
};

int rt_getattr(const char *path, struct stat *statbuf)
{
    printf("rt_getattr: %s\n", path);

    char remote_path[PATH_MAX];
    snprintf(remote_path, sizeof(remote_path), "%s%s", rtfs_conf.remote_base, path);

    // We’ll use ssh to run stat(1) in “terse” mode (-c) on the remote host.
    // %s: size, %f: raw mode (hex), %u: uid, %g: gid, %X: atime, %Y: mtime, %Z: ctime
    char cmd[PATH_MAX * 4];
    snprintf(cmd, sizeof(cmd),
             "ssh %s@%s 'stat -c \"%%s %%f %%u %%g %%X %%Y %%Z\" \"%s\"'",
             rtfs_conf.remote_user,
             rtfs_conf.remote_host,
             remote_path);

    printf("running: %s\n", cmd);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        perror("popen failed");
        return -EIO;
    }

    unsigned long size, mode_hex, uid, gid;
    long atime, mtime, ctime;
    if (fscanf(fp, "%lu %lx %lu %lu %ld %ld %ld",
               &size, &mode_hex, &uid, &gid, &atime, &mtime, &ctime) != 7) {
        perror("failed to parse stat output");
        pclose(fp);
        return -EIO;
    }
    pclose(fp);

    memset(statbuf, 0, sizeof(*statbuf));
    statbuf->st_size = size;
    statbuf->st_mode = (mode_t)mode_hex;
    statbuf->st_uid  = (uid_t)uid;
    statbuf->st_gid  = (gid_t)gid;
    statbuf->st_atime = atime;
    statbuf->st_mtime = mtime;
    statbuf->st_ctime = ctime;

    return 0;
}

int rt_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
               struct fuse_file_info *fi)
{
    printf("rt_readdir: %s\n", path);

    // Build the remote and local paths
    char remote_path[PATH_MAX];
    snprintf(remote_path, sizeof(remote_path), "%s%s", rtfs_conf.remote_base, path);

    // Required entries
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    // Command to run remotely
    char cmd[PATH_MAX * 4];
    snprintf(cmd, sizeof(cmd),
             "ssh %s@%s 'ls -1 \"%s\"'",
             rtfs_conf.remote_user,
             rtfs_conf.remote_host,
             remote_path);

    printf("running: %s\n", cmd);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        perror("popen failed");
        return -EIO;
    }

    char name[PATH_MAX];
    while (fgets(name, sizeof(name), fp)) {
        // strip newline
        name[strcspn(name, "\n")] = 0;

        if (strlen(name) > 0) {
            filler(buf, name, NULL, 0, 0);
        }
    }

    int ret = pclose(fp);
    if (ret != 0) {
        perror("ssh ls failed");
        return -EIO;
    }

    return 0;
}




int rt_open(const char *path, struct fuse_file_info *fi) {
    printf("rt_open: %s\n", path);

    char remote_path[PATH_MAX];
    snprintf(remote_path, sizeof(remote_path), "%s%s", rtfs_conf.remote_base, path);

    char local_path[PATH_MAX];
    snprintf(local_path, sizeof(local_path), "%s%s", LOCAL_CACHE, path);

    printf("DEBUG: user=%s host=%s base=%s local_base=%s, dir=%s\n",
           rtfs_conf.remote_user ? rtfs_conf.remote_user : "(null)",
           rtfs_conf.remote_host ? rtfs_conf.remote_host : "(null)",
           rtfs_conf.remote_base ? rtfs_conf.remote_base : "(null)",
           rtfs_conf.local_base  ? rtfs_conf.local_base  : "(null)");

    char *cmd = malloc(PATH_MAX * 3);
    if (!cmd) {
        perror("malloc failed");
        return -ENOMEM;
    }

    system("env > /tmp/rtfs_env.txt");

    snprintf(cmd, PATH_MAX * 3,
         "scp %s@%s:\"%s\" \"%s\"",
         rtfs_conf.remote_user, rtfs_conf.remote_host,
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
    snprintf(remote_path, sizeof(remote_path), "%s%s", rtfs_conf.remote_base, path);

    char local_path[PATH_MAX];
    snprintf(local_path, sizeof(local_path), "%s%s", LOCAL_CACHE, path);

    char cmd[PATH_MAX * 3];
    snprintf(cmd, sizeof(cmd),
             "scp \"%s\" %s@%s:\"%s\"",
             local_path, rtfs_conf.remote_user, rtfs_conf.remote_host, remote_path);

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
    rtfs_conf.local_base  = argv[argc - 1];

    int fuse_argc = argc - 3;
    char **fuse_argv = malloc(sizeof(char*) * (fuse_argc));
    fuse_argv[0] = argv[0];        // program name
    for (int i = 1; i < fuse_argc; i++) {
        fuse_argv[i] = argv[i + 3]; // shift everything left by 3
    }
    int ret = fuse_main(fuse_argc, fuse_argv, &rt_oper, NULL);
    free(fuse_argv);
    return ret;
}
