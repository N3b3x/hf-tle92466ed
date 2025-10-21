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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "TLE92466ED.hpp"
#include "ESP32C6_HAL.hpp"

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
static void sequential_channel_demo(TLE92466ED& driver) {
    ESP_LOGI(TAG, "=== Sequential Channel Activation Demo ===");
    
    constexpr std::array<uint16_t, 6> currents = {200, 400, 600, 800, 1000, 1200};
    
    // Enable channels one by one
    for (uint8_t channel = 0; channel < 6; channel++) {
        ESP_LOGI(TAG, "Activating channel %d with %dmA...", channel, currents[channel]);
        
        // Set current
        if (auto result = driver.setChannelCurrent(channel, currents[channel]); !result) {
            ESP_LOGE(TAG, "Failed to set current for channel %d", channel);
            continue;
        }
        
        // Enable channel
        if (auto result = driver.enableChannel(channel); !result) {
            ESP_LOGE(TAG, "Failed to enable channel %d", channel);
            continue;
        }
        
        ESP_LOGI(TAG, "Channel %d active at %dmA", channel, currents[channel]);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second between activations
    }
    
    // Monitor all channels for 5 seconds
    ESP_LOGI(TAG, "Monitoring all channels for 5 seconds...");
    for (int i = 0; i < 5; i++) {
        ESP_LOGI(TAG, "Status check %d/5:", i + 1);
        
        for (uint8_t channel = 0; channel < 6; channel++) {
            if (auto current = driver.readChannelCurrent(channel); current) {
                ESP_LOGI(TAG, "  CH%d: %dmA", channel, *current);
            }
        }
        
        // Check diagnostics
        if (auto diag = driver.readDiagnostics(); diag) {
            if (diag->hasAnyFault()) {
                ESP_LOGW(TAG, "  ⚠️  Faults detected!");
            } else {
                ESP_LOGI(TAG, "  ✅  All channels normal");
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Disable all channels
    ESP_LOGI(TAG, "Disabling all channels...");
    for (uint8_t channel = 0; channel < 6; channel++) {
        if (auto result = driver.disableChannel(channel); !result) {
            ESP_LOGE(TAG, "Failed to disable channel %d", channel);
        }
    }
}

/**
 * @brief Demonstrate synchronized channel control
 */
static void synchronized_channel_demo(TLE92466ED& driver) {
    ESP_LOGI(TAG, "=== Synchronized Channel Control Demo ===");
    
    // Set all channels to the same current
    constexpr uint16_t sync_current = 500; // 500mA
    
    ESP_LOGI(TAG, "Setting all channels to %dmA...", sync_current);
    for (uint8_t channel = 0; channel < 6; channel++) {
        if (auto result = driver.setChannelCurrent(channel, sync_current); !result) {
            ESP_LOGE(TAG, "Failed to set current for channel %d", channel);
        }
    }
    
    // Enable all channels simultaneously
    ESP_LOGI(TAG, "Enabling all channels simultaneously...");
    for (uint8_t channel = 0; channel < 6; channel++) {
        if (auto result = driver.enableChannel(channel); !result) {
            ESP_LOGE(TAG, "Failed to enable channel %d", channel);
        }
    }
    
    // Synchronized current ramping
    ESP_LOGI(TAG, "Synchronized current ramping (500mA -> 1500mA -> 500mA)...");
    
    // Ramp up
    for (uint16_t current = 500; current <= 1500; current += 100) {
        ESP_LOGI(TAG, "All channels: %dmA", current);
        for (uint8_t channel = 0; channel < 6; channel++) {
            if (auto result = driver.setChannelCurrent(channel, current); !result) {
                ESP_LOGE(TAG, "Failed to set current for channel %d", channel);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(300)); // 300ms between steps
    }
    
    // Ramp down
    for (uint16_t current = 1500; current >= 500; current -= 100) {
        ESP_LOGI(TAG, "All channels: %dmA", current);
        for (uint8_t channel = 0; channel < 6; channel++) {
            if (auto result = driver.setChannelCurrent(channel, current); !result) {
                ESP_LOGE(TAG, "Failed to set current for channel %d", channel);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(300)); // 300ms between steps
    }
    
    // Disable all channels
    ESP_LOGI(TAG, "Disabling all channels...");
    for (uint8_t channel = 0; channel < 6; channel++) {
        if (auto result = driver.disableChannel(channel); !result) {
            ESP_LOGE(TAG, "Failed to disable channel %d", channel);
        }
    }
}

/**
 * @brief Demonstrate wave pattern generation
 */
static void wave_pattern_demo(TLE92466ED& driver) {
    ESP_LOGI(TAG, "=== Wave Pattern Generation Demo ===");
    
    // Create a sine wave pattern across channels
    constexpr int steps = 60; // 60 steps for smooth wave
    constexpr uint16_t base_current = 800; // Base current in mA
    constexpr uint16_t amplitude = 400; // Amplitude in mA
    
    ESP_LOGI(TAG, "Generating sine wave pattern across channels...");
    
    for (int step = 0; step < steps; step++) {
        float angle = (2.0f * M_PI * step) / steps;
        
        for (uint8_t channel = 0; channel < 6; channel++) {
            // Phase shift each channel by 60 degrees
            float channel_angle = angle + (channel * M_PI / 3.0f);
            float sine_value = sin(channel_angle);
            
            // Calculate current (always positive)
            uint16_t current = base_current + (amplitude * (sine_value + 1.0f) / 2.0f);
            
            // Ensure current is within limits
            if (current > 2000) current = 2000;
            if (current < 100) current = 100;
            
            if (auto result = driver.setChannelCurrent(channel, current); !result) {
                ESP_LOGE(TAG, "Failed to set current for channel %d", channel);
            }
        }
        
        // Enable all channels on first step
        if (step == 0) {
            for (uint8_t channel = 0; channel < 6; channel++) {
                if (auto result = driver.enableChannel(channel); !result) {
                    ESP_LOGE(TAG, "Failed to enable channel %d", channel);
                }
            }
        }
        
        // Log current values every 10 steps
        if (step % 10 == 0) {
            ESP_LOGI(TAG, "Wave step %d/%d:", step, steps);
            for (uint8_t channel = 0; channel < 6; channel++) {
                if (auto current = driver.readChannelCurrent(channel); current) {
                    ESP_LOGI(TAG, "  CH%d: %dmA", channel, *current);
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // 100ms between steps
    }
    
    // Disable all channels
    ESP_LOGI(TAG, "Disabling all channels...");
    for (uint8_t channel = 0; channel < 6; channel++) {
        if (auto result = driver.disableChannel(channel); !result) {
            ESP_LOGE(TAG, "Failed to disable channel %d", channel);
        }
    }
}

/**
 * @brief Performance monitoring demonstration
 */
static void performance_monitoring_demo(TLE92466ED& driver) {
    ESP_LOGI(TAG, "=== Performance Monitoring Demo ===");
    
    constexpr int iterations = 100;
    constexpr uint8_t test_channel = 0;
    constexpr uint16_t test_current = 1000;
    
    // Measure communication performance
    ESP_LOGI(TAG, "Measuring communication performance (%d iterations)...", iterations);
    
    int64_t start_time = esp_timer_get_time();
    int successful_operations = 0;
    
    for (int i = 0; i < iterations; i++) {
        // Set current
        if (auto result = driver.setChannelCurrent(test_channel, test_current); result) {
            successful_operations++;
        }
        
        // Read current back
        if (auto current = driver.readChannelCurrent(test_channel); current) {
            // Verify the value is close to what we set
            if (abs(*current - test_current) <= 10) { // Allow 10mA tolerance
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
    auto hal = createTLE92466ED_HAL();
    if (auto result = hal->initialize(); !result) {
        ESP_LOGE(TAG, "Failed to initialize HAL");
        return;
    }
    ESP_LOGI(TAG, "HAL initialized successfully");

    // Create TLE92466ED driver instance
    TLE92466ED driver(*hal);
    
    // Initialize the driver
    ESP_LOGI(TAG, "Initializing TLE92466ED driver...");
    if (auto result = driver.initialize(); !result) {
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
