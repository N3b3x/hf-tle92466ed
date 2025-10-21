---
layout: default
title: IC Overview
nav_order: 3
parent: Documentation Index
description: "TLE92466ED hardware specifications and architecture"
---

# TLE92466ED IC Overview

## Device Description

The TLE92466ED is a sophisticated six-channel low-side solenoid driver IC designed for automotive and industrial applications requiring precise current control of inductive loads.

### Device Classification

```
Device Family: Infineon PROFET™ + (Protected High-Current Switch)
Type: Low-Side Switch with Current Control
Application: Solenoid actuators, valves, relays
```

## Key Features

### Current Control System
- **Integrated Current Controller (ICC)** with 15-bit resolution
- Range: 0-2000 mA (single channel), 0-4000 mA (parallel mode)
- Resolution: 0.061 mA per LSB (single), 0.122 mA (parallel)
- Dither support for precise current shaping (up to 1800 mA amplitude)
- PWM-based regulation with configurable frequency

### Channel Configuration
- **6 independent output channels**
- Parallel operation pairs: CH0/CH3, CH1/CH2, CH4/CH5
- Per-channel configuration and diagnostics
- Individual enable/disable control

### Protection & Diagnostics
- Open load detection (ON and OFF states)
- Short to ground detection
- Over-current protection (configurable threshold)
- Over-temperature monitoring (warning + error)
- Supply voltage monitoring (VBAT, VIO, VDD)
- Load current feedback

### Communication & Safety
- 32-bit SPI with 8-bit CRC (SAE J1850)
- SPI watchdog for communication monitoring
- Clock watchdog for oscillator verification
- Unique 48-bit chip identification

## Device Pinout

```
                           TLE92466ED
                     ┌─────────────────┐
        VBAT (1) ────┤  1          32  ├──── GND (32)
         OUT0 (2) ────┤  2          31  ├──── OUT5 (31)
         OUT1 (3) ────┤  3          30  ├──── OUT4 (30)
         OUT2 (4) ────┤  4          29  ├──── OUT3 (29)
         RESN (5) ────┤  5          28  ├──── CSN (28)
          CSN (6) ────┤  6          27  ├──── SCK (27)
          SCK (7) ────┤  7          26  ├──── SI (26)
           SI (8) ────┤  8          25  ├──── VIO (25)
          VIO (9) ────┤  9          24  ├──── SO (24)
           SO (10) ───┤ 10          23  ├──── EN (23)
           EN (11) ───┤ 11          22  ├──── DRV0 (22)
         DRV0 (12) ───┤ 12          21  ├──── DRV1 (21)
         DRV1 (13) ───┤ 13          20  ├──── FAULTN (20)
       FAULTN (14) ───┤ 14          19  ├──── VDD (19)
          VDD (15) ───┤ 15          18  ├──── GND (18)
          GND (16) ───┤ 16          17  ├──── VBAT (17)
                     └─────────────────┘
                       PG-DSO-32 Package
```

### Pin Functions

| Pin | Name | Type | Function |
|-----|------|------|----------|
| 1, 17 | VBAT | Power | Battery voltage supply (5.5V - 41V) |
| 2-4, 29-31 | OUT0-5 | Output | Low-side switch outputs |
| 5 | RESN | Input | Active-low reset (disables outputs, resets registers) |
| 6, 28 | CSN | Input | SPI chip select (active low) |
| 7, 27 | SCK | Input | SPI clock (up to 10 MHz) |
| 8, 26 | SI | Input | SPI data input (MOSI) |
| 9, 25 | VIO | Power | SPI I/O voltage (3.3V or 5.0V) |
| 10, 24 | SO | Output | SPI data output (MISO) |
| 11, 23 | EN | Input | Global enable (active high) |
| 12, 22 | DRV0 | Input | External drive input 0 |
| 13, 21 | DRV1 | Input | External drive input 1 |
| 14, 20 | FAULTN | Output | Active-low fault indication (open drain) |
| 15, 19 | VDD | Power | Internal 5V supply output |
| 16, 18, 32 | GND | Ground | Ground connection |

## Electrical Specifications

### Absolute Maximum Ratings

| Parameter | Min | Max | Unit |
|-----------|-----|-----|------|
| VBAT (Battery Voltage) | -0.3 | 45 | V |
| VIO (I/O Voltage) | -0.3 | 6 | V |
| VDD Load Current | - | 10 | mA |
| Output Current (per channel) | - | 2 | A |
| Output Current (parallel) | - | 4 | A |
| Junction Temperature | -40 | 175 | °C |
| Storage Temperature | -55 | 150 | °C |

