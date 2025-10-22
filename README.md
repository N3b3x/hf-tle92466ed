# TLE92466ED Driver

[![Documentation](https://img.shields.io/badge/docs-latest-blue.svg)](https://n3b3x.github.io/hf-tle92466ed-driver/)
[![License](https://img.shields.io/badge/license-Public%20Domain-green.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)

> **C++23 driver for Infineon TLE92466ED Six-Channel Low-Side Solenoid Driver IC**

A modern, production-ready C++23 driver implementation for the Infineon TLE92466ED with
comprehensive SPI interface, precision current control, and extensive diagnostics support.

## 🌟 Key Features

### Hardware Capabilities
- **6 Independent Channels**: Low-side solenoid/load control
- **Precision Current Control**: 15-bit resolution (0.061mA steps)
- **Current Range**: 0-2A per channel, 0-4A in parallel mode
- **32-bit SPI Interface**: With CRC-8 (SAE J1850) data integrity
- **PWM & Dither**: Configurable for noise reduction and precision
- **Comprehensive Diagnostics**: Overcurrent, overtemperature, load detection

### Driver Features
- **Modern C++23**: Using `std::expected` for error handling
- **Hardware Agnostic**: Clean HAL interface for easy platform porting
- **Zero Dependencies**: Freestanding-compatible, suitable for bare-metal
- **noexcept Guarantee**: All functions marked noexcept for embedded safety
- **Production Ready**: Thoroughly documented and tested

## 📚 Documentation

- **[Complete Documentation](https://n3b3x.github.io/hf-tle92466ed-driver/)** - Jekyll
  site with full guides
- **[API Reference](https://n3b3x.github.io/hf-tle92466ed-driver/doxygen/html/)** -
  Doxygen-generated API docs
- **[IC Overview](docs/01_IC_Overview.md)** - Hardware specifications
- **[Usage Examples](docs/09_Usage_Examples.md)** - Practical code examples
- **[HAL Implementation](docs/08_HAL_Implementation.md)** - Platform porting guide

## 🚀 Quick Start

### Basic Usage

```cpp
#include "TLE92466ED.hpp"
#include "your_platform_hal.hpp"  // Your platform-specific HAL

// Create HAL instance
YourPlatformHAL hal;

// Create driver instance
TLE92466ED driver(hal);

// Initialize the IC
if (auto result = driver.initialize(); !result) {
    // Handle initialization error
    return;
}

// Set current for channel 0 to 1.5A
if (auto result = driver.setChannelCurrent(0, 1500); !result) {
    // Handle error
    return;
}

// Enable channel 0
if (auto result = driver.enableChannel(0); !result) {
    // Handle error
    return;
}

// Read diagnostics
if (auto diag = driver.readDiagnostics(); diag) {
    // Process diagnostics
    if (diag->hasOvercurrent()) {
        // Handle overcurrent condition
    }
}
```text

### HAL Implementation

Implement the HAL interface for your platform:

```cpp
class YourPlatformHAL : public TLE92466ED_HAL {
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
```text

## 📋 Requirements

### Compiler Support
- **GCC** 12.0 or later
- **Clang** 16.0 or later  
- **MSVC** 19.33 (Visual Studio 2022) or later

### C++ Standard
- C++23 (requires `std::expected` support)

### Dependencies
- **None** - Freestanding compatible
- Only requires standard C++23 library features

## 🏗️ Building

### CMake Integration

```cmake
# Add to your CMakeLists.txt
add_subdirectory(external/hf-tle92466ed-driver)

target_link_libraries(your_target PRIVATE TLE92466ED)
```text

### Manual Integration

Simply include the header and source files in your project:
- `include/TLE92466ED.hpp`
- `include/TLE92466ED_Registers.hpp`
- `include/TLE92466ED_HAL.hpp`
- `src/TLE92466ED.cpp`

## 🎯 Use Cases

- Automotive solenoid control
- Industrial valve control
- Robotics actuator control
- Precision motor control
- Test and measurement equipment
- Any application requiring precision current control

## 📖 Documentation Structure

```text
docs/
├── 00_INDEX.md              - Documentation navigation
├── 01_IC_Overview.md        - Hardware specifications
├── 02_SPI_Protocol.md       - Communication protocol
├── 03_Register_Map.md       - Complete register reference
├── 04_Current_Control.md    - ICC system documentation
├── 05_Channel_Modes.md      - Operating modes
├── 06_Diagnostics.md        - Fault detection
├── 07_Driver_API.md         - C++ API reference
├── 08_HAL_Implementation.md - Platform porting guide
└── 09_Usage_Examples.md     - Practical examples
```text

## 🔧 IC Specifications

| Specification | Value |
|--------------|-------|
| **Channels** | 6 independent outputs |
| **Current per Channel** | 0-2A (single), 0-4A (parallel) |
| **Current Resolution** | 15-bit (0.061mA) |
| **SPI Interface** | 32-bit frames, CRC-8 |
| **Supply Voltage** | See datasheet |
| **Temperature Range** | See datasheet |
| **Package** | See datasheet |

## 🤝 Contributing

Contributions are welcome! Please feel free to submit issues, feature requests, or pull requests.

## 📄 License

This software is released into the **Public Domain**. You can use, modify, and distribute it freely without any restrictions.

## 🙏 Acknowledgments

- Based on Infineon TLE92466ED datasheet Rev. 1.2 (2022-02-01)
- Documentation extracted and verified from official datasheet
- All register definitions verified against hardware

## 🔗 Resources

- **[Official Infineon TLE92466ED Product Page](https://www.infineon.com/)**
- **[Datasheet](Datasheet/)** - Included in repository
- **[GitHub Repository](https://github.com/n3b3x/hf-tle92466ed-driver)**
- **[Issues & Support](https://github.com/n3b3x/hf-tle92466ed-driver/issues)**

## 📞 Support

- **Documentation**: [https://n3b3x.github.io/hf-tle92466ed-driver/](https://n3b3x.github.io/hf-tle92466ed-driver/)
- **Issues**: [GitHub Issues](https://github.com/n3b3x/hf-tle92466ed-driver/issues)
- **Discussions**: [GitHub Discussions](https://github.com/n3b3x/hf-tle92466ed-driver/discussions)

---

**Version**: 2.0.0 | **Status**: Production Ready | **Last Updated**: 2025-10-21
