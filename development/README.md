# HF-TLE92466ED Driver
**C++23 driver for Infineon TLE92466ED Six-Channel Low-Side Solenoid Driver IC**

[![License: Public Domain](https://img.shields.io/badge/license-Public%20Domain-green.svg)](LICENSE)

## 📚 Table of Contents
1. [Overview](#-overview)
2. [Features](#-features)
3. [Quick Start](#-quick-start)
4. [Installation](#-installation)
5. [API Reference](#-api-reference)
6. [Examples](#-examples)
7. [Documentation](#-documentation)
8. [Contributing](#-contributing)
9. [License](#-license)

## 📦 Overview

> **📖 [📚🌐 Live Complete Documentation](https://n3b3x.github.io/hf-tle92466ed-driver/)** - 
> Interactive guides, examples, and step-by-step tutorials

**HF-TLE92466ED** is a modern C++23 driver for the **Infineon TLE92466ED** Six-Channel Low-Side Solenoid Driver IC. The TLE92466ED provides six independent low-side outputs for controlling solenoids, valves, and other inductive loads with precision current regulation. Each channel supports up to 2A in single mode or 4A in parallel mode, with 15-bit resolution (0.061mA steps) for precise current control.

The driver uses a hardware-agnostic communication interface design, allowing it to run on any platform (ESP32, STM32, Arduino, etc.) with zero runtime overhead. It implements all major features from the TLE92466ED datasheet including Integrated Current Control (ICC), PWM frequency control, dither support, parallel channel operation, comprehensive diagnostics, and protection features.

## ✨ Features

- ✅ **Six Independent Channels**: Low-side outputs for solenoid/inductive load control
- ✅ **Precision Current Control**: 15-bit resolution (0.061mA steps), 0-2A single channel, 0-4A parallel mode
- ✅ **Integrated Current Controller (ICC)**: Automatic current regulation with configurable PWM frequency
- ✅ **Parallel Operation**: Channel pairs (0/3, 1/2, 4/5) for doubled current capability
- ✅ **Dither Support**: Configurable amplitude and frequency for precise current shaping
- ✅ **Comprehensive Diagnostics**: Overcurrent, overtemperature, open load, short-to-ground detection
- ✅ **Supply Monitoring**: VBAT, VIO, and VDD voltage monitoring with configurable thresholds
- ✅ **Hardware Agnostic**: SPI interface for platform independence
- ✅ **Modern C++23**: Using `std::expected` for type-safe error handling without exceptions
- ✅ **Zero Overhead**: All functions `noexcept`, freestanding-compatible

## 🚀 Quick Start

```cpp
#include "inc/tle92466ed.hpp"
#include "inc/tle92466ed_spi_interface.hpp"

// 1. Implement the SPI interface (see platform_integration.md)
class MySpi : public tle92466ed::SpiInterface<MySpi> {
    // ... implement required methods
};

// 2. Create driver instance
MySpi spi;
tle92466ed::Driver driver(spi);

// 3. Initialize and use
if (auto result = driver.Init(); !result) {
    // Handle error
    return;
}

driver.EnterMissionMode();
driver.SetChannelMode(tle92466ed::Channel::CH0, tle92466ed::ChannelMode::ICC);
driver.SetCurrentSetpoint(tle92466ed::Channel::CH0, 1500); // 1.5A
driver.EnableChannel(tle92466ed::Channel::CH0, true);
```

For detailed setup, see [Installation](docs/installation.md) and [Quick Start Guide](docs/quickstart.md).

## 🔧 Installation

1. **Clone or copy** the driver files into your project
2. **Implement the SPI interface** for your platform (see [Platform Integration](docs/platform_integration.md))
3. **Include the header** in your code:
   ```cpp
   #include "inc/tle92466ed.hpp"
   ```
4. Compile with a **C++23** or newer compiler with `std::expected` support

For detailed installation instructions, see [docs/installation.md](docs/installation.md).

## 📖 API Reference

| Method | Description |
|--------|-------------|
| `Init()` | Initialize the driver and hardware |
| `EnterMissionMode()` | Enter mission mode (enables channel control) |
| `SetChannelMode()` | Set channel operation mode (ICC, Direct Drive, etc.) |
| `SetCurrentSetpoint()` | Set current setpoint for a channel |
| `EnableChannel()` | Enable or disable a channel |
| `GetChannelDiagnostics()` | Get channel diagnostic information |
| `GetAllFaults()` | Get comprehensive fault report |

For complete API documentation, see [docs/api_reference.md](docs/api_reference.md).

## 📊 Examples

For ESP32 examples, see the [examples/esp32](examples/esp32/) directory.

Detailed example walkthroughs are available in [docs/examples.md](docs/examples.md).

## 📚 Documentation

For complete documentation, see the [docs directory](docs/index.md).

## 🤝 Contributing

Pull requests and suggestions are welcome! Please follow the existing code style and include tests for new features.

## 📄 License

This project is released into the **Public Domain**. You can use, modify, and distribute it freely without any restrictions.
