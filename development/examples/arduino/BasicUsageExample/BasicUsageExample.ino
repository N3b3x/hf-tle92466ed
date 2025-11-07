/**
 * @file BasicUsageExample.ino
 * @brief Basic usage example for TLE92466ED driver on Arduino
 * 
 * This example demonstrates fundamental TLE92466ED operations on Arduino
 * platforms using the TLE92466ED Evaluation Board.
 * 
 * Hardware Setup (Arduino Uno/Nano):
 * ==================================
 * TLE92466ED Evalboard    Arduino
 * --------------------    -------
 * MISO                -> Pin 12 (MISO)
 * MOSI                -> Pin 11 (MOSI)
 * SCK                 -> Pin 13 (SCK)
 * CS                  -> Pin 10 (SS)
 * EN                  -> Pin 9
 * RESN                -> Pin 8
 * VDD                 -> 5V
 * VIO                 -> 5V
 * GND                 -> GND
 * VBAT                -> External 12-24V supply
 * 
 * Features Demonstrated:
 * - Arduino HAL initialization
 * - TLE92466ED driver initialization
 * - Chip ID reading (communication verification)
 * - Single channel current control
 * - Channel enable/disable operations
 * - Basic diagnostics reading
 * - Current ramping demonstration
 * 
 * @author N3b3x
 * @date 2025-10-21
 * @version 2.0.0
 * 
 * @note Requires C++17 or later. Use Arduino IDE 2.0+ or PlatformIO.
 * @note Ensure TLE92466ED VBAT is supplied with 12-24V external power.
 */

#include <Arduino.h>
#include <SPI.h>
#include "../../inc/TLE92466ED.hpp"
#include "../Arduino_HAL.hpp"

// Global instances
Arduino_HAL* g_hal = nullptr;
TLE92466ED* g_driver = nullptr;

// Test configuration
const uint8_t TEST_CHANNEL = 0;              // Channel to test
const uint16_t TEST_CURRENT_MA = 500;        // Test current in mA
const uint16_t RAMP_START_MA = 100;          // Ramp start current
const uint16_t RAMP_END_MA = 1000;           // Ramp end current
const uint16_t RAMP_STEP_MA = 100;           // Ramp step size
const uint32_t STEP_DELAY_MS = 500;          // Delay between steps

/**
 * @brief Setup function - runs once at startup
 */
void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
        // Wait for serial port (with 3 second timeout)
    }
    
    Serial.println();
    Serial.println(F("╔══════════════════════════════════════════════════════════════╗"));
    Serial.println(F("║    TLE92466ED Basic Usage Example - Arduino Platform        ║"));
    Serial.println(F("║              HardFOC Core Drivers v2.0.0                     ║"));
    Serial.println(F("╚══════════════════════════════════════════════════════════════╝"));
    Serial.println();
    
    delay(1000);
    
    // Initialize HAL
    Serial.println(F("=== HAL Initialization ==="));
    if (!initializeHAL()) {
        Serial.println(F("❌ HAL initialization failed!"));
        Serial.println(F("System halted. Check wiring and reset Arduino."));
        while (1) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(200);
        }
    }
    Serial.println(F("✅ HAL initialized successfully"));
    Serial.println();
    
    // Initialize Driver
    Serial.println(F("=== Driver Initialization ==="));
    if (!initializeDriver()) {
        Serial.println(F("❌ Driver initialization failed!"));
        Serial.println(F("System halted. Check TLE92466ED power and connections."));
        while (1) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(200);
        }
    }
    Serial.println(F("✅ Driver initialized successfully"));
    Serial.println();
    
    // Verify communication by reading chip ID
    Serial.println(F("=== Communication Verification ==="));
    if (!verifyChipID()) {
        Serial.println(F("❌ Failed to read chip ID!"));
        Serial.println(F("System halted. Check SPI connections."));
        while (1) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(200);
        }
    }
    Serial.println();
    
    // All initialization successful
    Serial.println(F("╔══════════════════════════════════════════════════════════════╗"));
    Serial.println(F("║          Initialization Complete - Starting Demo            ║"));
    Serial.println(F("╚══════════════════════════════════════════════════════════════╝"));
    Serial.println();
    
    pinMode(LED_BUILTIN, OUTPUT);
}

/**
 * @brief Main loop - runs continuously
 */
void loop() {
    // Demonstrate basic channel control
    demonstrateBasicControl();
    delay(2000);
    
    // Demonstrate current ramping
    demonstrateCurrentRamping();
    delay(2000);
    
    // Demonstrate diagnostics
    demonstrateDiagnostics();
    delay(5000);
    
    Serial.println();
    Serial.println(F("╔══════════════════════════════════════════════════════════════╗"));
    Serial.println(F("║              Demo Complete - Restarting Sequence             ║"));
    Serial.println(F("╚══════════════════════════════════════════════════════════════╝"));
    Serial.println();
    delay(3000);
}

