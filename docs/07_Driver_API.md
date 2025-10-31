---
layout: default
title: "🔧 Driver API"
description: "C++23 driver interface reference with std::expected error handling"
nav_order: 7
parent: "📚 Documentation"
permalink: /docs/07_Driver_API/
---

# Driver API Reference

## Driver Class Overview

The `TLE92466ED::Driver` class provides the complete C++ interface for controlling the IC.

### Class Declaration

```cpp
namespace TLE92466ED {
    class Driver {
    public:
        explicit Driver(HAL& hal) noexcept;
        ~Driver() noexcept;
        
        // No copying, no moving (contains reference)
        Driver(const Driver&) = delete;
        Driver& operator=(const Driver&) = delete;
        Driver(Driver&&) = delete;
        Driver& operator=(Driver&&) = delete;
        
        // Initialization
        [[nodiscard]] DriverResult<void> init() noexcept;
        
        // Mode control
        [[nodiscard]] DriverResult<void> enter_mission_mode() noexcept;
        [[nodiscard]] DriverResult<void> enter_config_mode() noexcept;
        [[nodiscard]] bool is_mission_mode() const noexcept;
        
        // Channel control
        [[nodiscard]] DriverResult<void> enable_channel(Channel, bool) noexcept;
        [[nodiscard]] DriverResult<void> set_channel_mode(Channel, ChannelMode) noexcept;
        
        // Current control
        [[nodiscard]] DriverResult<void> set_current_setpoint(Channel, uint16_t, bool=false) noexcept;
        
        // Diagnostics
        [[nodiscard]] DriverResult<DeviceStatus> get_device_status() noexcept;
        [[nodiscard]] DriverResult<ChannelDiagnostics> get_channel_diagnostics(Channel) noexcept;
        
        // ... more methods
    };
}
```

## Core Types

### DriverError Enumeration

```cpp
enum class DriverError : uint8_t {
    None = 0,
    NotInitialized,
    HardwareError,
    InvalidChannel,
    InvalidParameter,
    DeviceNotResponding,
    WrongDeviceID,
    RegisterError,
    CRCError,
    FaultDetected,
    ConfigurationError,
    TimeoutError,
    WrongMode,
    SPIFrameError,
    WriteToReadOnly
};
```

### DriverResult Template

```cpp
template<typename T>
using DriverResult = std::expected<T, DriverError>;

// Usage:
DriverResult<void> init();  // Returns void or error
DriverResult<uint16_t> get_voltage();  // Returns value or error
```

### Channel Enumeration

```cpp
enum class Channel : uint8_t {
    CH0 = 0,
    CH1 = 1,
    CH2 = 2,
    CH3 = 3,
    CH4 = 4,
    CH5 = 5,
    COUNT = 6
};
```

### ChannelMode Enumeration

```cpp
enum class ChannelMode : uint8_t {
    OFF = 0x0,
    ICC = 0x1,
    DIRECT_DRIVE_SPI = 0x2,
    DIRECT_DRIVE_DRV0 = 0x3,
    DIRECT_DRIVE_DRV1 = 0x4,
    FREE_RUN_MEAS = 0xC
};
```

## Initialization Methods

### init()

Initialize driver and verify device.

```cpp
DriverResult<void> init() noexcept;
```

**Description**: Performs complete initialization sequence including HAL setup, device
verification, and default configuration.

**Returns**: Success or error code

**Example**:
```cpp
Driver driver(hal);
if (auto result = driver.init(); !result) {
    // Handle error
    log("Init failed: %d", static_cast<int>(result.error()));
    return;
}
```

**Sequence**:
1. Initialize HAL
2. Wait for power-up
3. Verify device ID
4. Apply default configuration
5. Clear power-on faults

## Mode Control Methods

### enter_mission_mode()

Transition to Mission Mode (enables channel control).

```cpp
DriverResult<void> enter_mission_mode() noexcept;
```

**Preconditions**: Driver initialized

