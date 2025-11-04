/**
 * @file TLE92466ED_Config.hpp
 * @brief Hardware configuration for TLE92466ED driver on ESP32-C6
 * 
 * This file contains the actual hardware configuration that is used by the HAL
 * and example applications. Modify these values to match your hardware setup.
 * 
 * @author N3b3x
 * @date 2025-10-21
 * @version 2.0.0
 */

#pragma once

#include <cstdint>

namespace TLE92466ED_Config {

/**
 * @brief SPI Configuration for ESP32-C6
 * 
 * These pins are used for SPI communication with the TLE92466ED.
 * Ensure your hardware matches these pin assignments or modify accordingly.
 */
struct SPIPins {
    static constexpr int MISO = 2;          ///< GPIO2 - SPI MISO (Master In Slave Out)
    static constexpr int MOSI = 7;          ///< GPIO7 - SPI MOSI (Master Out Slave In)
    static constexpr int SCLK = 6;          ///< GPIO6 - SPI Clock
    static constexpr int CS = 18;           ///< GPIO18 - Chip Select (active low)
};

/**
 * @brief SPI Communication Parameters
 * 
 * The TLE92466ED supports SPI frequencies up to 8MHz.
 * We use 1MHz for reliable communication with standard wiring.
 */
struct SPIParams {
    static constexpr int FREQUENCY = 1000000;   ///< 1MHz SPI frequency
    static constexpr int MODE = 1;              ///< SPI Mode 1 (CPOL=0, CPHA=1)
    static constexpr int QUEUE_SIZE = 1;        ///< Transaction queue size
};

/**
 * @brief Current Control Limits (milliamps)
 * 
 * These are hardware limits from the TLE92466ED datasheet.
 * Single channel: 0-2000mA
 * Parallel mode: 0-4000mA (channels paired)
 */
struct CurrentLimits {
    static constexpr uint16_t SINGLE_CHANNEL_MIN = 0;      ///< Minimum current (mA)
    static constexpr uint16_t SINGLE_CHANNEL_MAX = 2000;   ///< Maximum single channel current (mA)
    static constexpr uint16_t PARALLEL_CHANNEL_MAX = 4000; ///< Maximum parallel channel current (mA)
    static constexpr uint16_t RESOLUTION = 61;             ///< Current resolution (μA per LSB)
};

/**
 * @brief Supply Voltage Specifications (volts)
 * 
 * VBAT: Main power supply for the load outputs
 * VDD: Logic supply (3.3V for ESP32-C6)
 */
struct SupplyVoltage {
    static constexpr float VBAT_MIN = 8.0f;    ///< Minimum VBAT voltage (V)
    static constexpr float VBAT_NOM = 12.0f;   ///< Nominal VBAT voltage (V)
    static constexpr float VBAT_MAX = 28.0f;   ///< Maximum VBAT voltage (V)
    static constexpr float VDD_NOM = 3.3f;     ///< Logic supply voltage (V)
};

/**
 * @brief Temperature Specifications (celsius)
 * 
 * Operating temperature range and limits from datasheet.
 */
struct Temperature {
    static constexpr int OPERATING_MIN = -40;   ///< Minimum operating temperature (°C)
    static constexpr int OPERATING_MAX = 150;   ///< Maximum operating temperature (°C)
    static constexpr int JUNCTION_MAX = 150;    ///< Maximum junction temperature (°C)
    static constexpr int WARNING_THRESHOLD = 130; ///< Temperature warning threshold (°C)
};

/**
 * @brief Timing Parameters (microseconds)
 * 
 * Timing requirements from the TLE92466ED datasheet.
 */
struct Timing {
    static constexpr uint32_t CS_SETUP_US = 1;          ///< CS setup time before SCLK (μs)
    static constexpr uint32_t CS_HOLD_US = 1;           ///< CS hold time after SCLK (μs)
    static constexpr uint32_t INTER_FRAME_US = 10;      ///< Minimum time between frames (μs)
    static constexpr uint32_t POWER_ON_DELAY_MS = 50;   ///< Power-on initialization delay (ms)
    static constexpr uint32_t RESET_DELAY_MS = 10;      ///< Reset pulse duration (ms)
};

/**
 * @brief Diagnostic Thresholds
 * 
 * Thresholds for fault detection and diagnostics.
 */
struct Diagnostics {
    static constexpr uint16_t OVERCURRENT_THRESHOLD_MA = 2100;  ///< Overcurrent fault threshold (mA)
    static constexpr uint32_t POLL_INTERVAL_MS = 100;           ///< Diagnostic polling interval (ms)
    static constexpr uint8_t MAX_RETRY_COUNT = 3;               ///< Maximum communication retries
};

/**
 * @brief Test Configuration
 * 
 * Default parameters for testing and calibration.
 */
struct TestConfig {
    static constexpr uint16_t DEFAULT_TEST_CURRENT = 500;       ///< Default test current (mA)
    static constexpr uint32_t TEST_DURATION_MS = 5000;          ///< Test duration (ms)
    static constexpr uint16_t RAMP_STEP_MA = 100;               ///< Current ramp step size (mA)
    static constexpr uint32_t RAMP_STEP_DELAY_MS = 500;         ///< Delay between ramp steps (ms)
};

/**
 * @brief Channel Pairing for Parallel Mode
 * 
 * The TLE92466ED supports parallel operation of channel pairs
 * for higher current applications (up to 4A).
 */
struct ChannelPairs {
    static constexpr uint8_t PAIR_0_CH_A = 0;  ///< Channel 0 pairs with Channel 3
    static constexpr uint8_t PAIR_0_CH_B = 3;
    