### Recommended Operating Conditions

| Parameter | Min | Typ | Max | Unit |
|-----------|-----|-----|-----|------|
| VBAT (Battery Voltage) | 5.5 | 12/24 | 41 | V |
| VIO (I/O Voltage) | 3.0 | 3.3/5.0 | 5.5 | V |
| Junction Temperature | -40 | - | 150 | °C |
| SPI Clock Frequency | 0.1 | - | 10 | MHz |

### Current Control Specifications

| Parameter | Min | Typ | Max | Unit | Notes |
|-----------|-----|-----|-----|------|-------|
| Current Range (single) | 0 | - | 2000 | mA | Per channel |
| Current Range (parallel) | 0 | - | 4000 | mA | Paired channels |
| Current Resolution | - | 0.061 | - | mA | 15-bit |
| Current Accuracy | - | ±5 | ±10 | % | At 25°C |
| Dither Amplitude | 0 | - | 1800 | mA | Configurable |

### Slew Rate Control

| Setting | Value | Application |
|---------|-------|-------------|
| 00b | 1.0 V/µs | Low EMI, slow switching |
| 01b | 2.5 V/µs | Standard operation |
| 10b | 5.0 V/µs | Fast switching |
| 11b | 10.0 V/µs | Maximum speed |

## Functional Block Diagram

```
                            TLE92466ED Architecture
    
    ┌──────────────────────────────────────────────────────────────────────┐
    │                                                                      │
    │  ┌────────────┐     ┌───────────────┐     ┌──────────────────┐    │
    │  │  5V Reg    │────▶│   Clock Gen   │────▶│  SPI Interface   │    │
    │  │  (VDD)     │     │   & Watchdog  │     │    32-bit CRC    │    │
    │  └────────────┘     └───────────────┘     └──────────────────┘    │
    │         │                    │                      │               │
    │         │                    │                      │               │
    │  ┌──────▼────────────────────▼──────────────────────▼───────────┐  │
    │  │                  Control Logic & State Machine                │  │
    │  │          (Config Mode / Mission Mode Management)              │  │
    │  └───────────────────────────────────────────────────────────────┘  │
    │                                   │                                  │
    │         ┌─────────────────────────┼─────────────────────────┐       │
    │         │                         │                         │       │
    │  ┌──────▼───────┐         ┌──────▼───────┐         ┌──────▼──────┐│
    │  │   Channel 0  │         │   Channel 1  │   ...   │  Channel 5  ││
    │  │  ┌─────────┐ │         │  ┌─────────┐ │         │ ┌─────────┐ ││
    │  │  │   ICC   │ │         │  │   ICC   │ │         │ │   ICC   │ ││
    │  │  │  PWM    │ │         │  │  PWM    │ │         │ │  PWM    │ ││
    │  │  │ Dither  │ │         │  │ Dither  │ │         │ │ Dither  │ ││
    │  │  └────┬────┘ │         │  └────┬────┘ │         │ └────┬────┘ ││
    │  │       │      │         │       │      │         │      │      ││
    │  │  ┌────▼────┐ │         │  ┌────▼────┐ │         │ ┌────▼────┐ ││
    │  │  │  Driver │ │         │  │  Driver │ │         │ │  Driver │ ││
    │  │  │  Stage  │ │         │  │  Stage  │ │         │ │  Stage  │ ││
    │  │  └────┬────┘ │         │  └────┬────┘ │         │ └────┬────┘ ││
    │  │       │      │         │       │      │         │      │      ││
    │  │  ┌────▼────┐ │         │  ┌────▼────┐ │         │ ┌────▼────┐ ││
    │  │  │Diag &   │ │         │  │Diag &   │ │         │ │Diag &   │ ││
    │  │  │Feedback │ │         │  │Feedback │ │         │ │Feedback │ ││
    │  │  └────┬────┘ │         │  └────┬────┘ │         │ └────┬────┘ ││
    │  └───────┼──────┘         └───────┼──────┘         └───────┼─────┘│
    │          │                        │                        │       │
    │          ▼                        ▼                        ▼       │
    │       OUT0                     OUT1                     OUT5       │
    │          │                        │                        │       │
    └──────────┼────────────────────────┼────────────────────────┼───────┘
               │                        │                        │
               ▼                        ▼                        ▼
            Load 0                   Load 1                   Load 5
```

## Typical Application Circuit

