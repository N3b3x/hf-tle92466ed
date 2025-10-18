# Getting Started with TLE92466ED Driver

## Quick Start Guide

This guide will help you integrate and use the TLE92466ED driver in your project.

## Step 1: Prerequisites

### Hardware Requirements
- TLE92466ED IC on your circuit board
- SPI-capable microcontroller
- Power supply (8V - 40V for VSUP)
- Load devices connected to outputs

### Software Requirements
- C++20 or later compatible compiler
- SPI library for your platform
- GPIO library for your platform

### Compiler Support
```
┌──────────────────┬─────────────┬──────────────┐
│ Compiler         │ Min Version │ C++ Standard │
├──────────────────┼─────────────┼──────────────┤
│ GCC              │ 11.0+       │ -std=c++20   │
│ Clang            │ 13.0+       │ -std=c++20   │
│ MSVC             │ 19.30+      │ /std:c++20   │
│ ARM GCC          │ 11.0+       │ -std=c++20   │
└──────────────────┴─────────────┴──────────────┘
```

## Step 2: Project Integration

### Directory Structure
```
your_project/
├── include/
│   ├── TLE92466ED.hpp
│   ├── TLE92466ED_HAL.hpp
│   └── TLE92466ED_Registers.hpp
├── src/
│   └── TLE92466ED.cpp
├── platform/
│   └── your_hal_implementation.hpp
└── main.cpp
```

### CMake Integration
```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(TLE92466ED_Example CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add driver library
add_library(tle92466ed
    src/TLE92466ED.cpp
)

target_include_directories(tle92466ed PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Your application
add_executable(myapp
    main.cpp
    platform/your_hal_implementation.cpp
)

target_link_libraries(myapp PRIVATE tle92466ed)
```

### Makefile Integration
```makefile
# Makefile
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -I./include

SOURCES = src/TLE92466ED.cpp main.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = myapp

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
```

## Step 3: Implement Hardware Abstraction Layer

Create a HAL implementation for your platform:

```cpp
// platform/my_platform_hal.hpp
#include "TLE92466ED_HAL.hpp"
#include "spi.h"  // Your platform's SPI library

namespace TLE92466ED {

class MyPlatformHAL : public HAL {
public:
    MyPlatformHAL(SPI_HandleTypeDef* hspi, GPIO_Pin cs_pin)
        : hspi_(hspi), cs_pin_(cs_pin) {}

    [[nodiscard]] HALResult<void> init() override {
        // Initialize your SPI hardware
        // Initialize CS GPIO pin
        gpio_set_mode(cs_pin_, GPIO_OUTPUT);
        gpio_write(cs_pin_, GPIO_HIGH);
        return {};
    }

    [[nodiscard]] HALResult<uint16_t> transfer(uint16_t tx_data) override {
        uint16_t rx_data;
        
        // Assert CS
        gpio_write(cs_pin_, GPIO_LOW);
        
        // Transfer data (platform-specific)
        spi_transfer_16bit(hspi_, tx_data, &rx_data);
        
        // Deassert CS
        gpio_write(cs_pin_, GPIO_HIGH);
        
        return rx_data;
    }

    // Implement other required virtual functions...

private:
    SPI_HandleTypeDef* hspi_;
    GPIO_Pin cs_pin_;
};

} // namespace TLE92466ED
```

## Step 4: Basic Application

```cpp
// main.cpp
#include "TLE92466ED.hpp"
#include "my_platform_hal.hpp"
#include <iostream>

using namespace TLE92466ED;

int main() {
    // 1. Create HAL instance
    MyPlatformHAL hal(/* SPI handle */, /* CS pin */);
    
    // 2. Create driver instance
    Driver driver(hal);
    
    // 3. Initialize
    if (auto result = driver.init(); !result) {
        std::cerr << "Failed to initialize driver!" << std::endl;
        return 1;
    }
    
    // 4. Configure channel 0
    ChannelConfig config{
        .current_limit = CurrentLimit::LIMIT_2A,
        .slew_rate = SlewRate::MEDIUM,
        .pwm_enabled = false,
        .diagnostics_enabled = true,
        .inverted = false
    };
    
    driver.configure_channel(Channel::CH0, config);
    
    // 5. Turn on channel 0
    driver.set_channel(Channel::CH0, true);
    
    // 6. Monitor status
    if (auto status = driver.get_channel_status(Channel::CH0)) {
        if (status->has_fault) {
            std::cout << "Channel 0 has a fault!" << std::endl;
        } else {
            std::cout << "Channel 0 operating normally" << std::endl;
        }
    }
    
    return 0;
}
```

