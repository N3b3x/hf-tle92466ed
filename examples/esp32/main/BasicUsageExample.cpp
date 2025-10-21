/**
 * @file BasicUsageExample.cpp
 * @brief Basic usage example for TLE92466ED driver on ESP32-C6
 * 
 * This example demonstrates:
 * - HAL initialization for ESP32-C6
 * - TLE92466ED driver initialization
 * - Single channel current control
 * - Basic diagnostics reading
 * - Error handling with std::expected
 * - FreeRTOS task-based test execution
 * - GPIO14 test progression indicator
 * 
 * @author N3b3x
 * @date 2025-10-21
 * @version 2.0.0
 */

#include <stdio.h>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#include "TLE92466ED.hpp"
#include "ESP32C6_HAL.hpp"
#include "TLE92466ED_TestFramework.hpp"

static const char* TAG = "TLE92466ED_Basic";

//=============================================================================
// TEST CONFIGURATION
//=============================================================================

#define ENABLE_INITIALIZATION_TESTS 1
#define ENABLE_BASIC_OPERATION_TESTS 1
#define ENABLE_CURRENT_CONTROL_TESTS 1

//=============================================================================
// SHARED TEST RESOURCES
//=============================================================================

static std::unique_ptr<ESP32C6_HAL> g_hal;
static TLE92466ED* g_driver = nullptr;

//=============================================================================
// TEST FUNCTIONS
//=============================================================================

/**
 * @brief Test HAL initialization
 */
static bool test_hal_initialization() noexcept {
    ESP_LOGI(TAG, "Creating HAL instance...");
    g_hal = createTLE92466ED_HAL();
    
    if (!g_hal) {
        ESP_LOGE(TAG, "Failed to create HAL instance");
        return false;
    }
    
    ESP_LOGI(TAG, "Initializing HAL...");
    if (auto result = g_hal->initialize(); !result) {
        ESP_LOGE(TAG, "Failed to initialize HAL");
        return false;
    }
    
    ESP_LOGI(TAG, "✅ HAL initialized successfully");
    return true;
}

/**
 * @brief Test TLE92466ED driver initialization
 */
static bool test_driver_initialization() noexcept {
    if (!g_hal) {
        ESP_LOGE(TAG, "HAL not initialized");
        return false;
    }
    
    ESP_LOGI(TAG, "Creating TLE92466ED driver instance...");
    g_driver = new TLE92466ED(*g_hal);
    
    if (!g_driver) {
        ESP_LOGE(TAG, "Failed to create driver instance");
        return false;
    }
    
    ESP_LOGI(TAG, "Initializing TLE92466ED driver...");
    if (auto result = g_driver->initialize(); !result) {
        ESP_LOGE(TAG, "Failed to initialize driver");
        return false;
    }
    
    ESP_LOGI(TAG, "✅ TLE92466ED driver initialized successfully");
    return true;
}

/**
 * @brief Test chip ID reading
 */
static bool test_chip_id() noexcept {
    if (!g_driver) {
        ESP_LOGE(TAG, "Driver not initialized");
        return false;
    }
    
    ESP_LOGI(TAG, "Reading chip identification...");
    if (auto chip_id = g_driver->readChipId(); chip_id) {
        ESP_LOGI(TAG, "✅ Chip ID: 0x%012llX", *chip_id);
        return true;
    } else {
        ESP_LOGE(TAG, "❌ Failed to read chip ID");
        return false;
    }
}

/**
 * @brief Test channel enable/disable
 */
static bool test_channel_enable_disable() noexcept {
    if (!g_driver) {
        ESP_LOGE(TAG, "Driver not initialized");
        return false;
    }
    
    const uint8_t channel = 0;
    
    ESP_LOGI(TAG, "Enabling channel %d...", channel);
    if (auto result = g_driver->enableChannel(channel); !result) {
        ESP_LOGE(TAG, "❌ Failed to enable channel %d", channel);
        return false;
    }
    ESP_LOGI(TAG, "✅ Channel %d enabled", channel);
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGI(TAG, "Disabling channel %d...", channel);
    if (auto result = g_driver->disableChannel(channel); !result) {
        ESP_LOGE(TAG, "❌ Failed to disable channel %d", channel);
        return false;
    }
    ESP_LOGI(TAG, "✅ Channel %d disabled", channel);
    
    return true;
}

/**
 * @brief Test current setting
 */
