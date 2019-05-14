/* See LICENSE file for license and copyright information */

#ifndef LEAKY_MODULE_H
#define LEAKY_MODULE_H

#include <stddef.h>

#define LEAKY_DEVICE_NAME "leaky"
#define LEAKY_DEVICE_PATH "/dev/" LEAKY_DEVICE_NAME

#define LEAKY_IOCTL_MAGIC_NUMBER (long)0xc31

#define LEAKY_IOCTL_CMD_CACHE \
  _IOR(LEAKY_IOCTL_MAGIC_NUMBER, 1, size_t)

  #endif // LEAKY_MODULE_H
