---
layout: default
title: "💻 Usage Examples"
description: "Practical implementation examples and code samples for common use cases"
nav_order: 9
parent: "📚 Documentation"
permalink: /docs/09_Usage_Examples/
---

# Usage Examples

## Complete Application Examples

This document provides practical, ready-to-use code examples for common TLE92466ED applications.

## Example 1: Basic Current Control

### Single Channel ICC Mode

```cpp
#include "TLE92466ED.hpp"
#include "my_platform_hal.hpp"

using namespace tle92466ed;

int main() {
    // 1. Create HAL and driver
    MyPlatformHAL hal;
    Driver driver(hal);
    
    // 2. Initialize
    if (auto result = driver.init(); !result) {
        error("Init failed");
        return -1;
    }
    
    // 3. Configure channel (in Config Mode by default)
    driver.set_channel_mode(Channel::CH0, ChannelMode::ICC);
    driver.set_current_setpoint(Channel::CH0, 1500); // 1.5A
    
    // 4. Enter Mission Mode
    driver.enter_mission_mode();
    
    // 5. Enable channel
    driver.enable_channel(Channel::CH0, true);
    
    // 6. Monitor
    while (running) {
        auto current = driver.get_average_current(Channel::CH0);
        if (current) {
            log("Current: %d mA", *current);
        }
        
        delay(100ms);
    }
    
    // 7. Cleanup
    driver.disable_all_channels();
    
    return 0;
}
```

## Example 2: Parallel Operation (High Current)

### 4A Solenoid Control

```cpp
void configure_high_current_solenoid() {
    Driver driver(hal);
    driver.init();
    
    // Enter Config Mode
    driver.enter_config_mode();
    
    // Enable parallel operation CH0+CH3
    driver.set_parallel_operation(ParallelPair::CH0_CH3, true);
    
    // Configure both channels identically
    ChannelConfig config{
        .mode = ChannelMode::ICC,
        .current_setpoint_ma = 3500,  // 3.5A total
        .slew_rate = SlewRate::MEDIUM_2V5_US,
        .open_load_threshold = 4  // 4/8 = 50%
    };
    
    driver.configure_channel(Channel::CH0, config);
    driver.configure_channel(Channel::CH3, config);
    
    // Enter Mission Mode and enable
    driver.enter_mission_mode();
    driver.enable_channel(Channel::CH0, true);
    driver.enable_channel(Channel::CH3, true);
    
    log("High-current mode active: 3.5A");
}
```

## Example 3: Proportional Valve with Dither

### Precision Position Control

```cpp
class ProportionalValveController {
public:
    ProportionalValveController(Driver& driver, Channel channel)
        : driver_(driver), channel_(channel) {}
    
    void init() {
        driver_.enter_config_mode();
        
        // Configure ICC mode
        driver_.set_channel_mode(channel_, ChannelMode::ICC);
        
        // Configure dither for smooth positioning
        // 100mA amplitude, 50Hz frequency
        driver_.configure_dither(channel_,
            200,   // step_size (gives ~100mA amplitude)
            25,    // num_steps
            10);   // flat_steps
        
        // Set initial position (50% = 1000mA)
        driver_.set_current_setpoint(channel_, 1000);
        
        driver_.enter_mission_mode();
        driver_.enable_channel(channel_, true);
    }
    
    void set_position(float percent) {
        // Convert 0-100% to 0-2000mA
        uint16_t current = static_cast<uint16_t>(percent * 20.0f);
        driver_.set_current_setpoint(channel_, current);
    }
    
    float get_actual_position() {
        auto current = driver_.get_average_current(channel_);
        if (!current) return -1.0f;
        return *current / 20.0f;  // Convert to %
    }
    
private:
    Driver& driver_;
    Channel channel_;
};

// Usage
int main() {
    MyPlatformHAL hal;
    Driver driver(hal);
    driver.init();
    
    ProportionalValveController valve(driver, Channel::CH0);
    valve.init();
    
    // Ramp to 75% position
    for (float pos = 0.0f; pos <= 75.0f; pos += 1.0f) {
        valve.set_position(pos);
        delay(10ms);
    }
    
    // Monitor position
    float actual = valve.get_actual_position();
    log("Position: %.1f%%", actual);
}
```