/**
 * @brief Initialize Arduino HAL
 * @return true if successful
 */
bool initializeHAL() {
    Serial.println(F("Creating Arduino HAL instance..."));
    
    // Configure SPI pins for Arduino
    Arduino_HAL::SPIConfig config;
    config.cs_pin = 10;         // Arduino SS pin
    config.en_pin = 9;          // Enable pin
    config.resn_pin = 8;        // Reset pin
    config.spi_frequency = 1000000;  // 1MHz
    config.spi_mode = SPI_MODE0;     // TLE92466ED requires Mode 0
    config.spi_bit_order = MSBFIRST;
    
    g_hal = createTLE92466ED_HAL(config);
    if (!g_hal) {
        Serial.println(F("Failed to create HAL instance"));
        return false;
    }
    
    Serial.print(F("  CS Pin: ")); Serial.println(config.cs_pin);
    Serial.print(F("  EN Pin: ")); Serial.println(config.en_pin);
    Serial.print(F("  RESN Pin: ")); Serial.println(config.resn_pin);
    Serial.print(F("  SPI Frequency: ")); Serial.print(config.spi_frequency); Serial.println(F(" Hz"));
    Serial.print(F("  SPI Mode: ")); Serial.println(config.spi_mode);
    
    Serial.println(F("Initializing HAL..."));
    auto result = g_hal->initialize();
    if (!result) {
        Serial.println(F("HAL initialize() failed"));
        return false;
    }
    
    return true;
}

/**
 * @brief Initialize TLE92466ED driver
 * @return true if successful
 */
bool initializeDriver() {
    if (!g_hal) {
        Serial.println(F("HAL not initialized"));
        return false;
    }
    
    Serial.println(F("Creating TLE92466ED driver instance..."));
    g_driver = new TLE92466ED(*g_hal);
    if (!g_driver) {
        Serial.println(F("Failed to create driver instance"));
        return false;
    }
    
    Serial.println(F("Initializing driver..."));
    auto result = g_driver->initialize();
    if (!result) {
        Serial.println(F("Driver initialize() failed"));
        return false;
    }
    
    return true;
}

/**
 * @brief Verify SPI communication by reading chip ID
 * @return true if successful
 */
bool verifyChipID() {
    if (!g_driver) {
        Serial.println(F("Driver not initialized"));
        return false;
    }
    
    Serial.println(F("Reading chip identification..."));
    auto chip_id = g_driver->readChipId();
    if (!chip_id) {
        Serial.println(F("Failed to read chip ID"));
        return false;
    }
    
    Serial.print(F("✅ Chip ID: 0x"));
    char buf[13];
    sprintf(buf, "%012llX", *chip_id);
    Serial.println(buf);
    Serial.println(F("Communication verified!"));
    
    return true;
}

/**
 * @brief Demonstrate basic channel control
 */
void demonstrateBasicControl() {
    Serial.println(F("╔══════════════════════════════════════════════════════════════╗"));
    Serial.println(F("║              Basic Channel Control Demo                      ║"));
    Serial.println(F("╚══════════════════════════════════════════════════════════════╝"));
    Serial.println();
    
    // Set current
    Serial.print(F("Setting channel ")); Serial.print(TEST_CHANNEL);
    Serial.print(F(" to ")); Serial.print(TEST_CURRENT_MA); Serial.println(F(" mA..."));
    
    auto result = g_driver->setChannelCurrent(TEST_CHANNEL, TEST_CURRENT_MA);
    if (!result) {
        Serial.println(F("❌ Failed to set current"));
        return;
    }
    Serial.println(F("✅ Current set successfully"));
    
    // Enable channel
    Serial.print(F("Enabling channel ")); Serial.print(TEST_CHANNEL); Serial.println(F("..."));
    result = g_driver->enableChannel(TEST_CHANNEL);
    if (!result) {
        Serial.println(F("❌ Failed to enable channel"));
        return;
    }
    Serial.println(F("✅ Channel enabled"));
    
    // Blink LED to show active state
    for (int i = 0; i < 5; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(200);
        digitalWrite(LED_BUILTIN, LOW);
        delay(200);
    }
    
    // Disable channel
    Serial.print(F("Disabling channel ")); Serial.print(TEST_CHANNEL); Serial.println(F("..."));
    result = g_driver->disableChannel(TEST_CHANNEL);
    if (!result) {
        Serial.println(F("❌ Failed to disable channel"));
        return;
    }
    Serial.println(F("✅ Channel disabled"));
    Serial.println();
}

