---
layout: default
title: "📚 Documentation"
description: "Complete documentation for the HardFOC TLE92466ED Driver - IC specifications, API reference, and usage examples"
nav_order: 2
parent: "🔧 HardFOC TLE92466ED Driver"
permalink: /docs/
has_children: true
---

# TLE92466ED Driver Documentation

## Quick Navigation

This documentation provides complete information about the TLE92466ED IC and its C++ driver implementation.

### Documentation Structure

```text
docs/
├── 00_INDEX.md                    ← You are here
├── 01_IC_Overview.md              Device specifications and architecture
├── 02_SPI_Protocol.md             32-bit SPI communication with CRC
├── 03_Register_Map.md             Complete register reference
├── 04_Current_Control.md          ICC (Integrated Current Control) system
├── 05_Channel_Modes.md            Operation modes and configuration
├── 06_Diagnostics.md              Fault detection and monitoring
├── 07_Driver_API.md               C++ driver interface reference
├── 08_HAL_Implementation.md       Hardware abstraction guide
└── 09_Usage_Examples.md           Practical implementation examples
```

### Document Purpose

| Document | Content | Use Case |
|----------|---------|----------|
| **IC_Overview** | Device capabilities, pin functions, electrical specs | Understanding hardware |
| **SPI_Protocol** | 32-bit frame structure, CRC calculation | Low-level communication |
| **Register_Map** | All 108 registers with bit fields | Register programming |
| **Current_Control** | ICC configuration, PWM, dither | Precision current control |
| **Channel_Modes** | Config/Mission modes, parallel operation | System configuration |
| **Diagnostics** | Fault flags, protection features | System monitoring |
| **Driver_API** | C++ class methods and types | Software integration |
| **HAL_Implementation** | Platform-specific SPI interface | Hardware porting |
| **Usage_Examples** | Complete working code | Quick start |

### Quick Start Guide

For immediate implementation:

1. **Hardware Setup**: Review `01_IC_Overview.md` for pin connections
2. **HAL Creation**: Follow `08_HAL_Implementation.md` to create your platform HAL
3. **Basic Usage**: Copy code from `09_Usage_Examples.md`
4. **Advanced Features**: Consult specific feature documentation as needed

### Key Specifications

- **Device**: TLE92466ED Six-Channel Low-Side Solenoid Driver
- **Channels**: 6 independent outputs
- **Current Control**: 0-2A (single), 0-4A (parallel)
- **Resolution**: 15-bit (0.061mA steps)
- **SPI**: 32-bit frames with CRC-8
- **C++ Standard**: C++23
- **Driver Version**: 2.0.0

### Reference Information

- **Datasheet**: Infineon TLE92466ED Rev. 1.2 (2022-02-01)
- **Driver Source**: `include/TLE92466ED.hpp`, `src/TLE92466ED.cpp`
- **Examples**: `examples/basic_usage.cpp`
- **Build System**: CMake 3.15+

### Support & Resources

- **Compiler Requirements**: GCC 12.0+, Clang 16.0+, MSVC 19.33+ (C++23)
- **Dependencies**: None (freestanding-compatible)
- **License**: Public Domain

---

**Navigation**: [Next: IC Overview →](01_IC_Overview.md)
