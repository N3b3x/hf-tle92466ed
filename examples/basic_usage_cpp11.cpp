/**
 * @file basic_usage_cpp11.cpp
 * @brief Basic usage example for TLE92466ED driver - C++11 compatible
 * @author AI Generated Driver
 * @date 2025-10-20
 * @version 2.0.0
 *
 * @details
 * Demonstrates basic initialization and current control of the TLE92466ED IC.
 * This example shows how to use the Integrated Current Control (ICC) mode
 * to drive solenoid loads with precise current regulation.
 */

#include "TLE92466ED_Cpp11.hpp"
#include "example_hal_cpp11.hpp"
#include <iostream>
#include <cstdint>

using namespace TLE92466ED;

/**
 * @brief Basic usage example - Current Control Mode
 */
int main() {
    std::cout << "========================================\n";
    std::cout << "TLE92466ED Driver - Basic Usage Example (C++11)\n";
    std::cout << "========================================\n\n";

    // 1. Create HAL instance for your platform
    std::cout << "Creating HAL instance...\n";
    ExampleHAL hal;

    // 2. Create driver instance
    std::cout << "Creating driver instance...\n";
    Driver driver(hal);

    // 3. Initialize driver (device starts in Config Mode)
    std::cout << "\nInitializing driver...\n";
    DriverError error;
    if (!driver.init(&error)) {
        std::cerr << "ERROR: Driver initialization failed! Error: " << static_cast<int>(error) << "\n";
        return 1;
    }
    std::cout << "✓ Driver initialized successfully (Config Mode)\n";

    // 4. Configure global settings
    std::cout << "\nConfiguring global settings...\n";
    GlobalConfig global_config;
    global_config.crc_enabled = true;
    global_config.spi_watchdog_enabled = true;
    global_config.clock_watchdog_enabled = true;
    global_config.vio_5v = false;  // 3.3V logic
    global_config.vbat_uv_threshold = 43;   // ~7V (43 * 0.16208V)
    global_config.vbat_ov_threshold = 247;  // ~40V
    global_config.spi_watchdog_reload = 1000;

    if (!driver.configure_global(global_config, &error)) {
        std::cerr << "ERROR: Global configuration failed! Error: " << static_cast<int>(error) << "\n";
        return 1;
    }
    std::cout << "✓ Global configuration complete\n";

    // 5. Configure Channel 0 for ICC current control
    std::cout << "\nConfiguring Channel 0...\n";
    ChannelConfig ch0_config;
    ch0_config.mode = ChannelMode::ICC;        // Integrated Current Control
    ch0_config.current_setpoint_ma = 1500;     // 1.5A setpoint
    ch0_config.slew_rate = SlewRate::MEDIUM_2V5_US;
    ch0_config.diag_current = DiagCurrent::I_190UA;
    ch0_config.open_load_threshold = 3;        // 3/8 of setpoint
    ch0_config.pwm_period_mantissa = 100;
    ch0_config.pwm_period_exponent = 4;
    ch0_config.auto_limit_disabled = false;
    ch0_config.olsg_warning_enabled = true;
    ch0_config.deep_dither_enabled = false;
    ch0_config.dither_step_size = 0;  // No dither for this example
    ch0_config.dither_steps = 0;
    ch0_config.dither_flat = 0;

    if (!driver.configure_channel(Channel::CH0, ch0_config, &error)) {
        std::cerr << "ERROR: Channel 0 configuration failed! Error: " << static_cast<int>(error) << "\n";
        return 1;
    }
    std::cout << "✓ Channel 0 configured (ICC mode, 1.5A setpoint)\n";

    // 6. Enter Mission Mode to enable channel control
    std::cout << "\nEntering Mission Mode...\n";
    if (!driver.enter_mission_mode(&error)) {
        std::cerr << "ERROR: Failed to enter Mission Mode! Error: " << static_cast<int>(error) << "\n";
        return 1;
    }
    std::cout << "✓ Mission Mode active - channels can now be enabled\n";

    // 7. Enable Channel 0
    std::cout << "\nEnabling Channel 0...\n";
    if (!driver.enable_channel(Channel::CH0, true, &error)) {
        std::cerr << "ERROR: Failed to enable Channel 0! Error: " << static_cast<int>(error) << "\n";
        return 1;
    }
    std::cout << "✓ Channel 0 enabled - current regulation active at 1.5A\n";

    // 8. Monitor for a few seconds
    std::cout << "\nMonitoring for 5 seconds...\n";
    for (int i = 0; i < 5; ++i) {
        // Simple delay (replace with platform-specific delay)
        for (volatile int j = 0; j < 1000000; ++j) {
            // Busy wait delay
        }

        // Reload SPI watchdog
        (void)driver.reload_spi_watchdog(1000, nullptr);

        // Get average current
        uint16_t current;
        if (driver.get_average_current(Channel::CH0, &current, false, &error)) {
            std::cout << "  [" << i+1 << "s] Average current: " << current << " mA\n";
        }

        // Check for faults
        ChannelDiagnostics diag;
        if (driver.get_channel_diagnostics(Channel::CH0, &diag, &error)) {
            if (diag.overcurrent) {
                std::cout << "  WARNING: Over-current detected!\n";
            }
            if (diag.open_load) {
                std::cout << "  WARNING: Open load detected!\n";
            }
            if (diag.short_to_ground) {
                std::cout << "  ERROR: Short to ground detected!\n";
                break;
            }
        }
    }

    // 9. Demonstrate setpoint change
    std::cout << "\nChanging setpoint to 1.0A...\n";
    if (!driver.set_current_setpoint(Channel::CH0, 1000, false, &error)) {
        std::cerr << "ERROR: Failed to set current! Error: " << static_cast<int>(error) << "\n";
    } else {
        std::cout << "✓ Setpoint updated to 1.0A\n";
        // Simple delay
        for (volatile int j = 0; j < 2000000; ++j) {
            // Busy wait delay
        }
    }

    // 10. Check global device status
    std::cout << "\nChecking device status...\n";
    DeviceStatus status;
    if (driver.get_device_status(&status, &error)) {
        std::cout << "  Device Status:\n";
        std::cout << "    Mode: " << (status.config_mode ? "Config" : "Mission") << "\n";
        std::cout << "    Init Done: " << (status.init_done ? "YES" : "NO") << "\n";
        std::cout << "    Any Fault: " << (status.any_fault ? "YES" : "NO") << "\n";
        std::cout << "    VBAT UV: " << (status.vbat_uv ? "YES" : "NO") << "\n";
        std::cout << "    VBAT OV: " << (status.vbat_ov ? "YES" : "NO") << "\n";
        std::cout << "    OT Warning: " << (status.ot_warning ? "YES" : "NO") << "\n";
        std::cout << "    OT Error: " << (status.ot_error ? "YES" : "NO") << "\n";
        std::cout << "    SPI WD Error: " << (status.spi_wd_error ? "YES" : "NO") << "\n";
    }

    // 11. Get voltage readings
    uint16_t vbat;
    if (driver.get_vbat_voltage(&vbat, &error)) {
        std::cout << "  VBAT: " << vbat << " (raw value)\n";
    }

    // 12. Disable channel
    std::cout << "\nDisabling Channel 0...\n";
    if (!driver.enable_channel(Channel::CH0, false, &error)) {
        std::cerr << "ERROR: Failed to disable Channel 0! Error: " << static_cast<int>(error) << "\n";
    } else {
        std::cout << "✓ Channel 0 disabled\n";
    }

    // 13. Return to config mode
    std::cout << "\nReturning to Config Mode...\n";
    if (!driver.enter_config_mode(&error)) {
        std::cerr << "ERROR: Failed to enter Config Mode! Error: " << static_cast<int>(error) << "\n";
    } else {
        std::cout << "✓ Config Mode active\n";
    }

    std::cout << "\n========================================\n";
    std::cout << "Example completed successfully!\n";
    std::cout << "========================================\n";
    
    return 0;
}