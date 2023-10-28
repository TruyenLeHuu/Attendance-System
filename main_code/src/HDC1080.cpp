#include "HDC1080.h"
#include <iostream>
int init_hdc1080()
{
    int i2c_fd = open("/dev/i2c-1", O_RDWR); // Open the I2C file

    if (i2c_fd < 0) {
        std::cerr << "Unable to open I2C connection" << std::endl;
        return 0;
    }

    if (ioctl(i2c_fd, I2C_SLAVE, HDC1080_ADDRESS) < 0) {
        std::cerr << "Unable to set I2C address" << std::endl;
        return 0;
    }
    uint8_t config_data[] = {0x02, TEMP_RES_14BIT | ACQ_MODE_CONTINUOUS};
    if (write(i2c_fd, config_data, sizeof(config_data)) != sizeof(config_data)) {
        std::cerr << "Error sending configuration data" << std::endl;
        return 0;
    }
    return i2c_fd;
} 
int read_hdc1080(int& i2c_fd, float& temperature, float& humidity)
{
    uint8_t write_buffer[] = {0x00};
    if (write(i2c_fd, write_buffer, sizeof(write_buffer)) != sizeof(write_buffer)) {
        std::cerr << "Error sending read command" << std::endl;
        return 0;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // Read temperature and humidity data from the HDC1080 sensor
    uint8_t read_buffer[4] = {0};
    if (read(i2c_fd, read_buffer, sizeof(read_buffer)) != sizeof(read_buffer)) {
        std::cerr << "Error reading data" << std::endl;
        return 0;
    }

    int raw_temperature = (read_buffer[0] << 8) | read_buffer[1];
    temperature = (raw_temperature / 65536.0) * 165.0 - 40.0;

    uint8_t write_buffer1[] = {0x01};
    if (write(i2c_fd, write_buffer1, sizeof(write_buffer1)) != sizeof(write_buffer1)) {
        std::cerr << "Error sending read command" << std::endl;
        return 0;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (read(i2c_fd, read_buffer, sizeof(read_buffer)) != sizeof(read_buffer)) {
        std::cerr << "Error reading data" << std::endl;
        return 0;
    }
    int raw_humidity = (read_buffer[0] << 8) | read_buffer[1];
    humidity = (raw_humidity / 65536.0) * 100.0;
    return 1; 
}