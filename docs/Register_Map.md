# TLE92466ED Register Map

## Complete Register Address Map

```
┌─────────┬──────────────────────────────────────────────────────────┐
│ Address │ Register Name                    │ R/W │ Default │ Page │
├─────────┼──────────────────────────────────┼─────┼─────────┼──────┤
│  0x00   │ CTRL1     - Control Register 1   │ R/W │  0x00   │  2   │
│  0x01   │ CTRL2     - Control Register 2   │ R/W │  0xFE   │  3   │
│  0x02   │ CTRL3     - Control Register 3   │ R/W │  0x00   │  4   │
│  0x03   │ OUT_CTRL  - Output Control       │ R/W │  0x00   │  5   │
│  0x04   │ PWM_CTRL  - PWM Control          │ R/W │  0x60   │  6   │
│  0x05   │ CLK_CTRL  - Clock Control        │ R/W │  0x00   │  7   │
│  0x06   │ RESET_CTRL- Reset Control        │ R/W │  0x00   │  8   │
├─────────┼──────────────────────────────────┼─────┼─────────┼──────┤
│  0x10   │ STATUS1   - Status Register 1    │ R   │  0x01   │  9   │
│  0x11   │ STATUS2   - Status Register 2    │ R   │  0x00   │  10  │
│  0x12   │ STATUS3   - Status Register 3    │ R   │  0x00   │  11  │
│  0x13   │ DIAG_STATUS - Diagnostic Status  │ R   │  0x00   │  12  │
│  0x14   │ FAULT_STATUS - Fault Status      │ R/C │  0x00   │  13  │
├─────────┼──────────────────────────────────┼─────┼─────────┼──────┤
│  0x20   │ CFG1      - Configuration 1      │ R/W │  0x00   │  14  │
│  0x21   │ CFG2      - Configuration 2      │ R/W │  0x00   │  15  │
│  0x22   │ CFG3      - Configuration 3      │ R/W │  0x00   │  16  │
│  0x23   │ CFG_ILIM  - Current Limit Config │ R/W │  0x55   │  17  │
│  0x24   │ CFG_OT    - Over-Temp Config     │ R/W │  0x96   │  18  │
│  0x25   │ CFG_UV    - Under-Voltage Config │ R/W │  0x1C   │  19  │
│  0x26   │ CFG_OV    - Over-Voltage Config  │ R/W │  0xA0   │  20  │
│  0x27   │ CFG_DIAG  - Diagnostic Config    │ R/W │  0xFF   │  21  │
├─────────┼──────────────────────────────────┼─────┼─────────┼──────┤
│  0x30   │ CH0_CFG   - Channel 0 Config     │ R/W │  0x48   │  22  │
│  0x31   │ CH1_CFG   - Channel 1 Config     │ R/W │  0x48   │  22  │
│  0x32   │ CH2_CFG   - Channel 2 Config     │ R/W │  0x48   │  22  │
│  0x33   │ CH3_CFG   - Channel 3 Config     │ R/W │  0x48   │  22  │
│  0x34   │ CH4_CFG   - Channel 4 Config     │ R/W │  0x48   │  22  │
│  0x35   │ CH5_CFG   - Channel 5 Config     │ R/W │  0x48   │  22  │
├─────────┼──────────────────────────────────┼─────┼─────────┼──────┤
│  0x40   │ CH0_STATUS - Channel 0 Status    │ R   │  0x00   │  23  │
│  0x41   │ CH1_STATUS - Channel 1 Status    │ R   │  0x00   │  23  │
│  0x42   │ CH2_STATUS - Channel 2 Status    │ R   │  0x00   │  23  │
│  0x43   │ CH3_STATUS - Channel 3 Status    │ R   │  0x00   │  23  │
│  0x44   │ CH4_STATUS - Channel 4 Status    │ R   │  0x00   │  23  │
│  0x45   │ CH5_STATUS - Channel 5 Status    │ R   │  0x00   │  23  │
├─────────┼──────────────────────────────────┼─────┼─────────┼──────┤
│  0x50   │ DIAG_CH0  - Channel 0 Diagnostic │ R   │  0x00   │  24  │
│  0x51   │ DIAG_CH1  - Channel 1 Diagnostic │ R   │  0x00   │  24  │
│  0x52   │ DIAG_CH2  - Channel 2 Diagnostic │ R   │  0x00   │  24  │
│  0x53   │ DIAG_CH3  - Channel 3 Diagnostic │ R   │  0x00   │  24  │
│  0x54   │ DIAG_CH4  - Channel 4 Diagnostic │ R   │  0x00   │  24  │
│  0x55   │ DIAG_CH5  - Channel 5 Diagnostic │ R   │  0x00   │  24  │
├─────────┼──────────────────────────────────┼─────┼─────────┼──────┤
│  0x70   │ DEVICE_ID - Device Identification│ R   │  0x46   │  25  │
│  0x71   │ REVISION_ID - Silicon Revision   │ R   │  varies │  26  │
│  0x72   │ TEMP_SENSOR - Temperature Reading│ R   │  -      │  27  │
│  0x73   │ VSUP_MON  - Supply Voltage Mon.  │ R   │  -      │  28  │
└─────────┴──────────────────────────────────┴─────┴─────────┴──────┘

Legend:
R   = Read Only
W   = Write Only
R/W = Read/Write
R/C = Read / Clear on Write
```