static bool test_current_setting() noexcept {
    if (!g_driver) {
        ESP_LOGE(TAG, "Driver not initialized");
        return false;
    }
    
    const uint8_t channel = 0;
    const uint16_t test_currents[] = {100, 500, 1000, 1500};
    
    for (uint16_t current : test_currents) {
        ESP_LOGI(TAG, "Setting channel %d current to %dmA...", channel, current);
        if (auto result = g_driver->setChannelCurrent(channel, current); !result) {
            ESP_LOGE(TAG, "❌ Failed to set current to %dmA", current);
            return false;
        }
        ESP_LOGI(TAG, "✅ Current set to %dmA successfully", current);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    return true;
}

/**
 * @brief Test diagnostics reading
 */
static bool test_diagnostics() noexcept {
    if (!g_driver) {
        ESP_LOGE(TAG, "Driver not initialized");
        return false;
    }
    
    const uint8_t channel = 0;
    
    ESP_LOGI(TAG, "Reading diagnostics...");
    if (auto diag = g_driver->readDiagnostics(); diag) {
        ESP_LOGI(TAG, "✅ Diagnostics read successfully");
        
        // Check for faults
        bool has_faults = false;
        if (diag->hasOvercurrent()) {
            ESP_LOGW(TAG, "  ⚠️  Overcurrent detected");
            has_faults = true;
        }
        if (diag->hasOvertemperature()) {
            ESP_LOGW(TAG, "  🌡️  Overtemperature detected");
            has_faults = true;
        }
        if (diag->hasOpenLoad(channel)) {
            ESP_LOGW(TAG, "  🔌  Open load on channel %d", channel);
            has_faults = true;
        }
        if (diag->hasShortCircuit(channel)) {
            ESP_LOGW(TAG, "  ⚡  Short circuit on channel %d", channel);
            has_faults = true;
        }
        
        if (!has_faults) {
            ESP_LOGI(TAG, "  ✅  All systems normal");
        }
        
        return true;
    } else {
        ESP_LOGE(TAG, "❌ Failed to read diagnostics");
        return false;
    }
}

/**
 * @brief Test current ramping
 */
static bool test_current_ramping() noexcept {
    if (!g_driver) {
        ESP_LOGE(TAG, "Driver not initialized");
        return false;
    }
    
    const uint8_t channel = 0;
    
    // Enable channel first
    ESP_LOGI(TAG, "Enabling channel %d for ramping test...", channel);
    if (auto result = g_driver->enableChannel(channel); !result) {
        ESP_LOGE(TAG, "❌ Failed to enable channel");
        return false;
    }
    
    // Ramp up
    ESP_LOGI(TAG, "Ramping up from 0 to 1000mA...");
    for (uint16_t current = 0; current <= 1000; current += 100) {
        if (auto result = g_driver->setChannelCurrent(channel, current); !result) {
            ESP_LOGE(TAG, "❌ Failed to set current to %dmA", current);
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    
    // Ramp down
    ESP_LOGI(TAG, "Ramping down from 1000 to 0mA...");
    for (int current = 1000; current >= 0; current -= 100) {
        if (auto result = g_driver->setChannelCurrent(channel, static_cast<uint16_t>(current)); !result) {
            ESP_LOGE(TAG, "❌ Failed to set current to %dmA", current);
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    
    // Disable channel
    if (auto result = g_driver->disableChannel(channel); !result) {
        ESP_LOGE(TAG, "❌ Failed to disable channel");
        return false;
    }
    
    ESP_LOGI(TAG, "✅ Current ramping test completed successfully");
    return true;
}

//=============================================================================
// MAIN APPLICATION
//=============================================================================

extern "C" void app_main() {
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║              TLE92466ED BASIC USAGE TEST SUITE - ESP32-C6                   ║");
    ESP_LOGI(TAG, "║                         HardFOC Core Drivers                                 ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════════════════════╝");
    
    ESP_LOGI(TAG, "Target: %s", CONFIG_IDF_TARGET);
    ESP_LOGI(TAG, "ESP-IDF Version: %s", esp_get_idf_version());
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Initialize GPIO14 test progress indicator
    init_test_progress_indicator();
    
    // Report test section configuration
    print_test_section_status(TAG, "TLE92466ED");
    
    // Run test suite based on configuration
    RUN_TEST_SECTION_IF_ENABLED(
        ENABLE_INITIALIZATION_TESTS, "INITIALIZATION TESTS",
        ESP_LOGI(TAG, "Running initialization tests...");
        RUN_TEST_IN_TASK("hal_initialization", test_hal_initialization, 8192, 5);
        RUN_TEST_IN_TASK("driver_initialization", test_driver_initialization, 8192, 5);
        RUN_TEST_IN_TASK("chip_id", test_chip_id, 8192, 5);
    );
    
    RUN_TEST_SECTION_IF_ENABLED(
        ENABLE_BASIC_OPERATION_TESTS, "BASIC OPERATION TESTS",
        ESP_LOGI(TAG, "Running basic operation tests...");
        RUN_TEST_IN_TASK("channel_enable_disable", test_channel_enable_disable, 8192, 5);
        RUN_TEST_IN_TASK("current_setting", test_current_setting, 8192, 5);
        RUN_TEST_IN_TASK("diagnostics", test_diagnostics, 8192, 5);
    );
    
    RUN_TEST_SECTION_IF_ENABLED(
        ENABLE_CURRENT_CONTROL_TESTS, "CURRENT CONTROL TESTS",
        ESP_LOGI(TAG, "Running current control tests...");
        RUN_TEST_IN_TASK("current_ramping", test_current_ramping, 8192, 5);
    );
    
    // Print test results summary
    g_test_results.print_summary(TAG);
    
    // Cleanup
    if (g_driver) {
        delete g_driver;
        g_driver = nullptr;
    }
    g_hal.reset();
    cleanup_test_progress_indicator();
    
    // Final message
    if (g_test_results.failed_tests == 0) {
        ESP_LOGI(TAG, "✅ ALL TESTS PASSED! System will restart in 10 seconds...");
    } else {
        ESP_LOGE(TAG, "❌ SOME TESTS FAILED! System will restart in 10 seconds...");
    }
    
    vTaskDelay(pdMS_TO_TICKS(10000));
    esp_restart();
}
