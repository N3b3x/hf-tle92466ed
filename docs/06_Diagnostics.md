---
layout: default
title: Diagnostics
nav_order: 8
parent: Documentation Index
description: "Fault detection and monitoring capabilities"
---

# Diagnostics & Protection

## Diagnostic System Overview

The TLE92466ED provides comprehensive diagnostics for fault detection, protection, and system monitoring.

### Diagnostic Architecture

```
    Diagnostic Flow:
    
    Channels (6) ──┬──▶ Per-Channel Errors ──▶ DIAG_ERR_CHGRx
                   │
                   ├──▶ Per-Channel Warnings ─▶ DIAG_WARN_CHGRx
                   │
                   └──▶ Feedback Values ──────▶ FB_xxx registers
                   
    Global ────────┬──▶ Supply Faults ────────▶ GLOBAL_DIAG0
                   │
                   ├──▶ Internal Faults ──────▶ GLOBAL_DIAG1
                   │
                   └──▶ Memory Faults ────────▶ GLOBAL_DIAG2
                   
    Fault Output ──▶ FAULTN Pin (open drain, active low)
```

## Global Diagnostics

### GLOBAL_DIAG0 - Supply & Temperature

| Bit | Flag | Type | Description |
|-----|------|------|-------------|
| 14 | SPI_WD_ERR | Error | SPI watchdog timeout |
| 10 | POR_EVENT | Event | Power-on reset occurred |
| 9 | RES_EVENT | Event | External reset (RESN pin) |
| 8 | COTWARN | Warning | Central over-temperature warning |
| 7 | COTERR | Error | Central over-temperature error |
| 6 | CLK_NOK | Error | Clock fault detected |
| 5 | VDD_OV | Error | VDD overvoltage |
| 4 | VDD_UV | Error | VDD undervoltage |
| 3 | VIO_OV | Error | VIO overvoltage |
| 2 | VIO_UV | Error | VIO undervoltage |
| 1 | VBAT_OV | Error | VBAT overvoltage |
| 0 | VBAT_UV | Error | VBAT undervoltage |

**Clearing**: Write 1 to clear (rwh type)

### GLOBAL_DIAG1 - Internal Supplies

| Bit | Flag | Description |
|-----|------|-------------|
| 15 | HVADC_ERR | HV ADC error |
| 6 | VPRE_OV | Pre-regulator OV |
| 5 | REF_OV | Reference OV |
| 4 | REF_UV | Reference UV |
| 3 | VDD2V5_OV | 2.5V supply OV |
| 2 | VDD2V5_UV | 2.5V supply UV |
| 1 | VR_IREF_OV | Bias current OV |
| 0 | VR_IREF_UV | Bias current UV |

### GLOBAL_DIAG2 - Memory & ECC

| Bit | Flag | Description |
|-----|------|-------------|
| 4 | OTP_VIRGIN | OTP not configured |
| 3 | OTP_ECC_ERR | OTP multi-bit error |
| 1 | REG_ECC_ERR | Register ECC error |

## Channel Diagnostics

### DIAG_ERR_CHGRx - Error Flags

Per-channel error register (one per channel):

| Bit | Flag | Description | Action |
|-----|------|-------------|--------|
| 4 | OLSG | Open load or short to ground | Check load connection |
| 3 | OTE | Over-temperature error | Reduce current/improve cooling |
| 2 | OL | Open load | Check load connection |
| 1 | SG | Short to ground | Check wiring |
| 0 | OC | Over-current | Reduce setpoint/check load |

**Reading**:
```cpp
auto diag = driver.get_channel_diagnostics(Channel::CH0);
if (diag->overcurrent) {
    // Handle OC fault
}
```

### DIAG_WARN_CHGRx - Warning Flags

| Bit | Flag | Description |
|-----|------|-------------|
| 3 | OLSG_WARN | OLSG warning |
| 2 | PWM_REG_WARN | PWM regulation warning |
| 1 | I_REG_WARN | Current regulation warning |
| 0 | OTW | Over-temperature warning |

## Protection Features

### Over-Current Protection

**Detection**:
```
    I_load > I_setpoint + margin
    
    Threshold: Configurable per channel
    Response: Immediate shutdown
    Recovery: Clear fault, re-enable
```

**Configuration**:
- Automatic in ICC mode
- Threshold based on setpoint
- Fast response (<10µs)

### Open Load Detection

**ON-State Detection**:
```
    I_load < OL_TH × I_setpoint
    
    Where: OL_TH = 1/8 to 7/8 (configurable)
```

**OFF-State Detection**:
```
    Diagnostic current applied
    Voltage measured
    Load impedance calculated
```

**Configuration**:
```cpp
// Set OL threshold to 3/8 of setpoint
ChannelConfig config{
    .open_load_threshold = 3  // 3/8 threshold
};
driver.configure_channel(Channel::CH0, config);
```

### Short to Ground Detection

**Detection Method**:
- Current rises above setpoint
- Voltage drops below threshold
- Immediate detection

**Response**:
1. Channel disabled
2. SG flag set
3. FAULTN pin asserted (if enabled)

### Over-Temperature Protection

**Two Levels**:

1. **Warning (OTW)**: ~165°C
   - Warning flag set
   - Operation continues
   - Reduce load recommended

2. **Error (OTE)**: ~175°C
   - Channel disabled
   - Error flag set
   - Cooling required

**Temperature Monitoring**:
```
    Tj (Junction) ──▶ Comparison ──▶ Flags
         │                │
         │                ├──▶ OTW (165°C)
         │                └──▶ OTE (175°C)
         │
         ▼
    Thermal Shutdown (auto-recovery)
```

### Supply Voltage Monitoring