## Detailed Register Descriptions

### CTRL1 (0x00) - Main Control Register

```
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 7 │ 6 │ 5 │ 4 │ 3 │ 2 │ 1 │ 0 │
├───┴───┴───┴───┴───┴───┴───┴───┤
│ EN│SLP│RST│DEN│PWM│CLK│ - │ - │
└───────────────────────────────┘

Bit 7 - ENABLE: Global Enable
  0: Device disabled (low power)
  1: Device enabled (normal operation)

Bit 6 - SLEEP: Sleep Mode
  0: Active mode
  1: Sleep mode (SPI active, outputs disabled)

Bit 5 - RESET: Software Reset
  0: Normal operation
  1: Trigger software reset (auto-clears)

Bit 4 - DIAG_EN: Diagnostics Enable
  0: Diagnostics disabled
  1: Diagnostics enabled

Bit 3 - PWM_MODE: PWM Mode
  0: Static switching only
  1: PWM mode enabled

Bit 2 - CLK_SRC: Clock Source
  0: Internal oscillator
  1: External clock

Bits 1-0: Reserved (always 0)

Default: 0x00 (All disabled)
```

### CTRL2 (0x01) - Protection Control Register

```
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 7 │ 6 │ 5 │ 4 │ 3 │ 2 │ 1 │ 0 │
├───┴───┴───┴───┴───┴───┴───┴───┤
│ILE│OTE│UVE│OVE│OLE│SCE│LDE│ - │
└───────────────────────────────┘

Bit 7 - ILIM_EN: Current Limit Enable
Bit 6 - OT_EN: Over-Temperature Enable
Bit 5 - UV_EN: Under-Voltage Enable
Bit 4 - OV_EN: Over-Voltage Enable
Bit 3 - OL_EN: Open Load Detection Enable
Bit 2 - SC_EN: Short Circuit Detection Enable
Bit 1 - LOAD_EN: Load Detection Enable
Bit 0: Reserved

Default: 0xFE (All protections enabled)
```

### OUT_CTRL (0x03) - Output Control Register

```
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 7 │ 6 │ 5 │ 4 │ 3 │ 2 │ 1 │ 0 │
├───┴───┴───┴───┴───┴───┴───┴───┤
│ - │ - │O5 │O4 │O3 │O2 │O1 │O0 │
└───────────────────────────────┘

Bits 7-6: Reserved
Bit 5: OUT5 Enable (1=ON, 0=OFF)
Bit 4: OUT4 Enable (1=ON, 0=OFF)
Bit 3: OUT3 Enable (1=ON, 0=OFF)
Bit 2: OUT2 Enable (1=ON, 0=OFF)
Bit 1: OUT1 Enable (1=ON, 0=OFF)
Bit 0: OUT0 Enable (1=ON, 0=OFF)

Default: 0x00 (All outputs OFF)

Example Values:
  0x00 = All OFF
  0x01 = Only CH0 ON
  0x3F = All ON (0b00111111)
  0x2A = CH1, CH3, CH5 ON (0b00101010)
```

### PWM_CTRL (0x04) - PWM Control Register

```
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 7 │ 6 │ 5 │ 4 │ 3 │ 2 │ 1 │ 0 │
├───┴───┴───┴───┴───┴───┴───┴───┤
│ FREQ  │DTH│ -   -   -   -    │
└───────────────────────────────┘

Bits 7-5: PWM_FREQ (PWM Frequency Select)
  000: 100 Hz
  001: 200 Hz
  010: 500 Hz
  011: 1 kHz (default)
  100: 2 kHz
  101: 5 kHz
  110: 10 kHz
  111: 20 kHz

Bit 4: DITHER_EN (PWM Dithering)
  0: No dithering
  1: Frequency dithering enabled (±10%)

Bits 3-0: Reserved

Default: 0x60 (1 kHz, no dithering)
```

