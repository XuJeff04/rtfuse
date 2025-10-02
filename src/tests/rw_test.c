#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define BLOCK_SIZE 4096   // Alignment requirement
#define BUF_SIZE   4096   // Must be multiple of BLOCK_SIZE

// Compute difference in nanoseconds
long diff_ns(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000000000L +
           (end.tv_nsec - start.tv_nsec);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <read|write> <iterations> <file>\n", argv[0]);
        return 1;
    }

    // Parse arguments
    int do_write = 0;
    if (strcmp(argv[1], "write") == 0) {
        do_write = 1;
    } else if (strcmp(argv[1], "read") == 0) {
        do_write = 0;
    } else {
        fprintf(stderr, "First argument must be 'read' or 'write'\n");
        return 1;
    }

    int iterations = atoi(argv[2]);
    if (iterations <= 0) {
        fprintf(stderr, "Iterations must be > 0\n");
        return 1;
    }

    char *filename = argv[3];

    // Open file with O_DIRECT (file must already exist!)
    int fd = open(filename, O_RDWR | O_DIRECT);
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

    memset(buf, 'A', BUF_SIZE);

    struct timespec start, end;

    // --------------------
    // Benchmark
    // --------------------
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < iterations; i++) {
        off_t offset = (off_t)i * BUF_SIZE;

        if (do_write) {
            if (pwrite(fd, buf, BUF_SIZE, offset) != BUF_SIZE) {
                perror("pwrite");
                free(buf);
                close(fd);
                return 1;
            }
        } else {
            if (pread(fd, buf, BUF_SIZE, offset) < 0) {
                perror("pread");
                free(buf);
                close(fd);
                return 1;
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed_ms = diff_ns(start, end) / 1e6;
    if (do_write) {
        printf("Total write time (%d iterations): %.2f ms\n", iterations, elapsed_ms);
    } else {
        printf("Total read time (%d iterations):  %.2f ms\n", iterations, elapsed_ms);
    }

    free(buf);
    close(fd);
    return 0;
}
