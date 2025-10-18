/**
 * @file basic_usage.cpp
 * @brief Basic usage example for TLE92466ED driver
 * @author AI Generated Driver
 * @date 2025-10-18
 *
 * @details
 * Demonstrates basic initialization and control of the TLE92466ED IC.
 */

#include "TLE92466ED.hpp"
#include "example_hal.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace TLE92466ED;
using namespace std::chrono_literals;

/**
 * @brief Basic usage example
 */
int main() {
    std::cout << "TLE92466ED Driver - Basic Usage Example\n";
    std::cout << "=========================================\n\n";

    // 1. Create HAL instance for your platform
    ExampleHAL hal;

    // 2. Create driver instance
    Driver driver(hal);

    // 3. Initialize driver
    std::cout << "Initializing driver...\n";
    auto init_result = driver.init();
    if (!init_result) {
        std::cerr << "ERROR: Driver initialization failed!\n";
        return 1;
    }
    std::cout << "Driver initialized successfully!\n\n";

    // 4. Configure global settings
    std::cout << "Configuring global settings...\n";
    GlobalConfig global_config{
        .diagnostics_enabled = true,
        .pwm_mode = false,
        .current_limit_enabled = true,
        .over_temp_enabled = true,
        .under_voltage_enabled = true,
        .over_voltage_enabled = true,
        .open_load_enabled = true,
        .short_circuit_enabled = true,
        .pwm_frequency = PWMFrequency::FREQ_1KHZ,
        .pwm_dithering = false
    };

    if (auto result = driver.configure_global(global_config); !result) {
        std::cerr << "ERROR: Global configuration failed!\n";
        return 1;
    }
    std::cout << "Global configuration complete!\n\n";

    // 5. Configure individual channels
    std::cout << "Configuring channels...\n";
    ChannelConfig channel_config{
        .current_limit = CurrentLimit::LIMIT_2A,
        .slew_rate = SlewRate::MEDIUM,
        .pwm_enabled = false,
        .diagnostics_enabled = true,
        .inverted = false
    };

    for (int i = 0; i < 6; ++i) {
        auto ch = static_cast<Channel>(i);
        if (auto result = driver.configure_channel(ch, channel_config); !result) {
            std::cerr << "ERROR: Channel " << i << " configuration failed!\n";
            return 1;
        }
        std::cout << "  Channel " << i << " configured\n";
    }
    std::cout << "\n";

    // 6. Control outputs
    std::cout << "Testing output control...\n";

    // Turn on channel 0
    std::cout << "  Turning ON channel 0...\n";
    if (auto result = driver.set_channel(Channel::CH0, true); !result) {
        std::cerr << "ERROR: Failed to control channel 0!\n";
        return 1;
    }

    std::this_thread::sleep_for(1s);

    // Turn on channel 1
    std::cout << "  Turning ON channel 1...\n";
    driver.set_channel(Channel::CH1, true);

    std::this_thread::sleep_for(1s);

    // Turn on all channels
    std::cout << "  Turning ON all channels...\n";
    driver.enable_all_channels();

    std::this_thread::sleep_for(2s);

    // 7. Check status
    std::cout << "\nChecking device status...\n";
    if (auto status = driver.get_device_status()) {
        std::cout << "  Device Status:\n";
        std::cout << "    Any Fault: " << (status->any_fault ? "YES" : "NO") << "\n";
        std::cout << "    Over-Temp Warning: " << (status->over_temp_warning ? "YES" : "NO") << "\n";
        std::cout << "    Under-Voltage: " << (status->under_voltage ? "YES" : "NO") << "\n";
        std::cout << "    Over-Voltage: " << (status->over_voltage ? "YES" : "NO") << "\n";
    }

    // Check individual channel status
    std::cout << "\nChecking channel statuses...\n";
    for (int i = 0; i < 6; ++i) {
        auto ch = static_cast<Channel>(i);
        if (auto status = driver.get_channel_status(ch)) {
            std::cout << "  Channel " << i << ":\n";
            std::cout << "    Enabled: " << (status->enabled ? "YES" : "NO") << "\n";
            std::cout << "    Active: " << (status->active ? "YES" : "NO") << "\n";
            std::cout << "    Fault: " << (status->has_fault ? "YES" : "NO") << "\n";
            if (status->has_fault) {
                std::cout << "    - Open Load (ON): " << (status->open_load_on ? "YES" : "NO") << "\n";
                std::cout << "    - Open Load (OFF): " << (status->open_load_off ? "YES" : "NO") << "\n";
                std::cout << "    - Short to VBAT: " << (status->short_to_vbat ? "YES" : "NO") << "\n";
                std::cout << "    - Short to GND: " << (status->short_to_gnd ? "YES" : "NO") << "\n";
            }
        }
    }

    // 8. Turn off all channels
    std::cout << "\nTurning OFF all channels...\n";
    driver.disable_all_channels();

    std::cout << "\nExample completed successfully!\n";
    return 0;
}