**Example**:
```cpp
driver.enter_mission_mode();
// Channels can now be enabled
```

### enter_config_mode()

Transition to Config Mode (allows configuration).

```cpp
DriverResult<void> enter_config_mode() noexcept;
```

**Side Effects**: All channels automatically disabled

**Example**:
```cpp
driver.enter_config_mode();
// Configuration registers now writable
driver.set_channel_mode(Channel::CH0, ChannelMode::ICC);
```

## Channel Control Methods

### enable_channel()

Enable or disable a channel.

```cpp
DriverResult<void> enable_channel(Channel channel, bool enabled) noexcept;
```

**Parameters**:
- `channel`: Channel to control
- `enabled`: true=enable, false=disable

**Preconditions**: Must be in Mission Mode

**Example**:
```cpp
// Enable channel 0
driver.enable_channel(Channel::CH0, true);

// Disable channel 1
driver.enable_channel(Channel::CH1, false);
```

### set_channel_mode()

Configure channel operation mode.

```cpp
DriverResult<void> set_channel_mode(Channel channel, ChannelMode mode) noexcept;
```

**Preconditions**: Must be in Config Mode

**Example**:
```cpp
driver.enter_config_mode();
driver.set_channel_mode(Channel::CH0, ChannelMode::ICC);
driver.enter_mission_mode();
```

### set_parallel_operation()

Configure parallel channel operation.

```cpp
DriverResult<void> set_parallel_operation(ParallelPair pair, bool enabled) noexcept;
```

**Parameters**:
- `pair`: CH0_CH3, CH1_CH2, or CH4_CH5
- `enabled`: true=parallel, false=independent

**Preconditions**: Must be in Config Mode

**Example**:
```cpp
driver.enter_config_mode();
driver.set_parallel_operation(ParallelPair::CH0_CH3, true);
```

## Current Control Methods

### set_current_setpoint()

Set target current for a channel.

```cpp
DriverResult<void> set_current_setpoint(
    Channel channel,
    uint16_t current_ma,
    bool parallel_mode = false) noexcept;
```

**Parameters**:
- `channel`: Channel to configure
- `current_ma`: Desired current in milliamperes (0-2000 single, 0-4000 parallel)
- `parallel_mode`: true if channel is in parallel mode

**Example**:
```cpp
// Set 1.5A on single channel
driver.set_current_setpoint(Channel::CH0, 1500);

// Set 3.5A on parallel channels
driver.set_current_setpoint(Channel::CH0, 3500, true);
```

### get_current_setpoint()

Read configured current setpoint.

```cpp
DriverResult<uint16_t> get_current_setpoint(
    Channel channel,
    bool parallel_mode = false) noexcept;
```

**Returns**: Current in mA or error

**Example**:
```cpp
auto current = driver.get_current_setpoint(Channel::CH0);
if (current) {
    log("Setpoint: %d mA", *current);
}
```

### configure_pwm_period()

Configure PWM frequency for ICC.

```cpp
DriverResult<void> configure_pwm_period(
    Channel channel,
    uint8_t period_mantissa,
    uint8_t period_exponent,
    bool low_freq_range = false) noexcept;
```

**Example**:
```cpp
// Configure for ~5kHz PWM
driver.configure_pwm_period(Channel::CH0, 100, 4, false);
```

### configure_dither()

Configure current dither parameters.

```cpp
DriverResult<void> configure_dither(
    Channel channel,
    uint16_t step_size,
    uint8_t num_steps,
    uint8_t flat_steps) noexcept;
```

**Example**:
```cpp
// Add 200mA dither amplitude
driver.configure_dither(Channel::CH0, 100, 20, 5);
```

## Diagnostics Methods

### get_device_status()

Read global device status.

```cpp
DriverResult<DeviceStatus> get_device_status() noexcept;
```

**Returns**: DeviceStatus structure or error