## Example 4: Multi-Channel Solenoid Array

### Valve Bank Control

```cpp
class SolenoidArray {
public:
    SolenoidArray(Driver& driver) : driver_(driver) {}
    
    void init() {
        driver_.enter_config_mode();
        
        // Configure all 6 channels
        for (int i = 0; i < 6; i++) {
            Channel ch = static_cast<Channel>(i);
            
            driver_.set_channel_mode(ch, ChannelMode::ICC);
            
            ChannelConfig config{
                .mode = ChannelMode::ICC,
                .current_setpoint_ma = 1200,  // 1.2A per solenoid
                .slew_rate = SlewRate::FAST_5V0_US,
                .open_load_threshold = 3
            };
            driver_.configure_channel(ch, config);
        }
        
        driver_.enter_mission_mode();
    }
    
    void activate_pattern(uint8_t pattern) {
        // Pattern is 6-bit mask for channels
        for (int i = 0; i < 6; i++) {
            bool enable = (pattern & (1 << i)) != 0;
            driver_.enable_channel(static_cast<Channel>(i), enable);
        }
    }
    
    void sequence_test() {
        // Test each channel sequentially
        for (int i = 0; i < 6; i++) {
            log("Testing channel %d", i);
            activate_pattern(1 << i);
            delay(500ms);
            
            auto diag = driver_.get_channel_diagnostics(static_cast<Channel>(i));
            if (diag) {
                if (diag->overcurrent) log("  OC fault!");
                if (diag->open_load) log("  Open load!");
                log("  Current: %d mA", diag->average_current);
            }
        }
        activate_pattern(0);  // All off
    }
    
private:
    Driver& driver_;
};

// Usage
int main() {
    MyPlatformHAL hal;
    Driver driver(hal);
    driver.init();
    
    SolenoidArray array(driver);
    array.init();
    
    // Run sequence test
    array.sequence_test();
    
    // Activate specific pattern
    array.activate_pattern(0b00101010);  // CH1, CH3, CH5
}
```

## Example 5: Fault-Tolerant Operation

### Robust Error Handling

```cpp
class FaultTolerantDriver {
public:
    FaultTolerantDriver(Driver& driver) : driver_(driver), fault_count_(0) {}
    
    bool init_with_retry(int max_retries = 3) {
        for (int attempt = 1; attempt <= max_retries; attempt++) {
            log("Init attempt %d/%d", attempt, max_retries);
            
            if (auto result = driver_.init(); result) {
                log("Init successful");
                start_monitoring();
                return true;
            } else {
                log("Init failed: %d", static_cast<int>(result.error()));
                delay(1s);
            }
        }
        log("Init failed after %d attempts", max_retries);
        return false;
    }
    
    void monitor_and_recover() {
        auto status = driver_.get_device_status();
        if (!status) return;
        
        // Check for critical faults
        if (status->ot_error) {
            handle_overtemperature();
        }
        
        if (status->vbat_uv || status->vbat_ov) {
            handle_supply_fault();
        }
        
        // Check each channel
        for (int i = 0; i < 6; i++) {
            Channel ch = static_cast<Channel>(i);
            auto diag = driver_.get_channel_diagnostics(ch);
            
            if (diag) {
                if (diag->overcurrent) {
                    handle_overcurrent(ch);
                }
                if (diag->short_to_ground) {
                    handle_short_circuit(ch);
                }
                if (diag->open_load) {
                    handle_open_load(ch);
                }
            }
        }
    }
    
private:
    Driver& driver_;
    int fault_count_;
    
    void handle_overtemperature() {
        log("Over-temperature detected");
        
        // Reduce all currents by 50%
        for (int i = 0; i < 6; i++) {
            Channel ch = static_cast<Channel>(i);
            auto current = driver_.get_current_setpoint(ch);
            if (current) {
                driver_.set_current_setpoint(ch, *current / 2);
            }
        }
        
        // Clear fault after cooling
        delay(10s);
        driver_.clear_faults();
    }
    
    void handle_overcurrent(Channel ch) {
        log("Over-current on channel %d", static_cast<int>(ch));
        
        // Disable channel
        driver_.enable_channel(ch, false);
        
        // Reduce setpoint
        auto current = driver_.get_current_setpoint(ch);
        if (current && *current > 500) {
            uint16_t new_current = *current - 200;  // Reduce by 200mA
            driver_.set_current_setpoint(ch, new_current);
        }
        
        // Clear fault and retry
        delay(100ms);
        driver_.clear_faults();
        driver_.enable_channel(ch, true);
    }
    
    void handle_short_circuit(Channel ch) {
        log("Short circuit on channel %d", static_cast<int>(ch));
        
        // Permanent disable
        driver_.enable_channel(ch, false);
        
        fault_count_++;
        if (fault_count_ > 3) {
            log("Too many faults, entering safe mode");
            driver_.disable_all_channels();
        }
    }
    
    void handle_open_load(Channel ch) {
        log("Open load on channel %d", static_cast<int>(ch));
        
        // Check if transient
        delay(50ms);
        auto diag = driver_.get_channel_diagnostics(ch);
        if (diag && !diag->open_load) {
            log("Transient open load, continuing");
            return;
        }
        
        // Persistent open load - disable
        driver_.enable_channel(ch, false);
    }
    
    void handle_supply_fault() {
        log("Supply voltage fault");
        
        // Enter safe state
        driver_.disable_all_channels();
        
        // Wait for supply to stabilize
        delay(1s);
        
        // Re-enable if supply recovered
        auto status = driver_.get_device_status();
        if (status && !status->vbat_uv && !status->vbat_ov) {
            log("Supply recovered");
            driver_.clear_faults();
        }
    }
    
    void start_monitoring() {
        // Start periodic monitoring task
        // (platform-specific implementation)
    }
};
```

