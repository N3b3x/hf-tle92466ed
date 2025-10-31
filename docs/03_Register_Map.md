---
layout: default
title: "📋 Register Map"
description: "Complete register reference for all 108 registers with bit field definitions"
nav_order: 3
parent: "📚 Documentation"
permalink: /docs/03_Register_Map/
---

# Register Map

## Register Organization

The TLE92466ED contains 108 registers organized into two categories:

```text
Register Space (10-bit addressing: 0x000 - 0x3FF)

├── Central/Global Registers (0x0000 - 0x0207)
│   ├── Control & Config (0x0000 - 0x0019)
│   ├── Diagnostics (0x000A - 0x0018)
│   ├── Test & BIST (0x003F)
│   └── Feedback & ID (0x0200 - 0x0207)
│
└── Per-Channel Registers (0x0100 - 0x01BF)
    ├── Channel 0: 0x0100 - 0x011F
    ├── Channel 1: 0x0120 - 0x013F
    ├── Channel 2: 0x0140 - 0x015F
    ├── Channel 3: 0x0160 - 0x017F
    ├── Channel 4: 0x0180 - 0x019F
    └── Channel 5: 0x01A0 - 0x01BF

Channel Address Calculation:
  Base = 0x0100 + (Channel_Number × 0x0020)
  Register = Base + Offset
```text

## Register Access Types

| Type | Description | Behavior |
|------|-------------|----------|
| **r** | Read-only | Write has no effect |
| **rw** | Read/Write | Standard read and write |
| **rwh** | Read/Write, clear on write 1 | Write 1 to clear (fault flags) |
| **rh** | Read, hardware sets | Software read-only, HW updates |

## Central Registers Summary

### Control & Configuration

| Address | Name | Type | Default | Description |
|---------|------|------|---------|-------------|
| 0x0000 | CH_CTRL | rw | 0x0000 | Channel control and mode |
| 0x0002 | GLOBAL_CONFIG | rw | 0x4005 | Global configuration |
| 0x0006 | VBAT_TH | rwh | 0xFF19 | VBAT thresholds |
| 0x0007 | FB_FRZ | rw | 0x0000 | Feedback freeze control |
| 0x0008 | FB_UPD | rw | 0x0000 | Feedback update control |
| 0x0009 | WD_RELOAD | rw | 0x0000 | SPI watchdog reload |
| 0x0019 | CLK_DIV | rw | 0x0000 | Clock divider |
| 0x003F | SFF_BIST | rw | 0x0000 | Built-in self-test |

### Diagnostics

| Address | Name | Type | Default | Description |
|---------|------|------|---------|-------------|
| 0x0003 | GLOBAL_DIAG0 | rwh/rw | 0x0600 | Global diagnostics 0 |
| 0x0004 | GLOBAL_DIAG1 | rwh/rw | 0x0000 | Global diagnostics 1 |
| 0x0005 | GLOBAL_DIAG2 | rwh/rw | 0x0000 | Global diagnostics 2 |
| 0x000A-F | DIAG_ERR_CHGRx | rh | 0x0000 | Channel error flags (6) |
| 0x0010-15 | DIAG_WARN_CHGRx | rh | 0x0000 | Channel warning flags (6) |
| 0x0016 | FAULT_MASK0 | rw | 0x0000 | Fault mask register 0 |
| 0x0017 | FAULT_MASK1 | rw | 0x0000 | Fault mask register 1 |
| 0x0018 | FAULT_MASK2 | rw | 0x0000 | Fault mask register 2 |

### Feedback & Status

| Address | Name | Type | Default | Description |
|---------|------|------|---------|-------------|
| 0x0200 | ICVID | r | - | IC version and ID |
| 0x0201 | PIN_STAT | rh | varies | Pin status feedback |
| 0x0202 | FB_STAT | rh | varies | Feedback status |
| 0x0203 | FB_VOLTAGE1 | rh | varies | VBAT voltage |
| 0x0204 | FB_VOLTAGE2 | rh | varies | VIO/VDD voltage |
| 0x0205-7 | CHIPID0-2 | r | unique | Unique chip ID (48-bit) |

## Per-Channel Registers Summary

### Control & Setpoint