**Example**:
```cpp
auto status = driver.get_device_status();
if (status) {
    if (status->any_fault) {
        log("Fault detected!");
    }
    if (status->ot_warning) {
        log("Over-temperature warning");
    }
}
```

### get_channel_diagnostics()

Read channel diagnostic information.

```cpp
DriverResult<ChannelDiagnostics> get_channel_diagnostics(Channel channel) noexcept;
```

**Returns**: ChannelDiagnostics structure or error

**Example**:
```cpp
auto diag = driver.get_channel_diagnostics(Channel::CH0);
if (diag) {
    if (diag->overcurrent) {
        log("OC fault on CH0");
    }
    log("Current: %d mA", diag->average_current);
}
```

### get_average_current()

Read measured average current.

```cpp
DriverResult<uint16_t> get_average_current(
    Channel channel,
    bool parallel_mode = false) noexcept;
```

**Returns**: Current in mA or error

### clear_faults()

Clear all latched fault flags.

```cpp
DriverResult<void> clear_faults() noexcept;
```

**Example**:
```cpp
driver.clear_faults();
```

## Configuration Methods

### configure_global()

Apply global configuration.

```cpp
DriverResult<void> configure_global(const GlobalConfig& config) noexcept;
```

**Preconditions**: Must be in Config Mode

**Example**:
```cpp
GlobalConfig config{
    .crc_enabled = true,
    .spi_watchdog_enabled = true,
    .vio_5v = false
};
driver.configure_global(config);
```

### configure_channel()

Apply complete channel configuration.

```cpp
DriverResult<void> configure_channel(
    Channel channel,
    const ChannelConfig& config) noexcept;
```

**Example**:
```cpp
ChannelConfig config{
    .mode = ChannelMode::ICC,
    .current_setpoint_ma = 1500,
    .slew_rate = SlewRate::MEDIUM_2V5_US
};
driver.configure_channel(Channel::CH0, config);
```

## Watchdog Methods

### reload_spi_watchdog()

Reload SPI watchdog counter.

```cpp
DriverResult<void> reload_spi_watchdog(uint16_t reload_value) noexcept;
```

**Usage**: Call periodically when SPI watchdog enabled

**Example**:
```cpp
// In main loop
while (running) {
    driver.reload_spi_watchdog(1000);
    // ... operations
    delay(50ms);
}
```

## Register Access Methods

### read_register()

Read 16-bit register with CRC.

```cpp
DriverResult<uint16_t> read_register(
    uint16_t address,
    bool verify_crc = true) noexcept;
```

**Example**:
```cpp
auto value = driver.read_register(0x0003); // GLOBAL_DIAG0
```

### write_register()

Write 16-bit register with CRC.

```cpp
DriverResult<void> write_register(
    uint16_t address,
    uint16_t value,
    bool verify_crc = true) noexcept;
```

### modify_register()

Read-modify-write register bits.

```cpp
DriverResult<void> modify_register(
    uint16_t address,
    uint16_t mask,
    uint16_t value) noexcept;
```

**Example**:
```cpp
// Set bit 0, clear bit 1
driver.modify_register(0x0000, 0x0003, 0x0001);
```

## Error Handling

### Using std::expected

```cpp
// Check for error
auto result = driver.init();
if (!result) {
    DriverError error = result.error();
    handle_error(error);
    return;
}

// Get value
auto current = driver.get_average_current(Channel::CH0);
if (current) {
    uint16_t value = *current;
    log("Current: %d mA", value);
} else {
    log("Error: %d", static_cast<int>(current.error()));
}
```

### Error Mapping

```cpp
const char* error_to_string(DriverError err) {
    switch (err) {
        case DriverError::None: return "No error";
        case DriverError::NotInitialized: return "Not initialized";
        case DriverError::HardwareError: return "Hardware error";
        // ... etc
    }
}
```

---

**Navigation**: [← Diagnostics](06_Diagnostics.md) | [Next: HAL Implementation →](08_HAL_Implementation.md)
