---
layout: default
title: Documentation Home
nav_order: 1
description: "Complete technical documentation for the TLE92466ED Six-Channel Low-Side Solenoid Driver IC"
permalink: /
---

# TLE92466ED Driver Documentation

Complete technical documentation for the TLE92466ED Six-Channel Low-Side Solenoid Driver
IC and its C++23 driver implementation.

## Quick Links

- **[Documentation Index](00_INDEX.md)** - Start here for navigation
- **[IC Overview](01_IC_Overview.md)** - Device specifications and architecture
- **[SPI Protocol](02_SPI_Protocol.md)** - 32-bit communication protocol
- **[Register Map](03_Register_Map.md)** - Complete register reference
- **[Current Control](04_Current_Control.md)** - ICC system documentation

## What's Inside

### Hardware Documentation
Comprehensive IC documentation extracted from the official Infineon datasheet:
- Pin functions and electrical specifications
- Operating modes (Config/Mission)
- Protection features and diagnostics
- Current control (ICC) with 15-bit resolution
- Dither support for precision control

### Driver Documentation  
Complete C++23 driver implementation guide:
- Hardware abstraction layer (HAL) interface
- Register access with CRC verification
- High-level API for current control
- Error handling and diagnostics
- Platform integration examples

## Key Features

**IC Capabilities**:
- 6 independent low-side outputs
- 0-2A (single) / 0-4A (parallel) current range
- 0.061mA resolution (15-bit)
- Configurable PWM and dither
- Comprehensive diagnostics

**Driver Features**:
- C++23 with std::expected
- Hardware-agnostic via HAL
- 32-bit SPI with CRC-8 (SAE J1850)
- All functions noexcept
- Production-ready embedded code

## Documentation Quality

✅ Extracted from official datasheet (Rev. 1.2)  
✅ All 108 registers documented  
✅ Verified bit field definitions  
✅ ASCII art diagrams throughout  
✅ Practical examples included  

---

**Version**: 2.0.0 | **Date**: 2025-10-20 | **Status**: Complete
