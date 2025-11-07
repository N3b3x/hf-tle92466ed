---
layout: default
title: Home
nav_order: 1
description: "C++23 driver for Infineon TLE92466ED Six-Channel Low-Side Solenoid Driver IC"
permalink: /
---

# TLE92466ED Driver
{: .fs-9 }

Modern C++23 driver for Infineon TLE92466ED Six-Channel Low-Side Solenoid Driver IC
{: .fs-6 .fw-300 }

[Get Started](docs/00_INDEX.md){: .btn .btn-primary .fs-5 .mb-4 .mb-md-0 .mr-2 }
[View on GitHub](https://github.com/n3b3x/hf-tle92466ed-driver){: .btn .fs-5 .mb-4 .mb-md-0 }

---

## Overview

The TLE92466ED driver is a production-ready C++23 implementation for controlling the
Infineon TLE92466ED IC, a sophisticated six-channel low-side solenoid driver with
precision current control.

### Key Features

- **6 Independent Channels** - Low-side solenoid/load control
- **15-bit Current Control** - 0.061mA resolution, 0-2A per channel (0-4A parallel)
- **32-bit SPI with CRC-8** - Robust communication with data integrity
- **Modern C++23** - Using `std::expected` for error handling
- **Hardware Agnostic** - Clean HAL interface for platform portability
- **Zero Dependencies** - Freestanding-compatible for embedded systems
- **Comprehensive Diagnostics** - Overcurrent, temperature, load detection

## Quick Start

### 1. Include the Driver

```cpp
#include "TLE92466ED.hpp"
#include "your_platform_hal.hpp"
```text

### 2. Create and Initialize

```cpp
YourPlatformHAL hal;
TLE92466ED driver(hal);

if (auto result = driver.initialize(); !result) {
    // Handle initialization error
}
```text

### 3. Control Channels

```cpp
// Set current to 1.5A
driver.setChannelCurrent(0, 1500);

// Enable channel
driver.enableChannel(0);

// Read diagnostics
if (auto diag = driver.readDiagnostics(); diag) {
    if (diag->hasOvercurrent()) {
        // Handle overcurrent
    }
}
```text

## Documentation

<div class="code-example" markdown="1">

### 📖 Complete Guides

- [**Documentation Index**](docs/00_INDEX.md) - Navigation hub
- [**IC Overview**](docs/01_IC_Overview.md) - Hardware specifications
- [**SPI Protocol**](docs/02_SPI_Protocol.md) - Communication details
- [**Register Map**](docs/03_Register_Map.md) - All 108 registers
- [**Current Control**](docs/04_Current_Control.md) - ICC system
- [**Channel Modes**](docs/05_Channel_Modes.md) - Operating modes
- [**Diagnostics**](docs/06_Diagnostics.md) - Fault detection
- [**Driver API**](docs/07_Driver_API.md) - C++ interface
- [**HAL Implementation**](docs/08_HAL_Implementation.md) - Platform porting
- [**Usage Examples**](docs/09_Usage_Examples.md) - Code samples

### 🔧 API Reference

- [**Doxygen Documentation**](doxygen/html/) - Complete API reference

</div>

## Hardware Specifications

| Feature | Specification |
|---------|--------------|
| **Channels** | 6 independent outputs |
| **Current Range** | 0-2A (single), 0-4A (parallel) |
| **Resolution** | 15-bit (0.061mA) |
| **SPI Interface** | 32-bit frames with CRC-8 |
| **PWM Support** | Configurable frequency |
| **Dither** | Up to 1.8A amplitude |
| **Protection** | Overcurrent, overtemp, load detection |

## Requirements

### Compiler Support
- GCC 12.0+ or Clang 16.0+ or MSVC 19.33+
- C++23 standard library with `std::expected`

### Dependencies
- None - freestanding compatible

## Installation

### CMake

```cmake
add_subdirectory(external/hf-tle92466ed-driver)
target_link_libraries(your_target PRIVATE TLE92466ED)
```text

### Manual

Include the following files in your project:
- `inc/TLE92466ED.hpp`
- `inc/TLE92466ED_Registers.hpp`
- `inc/TLE92466ED_CommInterface.hpp`
- `src/TLE92466ED.cpp`

## Platform Integration

Implement the HAL interface for your platform:

```cpp
class YourPlatformHAL : public TLE92466ED::HAL {
public:
    auto spiTransfer(std::span<const uint8_t> txData, 
                     std::span<uint8_t> rxData) noexcept 
        -> std::expected<void, HALError> override {
        // Your SPI implementation
        return {};
    }
    
    void delayMicroseconds(uint32_t us) noexcept override {
        // Your delay implementation
    }
};
```text

## Use Cases

- 🚗 Automotive solenoid control
- 🏭 Industrial valve control
- 🤖 Robotics actuator control
- ⚡ Precision motor control
- 🔬 Test and measurement equipment

## Contributing

Contributions are welcome! Please feel free to submit issues, feature requests, or pull
requests on [GitHub](https://github.com/n3b3x/hf-tle92466ed-driver).

## License

This software is released into the **Public Domain**. You can use, modify, and distribute it freely without any restrictions.

## Resources

- [Infineon TLE92466ED Product Page](https://www.infineon.com/)
- [Datasheet](Datasheet/) (included in repository)
- [GitHub Repository](https://github.com/n3b3x/hf-tle92466ed-driver)
- [Issue Tracker](https://github.com/n3b3x/hf-tle92466ed-driver/issues)

---

**Version**: 2.0.0 | **Status**: Production Ready | **Last Updated**: 2025-10-21