## Example 6: Automotive Application

### Engine Management Actuators

```cpp
class EngineActuatorControl {
public:
    enum class Actuator {
        VVT_INTAKE,      // Variable valve timing (CH0)
        VVT_EXHAUST,     // Variable valve timing (CH1)
        EGR_VALVE,       // EGR control (CH2)
        WASTEGATE,       // Turbo wastegate (CH3)
        FUEL_PUMP,       // High-pressure fuel pump (CH4+CH5 parallel)
    };
    
    EngineActuatorControl(Driver& driver) : driver_(driver) {}
    
    void init() {
        driver_.enter_config_mode();
        
        // Configure VVT actuators (proportional, with dither)
        configure_vvt(Channel::CH0);
        configure_vvt(Channel::CH1);
        
        // Configure EGR valve (proportional)
        ChannelConfig egr_config{
            .mode = ChannelMode::ICC,
            .current_setpoint_ma = 0,  // Start closed
            .slew_rate = SlewRate::MEDIUM_2V5_US,
            .open_load_threshold = 4
        };
        driver_.configure_channel(Channel::CH2, egr_config);
        
        // Configure wastegate (fast response)
        ChannelConfig wg_config{
            .mode = ChannelMode::ICC,
            .current_setpoint_ma = 0,
            .slew_rate = SlewRate::FASTEST_10V0_US,
            .open_load_threshold = 3
        };
        driver_.configure_channel(Channel::CH3, wg_config);
        
        // Configure fuel pump (parallel, high current)
        driver_.set_parallel_operation(ParallelPair::CH4_CH5, true);
        ChannelConfig pump_config{
            .mode = ChannelMode::ICC,
            .current_setpoint_ma = 3000,  // 3A
            .slew_rate = SlewRate::MEDIUM_2V5_US,
            .open_load_threshold = 5
        };
        driver_.configure_channel(Channel::CH4, pump_config);
        driver_.configure_channel(Channel::CH5, pump_config);
        
        driver_.enter_mission_mode();
        
        // Enable critical actuators
        driver_.enable_channel(Channel::CH4, true);  // Fuel pump
        driver_.enable_channel(Channel::CH5, true);
    }
    
    void set_vvt_position(Actuator actuator, float degrees) {
        Channel ch = (actuator == Actuator::VVT_INTAKE) ? Channel::CH0 : Channel::CH1;
        
        // Convert degrees to current (example: 0-50° = 500-1500mA)
        uint16_t current = 500 + static_cast<uint16_t>(degrees * 20.0f);
        driver_.set_current_setpoint(ch, current);
    }
    
    void set_egr_opening(float percent) {
        // 0-100% = 0-1800mA
        uint16_t current = static_cast<uint16_t>(percent * 18.0f);
        driver_.set_current_setpoint(Channel::CH2, current);
        
        if (percent > 0) {
            driver_.enable_channel(Channel::CH2, true);
        }
    }
    
private:
    Driver& driver_;
    
    void configure_vvt(Channel ch) {
        ChannelConfig config{
            .mode = ChannelMode::ICC,
            .current_setpoint_ma = 1000,  // Mid position
            .slew_rate = SlewRate::MEDIUM_2V5_US,
            .open_load_threshold = 4,
            .deep_dither_enabled = true,
            .dither_step_size = 50,  // Small dither for positioning
            .dither_steps = 10,
            .dither_flat = 5
        };
        driver_.configure_channel(ch, config);
    }
};
```

