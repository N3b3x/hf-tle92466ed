---
layout: default
title: "🎛️ Channel Modes"
description: "Operating modes and configuration options for Config and Mission modes"
nav_order: 5
parent: "📚 Documentation"
permalink: /docs/05_Channel_Modes/
---

# Channel Modes

## Operation Modes Overview

The TLE92466ED supports multiple channel operation modes, each designed for specific
applications. Modes are configured per-channel in Config Mode.

### Mode Architecture

```text
    Device Modes:              Channel Modes:
    
    ┌──────────────┐          ┌─────────────────┐
    │ Config Mode  │          │ 0x0: Off        │
    │              │          │ 0x1: ICC        │
    │ - Configure  │◄─────────┤ 0x2: Direct SPI │
    │ - Set modes  │          │ 0x3: Direct DRV0│
    │ - No outputs │          │ 0x4: Direct DRV1│
    └──────┬───────┘          │ 0xC: Measurement│
           │                  └─────────────────┘
           │ OP_MODE=1
           ▼
    ┌──────────────┐
    │ Mission Mode │
    │              │
    │ - Enable CH  │
    │ - Outputs ON │
    │ - Control    │
    └──────────────┘
```text

## Device Operating Modes

### Config Mode (OP_MODE=0)

**Purpose**: Configuration and initialization

**Characteristics**:
- All channels disabled (outputs OFF)
- Mode-locked registers writable
- Parallel operation configurable
- Channel modes changeable

**Use Cases**:
- Initial device setup
- Changing channel modes
- Configuring parallel operation
- Reconfiguring after faults

**Restrictions**:
- Channels cannot be enabled
- No output switching possible

### Mission Mode (OP_MODE=1)

**Purpose**: Normal operation

**Characteristics**:
- Channels can be enabled
- Outputs controllable
- Real-time current control
- Diagnostics active

**Use Cases**:
- Normal operation
- Current control
- Real-time monitoring

**Restrictions**:
- Mode-locked registers read-only
- Cannot change channel modes
- Cannot configure parallel operation

### Mode Switching

```text
    Transition Sequence:
    
    Config → Mission:
    1. Write CH_CTRL with OP_MODE=1
    2. Device transitions (immediate)
    3. Channels can now be enabled
    4. System ready for operation
    
    Mission → Config:
    1. Write CH_CTRL with OP_MODE=0
    2. All channels disabled automatically
    3. Device transitions (immediate)
    4. Configuration registers unlocked
```text

## Channel Operation Modes

### Mode 0x0: Off

**Description**: Channel disabled

**Configuration**:
```cpp
driver.set_channel_mode(Channel::CH0, ChannelMode::OFF);
```text

**Behavior**:
- Output stage disabled
- No current flow
- Diagnostics inactive (except OFF-state)
- Lowest power consumption

**Use Cases**:
- Unused channels
- Disable during fault
- Power saving

### Mode 0x1: ICC (Integrated Current Control)

**Description**: Precision current regulation

**Configuration**:
```cpp
driver.set_channel_mode(Channel::CH0, ChannelMode::ICC);
driver.set_current_setpoint(Channel::CH0, 1500); // 1.5A
```text

**Behavior**:
- Closed-loop current control
- PWM-based regulation
- 15-bit resolution (0.061mA steps)
- Dither support
- Current feedback available

**Control Loop**:
```text
    Setpoint → Difference → Integrator → PWM → Output
                   ▲                              │
                   └──────── Sense ◄──────────────┘
```text

**Use Cases**:
- Solenoid actuators (primary use)
- Valve control
- Precise current applications
- Variable force control

**Parameters**:
- SETPOINT: Current target (15-bit)
- PERIOD: PWM frequency
- INTEGRATOR_LIMIT: Windup prevention
- DITHER_xxx: Current shaping

### Mode 0x2: Direct Drive (SPI)

**Description**: Time-based output control via SPI

**Configuration**:
```cpp
driver.set_channel_mode(Channel::CH0, ChannelMode::DIRECT_DRIVE_SPI);
// Configure on-time in TON register
```text

**Behavior**:
- Output ON for specified duration
- No current regulation
- Time configured in TON register
- Repeated automatically

**Timing**:
```text
    t_on = [(TON_MANT + 1) × 2^EXP] × (1/f_sys)
    
    Where: EXP from DITHER_CLK_DIV register
           f_sys ≈ 8 MHz
```text

**Use Cases**:
- Simple ON/OFF control
- Pulsed operation
- Fixed duty cycle
- No current feedback needed

### Mode 0x3: Direct Drive (DRV0 Pin)

**Description**: External hardware control via DRV0 pin

**Configuration**:
```cpp
driver.set_channel_mode(Channel::CH0, ChannelMode::DIRECT_DRIVE_DRV0);
```text

**Behavior**:
- Output follows DRV0 pin state
- Hardware-controlled timing
- Low latency response
- No SPI communication needed

**Pin Control**:
```text
    DRV0 Pin:  ─────┐      ┌─────┐      ┌─────
                    └──────┘     └──────┘
                    
    Output:    ─────┐      ┌─────┐      ┌─────
                    └──────┘     └──────┘
                    (Follows DRV0)
```text

**Use Cases**:
- External PWM control
- Hardware sequencing
- Emergency override
- Minimal software overhead

### Mode 0x4: Direct Drive (DRV1 Pin)

**Description**: External hardware control via DRV1 pin

**Configuration**:
```cpp
driver.set_channel_mode(Channel::CH0, ChannelMode::DIRECT_DRIVE_DRV1);
```text

**Behavior**: Same as DRV0 but using DRV1 pin

**Use Cases**:
- Dual external control
- Independent hardware sequencing
- Multi-source control