## Step 5: Platform-Specific Examples

### STM32 HAL Example

```cpp
#include "TLE92466ED_HAL.hpp"
#include "stm32f4xx_hal.h"

class STM32_HAL : public TLE92466ED::HAL {
public:
    STM32_HAL(SPI_HandleTypeDef* hspi, GPIO_TypeDef* cs_port, uint16_t cs_pin)
        : hspi_(hspi), cs_port_(cs_port), cs_pin_(cs_pin) {}

    HALResult<void> init() override {
        // CS pin already initialized by CubeMX
        HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_SET);
        return {};
    }

    HALResult<uint16_t> transfer(uint16_t tx_data) override {
        uint16_t rx_data = 0;
        
        HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_RESET);
        HAL_SPI_TransmitReceive(hspi_, 
                                (uint8_t*)&tx_data, 
                                (uint8_t*)&rx_data, 
                                1, 
                                100);
        HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_SET);
        
        return rx_data;
    }

    HALResult<void> delay(std::chrono::microseconds duration) override {
        HAL_Delay(duration.count() / 1000);  // HAL_Delay uses ms
        return {};
    }

    // ... implement other methods

private:
    SPI_HandleTypeDef* hspi_;
    GPIO_TypeDef* cs_port_;
    uint16_t cs_pin_;
};
```

### ESP32 Example

```cpp
#include "TLE92466ED_HAL.hpp"
#include "driver/spi_master.h"
#include "driver/gpio.h"

class ESP32_HAL : public TLE92466ED::HAL {
public:
    ESP32_HAL(spi_host_device_t host, gpio_num_t cs_pin)
        : host_(host), cs_pin_(cs_pin) {}

    HALResult<void> init() override {
        // Configure SPI bus
        spi_bus_config_t bus_cfg = {
            .mosi_io_num = MOSI_PIN,
            .miso_io_num = MISO_PIN,
            .sclk_io_num = SCLK_PIN,
            .max_transfer_sz = 4096
        };
        spi_bus_initialize(host_, &bus_cfg, SPI_DMA_CH_AUTO);
        
        // Configure device
        spi_device_interface_config_t dev_cfg = {
            .mode = 0,
            .clock_speed_hz = 1000000,  // 1 MHz
            .spics_io_num = cs_pin_,
            .queue_size = 1
        };
        spi_bus_add_device(host_, &dev_cfg, &spi_);
        
        return {};
    }

    HALResult<uint16_t> transfer(uint16_t tx_data) override {
        spi_transaction_t trans = {};
        trans.length = 16;
        trans.tx_data[0] = (tx_data >> 8) & 0xFF;
        trans.tx_data[1] = tx_data & 0xFF;
        trans.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
        
        spi_device_transmit(spi_, &trans);
        
        uint16_t rx_data = (trans.rx_data[0] << 8) | trans.rx_data[1];
        return rx_data;
    }

    HALResult<void> delay(std::chrono::microseconds duration) override {
        vTaskDelay(pdMS_TO_TICKS(duration.count() / 1000));
        return {};
    }

    // ... implement other methods

private:
    spi_host_device_t host_;
    gpio_num_t cs_pin_;
    spi_device_handle_t spi_;
};
```

### Linux (spidev) Example

