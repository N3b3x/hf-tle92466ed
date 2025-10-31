---
layout: default
title: "⚡ Current Control"
description: "Integrated Current Control (ICC) system documentation with 15-bit resolution"
nav_order: 4
parent: "📚 Documentation"
permalink: /docs/04_Current_Control/
---

# Current Control (ICC)

## Integrated Current Controller

The ICC (Integrated Current Controller) is the primary current regulation system
providing precise 15-bit current control.

### ICC Architecture

```text
    Current Control Loop:

    Setpoint ───▶┌────────────┐     ┌──────────┐     ┌─────────┐
    (15-bit)     │ Difference │────▶│Integrator│────▶│   PWM   │
                 │    Amp     │     │  (ICC)   │     │  Driver │
                 └─────▲──────┘     └──────────┘     └────┬────┘
                       │                                   │
                       │    ┌───────────┐                  │
                       └────┤  Current  │◀─────────────────┘
                            │  Sense    │        Output
                            └───────────┘

    PWM Frequency: Configurable via PERIOD register
    Dither Overlay: Optional current modulation
    Regulation: Closed-loop with integrator
```

### Current Resolution

**15-Bit Precision**:
- Single Mode: 0-2000 mA / 32767 steps = 0.061 mA/step
- Parallel Mode: 0-4000 mA / 32767 steps = 0.122 mA/step

### Setpoint Configuration

**Formula**:

```text
TARGET = (I_desired × 32767) / I_max
Where: I_max = 2000mA (single) or 4000mA (parallel)
```

**Examples**:

| Desired Current | TARGET Value | Hex |
|-----------------|--------------|-----|
| 100 mA | 1638 | 0x0666 |
| 500 mA | 8192 | 0x2000 |
| 1000 mA | 16384 | 0x4000 |
| 1500 mA | 24576 | 0x6000 |
| 2000 mA | 32767 | 0x7FFF |

## PWM Frequency Control

### PERIOD Register Configuration

```text
Bits: 15   12  11   10    8 7              0
     ┌───────┬────┬────────┬───────────────┐
     │PWM_CTL│LOW │PERIOD  │   PERIOD      │
     │_PARAM │FREQ│  _EXP  │    _MANT      │
     └───────┴────┴────────┴───────────────┘
```

**Formula**:

```text
Standard: T_pwm = PERIOD_MANT × 2^PERIOD_EXP × (1/f_sys)
Low Freq: T_pwm = PERIOD_MANT × 8 × 2^PERIOD_EXP × (1/f_sys)

Where f_sys ≈ 8 MHz (internal clock)
```

**Example Calculations**:

| MANT | EXP | Low Freq | Frequency | Period |
|------|-----|----------|-----------|--------|
| 100 | 0 | No | 80 kHz | 12.5 µs |
| 100 | 4 | No | 5 kHz | 200 µs |
| 50 | 6 | Yes | 312.5 Hz | 3.2 ms |

## Dither Support

### Dither Waveform

```text
    Current vs Time (Dither Enabled):

    I_max  ┐     ╱▔▔▔▔▔▔▔╲     ╱▔▔▔▔▔▔▔╲
           │   ╱            ╲ ╱            ╲
    I_set  ├──────────────────O──────────────────
           │                 ╲ ╱            ╲ ╱
    I_min  ┘                  ╲___________╱
           └────────────────────────────────────▶ Time
           ◄─────  T_dither  ─────▶

    Components:
    - Steps: Number of increments in quarter period
    - Flat: Hold time at peak/valley
    - Step Size: Amplitude of each increment
```

### Configuration Registers

1. **DITHER_CLK_DIV**: Reference clock period
2. **DITHER_STEP**: Steps and flat period
3. **DITHER_CTRL**: Step size and control

**Dither Amplitude**:

```text
I_dither = STEPS × STEP_SIZE × 2A / 32767
```

**Dither Period**:

