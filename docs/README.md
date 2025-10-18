# TLE92466ED Driver Documentation

## Overview

This documentation provides comprehensive information about the TLE92466ED Six-Channel High-Side Switch IC and its C++20/21 driver implementation.

## Contents

1. [IC Overview](IC_Overview.md) - Hardware specifications and features
2. [Driver Architecture](Driver_Architecture.md) - Software design and structure
3. [Getting Started](Getting_Started.md) - Quick start guide
4. [API Reference](API_Reference.md) - Complete API documentation
5. [Register Map](Register_Map.md) - Detailed register descriptions
6. [Diagnostics Guide](Diagnostics_Guide.md) - Fault detection and handling
7. [Examples](Examples.md) - Usage examples and patterns
8. [Hardware Integration](Hardware_Integration.md) - Platform-specific guides

## Quick Links

- [Basic Usage Example](../examples/basic_usage.cpp)
- [HAL Implementation Guide](HAL_Implementation.md)
- [Troubleshooting](Troubleshooting.md)

## Features at a Glance

### IC Features
- ✅ 6 independent high-side switches
- ✅ Up to 4A current per channel (configurable)
- ✅ SPI control interface (up to 10 MHz)
- ✅ Comprehensive diagnostics
- ✅ PWM capability (100 Hz - 20 kHz)
- ✅ Multiple protection features
- ✅ Automotive-grade (AEC-Q100)

### Driver Features
- ✅ Hardware-agnostic design
- ✅ Modern C++20/21 implementation
- ✅ Type-safe API with std::expected
- ✅ Complete Doxygen documentation
- ✅ RAII resource management
- ✅ Compile-time safety with concepts
- ✅ Zero-cost abstractions

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer                       │
│  (Your Code - Control Logic, State Machines, etc.)         │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│                   TLE92466ED::Driver                        │
│  ┌────────────────────────────────────────────────────┐    │
│  │ • init(), configure_global()                       │    │
│  │ • set_channel(), get_channel_status()              │    │
│  │ • get_diagnostics(), clear_faults()                │    │
│  │ • Software API with error handling                 │    │
│  └────────────────────────────────────────────────────┘    │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│               Hardware Abstraction Layer (HAL)              │
│  ┌────────────────────────────────────────────────────┐    │
│  │ • transfer(), chip_select()                        │    │
│  │ • delay(), configure()                             │    │
│  │ • Platform-independent interface                   │    │
│  └────────────────────────────────────────────────────┘    │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│              Platform-Specific Implementation               │
│     (STM32 HAL, ESP32 ESP-IDF, Linux spidev, etc.)         │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│                    Physical Hardware                        │
│                   SPI Bus + TLE92466ED IC                   │
└─────────────────────────────────────────────────────────────┘
```

## IC Pin Configuration

```
                            TLE92466ED
                         ┌──────────────┐
               VSUP ─────┤ 1        24 │───── GND
                GND ─────┤ 2        23 │───── OUT0
              OUT5 ─────┤ 3        22 │───── OUT1
              OUT4 ─────┤ 4        21 │───── OUT2
              OUT3 ─────┤ 5        20 │───── VSUP
               GND ─────┤ 6        19 │───── GND
                RST ─────┤ 7        18 │───── CS (Chip Select)
                INT ─────┤ 8        17 │───── SCLK (SPI Clock)
           RESERVED ─────┤ 9        16 │───── SDI (MOSI)
           RESERVED ─────┤10        15 │───── SDO (MISO)
               VSUP ─────┤11        14 │───── GND
                GND ─────┤12        13 │───── VSUP
                         └──────────────┘

    VSUP: Supply Voltage (8V - 40V typical)
    GND:  Ground
    OUT0-OUT5: Output Channels (High-Side Switches)
    CS:   Chip Select (Active Low)
    SCLK: SPI Clock (up to 10 MHz)
    SDI:  SPI Data In (MOSI)
    SDO:  SPI Data Out (MISO)
    RST:  Reset Input (Active Low)
    INT:  Interrupt Output (Active Low)
```

## SPI Communication Format

```
┌───────┬─────────────┬───────────────────┐
│ Bit   │ Bit(s)      │ Description       │
├───────┼─────────────┼───────────────────┤
│  15   │      1      │ R/W (1=Read)      │
│ 14-8  │      7      │ Register Address  │
│  7-0  │      8      │ Data Byte         │
└───────┴─────────────┴───────────────────┘