### Mode 0xC: Free-Running Measurement

**Description**: Continuous current measurement without output

**Configuration**:
```cpp
driver.set_channel_mode(Channel::CH0, ChannelMode::FREE_RUN_MEAS);
```text

**Behavior**:
- Output disabled
- Current sensing active
- 2^16 samples collected
- Feedback registers updated

**Use Cases**:
- Load characterization
- Diagnostic measurements
- System calibration
- Fault analysis

## Parallel Operation

### Channel Pairing

The TLE92466ED allows three parallel pairs:

```text
    ┌─────────┬─────────┐
    │  CH0    │   CH3   │ ◄── Pair 0/3 (4A max)
    ├─────────┼─────────┤
    │  CH1    │   CH2   │ ◄── Pair 1/2 (4A max)
    ├─────────┼─────────┤
    │  CH4    │   CH5   │ ◄── Pair 4/5 (4A max)
    └─────────┴─────────┘
    
    Configuration: CH_CTRL register
    - Bit 14: CH_PAR_1_2
    - Bit 13: CH_PAR_0_3
    - Bit 12: CH_PAR_4_5
```text

### Parallel Configuration Sequence

```text
1. Enter Config Mode
   └─ Write CH_CTRL with OP_MODE=0

2. Configure Parallel Pair (e.g., CH0/CH3)
   └─ Set CH_PAR_0_3 bit in CH_CTRL

3. Configure Both Channels Identically
   ├─ Same MODE (typically ICC)
   ├─ Same SETPOINT
   ├─ Same CH_CONFIG
   └─ Same PWM/Dither settings

4. Enter Mission Mode
   └─ Write CH_CTRL with OP_MODE=1

5. Enable Both Channels
   └─ Set EN_CH0 and EN_CH3 in CH_CTRL
```text

### Parallel Mode Considerations

**Current Calculation**:
```text
Single:   I_max = 2000 mA
Parallel: I_max = 4000 mA

TARGET value same for both, but total current doubles
```text

**Load Connection**:
```text
    Parallel Connection:
    
    VBAT ──┬─────────────┬─────────────┐
           │             │             │
        ┌──┴──┐       ┌──┴──┐          │
        │ CH0 │       │ CH3 │          │
        └──┬──┘       └──┬──┘          │
           │             │             │
           └─────┬───────┘             │
                 │                     │
              ┌──┴──┐                  │
              │Load │ (Heavy, 0-4A)    │
              └──┬──┘                  │
                 │                     │
                GND◄───────────────────┘
```text

**Restrictions**:
- Only predefined pairs can be paralleled
- Both channels must have identical configuration
- Parallel configuration only in Config Mode
- Cannot mix modes in parallel pair

## Mode Selection Guidelines

### Decision Matrix

| Application | Mode | Rationale |
|-------------|------|-----------|
| Solenoid actuator | ICC | Precise force control |
| Proportional valve | ICC + Dither | Smooth positioning |
| On/off valve | Direct SPI | Simple control |
| PWM-controlled load | Direct DRV0/1 | Hardware timing |
| Heavy solenoid (>2A) | ICC + Parallel | High current |
| System diagnostic | Measurement | Characterization |

### Mode Switching at Runtime

**Not Allowed in Mission Mode**:
- Cannot change channel mode without Config Mode
- Must return to Config Mode first

**Proper Sequence**:
```text
1. Disable channel (clear EN_CHx)
2. Enter Config Mode (OP_MODE=0)
3. Change mode (write MODE register)
4. Reconfigure channel parameters
5. Enter Mission Mode (OP_MODE=1)
6. Enable channel (set EN_CHx)
```text

## Mode Configuration Examples

### Example 1: ICC with Dither

```cpp
// Enter Config Mode
driver.enter_config_mode();

// Configure channel for ICC mode
driver.set_channel_mode(Channel::CH0, ChannelMode::ICC);

// Set 1.2A current with dither
driver.set_current_setpoint(Channel::CH0, 1200);
driver.configure_dither(Channel::CH0, 
    100,   // step_size
    20,    // num_steps  
    5);    // flat_steps

// Enter Mission Mode and enable
driver.enter_mission_mode();
driver.enable_channel(Channel::CH0, true);
```text

### Example 2: Parallel Operation

```cpp
// Enter Config Mode
driver.enter_config_mode();

// Enable parallel operation CH0/CH3
driver.set_parallel_operation(ParallelPair::CH0_CH3, true);

// Configure both channels identically
ChannelConfig config{
    .mode = ChannelMode::ICC,
    .current_setpoint_ma = 3000, // 3A total
    .slew_rate = SlewRate::MEDIUM_2V5_US
};

driver.configure_channel(Channel::CH0, config);
driver.configure_channel(Channel::CH3, config);

// Enter Mission Mode and enable both
driver.enter_mission_mode();
driver.enable_channel(Channel::CH0, true);
driver.enable_channel(Channel::CH3, true);
```text

### Example 3: Direct Drive

```cpp
// Enter Config Mode
driver.enter_config_mode();

// Configure for direct drive via SPI
driver.set_channel_mode(Channel::CH0, ChannelMode::DIRECT_DRIVE_SPI);

// Configure on-time (example: 10ms)
// TON_MANT × 2^EXP × (1/8MHz) = 10ms
// TON_MANT = 500, EXP = 4: 500 × 16 / 8MHz ≈ 1ms

// Enter Mission Mode and enable
driver.enter_mission_mode();
driver.enable_channel(Channel::CH0, true);
```text

---

**Navigation**: [← Current Control](04_Current_Control.md) | [Next: Diagnostics →](06_Diagnostics.md)