```text
T_dither = [4×STEPS + 2×FLAT] × t_ref_clk
```

### Dither Use Cases

| Application | Configuration | Purpose |
|-------------|---------------|---------|
| Solenoid Positioning | Small steps, high freq | Precise position control |
| Valve Control | Medium steps, low freq | Smooth flow transitions |
| Noise Reduction | Random steps | Break up acoustic noise |

## Integrator Control

### INTEGRATOR_LIMIT Register

```text
Bits: 15 14              10 9              0
     ┌──┬─────────────────┬─────────────────┐
     │Rs│AUTO_LIM_VALUE   │  LIM_VALUE      │
     │  │     _ABS        │    _ABS         │
     └──┴─────────────────┴─────────────────┘
```

**Purpose**:
- Prevents integrator windup
- Faster settling after setpoint changes
- Two limits: Normal and Auto (after setpoint change)

**Recommended Values**:
- Normal operation: 0x3FF (maximum)
- Fast response: 0x1FF (reduced)
- Auto-limit: 0x0FF (aggressive)

## Current Measurement

### Feedback Mechanism

```text
    Measurement Path:

    Load Current ──▶ Sense ──▶ ADC ──▶ Filter ──▶ FB_I_AVG
                       │
                       ├──▶ Min/Max ──▶ FB_IMIN_IMAX
                       │
                       └──▶ DC Calc ──▶ FB_DC
```

### Feedback Registers (Per Channel)

| Register | Content | Update Rate |
|----------|---------|-------------|
| FB_I_AVG | Average current over dither period | Dither period |
| FB_DC | PWM duty cycle (0-100%) | PWM period |
| FB_VBAT | Average VBAT | Configurable |
| FB_IMIN_IMAX | Min/Max current | Dither period |

## Parallel Operation

### Channel Pairing

```text
    Available Pairs:

    ┌─────────┬─────────┐
    │  CH0    │   CH3   │ ◀── Can be paralleled (4A)
    ├─────────┼─────────┤
    │  CH1    │   CH2   │ ◀── Can be paralleled (4A)
    ├─────────┼─────────┤
    │  CH4    │   CH5   │ ◀── Can be paralleled (4A)
    └─────────┴─────────┘
```

**Configuration**:
1. Enter Config Mode
2. Set CH_PAR_x_y bit in CH_CTRL
3. Configure both channels identically
4. Enter Mission Mode
5. Enable both channels

**Current Calculation** (Parallel):

```text
I_total = 4000mA × TARGET / 32767
Max: 4000 mA (4A)
```

## ICC Tuning Guidelines

### Response Time vs Stability

```text
    Fast Response          Stability
    (High Gains)           (Low Gains)

    ◄────────────────────────────────▶
    │         │          │           │
    Unstable  Aggressive  Balanced   Slow

    Recommended: Balanced (middle range)
```

### Parameter Recommendations

| Load Type | PWM_CTRL_PARAM | INT_THRESH | Response |
|-----------|----------------|------------|----------|
| Resistive | 0x8 | 0x100 | Fast |
| Inductive (low L) | 0x6 | 0x80 | Medium |
| Inductive (high L) | 0x4 | 0x40 | Slow |
| Solenoid | 0x5 | 0x60 | Balanced |

### Tuning Process

```text
1. Start with default values
   ├─ PWM_CTRL_PARAM = 0x6
   ├─ INT_THRESH = 0x80
   └─ INTEGRATOR_LIMIT = 0x3FF

2. Observe step response
   └─ Apply setpoint step change

3. Adjust based on behavior
   ├─ Oscillation? → Reduce PWM_CTRL_PARAM
   ├─ Too slow? → Increase PWM_CTRL_PARAM
   └─ Overshoot? → Reduce INT_THRESH

4. Fine-tune integrator limits
   └─ Reduce for faster settling
```

---

**Navigation**: [← Register Map](03_Register_Map.md) | [Next: Channel Modes →](05_Channel_Modes.md)