/**
 * @brief Demonstrate current ramping
 */
void demonstrateCurrentRamping() {
    Serial.println(F("╔══════════════════════════════════════════════════════════════╗"));
    Serial.println(F("║               Current Ramping Demo                           ║"));
    Serial.println(F("╚══════════════════════════════════════════════════════════════╝"));
    Serial.println();
    
    // Enable channel
    Serial.println(F("Enabling channel for ramping..."));
    auto result = g_driver->enableChannel(TEST_CHANNEL);
    if (!result) {
        Serial.println(F("❌ Failed to enable channel"));
        return;
    }
    
    // Ramp up
    Serial.print(F("Ramping up from ")); Serial.print(RAMP_START_MA);
    Serial.print(F(" to ")); Serial.print(RAMP_END_MA); Serial.println(F(" mA..."));
    
    for (uint16_t current = RAMP_START_MA; current <= RAMP_END_MA; current += RAMP_STEP_MA) {
        Serial.print(F("  Current: ")); Serial.print(current); Serial.println(F(" mA"));
        
        result = g_driver->setChannelCurrent(TEST_CHANNEL, current);
        if (!result) {
            Serial.println(F("❌ Failed to set current"));
            break;
        }
        
        digitalWrite(LED_BUILTIN, HIGH);
        delay(STEP_DELAY_MS / 2);
        digitalWrite(LED_BUILTIN, LOW);
        delay(STEP_DELAY_MS / 2);
    }
    
    // Ramp down
    Serial.print(F("Ramping down from ")); Serial.print(RAMP_END_MA);
    Serial.print(F(" to ")); Serial.print(RAMP_START_MA); Serial.println(F(" mA..."));
    
    for (int current = RAMP_END_MA; current >= RAMP_START_MA; current -= RAMP_STEP_MA) {
        Serial.print(F("  Current: ")); Serial.print(current); Serial.println(F(" mA"));
        
        result = g_driver->setChannelCurrent(TEST_CHANNEL, static_cast<uint16_t>(current));
        if (!result) {
            Serial.println(F("❌ Failed to set current"));
            break;
        }
        
        digitalWrite(LED_BUILTIN, HIGH);
        delay(STEP_DELAY_MS / 2);
        digitalWrite(LED_BUILTIN, LOW);
        delay(STEP_DELAY_MS / 2);
    }
    
    // Disable channel
    Serial.println(F("Disabling channel..."));
    g_driver->disableChannel(TEST_CHANNEL);
    Serial.println(F("✅ Ramping complete"));
    Serial.println();
}

/**
 * @brief Demonstrate diagnostics reading
 */
void demonstrateDiagnostics() {
    Serial.println(F("╔══════════════════════════════════════════════════════════════╗"));
    Serial.println(F("║                  Diagnostics Demo                            ║"));
    Serial.println(F("╚══════════════════════════════════════════════════════════════╝"));
    Serial.println();
    
    // Enable channel for diagnostics
    Serial.println(F("Enabling channel for diagnostics..."));
    g_driver->setChannelCurrent(TEST_CHANNEL, TEST_CURRENT_MA);
    g_driver->enableChannel(TEST_CHANNEL);
    
    // Read diagnostics multiple times
    Serial.println(F("Reading diagnostics (5 samples)..."));
    for (int i = 0; i < 5; i++) {
        Serial.print(F("Sample ")); Serial.print(i + 1); Serial.println(F("/5:"));
        
        auto diag = g_driver->readDiagnostics();
        if (!diag) {
            Serial.println(F("  ❌ Failed to read diagnostics"));
            continue;
        }
        
        // Check for faults
        bool has_faults = false;
        
        if (diag->hasOvercurrent()) {
            Serial.println(F("  ⚠️  Overcurrent detected"));
            has_faults = true;
        }
        
        if (diag->hasOvertemperature()) {
            Serial.println(F("  🌡️  Overtemperature detected"));
            has_faults = true;
        }
        
        if (diag->hasOpenLoad(TEST_CHANNEL)) {
            Serial.println(F("  🔌  Open load detected"));
            has_faults = true;
        }
        
        if (diag->hasShortCircuit(TEST_CHANNEL)) {
            Serial.println(F("  ⚡  Short circuit detected"));
            has_faults = true;
        }
        
        if (!has_faults) {
            Serial.println(F("  ✅  All systems normal"));
        }
        
        delay(1000);
    }
    
    // Disable channel
    g_driver->disableChannel(TEST_CHANNEL);
    Serial.println(F("✅ Diagnostics complete"));
    Serial.println();
}

