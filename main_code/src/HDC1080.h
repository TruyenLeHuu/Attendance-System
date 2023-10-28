#ifndef HDC1080_h
#define HDC1080_h
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define HDC1080_ADDRESS 0x40 // Address of the HDC1080 sensor

// Define the configuration bits
#define TEMP_RES_BIT 7
#define ACQ_MODE_BIT 0

// Define acquisition mode values
#define TEMP_RES_14BIT (0 << TEMP_RES_BIT)
#define TEMP_RES_11BIT (1 << TEMP_RES_BIT)
#define TEMP_RES_9BIT  (2 << TEMP_RES_BIT)
#define ACQ_MODE_ONESHOT (0 << ACQ_MODE_BIT)
#define ACQ_MODE_CONTINUOUS (1 << ACQ_MODE_BIT)

int init_hdc1080();
int read_hdc1080(int& i2c_fd, float& temperature, float& humidity);

#endif