### STATUS1 (0x10) - Global Status Register

```
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 7 │ 6 │ 5 │ 4 │ 3 │ 2 │ 1 │ 0 │
├───┴───┴───┴───┴───┴───┴───┴───┤
│FLT│OTW│OTS│UVF│OVF│SPE│ - │POR│
└───────────────────────────────┘

Bit 7 - FAULT: Any Fault Present
Bit 6 - OT_WARN: Over-Temperature Warning
Bit 5 - OT_SD: Over-Temperature Shutdown
Bit 4 - UV_FAULT: Under-Voltage Fault
Bit 3 - OV_FAULT: Over-Voltage Fault
Bit 2 - SPI_ERR: SPI Communication Error
Bit 1: Reserved
Bit 0 - POR: Power-On Reset Flag

Status Bits (Read-Only):
• Set automatically by hardware
• Clear by writing 1 to bit (R/C type)
• Or clear automatically when condition ends

Default: 0x01 (POR set after power-up)
```

### CHx_CFG (0x30-0x35) - Channel Configuration

```
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 7 │ 6 │ 5 │ 4 │ 3 │ 2 │ 1 │ 0 │
├───┴───┴───┴───┴───┴───┴───┴───┤
│ILIM   │PWM│DEN│ SLEW  │INV│ - │
└───────────────────────────────┘

Bits 7-6: ILIM_SEL (Current Limit)
  00: 1A limit
  01: 2A limit (default)
  10: 3A limit
  11: 4A limit

Bit 5: PWM_EN (PWM Enable for this channel)
  0: PWM disabled
  1: PWM enabled

Bit 4: DIAG_EN (Diagnostics Enable for this channel)
  0: Diagnostics disabled
  1: Diagnostics enabled

Bits 3-2: SLEW_RATE (Output Slew Rate)
  00: Slow (~10 V/µs)
  01: Medium (~20 V/µs) [default]
  10: Fast (~40 V/µs)
  11: Fastest (~80 V/µs)

Bit 1: INV (Output Invert)
  0: Normal (1=ON)
  1: Inverted (0=ON)

Bit 0: Reserved

Default: 0x48 (2A limit, Medium slew, Diag enabled)
Binary: 01001000

Configuration Examples:
┌──────────────┬──────┬────────────────────┐
│ Use Case     │ Hex  │ Configuration      │
├──────────────┼──────┼────────────────────┤
│ Low-EMI LED  │ 0x40 │ 2A, Slow slew      │
│ Fast PWM     │ 0xEC │ 4A, Fastest slew   │
│ Safe Default │ 0x58 │ 2A, Medium slew    │
│ High Current │ 0xC8 │ 4A, Medium slew    │
└──────────────┴──────┴────────────────────┘
```

### CHx_STATUS (0x40-0x45) - Channel Status

```
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 7 │ 6 │ 5 │ 4 │ 3 │ 2 │ 1 │ 0 │
├───┴───┴───┴───┴───┴───┴───┴───┤
│FLT│OLO│OLF│ILM│SCH│SCL│ACT│ENA│
└───────────────────────────────┘

Bit 7 - CH_FAULT: Channel Fault
Bit 6 - OL_ON: Open Load (output ON)
Bit 5 - OL_OFF: Open Load (output OFF)
Bit 4 - ILIM: Current Limit Active
Bit 3 - SC_HS: Short Circuit to High Side (VBAT)
Bit 2 - SC_LS: Short Circuit to Low Side (GND)
Bit 1 - ACTIVE: Channel Actively Driving
Bit 0 - ENABLED: Channel Enabled

Read-Only Status Register

Fault Priority:
1. Short Circuit (highest - immediate shutdown)
2. Over-Current (current limiting)
3. Over-Temperature
4. Open Load (lowest - just flag)
```

### DIAG_CHx (0x50-0x55) - Channel Diagnostics

