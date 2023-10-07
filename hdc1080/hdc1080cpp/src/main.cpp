#include <iostream>
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


int main() {
    int i2c_fd = open("/dev/i2c-1", O_RDWR); // Open the I2C file

    if (i2c_fd < 0) {
        std::cerr << "Unable to open I2C connection" << std::endl;
        return 1;
    }

    if (ioctl(i2c_fd, I2C_SLAVE, HDC1080_ADDRESS) < 0) {
        std::cerr << "Unable to set I2C address" << std::endl;
        return 1;
    }
    uint8_t config_data[] = {0x02, TEMP_RES_14BIT | ACQ_MODE_CONTINUOUS};
    if (write(i2c_fd, config_data, sizeof(config_data)) != sizeof(config_data)) {
        std::cerr << "Error sending configuration data" << std::endl;
        return 1;
    }
    // Send a command to read data from the HDC1080 sensor
    while (1)
    {
      uint8_t write_buffer[] = {0x00};
      if (write(i2c_fd, write_buffer, sizeof(write_buffer)) != sizeof(write_buffer)) {
          std::cerr << "Error sending read command" << std::endl;
          return 1;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      // Read temperature and humidity data from the HDC1080 sensor
      uint8_t read_buffer[4] = {0};
      if (read(i2c_fd, read_buffer, sizeof(read_buffer)) != sizeof(read_buffer)) {
          std::cerr << "Error reading data" << std::endl;
          return 1;
      }

      int raw_temperature = (read_buffer[0] << 8) | read_buffer[1];
      float temperature = (raw_temperature / 65536.0) * 165.0 - 40.0;

      uint8_t write_buffer1[] = {0x01};
      if (write(i2c_fd, write_buffer1, sizeof(write_buffer1)) != sizeof(write_buffer1)) {
          std::cerr << "Error sending read command" << std::endl;
          return 1;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if (read(i2c_fd, read_buffer, sizeof(read_buffer)) != sizeof(read_buffer)) {
          std::cerr << "Error reading data" << std::endl;
          return 1;
      }
      int raw_humidity = (read_buffer[0] << 8) | read_buffer[1];
      float humidity = (raw_humidity / 65536.0) * 100.0;

      std::cout << "Temperature: " << temperature << " °C, Humidity: " << humidity << " %" << std::endl;
    }
    close(i2c_fd); // Close the I2C connection

    return 0;
}