| Offset | Name | Type | Default | Description |
|--------|------|------|---------|-------------|
| 0x0000 | SETPOINT | rwh | 0x0000 | Current setpoint (15-bit) |
| 0x0001 | CTRL | rw | 0x4600 | Channel control |
| 0x000C | MODE | rw | 0x0000 | Channel operation mode |
| 0x000D | TON | rw | 0x0000 | Direct drive on-time |
| 0x000E | CTRL_INT_THRESH | rw | 0x0003 | Integrator threshold |

### ICC & PWM

| Offset | Name | Type | Default | Description |
|--------|------|------|---------|-------------|
| 0x0002 | PERIOD | rw | 0x0000 | PWM frequency control |
| 0x0003 | INTEGRATOR_LIMIT | rw | 0x43FF | Integrator limits |

### Dither

| Offset | Name | Type | Default | Description |
|--------|------|------|---------|-------------|
| 0x0004 | DITHER_CLK_DIV | rw | 0x0000 | Dither clock divider |
| 0x0005 | DITHER_STEP | rw | 0x0000 | Dither steps |
| 0x0006 | DITHER_CTRL | rw | 0x0000 | Dither control |

### Configuration & Feedback

| Offset | Name | Type | Default | Description |
|--------|------|------|---------|-------------|
| 0x0007 | CH_CONFIG | rwh | 0x0003 | Channel configuration |
| 0x0200 | FB_DC | rh | varies | Duty cycle feedback |
| 0x0201 | FB_VBAT | rh | varies | VBAT feedback |
| 0x0202 | FB_I_AVG | rh | varies | Average current |

## Detailed Register Descriptions

### CH_CTRL (0x0000) - Channel Control

```text
Bits: 15  14  13  12  11-6  5   4   3   2   1   0
     ┌───┬───┬───┬───┬────┬───┬───┬───┬───┬───┬───┐
     │OP │PR │PR │PR │Res │EN │EN │EN │EN │EN │EN │
     │MOD│1_2│0_3│4_5│    │CH5│CH4│CH3│CH2│CH1│CH0│
     └───┴───┴───┴───┴────┴───┴───┴───┴───┴───┴───┘
```text

| Bit | Field | Type | Description |
|-----|-------|------|-------------|
| 15 | OP_MODE | rw | 0=Config Mode, 1=Mission Mode |
| 14 | CH_PAR_1_2 | rw | Parallel channels 1/2 |
| 13 | CH_PAR_0_3 | rw | Parallel channels 0/3 |
| 12 | CH_PAR_4_5 | rw | Parallel channels 4/5 |
| 5-0 | EN_CHx | rw | Enable channel x (Mission mode only) |

**Notes**: 
- Channel enable requires Mission Mode
- Parallel configuration requires Config Mode

### GLOBAL_CONFIG (0x0002) - Global Configuration

```text
Bits: 15 14  13  12  11-6 5   4   3  2   1   0
     ┌──┬───┬───┬───┬────┬───┬───┬──┬───┬───┬───┐
     │Res│VIO│UV │OT │Res │V15│V15│Rs│CRC│SPI│CLK│
     │   │SEL│OV │TST│    │OV │UV │  │EN │WD │WD │
     └──┴───┴───┴───┴────┴───┴───┴──┴───┴───┴───┘
```text

| Bit | Field | Description |
|-----|-------|-------------|
| 14 | VIO_SEL | VIO voltage: 0=3.3V, 1=5.0V |
| 13 | UV_OV_SWAP | Test UV/OV swap |
| 12 | OT_TEST | Test over-temperature |
| 5 | V1V5_OV_TEST | Test 1.5V OV detection |
| 4 | V1V5_UV_TEST | Test 1.5V UV detection |
| 2 | CRC_EN | Enable CRC checking |
| 1 | SPI_WD_EN | Enable SPI watchdog |
| 0 | CLK_WD_EN | Enable clock watchdog |

**Default**: 0x4005 (CRC + watchdogs enabled, 3.3V VIO)

### SETPOINT (0x0100 + Ch×0x20) - Current Setpoint

```text
Bits: 15        14                                0
     ┌──────────┬──────────────────────────────────┐
     │AUTO_LIMIT│         TARGET (15-bit)          │
     │   _DIS   │                                  │
     └──────────┴──────────────────────────────────┘
```text