    static constexpr uint8_t PAIR_1_CH_A = 1;  ///< Channel 1 pairs with Channel 2
    static constexpr uint8_t PAIR_1_CH_B = 2;
    
    static constexpr uint8_t PAIR_2_CH_A = 4;  ///< Channel 4 pairs with Channel 5
    static constexpr uint8_t PAIR_2_CH_B = 5;
};

/**
 * @brief Application-specific Configuration
 * 
 * Configuration values that can be adjusted per application.
 */
struct AppConfig {
    // Logging
    static constexpr bool ENABLE_DEBUG_LOGGING = true;     ///< Enable detailed debug logs
    static constexpr bool ENABLE_SPI_LOGGING = false;      ///< Enable SPI transaction logs
    
    // Performance
    static constexpr bool ENABLE_PERFORMANCE_MONITORING = true;  ///< Enable performance metrics
    static constexpr uint32_t STATS_REPORT_INTERVAL_MS = 10000;  ///< Statistics reporting interval
    
    // Error handling
    static constexpr bool ENABLE_AUTO_RECOVERY = true;     ///< Enable automatic error recovery
    static constexpr uint8_t MAX_ERROR_COUNT = 10;         ///< Maximum errors before failsafe
};

} // namespace TLE92466ED_Config

/**
 * @brief Hardware configuration validation
 * 
 * Compile-time checks to ensure configuration is valid.
 */
static_assert(TLE92466ED_Config::CurrentLimits::SINGLE_CHANNEL_MAX <= 2000,
              "Single channel current exceeds hardware limit of 2000mA");

static_assert(TLE92466ED_Config::CurrentLimits::PARALLEL_CHANNEL_MAX <= 4000,
              "Parallel channel current exceeds hardware limit of 4000mA");

static_assert(TLE92466ED_Config::SPIParams::FREQUENCY <= 8000000,
              "SPI frequency exceeds TLE92466ED maximum of 8MHz");

static_assert(TLE92466ED_Config::SPIParams::MODE == 1,
              "TLE92466ED requires SPI Mode 1 (CPOL=0, CPHA=1)");

/**
 * @brief Helper macro for compile-time configuration validation
 */
#define TLE92466ED_VALIDATE_CURRENT(current_ma) \
    static_assert((current_ma) <= TLE92466ED_Config::CurrentLimits::SINGLE_CHANNEL_MAX, \
                  "Current exceeds maximum limit")

/**
 * @brief Helper macro for GPIO pin validation
 */
#define TLE92466ED_VALIDATE_GPIO(pin) \
    static_assert((pin) >= 0 && (pin) < 30, "Invalid GPIO pin number for ESP32-C6")
