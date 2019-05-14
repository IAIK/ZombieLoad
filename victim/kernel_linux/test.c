#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "leaky.h"

int main(void) {
    int leaky_fd = open(LEAKY_DEVICE_PATH, O_RDONLY);
    if (leaky_fd < 0) {
        fprintf(stderr, "Error: Could not open leaky kernel module: %s\n", LEAKY_DEVICE_PATH);
        return 0;
    }
    
    // trigger kernel to load data
    while(1) {
        ioctl(leaky_fd, LEAKY_IOCTL_CMD_CACHE, (size_t)0);
    }
    exit(EXIT_SUCCESS);
}