**VBAT Monitoring**:
```
Configurable thresholds:
  UV: V_BAT_UV = VBAT_UV_TH × 0.16208V
  OV: V_BAT_OV = VBAT_OV_TH × 0.16208V
  
Example: UV=7V, OV=40V
  VBAT_UV_TH = 7V / 0.16208V ≈ 43 (0x2B)
  VBAT_OV_TH = 40V / 0.16208V ≈ 247 (0xF7)
```

**Configuration**:
```cpp
driver.set_vbat_thresholds(
    43,   // UV threshold (7V)
    247   // OV threshold (40V)
);
```

## Watchdog Systems

### SPI Watchdog

**Purpose**: Detect communication loss

**Operation**:
```
    WD_RELOAD counter ──▶ Decrement ──▶ Timeout?
         ▲                               │
         │                               ├─▶ No: Continue
         │                               │
         └───────── Reload ◄─────────────┴─▶ Yes: Fault
```

**Configuration**:
```cpp
// Enable and reload periodically
driver.reload_spi_watchdog(1000);

// In main loop:
while (running) {
    driver.reload_spi_watchdog(1000);
    // ... other operations
}
```

**Timeout Calculation**:
```
t_timeout = WD_TIME / f_spi_wd

Where: f_spi_wd configurable
```

### Clock Watchdog

**Purpose**: Monitor oscillator

**Detection**:
- Internal oscillator failure
- Clock frequency deviation
- Automatic detection

**Response**:
- CLK_NOK flag set
- Device enters safe state
- All outputs disabled

## Fault Masking

### FAULT_MASK Registers

Control which faults assert the FAULTN pin:

**FAULT_MASK0** (0x0016):
```
Per-channel error masking:
Bit 0: CH0_ERR_MASK
Bit 1: CH1_ERR_MASK
...
Bit 5: CH5_ERR_MASK
```

**FAULT_MASK1** (0x0017):
```
Per-channel warning masking:
Bit 0: CH0_WARN_MASK
Bit 1: CH1_WARN_MASK
...
Bit 5: CH5_WARN_MASK
```

**FAULT_MASK2** (0x0018):
```
Global fault masking:
- Supply faults
- Temperature faults
- Communication faults
```

**Usage**:
```cpp
// Mask CH0 warnings from FAULTN
write_register(FAULT_MASK1, (1 << 0));

// Enable all error reporting
write_register(FAULT_MASK0, 0x0000);
```

## Diagnostic Monitoring Strategy

### Polling Approach

```cpp
void monitor_system() {
    // Check global status
    auto status = driver.get_device_status();
    
    if (status->any_fault) {
        // Handle global faults
        if (status->vbat_uv) {
            log("VBAT undervoltage!");
        }
        if (status->ot_error) {
            log("Over-temperature!");
        }
    }
    
    // Check each active channel
    for (int ch = 0; ch < 6; ch++) {
        auto diag = driver.get_channel_diagnostics(Channel(ch));
        
        if (diag->overcurrent) {
            log("CH%d: Over-current!", ch);
            driver.enable_channel(Channel(ch), false);
        }
        
        if (diag->open_load) {
            log("CH%d: Open load!", ch);
        }
        
        if (diag->short_to_ground) {
            log("CH%d: Short to ground!", ch);
            driver.enable_channel(Channel(ch), false);
        }
    }
}
```

### Interrupt-Driven Approach

```cpp
// FAULTN pin interrupt handler
void FAULTN_IRQHandler() {
    // Read global diagnostics
    uint16_t diag0 = read_register(GLOBAL_DIAG0);
    
    // Identify fault source
    if (diag0 & VBAT_UV) {
        handle_vbat_fault();
    }
    
    // Check channel faults
    for (int ch = 0; ch < 6; ch++) {
        uint16_t ch_err = read_register(DIAG_ERR_CHGR0 + ch);
        if (ch_err) {
            handle_channel_fault(ch, ch_err);
        }
    }
    
    // Clear faults
    driver.clear_faults();
}
```

## Fault Recovery Procedures

### Over-Current Recovery

```
1. Disable affected channel
2. Read actual current (FB_I_AVG)
3. Check if transient or persistent
4. Reduce setpoint if needed
5. Clear fault flags
6. Re-enable with lower current
7. Monitor for recurrence
```

### Open Load Recovery

```
1. Verify load connection
2. Check wiring integrity
3. Confirm load not damaged
4. Clear fault flags
5. Re-enable channel
6. Adjust OL threshold if false positive
```

### Temperature Recovery

```
1. Disable channel immediately (automatic)
2. Allow cooling period (>10s)
3. Check thermal management
4. Reduce current setpoint
5. Clear fault flags
6. Re-enable with lower current
7. Monitor temperature (OTW flag)
```

### Supply Fault Recovery

```
1. Check supply voltage
2. Verify within operating range
3. Check supply stability
4. Adjust thresholds if transient
5. Clear fault flags
6. Resume operation
```

## Best Practices

### Monitoring Frequency

```
Recommended polling rates:
- Critical channels: 10 Hz (100ms)
- Normal channels: 1 Hz (1s)
- Global status: 1 Hz (1s)
- FAULTN pin: Interrupt-driven (immediate)
```

### Fault Logging

```cpp
struct FaultLog {
    uint32_t timestamp;
    Channel channel;
    uint16_t error_flags;
    uint16_t current_setpoint;
    uint16_t actual_current;
    uint16_t vbat;
};

// Log faults for analysis
void log_fault(const FaultLog& log);
```

### Preventive Measures

1. **Set appropriate thresholds**
2. **Monitor warnings before errors**
3. **Implement graceful degradation**
4. **Log all fault events**
5. **Periodic system health checks**

---

**Navigation**: [← Channel Modes](05_Channel_Modes.md) | [Next: Driver API →](07_Driver_API.md)