```
                    Microcontroller                    TLE92466ED
                   ┌─────────────┐                   ┌──────────┐
                   │             │                   │          │
    VCC (3.3/5V) ──┤ VCC         │                   │ VIO ─────┼── VCC
                   │             │                   │          │
         ┌─────────┤ SPI_CS   CSN├───────────────────┤ CSN      │
         │         │             │                   │          │
         ├─────────┤ SPI_CLK  SCK├───────────────────┤ SCK      │
         │         │             │                   │          │
         ├─────────┤ SPI_MOSI  SI├───────────────────┤ SI       │
         │         │             │                   │          │
         └─────────┤ SPI_MISO  SO├───────────────────┤ SO       │
                   │             │                   │          │
                   │ GPIO      EN├───────────────────┤ EN       │
                   │             │                   │          │
                   │ GPIO   RESN ├───────────────────┤ RESN     │
                   │             │                   │          │
                   │ GPIO  FAULTN├───────────────────┤ FAULTN   │
                   │             │                   │          │
                   │         GND ├───────────────────┤ GND      │
                   │             │                   │          │
                   └─────────────┘                   │ VBAT ────┼── 12/24V
                                                     │          │
                                                     │ OUT0 ────┼──┐
                                                     │ OUT1 ────┼──┤
                                                     │ OUT2 ────┼──┼── Solenoids
                                                     │ OUT3 ────┼──┤  (Inductive)
                                                     │ OUT4 ────┼──┤
                                                     │ OUT5 ────┼──┘
                                                     │          │
                                                     └──────────┘

   External Components:
   - Decoupling: 100nF ceramic + 10µF electrolytic on VBAT
   - VIO decoupling: 100nF ceramic
   - Pull-up on FAULTN: 10kΩ to VIO
   - Pull-down on RESN: 10kΩ to GND (optional)
```

## Operating Modes

### Mode Structure

```
    Power-On Reset
         │
         ▼
    ┌──────────────┐
    │ Config Mode  │ ◄──────────────┐
    │              │                │
    │ - Configure  │                │
    │ - Set modes  │      OP_MODE=0 │
    │ - Parallel   │                │
    │ - No outputs │                │
    └──────┬───────┘                │
           │ OP_MODE=1              │
           │                        │
           ▼                        │
    ┌──────────────┐                │
    │ Mission Mode │                │
    │              │                │
    │ - Enable CH  │                │
    │ - Outputs ON │     Faults or  │
    │ - Monitor    │     Reset      │
    │ - Control    │ ───────────────┘
    └──────────────┘
```

### Config Mode
- **Purpose**: Configuration and initialization
- **Access**: Register writes to mode-locked registers
- **Outputs**: All channels disabled
- **Use Cases**: Startup configuration, mode changes, parallel setup

### Mission Mode
- **Purpose**: Normal operation
- **Access**: Channel enable/disable, current control
- **Outputs**: Enabled channels active
- **Use Cases**: Runtime operation, current control, monitoring

## Power Sequencing

```
    Time ─────────────────────────────────────────────────────▶

    VBAT    ┌────────────────────────────────────────────
            │
    ────────┘

    VIO     ────┌──────────────────────────────────────────
                │
    ────────────┘

    RESN    ─────────┐         ┌──────────────────────────
                     │ tRES    │
    ─────────────────┘         └──────────────────────────

    SPI     ─────────────────────┐
    Access                       │  Communication ready
    ─────────────────────────────┴────────────────────────

    Notes:
    - tRES (min): 1 µs pulse width
    - Power-up delay: ~1 ms after VBAT/VIO stable
    - SPI accessible after tPOR + tRES
```

## Performance Characteristics

### Current Control Accuracy

```
    Output Current vs Setpoint (Typical, 25°C)
    
    Iout
     │                                          ╱
   2A│                                      ╱
     │                                  ╱
 1.5A│                              ╱
     │                          ╱
   1A│                      ╱
     │                  ╱
 0.5A│              ╱
     │          ╱
   0A└──────────────────────────────────────── Setpoint
      0      0.5A    1A     1.5A    2A
      
      Accuracy: ±5% typical, ±10% maximum
```

### Temperature Characteristics

- Operating: -40°C to +150°C junction
- Over-temperature warning: ~165°C
- Over-temperature error: ~175°C
- Thermal shutdown: Automatic channel disable

---

**Navigation**: [← Index](00_INDEX.md) | [Next: SPI Protocol →](02_SPI_Protocol.md)

