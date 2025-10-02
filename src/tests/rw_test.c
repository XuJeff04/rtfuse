#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define BLOCK_SIZE 4096   // Must match alignment requirement
#define BUF_SIZE   4096   // Read/write size (multiple of BLOCK_SIZE)
#define ITERATIONS 1000   // Number of operations

// Compute difference in nanoseconds
long diff_ns(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000000000L +
           (end.tv_nsec - start.tv_nsec);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <existing-file>\n", argv[0]);
        return 1;
    }

    // Open file with O_DIRECT (file must already exist!)
    int fd = open(argv[1], O_RDWR | O_DIRECT);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    // Allocate aligned buffer
    void *buf;
    if (posix_memalign(&buf, BLOCK_SIZE, BUF_SIZE) != 0) {
        perror("posix_memalign");
        close(fd);
        return 1;
    }

    memset(buf, 'A', BUF_SIZE); // fill buffer with data

    struct timespec start, end;

    // --------------------
    // Measure write loop
    // --------------------
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < ITERATIONS; i++) {
        off_t offset = (off_t)(i % 100) * BUF_SIZE;
        if (pwrite(fd, buf, BUF_SIZE, offset) != BUF_SIZE) {
            perror("pwrite");
            free(buf);
            close(fd);
            return 1;
        }
    }
    fsync(fd);  // flush to disk
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Total write time: %.2f ms\n", diff_ns(start, end) / 1e6);

    // --------------------
    // Measure read loop
    // --------------------
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < ITERATIONS; i++) {
        off_t offset = (off_t)(i % 100) * BUF_SIZE;
        if (pread(fd, buf, BUF_SIZE, offset) < 0) {
            perror("pread");
            free(buf);
            close(fd);
            return 1;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Total read time:  %.2f ms\n", diff_ns(start, end) / 1e6);

    free(buf);
    close(fd);
    return 0;
}