```cpp
#include "TLE92466ED_HAL.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

class Linux_HAL : public TLE92466ED::HAL {
public:
    Linux_HAL(const char* device) : device_(device), fd_(-1) {}

    HALResult<void> init() override {
        fd_ = open(device_, O_RDWR);
        if (fd_ < 0) {
            return std::unexpected(HALError::HardwareNotReady);
        }
        
        uint8_t mode = SPI_MODE_0;
        uint32_t speed = 1000000;
        uint8_t bits = 8;
        
        ioctl(fd_, SPI_IOC_WR_MODE, &mode);
        ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
        ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &bits);
        
        return {};
    }

    HALResult<uint16_t> transfer(uint16_t tx_data) override {
        uint8_t tx[2] = {
            static_cast<uint8_t>((tx_data >> 8) & 0xFF),
            static_cast<uint8_t>(tx_data & 0xFF)
        };
        uint8_t rx[2] = {0};
        
        struct spi_ioc_transfer spi_trans = {};
        spi_trans.tx_buf = (unsigned long)tx;
        spi_trans.rx_buf = (unsigned long)rx;
        spi_trans.len = 2;
        spi_trans.speed_hz = 1000000;
        spi_trans.bits_per_word = 8;
        
        if (ioctl(fd_, SPI_IOC_MESSAGE(1), &spi_trans) < 0) {
            return std::unexpected(HALError::TransferError);
        }
        
        uint16_t rx_data = (rx[0] << 8) | rx[1];
        return rx_data;
    }

    // ... implement other methods

private:
    const char* device_;
    int fd_;
};
```

## Step 6: Common Usage Patterns

### Pattern 1: Simple On/Off Control
```cpp
// Turn on channel
driver.set_channel(Channel::CH0, true);

// Wait
std::this_thread::sleep_for(std::chrono::seconds(1));

// Turn off channel
driver.set_channel(Channel::CH0, false);
```

### Pattern 2: PWM Control
```cpp
// Configure for PWM
GlobalConfig config{
    .pwm_mode = true,
    .pwm_frequency = PWMFrequency::FREQ_1KHZ
};
driver.configure_global(config);

// Enable PWM on channel 0
ChannelConfig ch_config{
    .pwm_enabled = true
};
driver.configure_channel(Channel::CH0, ch_config);

// Control via your PWM duty cycle logic
// (external PWM signal or internal PWM)
```

### Pattern 3: Fault Monitoring
```cpp
// Periodic monitoring loop
while (true) {
    for (int i = 0; i < 6; ++i) {
        auto ch = static_cast<Channel>(i);
        
        if (auto status = driver.get_channel_status(ch)) {
            if (status->has_fault) {
                std::cout << "Channel " << i << " fault detected!" << std::endl;
                
                // Check specific faults
                if (status->short_to_gnd) {
                    std::cout << "  - Short to ground!" << std::endl;
                    // Take corrective action
                    driver.set_channel(ch, false);
                }
                
                if (status->open_load_on) {
                    std::cout << "  - Open load detected" << std::endl;
                }
            }
        }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```

### Pattern 4: Configuration with Error Handling
```cpp
auto init_result = driver.init();
if (!init_result) {
    switch (init_result.error()) {
        case DriverError::HardwareError:
            std::cerr << "SPI communication failed" << std::endl;
            break;
        case DriverError::WrongDeviceID:
            std::cerr << "Wrong device detected" << std::endl;
            break;
        default:
            std::cerr << "Unknown error" << std::endl;
    }
    return 1;
}
```

## Troubleshooting

### Common Issues

1. **Device Not Responding**
   ```
   Problem: Device ID read fails
   Solutions:
   - Check SPI connections (MOSI, MISO, SCLK, CS)
   - Verify VSUP power supply (8V - 40V)
   - Check SPI clock frequency (≤ 10 MHz)
   - Verify SPI mode (Mode 0: CPOL=0, CPHA=0)
   ```

2. **Outputs Not Switching**
   ```
   Problem: set_channel() doesn't activate output
   Solutions:
   - Ensure driver.init() was successful
   - Check CTRL1 ENABLE bit is set
   - Verify load is connected
   - Check for fault conditions
   - Verify VSUP voltage is adequate
   ```

3. **Constant Faults**
   ```
   Problem: Channels always show faults
   Solutions:
   - Check load impedance (not too high/low)
   - Verify current limit setting is appropriate
   - Check for actual short circuits
   - Ensure proper thermal management
   ```

## Next Steps

- Read [API Reference](API_Reference.md) for detailed function documentation
- See [Examples](Examples.md) for more usage patterns
- Consult [Diagnostics Guide](Diagnostics_Guide.md) for fault handling
- Review [Hardware Integration](Hardware_Integration.md) for PCB design

---

**Document Version**: 1.0.0  
**Last Updated**: 2025-10-18