Example Write Transaction:
┌────────┬──────────────┬────────────┐
│  R/W   │   Address    │    Data    │
├────────┼──────────────┼────────────┤
│   0    │  0x03 (OUT)  │  0x3F (on) │
└────────┴──────────────┴────────────┘
 = 0x033F (all channels on)

Example Read Transaction:
┌────────┬──────────────┬────────────┐
│  R/W   │   Address    │    Data    │
├────────┼──────────────┼────────────┤
│   1    │ 0x10 (STAT)  │  0x00 (N/A)│
└────────┴──────────────┴────────────┘
 = 0x9000 (read status)
```

## Timing Diagram

```
CS   ────┐                                      ┌────
         │                                      │
         └──────────────────────────────────────┘

SCLK ────────┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌────
             └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘

SDI  ───< R/W >< A14-8 ><    Data D7-D0     >───

SDO  ─────────< Response Data from Previous >───
              <        Transaction          >

     ├────────────────────────────────────────┤
              16-bit SPI Transfer
```

## Protection Features

```
┌─────────────────────────────────────────────────────────┐
│                  TLE92466ED Protections                 │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  ┌────────────────┐      ┌───────────────────┐         │
│  │ Over-Current   │──────│ 1A / 2A / 3A / 4A │         │
│  │ Protection     │      │ (Configurable)     │         │
│  └────────────────┘      └───────────────────┘         │
│                                                          │
│  ┌────────────────┐      ┌───────────────────┐         │
│  │ Over-Temp      │──────│ Warning: ~150°C   │         │
│  │ Protection     │      │ Shutdown: ~175°C  │         │
│  └────────────────┘      └───────────────────┘         │
│                                                          │
│  ┌────────────────┐      ┌───────────────────┐         │
│  │ Under-Voltage  │──────│ Threshold: ~7V    │         │
│  │ Protection     │      │ (Configurable)     │         │
│  └────────────────┘      └───────────────────┘         │
│                                                          │
│  ┌────────────────┐      ┌───────────────────┐         │
│  │ Over-Voltage   │──────│ Threshold: ~45V   │         │
│  │ Protection     │      │ (Configurable)     │         │
│  └────────────────┘      └───────────────────┘         │
│                                                          │
│  ┌────────────────┐      ┌───────────────────┐         │
│  │ Short Circuit  │──────│ To GND / To VBAT  │         │
│  │ Detection      │      │ Fast shutdown      │         │
│  └────────────────┘      └───────────────────┘         │
│                                                          │
│  ┌────────────────┐      ┌───────────────────┐         │
│  │ Open Load      │──────│ Detection ON/OFF  │         │
│  │ Detection      │      │ State             │         │
│  └────────────────┘      └───────────────────┘         │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

## Typical Application

```
┌──────────────┐
│ Microcontroller │
│   (STM32)    │
└─────┬────────┘
      │ SPI
      │ (CS, SCLK, MOSI, MISO)
      │
┌─────▼────────┐       ┌──────────┐
│  TLE92466ED  │──OUT0─│ LED Bank │
│              │──OUT1─│  Relay   │
│              │──OUT2─│  Solenoid│
│              │──OUT3─│  Heater  │
│              │──OUT4─│  Motor   │
│              │──OUT5─│  Lamp    │
└──────────────┘       └──────────┘
     │                      │
     └──────VSUP────────────┘
     │                      │
     └──────GND─────────────┘
```

## Compiler Requirements

- **C++ Standard**: C++20 or later
- **Required Features**:
  - Concepts (`template<typename T> concept`)
  - `std::expected` (C++23 or backport)
  - `std::span`
  - `std::chrono`
  - Range-based algorithms

### Tested Compilers
- GCC 11.0+
- Clang 13.0+
- MSVC 19.30+
- ARM GCC 11.0+ (embedded)

## License

This driver is free and unencumbered software released into the public domain.

## Support

For issues, questions, or contributions, please refer to the project repository.

---

**Document Version**: 1.0.0  
**Last Updated**: 2025-10-18  
**Driver Version**: 1.0.0