| Bits | Field | Description |
|------|-------|-------------|
| 14:0 | TARGET | Current setpoint value |
| 15 | AUTO_LIMIT_DIS | Disable auto-limit feature |

**Current Calculation**:
```text
Single mode: I_set = 2000mA × TARGET / 32767
Parallel mode: I_set = 4000mA × TARGET / 32767
Resolution: 0.061mA (single), 0.122mA (parallel)
```text

**Example Values**:

| TARGET | Current (Single) | Current (Parallel) |
|--------|------------------|---------------------|
| 0x0000 | 0 mA | 0 mA |
| 0x1000 | 250 mA | 500 mA |
| 0x4000 | 1000 mA | 2000 mA |
| 0x6000 | 1500 mA | 3000 mA |
| 0x7FFF | 2000 mA | 4000 mA |

### CH_CONFIG (0x0107 + Ch×0x20) - Channel Configuration

```text
Bits: 15 14     13   12        7 6    4 3  2 1  0
     ┌───┬───┬──────┬───────────┬──────┬────┬────┐
     │OFF│OFF│OC_DG │ OL_TH_FIX │OL_TH │IDAG│SLEW│
     │DG1│DG0│  _EN │           │      │    │    │
     └───┴───┴──────┴───────────┴──────┴────┴────┘
```text

| Bits | Field | Description |
|------|-------|-------------|
| 1:0 | SLEWR | Slew rate: 00=1V/µs, 01=2.5V/µs, 10=5V/µs, 11=10V/µs |
| 3:2 | I_DIAG | OFF diagnostic current: 00=80µA, 01=190µA, 10=720µA, 11=1250µA |
| 6:4 | OL_TH | Open load threshold (relative to setpoint, 0=disabled, 1-7=1/8 to 7/8) |
| 12:7 | OL_TH_FIXED | Fixed open load threshold |
| 13 | OC_DIAG_EN | Enable OC diagnosis in OFF state |
| 15:14 | OFF_DIAG_CH | OFF diagnostic control: 00=enabled, 01=LS only, 10=HS only |

### MODE (0x010C + Ch×0x20) - Channel Mode

```text
Bits: 15                    4 3            0
     ┌──────────────────────┬──────────────┐
     │      Reserved        │   CH_MODE    │
     └──────────────────────┴──────────────┘
```text

| Value | Mode | Description |
|-------|------|-------------|
| 0x0 | Off | Channel disabled |
| 0x1 | ICC | Integrated Current Control (main mode) |
| 0x2 | Direct SPI | Direct drive via TON register |
| 0x3 | Direct DRV0 | Direct drive via DRV0 pin |
| 0x4 | Direct DRV1 | Direct drive via DRV1 pin |
| 0xC | Measurement | Free-running measurement mode |

**Note**: Mode changes require Config Mode

## Register Access Patterns

### Initialization Sequence

```text
1. Power-on / Reset
   ├─ Device in Config Mode (OP_MODE=0)
   └─ All channels disabled

2. Configure Global Settings
   ├─ Write GLOBAL_CONFIG (CRC, watchdogs, VIO)
   ├─ Write VBAT_TH (voltage thresholds)
   └─ Write WD_RELOAD (if SPI watchdog enabled)

3. Configure Channels (per channel)
   ├─ Write MODE (select ICC/Direct/etc.)
   ├─ Write CH_CONFIG (slew, diagnostics)
   ├─ Write SETPOINT (current target)
   ├─ Write PERIOD (if using ICC PWM)
   └─ Write DITHER_xxx (if using dither)

4. Enter Mission Mode
   └─ Write CH_CTRL with OP_MODE=1

5. Enable Channels
   └─ Write CH_CTRL with EN_CHx=1
```text

### Runtime Monitoring

```text
Periodic Reads:
├─ GLOBAL_DIAG0 (supply faults, temperature)
├─ DIAG_ERR_CHGRx (channel error flags)
├─ DIAG_WARN_CHGRx (channel warnings)
├─ FB_I_AVG (actual current per channel)
├─ FB_DC (duty cycle per channel)
└─ FB_VOLTAGE1/2 (supply voltages)

Watchdog Service:
└─ Write WD_RELOAD periodically (if enabled)
```text

---

**Navigation**: [← SPI Protocol](02_SPI_Protocol.md) | [Next: Current Control →](04_Current_Control.md)
