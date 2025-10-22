# TLE92466ED ESP-IDF Component

This directory contains the ESP-IDF component configuration for the TLE92466ED driver.

## Overview

This component wraps the TLE92466ED driver for use as an ESP-IDF managed component,
making it easy to integrate into ESP32 projects.

## Component Structure

```text
tle92466ed-espidf/
├── CMakeLists.txt          # Component build configuration
├── idf_component.yml       # ESP-IDF Component Manager manifest
└── README.md              # This file
```text

## How It Works

### Component Resolution

The component automatically locates the TLE92466ED driver source files:

1. **CI/Build Environment**: Looks for `src/` and `include/` in the project root
2. **Development Environment**: Looks for `../../src/` and `../../include/` (driver root)

### Source Files Included

- `src/TLE92466ED.cpp` - Main driver implementation

### Headers Included

- `include/TLE92466ED.hpp` - Driver class
- `include/TLE92466ED_Registers.hpp` - Register definitions
- `include/TLE92466ED_HAL.hpp` - Hardware abstraction layer interface

## Usage in Projects

### In ESP-IDF Component Manager

This component can be used with the ESP-IDF Component Manager by adding it to your project's dependencies.

### In Local Projects

The component is automatically discovered by ESP-IDF when located in the `components/`
directory of your project.

### Requirements

Add to your main component:

```cmake
idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES tle92466ed-espidf  # Add this requirement
)
```text

## Configuration

The component provides compile-time definitions based on the target:

- `TLE92466ED_TARGET_ESP32` - ESP32 classic target
- `TLE92466ED_TARGET_ESP32C6` - ESP32-C6 target
- `TLE92466ED_TARGET_ESP32S3` - ESP32-S3 target
- `TLE92466ED_TARGET_ESP32C3` - ESP32-C3 target
- `TLE92466ED_MCU_FAMILY_ESP32` - ESP32 family marker

## Build Features

### C++23 Standard

The component is configured to use C++23 standard:

```cmake
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_23)
```text

### Compiler Optimizations

- **Link Time Optimization (LTO)**: Enabled when configured
- **Function/Data Sections**: For better code size optimization
- **No RTTI**: `-fno-rtti` for smaller binary size
- **No Exceptions**: `-fno-exceptions` for embedded systems

### Compiler Flags

```text
-Wall -Wextra -Wpedantic
-O2 -g
-ffunction-sections -fdata-sections
-fno-rtti -fno-exceptions
```text

## ESP-IDF Dependencies

The component requires these ESP-IDF components:

### Core Components
- `driver` - SPI drivers
- `esp_timer` - Timing functions
- `freertos` - RTOS support
- `esp_system` - System utilities
- `hal` - Hardware abstraction layer
- `soc` - SoC definitions
- `log` - Logging

### Driver Components
- `esp_driver_gpio` - GPIO driver
- `esp_driver_spi` - SPI driver

## Supported ESP32 Targets

- ESP32 (classic)
- ESP32-S2
- ESP32-S3
- ESP32-C2
- ESP32-C3
- ESP32-C6 (primary target)
- ESP32-H2

## Development

### Testing Component Configuration

```bash
cd examples/esp32
idf.py reconfigure
```text

### Viewing Component Information

```bash
idf.py show-components
```text

### Debugging Component Issues

Enable verbose output:

```bash
idf.py -v build
```text

## Component Manager

### Publishing to Component Registry

To publish this component to the ESP-IDF Component Registry:

1. Update version in `idf_component.yml`
2. Create git tag matching version
3. Follow ESP-IDF Component Manager publishing guidelines

### Local Installation

The component is automatically available when building the examples from the `examples/esp32` directory.

## Build Messages

During build, the component will display:

```text
TLE92466ED Driver - ESP32 Component Configuration
Project Root: /path/to/project
Source Root: /path/to/driver
Found source: /path/to/TLE92466ED.cpp
=== TLE92466ED Driver Component Summary ===
Component configured successfully
Included 1 source files
Target MCU: ESP32-C6
C++ Standard: C++23
Driver Version: 2.0.0
Optimization: LTO enabled (if configured)
==========================================
```text

## Troubleshooting

### Source Files Not Found

**Issue**: "Source file not found" warnings

**Solution**: Verify the driver is in the correct location:
- Development: `../../src/TLE92466ED.cpp` relative to component
- CI: `${PROJECT_ROOT}/src/TLE92466ED.cpp`

### C++23 Not Supported

**Issue**: Compiler doesn't support C++23

**Solution**: Update to:
- GCC 12.0 or later
- Clang 16.0 or later
- ESP-IDF v5.5 or later

### Component Not Found

**Issue**: "Component 'tle92466ed-espidf' not found"

**Solution**: Ensure the component is in the `components/` directory of your ESP-IDF project.

## References

- [ESP-IDF Component Manager Documentation](https://docs.espressif.com/projects/idf-component-manager/)
- [ESP-IDF CMake Build System](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html)
- [TLE92466ED Driver Documentation](../../docs/)

---

**Component Version**: 2.0.0  
**ESP-IDF Version**: ≥5.0.0  
**C++ Standard**: C++23