```
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 7 │ 6 │ 5 │ 4 │ 3 │ 2 │ 1 │ 0 │
├───┴───┴───┴───┴───┴───┴───┴───┤
│ LOAD_CURRENT  │OLH│SCH│OTH│VAL│
└───────────────────────────────┘

Bits 7-4: LOAD_CURRENT (Load Current Indicator)
  0000: No load / Very light (<100mA)
  0001-1110: Incremental load indication
  1111: Maximum load / Over-current

Bit 3: OL_HISTORY (Open Load History)
  0: No open load detected
  1: Open load detected (latched)

Bit 2: SC_HISTORY (Short Circuit History)
  0: No short circuit detected
  1: Short circuit detected (latched)

Bit 1: OT_HISTORY (Over-Temp History)
  0: No over-temp detected
  1: Over-temp detected (latched)

Bit 0: DIAG_VALID (Diagnostic Data Valid)
  0: Diagnostic data not valid
  1: Diagnostic data valid and up-to-date

Load Current Interpretation:
┌──────┬────────────┬───────────────┐
│ Value│ Binary     │ Approx Current│
├──────┼────────────┼───────────────┤
│  0   │ 0000       │ < 100mA       │
│  4   │ 0100       │ ~1A           │
│  8   │ 1000       │ ~2A           │
│  12  │ 1100       │ ~3A           │
│  15  │ 1111       │ > 3.5A        │
└──────┴────────────┴───────────────┘

Diagnostic Update Rate: ~10ms
```

### DEVICE_ID (0x70) - Device Identification

```
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 7 │ 6 │ 5 │ 4 │ 3 │ 2 │ 1 │ 0 │
├───┴───┴───┴───┴───┴───┴───┴───┤
│ 0 │ 1 │ 0 │ 0 │ 0 │ 1 │ 1 │ 0 │
└───────────────────────────────┘

Fixed Value: 0x46 (70 decimal)

This register should always read 0x46.
Use for device presence detection and verification.

Verification Example:
if (read_register(0x70) == 0x46) {
    // Correct device detected
} else {
    // Wrong device or communication error
}
```

### TEMP_SENSOR (0x72) - Temperature Sensor

```
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 7 │ 6 │ 5 │ 4 │ 3 │ 2 │ 1 │ 0 │
├───┴───┴───┴───┴───┴───┴───┴───┤
│     Temperature Value (8-bit)  │
└───────────────────────────────┘

Temperature Encoding:
Value = (Temp_°C + 40) / 2

Temperature Range: -40°C to 215°C
Resolution: 2°C per LSB

Conversion Formula:
Temp_°C = (Value × 2) - 40

Examples:
┌───────┬──────────┐
│ Value │   Temp   │
├───────┼──────────┤
│  0x00 │  -40°C   │
│  0x28 │   40°C   │
│  0x43 │   94°C   │
│  0x5F │  150°C   │
│  0x6B │  174°C   │
│  0xFF │  470°C*  │
└───────┴──────────┘
*Out of range
```

### VSUP_MON (0x73) - Supply Voltage Monitor

```
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 7 │ 6 │ 5 │ 4 │ 3 │ 2 │ 1 │ 0 │
├───┴───┴───┴───┴───┴───┴───┴───┤
│  Supply Voltage Value (8-bit)  │
└───────────────────────────────┘

Voltage Encoding:
Value = VSUP_V / 0.25V

Voltage Range: 0V to 63.75V
Resolution: 0.25V per LSB

Conversion Formula:
VSUP_V = Value × 0.25

Examples:
┌───────┬──────────┐
│ Value │ Voltage  │
├───────┼──────────┤
│  0x00 │   0.0V   │
│  0x20 │   8.0V   │
│  0x36 │  13.5V   │
│  0x64 │  25.0V   │
│  0xA0 │  40.0V   │
│  0xFF │  63.75V  │
└───────┴──────────┘
```

## Register Access Timing

```
SPI Transaction Timing:

        ┌───CS───┐
CS  ────┘        └────────────

        ┌─┬─┬─┬─┬─┬─┬─┬─┬─┬─┐
SCLK ───┘ └┘ └┘ └┘ └┘ └┘ └┘ └─

        ├R├─Addr──┤───Data───┤
MOSI  ──┤ ┤       ┤          ┤

        ├──Previous Data─────┤
MISO  ──┤                    ┤

Timing Parameters:
• t_CS_setup: 50ns min
• t_CS_hold: 50ns min
• t_SCLK_high: 40ns min
• t_SCLK_low: 40ns min
• t_setup: 10ns min
• t_hold: 10ns min
```

---

**Document Version**: 1.0.0  
**Last Updated**: 2025-10-18