## Example 7: Diagnostic Dashboard

### System Health Monitoring

```cpp
void print_system_status(Driver& driver) {
    std::cout << "\n=== TLE92466ED System Status ===\n";
    
    // Global status
    auto status = driver.get_device_status();
    if (status) {
        std::cout << "Mode: " << (status->config_mode ? "Config" : "Mission") << "\n";
        std::cout << "Init Done: " << (status->init_done ? "Yes" : "No") << "\n";
        std::cout << "Faults: " << (status->any_fault ? "YES" : "No") << "\n";
        
        if (status->any_fault) {
            std::cout << "  VBAT UV: " << status->vbat_uv << "\n";
            std::cout << "  VBAT OV: " << status->vbat_ov << "\n";
            std::cout << "  OT Warn: " << status->ot_warning << "\n";
            std::cout << "  OT Error: " << status->ot_error << "\n";
        }
    }
    
    // Channel status
    std::cout << "\nChannel Status:\n";
    std::cout << "CH | Setpt | Actual | DC%  | Faults\n";
    std::cout << "---+-------+--------+------+--------\n";
    
    for (int i = 0; i < 6; i++) {
        Channel ch = static_cast<Channel>(i);
        
        auto setpoint = driver.get_current_setpoint(ch);
        auto actual = driver.get_average_current(ch);
        auto dc = driver.get_duty_cycle(ch);
        auto diag = driver.get_channel_diagnostics(ch);
        
        std::cout << " " << i << " | ";
        std::cout << (setpoint ? std::to_string(*setpoint) : "ERR") << " | ";
        std::cout << (actual ? std::to_string(*actual) : "ERR") << " | ";
        std::cout << (dc ? std::to_string(*dc / 655) : "ERR") << " | ";
        
        if (diag) {
            if (diag->overcurrent) std::cout << "OC ";
            if (diag->short_to_ground) std::cout << "SG ";
            if (diag->open_load) std::cout << "OL ";
        }
        std::cout << "\n";
    }
    
    std::cout << "================================\n";
}
```

## Practical Tips

### 1. Always Check Return Values

```cpp
// Bad
driver.init();
driver.enable_channel(Channel::CH0, true);

// Good
if (auto result = driver.init(); !result) {
    handle_error(result.error());
    return;
}
```

### 2. Service Watchdogs

```cpp
// In main loop
while (running) {
    driver.reload_spi_watchdog(1000);
    // ... other operations
    delay(50ms);  // Must be < watchdog timeout
}
```

### 3. Monitor Before Critical Operations

```cpp
// Before enabling high-current load
auto status = driver.get_device_status();
if (status && !status->vbat_uv && !status->ot_error) {
    driver.enable_channel(Channel::CH0, true);
} else {
    log("Cannot enable: system fault");
}
```

### 4. Graceful Shutdown

```cpp
void shutdown() {
    // Disable all outputs
    driver.disable_all_channels();
    
    // Return to safe state
    driver.enter_config_mode();
    
    // Device is now safe
}
```

---

**Navigation**: [← HAL Implementation](08_HAL_Implementation.md) | [Back to Index](00_INDEX.md)
