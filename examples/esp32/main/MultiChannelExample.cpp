/**
 * @file MultiChannelExample.cpp
 * @brief Multi-channel control example for TLE92466ED driver on ESP32-C6
 * 
 * This example demonstrates:
 * - Independent control of all 6 channels
 * - Current ramping and smooth transitions
 * - Channel synchronization
 * - Performance monitoring
 * - Advanced diagnostics
 * 
 * @author N3b3x
 * @date 2025-10-21
 * @version 2.0.0
 */

#include <stdio.h>
#include <memory>
#include <array>
#include <cmath>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "TLE92466ED.hpp"
#include "Esp32TleCommInterface.hpp"

using namespace TLE92466ED;

static const char* TAG = "TLE92466ED_Multi";

/**
 * @brief Initialize NVS (required for ESP32)
 */
static void initialize_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

/**
 * @brief Demonstrate sequential channel activation
 */
static void sequential_channel_demo(Driver& driver) {
    ESP_LOGI(TAG, "=== Sequential Channel Activation Demo ===");
    
    constexpr std::array<uint16_t, 6> currents = {200, 400, 600, 800, 1000, 1200};
    constexpr std::array<Channel, 6> channels = {
        Channel::CH0, Channel::CH1, Channel::CH2, 
        Channel::CH3, Channel::CH4, Channel::CH5
    };
    
    // Enable channels one by one
    for (size_t i = 0; i < 6; i++) {
        Channel channel = channels[i];
        ESP_LOGI(TAG, "Activating channel %d with %dmA...", static_cast<int>(channel), currents[i]);
        
        // Set current
        if (auto result = driver.SetCurrentSetpoint(channel, currents[i]); !result) {
            ESP_LOGE(TAG, "Failed to set current for channel %d", static_cast<int>(channel));
            continue;
        }
        
        // Enable channel
        if (auto result = driver.EnableChannel(channel, true); !result) {
            ESP_LOGE(TAG, "Failed to enable channel %d", static_cast<int>(channel));
            continue;
        }
        
        ESP_LOGI(TAG, "Channel %d active at %dmA", static_cast<int>(channel), currents[i]);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second between activations
    }
    
    // Monitor all channels for 5 seconds
    ESP_LOGI(TAG, "Monitoring all channels for 5 seconds...");
    for (int i = 0; i < 5; i++) {
        ESP_LOGI(TAG, "Status check %d/5:", i + 1);
        
        for (size_t j = 0; j < 6; j++) {
            Channel channel = channels[j];
            if (auto current = driver.GetAverageCurrent(channel); current) {
                ESP_LOGI(TAG, "  CH%d: %dmA", static_cast<int>(channel), *current);
            }
        }
        
        // Check diagnostics for first channel as example
        if (auto diag = driver.GetChannelDiagnostics(Channel::CH0); diag) {
            ESP_LOGI(TAG, "  ✅  Diagnostics read successfully");
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Disable all channels
    ESP_LOGI(TAG, "Disabling all channels...");
    for (size_t i = 0; i < 6; i++) {
        Channel channel = channels[i];
        if (auto result = driver.EnableChannel(channel, false); !result) {
            ESP_LOGE(TAG, "Failed to disable channel %d", static_cast<int>(channel));
        }
    }
}

/**
 * @brief Demonstrate synchronized channel control
 */
static void synchronized_channel_demo(Driver& driver) {
    ESP_LOGI(TAG, "=== Synchronized Channel Control Demo ===");
    
    // Set all channels to the same current
    constexpr uint16_t sync_current = 500; // 500mA
    constexpr std::array<Channel, 6> channels = {
        Channel::CH0, Channel::CH1, Channel::CH2, 
        Channel::CH3, Channel::CH4, Channel::CH5
    };
    
    ESP_LOGI(TAG, "Setting all channels to %dmA...", sync_current);
    for (Channel channel : channels) {
        if (auto result = driver.SetCurrentSetpoint(channel, sync_current); !result) {
            ESP_LOGE(TAG, "Failed to set current for channel %d", static_cast<int>(channel));
        }
    }
    
    // Enable all channels simultaneously
    ESP_LOGI(TAG, "Enabling all channels simultaneously...");
    for (Channel channel : channels) {
        if (auto result = driver.EnableChannel(channel, true); !result) {
            ESP_LOGE(TAG, "Failed to enable channel %d", static_cast<int>(channel));
        }
    }
    
    // Synchronized current ramping
    ESP_LOGI(TAG, "Synchronized current ramping (500mA -> 1500mA -> 500mA)...");
    
    // Ramp up
    for (uint16_t current = 500; current <= 1500; current += 100) {
        ESP_LOGI(TAG, "All channels: %dmA", current);
        for (Channel channel : channels) {
            if (auto result = driver.SetCurrentSetpoint(channel, current); !result) {
                ESP_LOGE(TAG, "Failed to set current for channel %d", static_cast<int>(channel));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(300)); // 300ms between steps
    }
    
    // Ramp down
    for (uint16_t current = 1500; current >= 500; current -= 100) {
        ESP_LOGI(TAG, "All channels: %dmA", current);
        for (Channel channel : channels) {
            if (auto result = driver.SetCurrentSetpoint(channel, current); !result) {
                ESP_LOGE(TAG, "Failed to set current for channel %d", static_cast<int>(channel));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(300)); // 300ms between steps
    }
    
    // Disable all channels
    ESP_LOGI(TAG, "Disabling all channels...");
    for (Channel channel : channels) {
        if (auto result = driver.EnableChannel(channel, false); !result) {
            ESP_LOGE(TAG, "Failed to disable channel %d", static_cast<int>(channel));
        }
    }
}

/**
 * @brief Demonstrate wave pattern generation
 */
static void wave_pattern_demo(Driver& driver) {
    ESP_LOGI(TAG, "=== Wave Pattern Generation Demo ===");
    
    // Create a sine wave pattern across channels
    constexpr int steps = 60; // 60 steps for smooth wave
    constexpr uint16_t base_current = 800; // Base current in mA
    constexpr uint16_t amplitude = 400; // Amplitude in mA
    constexpr std::array<Channel, 6> channels = {
        Channel::CH0, Channel::CH1, Channel::CH2, 
        Channel::CH3, Channel::CH4, Channel::CH5
    };
    
    ESP_LOGI(TAG, "Generating sine wave pattern across channels...");
    
    for (int step = 0; step < steps; step++) {
        float angle = (2.0f * M_PI * step) / steps;
        
        for (size_t i = 0; i < 6; i++) {
            Channel channel = channels[i];
            // Phase shift each channel by 60 degrees
            float channel_angle = angle + (i * M_PI / 3.0f);
            float sine_value = sin(channel_angle);
            
            // Calculate current (always positive)
            uint16_t current = base_current + (amplitude * (sine_value + 1.0f) / 2.0f);
            
            // Ensure current is within limits
            if (current > 2000) current = 2000;
            if (current < 100) current = 100;
            
            if (auto result = driver.SetCurrentSetpoint(channel, current); !result) {
                ESP_LOGE(TAG, "Failed to set current for channel %d", static_cast<int>(channel));
            }
        }
        
        // Enable all channels on first step
        if (step == 0) {
            for (Channel channel : channels) {
                if (auto result = driver.EnableChannel(channel, true); !result) {
                    ESP_LOGE(TAG, "Failed to enable channel %d", static_cast<int>(channel));
                }
            }
        }
        
        // Log current values every 10 steps
        if (step % 10 == 0) {
            ESP_LOGI(TAG, "Wave step %d/%d:", step, steps);
            for (Channel channel : channels) {
                if (auto current = driver.GetAverageCurrent(channel); current) {
                    ESP_LOGI(TAG, "  CH%d: %dmA", static_cast<int>(channel), *current);
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // 100ms between steps
    }
    
    // Disable all channels
    ESP_LOGI(TAG, "Disabling all channels...");
    for (Channel channel : channels) {
        if (auto result = driver.EnableChannel(channel, false); !result) {
            ESP_LOGE(TAG, "Failed to disable channel %d", static_cast<int>(channel));
        }
    }
}

/**
 * @brief Performance monitoring demonstration
 */
static void performance_monitoring_demo(Driver& driver) {
    ESP_LOGI(TAG, "=== Performance Monitoring Demo ===");
    
    constexpr int iterations = 100;
    constexpr Channel test_channel = Channel::CH0;
    constexpr uint16_t test_current = 1000;
    
    // Measure communication performance
    ESP_LOGI(TAG, "Measuring communication performance (%d iterations)...", iterations);
    
    int64_t start_time = esp_timer_get_time();
    int successful_operations = 0;
    
    for (int i = 0; i < iterations; i++) {
        // Set current
        if (auto result = driver.SetCurrentSetpoint(test_channel, test_current); result) {
            successful_operations++;
        }
        
        // Read current back
        if (auto current = driver.GetAverageCurrent(test_channel); current) {
            // Verify the value is close to what we set
            if (abs(static_cast<int>(*current) - static_cast<int>(test_current)) <= 10) { // Allow 10mA tolerance
                // Success
            } else {
                ESP_LOGW(TAG, "Current mismatch: set %dmA, read %dmA", test_current, *current);
            }
        }
    }
    
    int64_t end_time = esp_timer_get_time();
    int64_t total_time_us = end_time - start_time;
    
    ESP_LOGI(TAG, "Performance Results:");
    ESP_LOGI(TAG, "  Total time: %lld μs", total_time_us);
    ESP_LOGI(TAG, "  Average per operation: %lld μs", total_time_us / (iterations * 2));
    ESP_LOGI(TAG, "  Successful operations: %d/%d (%.1f%%)", 
             successful_operations, iterations, 
             (100.0f * successful_operations) / iterations);
    ESP_LOGI(TAG, "  Operations per second: %.1f", 
             (1000000.0f * iterations * 2) / total_time_us);
}

/**
 * @brief Main multi-channel demonstration
 */
static void multi_channel_demo() {
    ESP_LOGI(TAG, "=== TLE92466ED Multi-Channel Demo ===");
    
    // Create and initialize HAL
    auto hal = CreateEsp32TleCommInterface();
    if (auto result = hal->Init(); !result) {
        ESP_LOGE(TAG, "Failed to initialize HAL");
        return;
    }
    ESP_LOGI(TAG, "HAL initialized successfully");

    // Create TLE92466ED driver instance
    Driver driver(*hal);
    
    // Initialize the driver
    ESP_LOGI(TAG, "Initializing TLE92466ED driver...");
    if (auto result = driver.Init(); !result) {
        ESP_LOGE(TAG, "Failed to initialize TLE92466ED driver");
        return;
    }
    ESP_LOGI(TAG, "TLE92466ED driver initialized successfully");

    // Run demonstrations
    sequential_channel_demo(driver);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    synchronized_channel_demo(driver);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    wave_pattern_demo(driver);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    performance_monitoring_demo(driver);
    
    ESP_LOGI(TAG, "=== Multi-Channel Demo Complete ===");
}

/**
 * @brief Main application task
 */
extern "C" void app_main() {
    ESP_LOGI(TAG, "TLE92466ED Multi-Channel Example Starting...");
    ESP_LOGI(TAG, "ESP32-C6 Target, ESP-IDF %s", esp_get_idf_version());
    
    // Initialize NVS
    initialize_nvs();
    
    // Run the multi-channel demonstration
    multi_channel_demo();
    
    ESP_LOGI(TAG, "Demo completed. System will restart in 10 seconds...");
    vTaskDelay(pdMS_TO_TICKS(10000));
    esp_restart();
}
