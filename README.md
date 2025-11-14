---
layout: default
title: "🔧 HardFOC TLE92466ED Driver"
description: "C++23 driver for Infineon TLE92466ED Six-Channel Low-Side Solenoid Driver IC with comprehensive SPI interface, current control, and diagnostics"
nav_order: 1
permalink: /
has_children: true
---

# 🔧 HardFOC TLE92466ED Driver
**Six-Channel Low-Side Solenoid Driver**

![TLE92466ED](https://img.shields.io/badge/TLE92466ED-Solenoid%20Driver-blue?style=for-the-badge&logo=microchip)
![C++23](https://img.shields.io/badge/C%2B%2B-23-blue?style=for-the-badge&logo=cplusplus)
![Hardware-Agnostic](https://img.shields.io/badge/Hardware--Agnostic-Universal-green?style=for-the-badge&logo=hardware)
![License](https://img.shields.io/badge/license-Public%20Domain-green?style=for-the-badge&logo=opensourceinitiative)

## 🎯 Modern Solenoid Control Interface for Multi-MCU Development

*A production-ready C++23 driver implementation for the Infineon TLE92466ED with
comprehensive SPI interface, precision current control, and extensive diagnostics
support*

---

## 📚 **Table of Contents**

- [🎯 **Overview**](#-overview)
- [🏗️ **Architecture**](#-architecture)
- [🔌 **Features**](#-features)
- [🚀 **Quick Start**](#-quick-start)
- [📖 **API Documentation**](#-api-documentation)
- [🔧 **Building**](#-building)
- [📊 **Examples**](#-examples)
- [🤝 **Contributing**](#-contributing)
- [📄 **License**](#-license)

---

## 🎯 **Overview**

> **📖 [📚🌐 Live Complete Documentation](https://n3b3x.github.io/hf-tle92466ed-driver/)** -
> Interactive guides, examples, and step-by-step tutorials

**TLE92466ED Driver** is a modern, production-ready C++23 driver implementation for
the Infineon TLE92466ED Six-Channel Low-Side Solenoid Driver IC. The driver
provides comprehensive SPI interface, precision current control, and extensive
diagnostics support with a clean hardware-agnostic HAL interface.

### 🏆 **Core Benefits**

- **🔄 Hardware Portability** - Clean HAL interface for easy platform porting
- **🎯 Modern C++23** - Using `std::expected` for type-safe error handling
- **⚡ Precision Control** - 15-bit current resolution (0.061mA steps)
- **🛡️ Safety Features** - Comprehensive diagnostics and protection systems
- **📈 Zero Dependencies** - Freestanding-compatible, suitable for bare-metal
- **🔌 Complete Coverage** - Full access to all 108 registers and features

### 🎨 **Design Philosophy**

```cpp
// Write hardware-agnostic solenoid control code
YourPlatformHAL hal;  // Your SPI implementation
TLE92466ED driver(hal);

// Initialize and control channels
driver.initialize();
driver.setChannelCurrent(0, 1500);  // 1.5A
driver.enableChannel(0);
```

---

## 🏗️ **Architecture**

### **HAL-Based Design**

```text
📦 TLE92466ED Driver Architecture
├── 🎯 Driver Layer (TLE92466ED class)     # High-level channel control API
│   ├── Channel Control                  # Current setting, enable/disable
│   ├── Diagnostics                      # Fault detection and monitoring
│   ├── Configuration                    # Channel modes, PWM settings
│   └── Register Access                  # Direct register manipulation
│
├── 🔌 Hardware Abstraction Layer        # Platform-independent interface
│   ├── TLE92466ED_CommInterface         # Communication interface
│   ├── SPI Transfer                     # 32-bit frame communication
│   └── Delay Functions                  # Timing control
│
└── 🔧 Hardware Layer                   # Platform-specific implementations
    ├── ESP32 HAL                        # ESP32 family support
    ├── STM32 HAL                        # STM32 support
    └── Any MCU with SPI                 # Your implementation
```

### **Abstraction Benefits**

#### **1. MCU Independence**
```cpp
// Application code remains the same across MCUs
class SolenoidController {
    TLE92466ED* driver;
    
public:
    void Initialize() {
        // Platform-specific HAL, same driver code
        YourPlatformHAL hal;  // ESP32, STM32, or any MCU
        driver = new TLE92466ED(hal);
        
        // Same API regardless of MCU
        driver->initialize();
        driver->setChannelCurrent(0, 1500);
        driver->enableChannel(0);
    }
};
```

---

## 🔌 **Features**

### **Hardware Capabilities**

| **Feature** | **Specification** | **Details** |
|-------------|-------------------|-------------|
| **Channels** | 6 independent outputs | Low-side solenoid/load control |
| **Current Range** | 0-2A per channel | Single channel operation |
| **Current Range** | 0-4A per channel | Parallel mode operation |
| **Resolution** | 15-bit (0.061mA) | Precision current control |
| **SPI Interface** | 32-bit frames | With CRC-8 (SAE J1850) |
| **PWM & Dither** | Configurable | Noise reduction and precision |
| **Diagnostics** | Comprehensive | Overcurrent, overtemperature, load detection |

### **Driver Features**

| **Feature** | **Description** | **Benefits** |
|-------------|-----------------|--------------|
| **Modern C++23** | Using `std::expected` | Type-safe error handling |
| **Hardware Agnostic** | Clean HAL interface | Easy platform porting |
| **Zero Dependencies** | Freestanding-compatible | Suitable for bare-metal |
| **noexcept Guarantee** | All functions noexcept | Embedded safety |
| **Production Ready** | Thoroughly documented | Industry-grade quality |

---

## 🚀 **Quick Start**

### **1. Clone Repository**
```bash
git clone https://github.com/n3b3x/hf-tle92466ed-driver.git
cd hf-tle92466ed-driver
```

### **2. Implement HAL Interface**
```cpp
#include "inc/TLE92466ED_CommInterface.hpp"

class YourPlatformHAL : public TLE92466ED::HAL {
public:
    auto spiTransfer(std::span<const uint8_t> txData, 
                     std::span<uint8_t> rxData) noexcept 
        -> std::expected<void, HALError> override {
        // Your SPI transfer implementation
        return {};
    }
    
    void delayMicroseconds(uint32_t us) noexcept override {
        // Your delay implementation
    }
};
```

### **3. Basic Usage**
```cpp
#include "include/TLE92466ED.hpp"

YourPlatformHAL hal;
TLE92466ED driver(hal);

// Initialize the IC
if (auto result = driver.initialize(); !result) {
    return -1;
}

// Set current for channel 0 to 1.5A
driver.setChannelCurrent(0, 1500);

// Enable channel 0
driver.enableChannel(0);
```

### **4. Build Example**
```bash
cd examples/esp32
./scripts/setup_repo.sh
./scripts/build_app.sh basic_usage Release
```

---

## 📖 **API Documentation**

### **Generated Documentation**
- **[📚 Complete Documentation](https://n3b3x.github.io/hf-tle92466ed-driver/)** -
  Interactive guides and tutorials
- **[API Reference](docs/00_INDEX.md)** - Complete driver API documentation
- **[IC Overview](docs/01_IC_Overview.md)** - Hardware specifications

### **Key Concepts**

#### **Initialization**
```cpp
// Initialize the driver
if (auto result = driver.initialize(); !result) {
    // Handle initialization error
    return;
}
```

#### **Channel Control**
```cpp
// Set current (0-2000 mA)
driver.setChannelCurrent(0, 1500);  // 1.5A

// Enable/disable channels
driver.enableChannel(0);
driver.disableChannel(0);
```

#### **Diagnostics**
```cpp
// Read diagnostics
if (auto diag = driver.readDiagnostics(); diag) {
    if (diag->hasOvercurrent()) {
        // Handle overcurrent condition
    }
}
```

---

## 🔧 **Building**

### **Build System Features**
- **CMake Integration** - Easy project integration
- **ESP32 Examples** - Comprehensive test applications
- **Manual Integration** - Simple header/source inclusion
- **CI/CD Integration** - Automated builds and validation

### **Build Commands**
```bash
## For ESP32 development
cd examples/esp32
./scripts/setup_repo.sh
./scripts/build_app.sh <app_name> <build_type>

## Examples:
./scripts/build_app.sh basic_usage Release
./scripts/build_app.sh multi_channel Debug
```

### **CMake Integration**
```cmake
add_subdirectory(external/hf-tle92466ed-driver)
target_link_libraries(your_target PRIVATE TLE92466ED)
```

---

## 📊 **Examples**

### **Available Test Applications**

| **Application** | **Tests** | **Purpose** |
|-----------------|-----------|-------------|
| **basic_usage** | Single channel control | Basic current setting and enable |
| **multi_channel** | Multi-channel operation | Parallel channel control |
| **diagnostics** | Fault detection | Overcurrent and overtemperature |

### **Usage Examples**
```cpp
// Single Channel Control
driver.setChannelCurrent(0, 1500);  // 1.5A
driver.enableChannel(0);

// Multi-Channel Control
for (uint8_t ch = 0; ch < 6; ch++) {
    driver.setChannelCurrent(ch, 1000);  // 1A per channel
    driver.enableChannel(ch);
}

// Diagnostics Monitoring
if (auto diag = driver.readDiagnostics(); diag) {
    if (diag->hasOvercurrent()) {
        driver.disableChannel(0);
    }
}
```

---

## 🤝 **Contributing**

Contributions are welcome! For detailed contribution guidelines, coding standards, and development workflow,
please visit the **[HardFOC Organization GitHub page](https://github.com/hardfoc)**.

### **Development Workflow**
1. **Fork** the repository
2. **Create** feature branch (`feature/new-feature`)
3. **Implement** following coding standards
4. **Test** with existing applications
5. **Document** your changes
6. **Submit** pull request

### **Coding Standards**
- **Functions**: PascalCase (`SetChannelCurrent`, `ReadDiagnostics`)
- **Types**: snake_case with `*t` suffix (`hal_error_t`)
- **Error Handling**: Use `std::expected` for type safety
- **Code Formatting**: Use `clang-format`

---

## 📄 **License**

This software is released into the **Public Domain**. You can use, modify, and
distribute it freely without any restrictions.

---

## 🔗 **Quick Links**

### **Documentation**
- 📚 [Complete Documentation](https://n3b3x.github.io/hf-tle92466ed-driver/) -
  Interactive guides and tutorials
- 📋 [Documentation Index](docs/00_INDEX.md) - Complete documentation
  navigation
- 🔌 [IC Overview](docs/01_IC_Overview.md) - Hardware specifications
- 📡 [SPI Protocol](docs/02_SPI_Protocol.md) - Communication protocol
- 🎛️ [Driver API](docs/07_Driver_API.md) - C++ API reference
- 🔧 [HAL Implementation](docs/08_HAL_Implementation.md) - Platform porting
  guide
- 💻 [Usage Examples](docs/09_Usage_Examples.md) - Practical code examples

### **Development**
- 🚀 [Examples](examples/esp32/) - Test applications and usage examples
- 🧪 [Test Documentation](examples/esp32/docs/README.md) - Comprehensive test
  documentation
- 🔧 [Scripts](examples/esp32/scripts/) - Build, flash, and development tools
- 📊 [Configuration](examples/esp32/app_config.yml) - Application and build
  settings

### **Community**
- 🤝 [Contributing](#-contributing) - How to contribute
- 🐛 [Issue Tracker](https://github.com/n3b3x/hf-tle92466ed-driver/issues)
- 💬 [Discussions](https://github.com/n3b3x/hf-tle92466ed-driver/discussions)

---

**Built for the HardFOC ecosystem - Enabling seamless solenoid control**

*Precision current control that just works™*
