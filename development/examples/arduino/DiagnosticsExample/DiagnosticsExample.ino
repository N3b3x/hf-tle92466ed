/**
 * @file DiagnosticsExample.ino
 * @brief Comprehensive diagnostics example for TLE92466ED driver on Arduino
 * 
 * This example demonstrates advanced fault detection and diagnostics features
 * of the TLE92466ED, including overcurrent, overtemperature, open load,
 * short circuit detection, and diagnostic logging.
 * 
 * Hardware Setup (Arduino Uno/Nano):
 * ==================================
 * Same as BasicUsageExample - see BasicUsageExample.ino or README.md
 * 
 * Features Demonstrated:
 * - Comprehensive fault detection
 * - Real-time diagnostics monitoring
 * - Overcurrent detection and handling
 * - Overtemperature monitoring
 * - Open load detection (ON and OFF states)
 * - Short circuit detection
 * - Supply voltage monitoring
 * - Diagnostic history logging
 * - Fault recovery procedures
 * 
 * @author N3b3x
 * @date 2025-10-21
 * @version 2.0.0
 */

#include <Arduino.h>
#include <SPI.h>
#include "../../inc/TLE92466ED.hpp"
#include "../Arduino_HAL.hpp"

// Global instances
Arduino_HAL* g_hal = nullptr;
TLE92466ED* g_driver = nullptr;

// Configuration
const uint8_t TEST_CHANNEL = 0;
const uint16_t TEST_CURRENT_MA = 1000;
const uint32_t DIAGNOSTIC_INTERVAL_MS = 500;  // Poll every 500ms

// Diagnostic counters
struct DiagnosticCounters {
    uint32_t total_reads = 0;
    uint32_t overcurrent_count = 0;
    uint32_t overtemp_count = 0;
    uint32_t openload_count = 0;
    uint32_t shortcircuit_count = 0;
    uint32_t last_fault_time = 0;
};

DiagnosticCounters g_counters;

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
    Serial.println(F("║   TLE92466ED Diagnostics Example - Arduino Platform         ║"));
    Serial.println(F("║              HardFOC Core Drivers v2.0.0                     ║"));
    Serial.println(F("╚══════════════════════════════════════════════════════════════╝"));
    Serial.println();
    
    delay(1000);
    
    // Initialize HAL
    Serial.println(F("=== HAL Initialization ==="));
    if (!initializeHAL()) {
        Serial.println(F("❌ HAL initialization failed!"));
        haltWithError();
    }
    Serial.println(F("✅ HAL initialized successfully"));
    Serial.println();
    
    // Initialize Driver
    Serial.println(F("=== Driver Initialization ==="));
    if (!initializeDriver()) {
        Serial.println(F("❌ Driver initialization failed!"));
        haltWithError();
    }
    Serial.println(F("✅ Driver initialized successfully"));
    Serial.println();
    
    // Verify communication
    Serial.println(F("=== Communication Verification ==="));
    if (!verifyChipID()) {
        Serial.println(F("❌ Failed to read chip ID!"));
        haltWithError();
    }
    Serial.println();
    
    // Setup test channel
    Serial.println(F("=== Test Channel Setup ==="));
    Serial.print(F("Configuring channel ")); Serial.print(TEST_CHANNEL);
    Serial.print(F(" for ")); Serial.print(TEST_CURRENT_MA); Serial.println(F(" mA..."));
    
    auto result = g_driver->setChannelCurrent(TEST_CHANNEL, TEST_CURRENT_MA);
    if (!result) {
        Serial.println(F("❌ Failed to set current"));
        haltWithError();
    }
    
    result = g_driver->enableChannel(TEST_CHANNEL);
    if (!result) {
        Serial.println(F("❌ Failed to enable channel"));
        haltWithError();
    }
    
    Serial.println(F("✅ Test channel configured and enabled"));
    Serial.println();
    
    // Start diagnostics monitoring
    Serial.println(F("╔══════════════════════════════════════════════════════════════╗"));
    Serial.println(F("║        Starting Continuous Diagnostics Monitoring           ║"));
    Serial.println(F("╚══════════════════════════════════════════════════════════════╝"));
    Serial.println();
    
    pinMode(LED_BUILTIN, OUTPUT);
}

/**
 * @brief Main loop - continuous diagnostics monitoring
 */
void loop() {
    static uint32_t lastDiagnosticTime = 0;
    static uint32_t lastSummaryTime = 0;
    
    // Read diagnostics at regular intervals
    if (millis() - lastDiagnosticTime >= DIAGNOSTIC_INTERVAL_MS) {
        readAndDisplayDiagnostics();
        lastDiagnosticTime = millis();
        
        // Toggle LED to show activity
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
    
    // Print summary every 10 seconds
    if (millis() - lastSummaryTime >= 10000) {
        printDiagnosticSummary();
        lastSummaryTime = millis();
    }
}

/**
 * @brief Initialize Arduino HAL
 */
bool initializeHAL() {
    Serial.println(F("Creating Arduino HAL instance..."));
    
    Arduino_HAL::SPIConfig config;
    config.cs_pin = 10;
    config.en_pin = 9;
    config.resn_pin = 8;
    config.spi_frequency = 1000000;
    config.spi_mode = SPI_MODE0;
    config.spi_bit_order = MSBFIRST;
    
    g_hal = createTLE92466ED_HAL(config);
    if (!g_hal) {
        return false;
    }
    
    Serial.println(F("Initializing HAL..."));
    auto result = g_hal->initialize();
    return result.has_value();
}

/**
 * @brief Initialize TLE92466ED driver
 */
bool initializeDriver() {
    if (!g_hal) {
        return false;
    }
    
    Serial.println(F("Creating TLE92466ED driver instance..."));
    g_driver = new TLE92466ED(*g_hal);
    if (!g_driver) {
        return false;
    }
    
    Serial.println(F("Initializing driver..."));
    auto result = g_driver->initialize();
    return result.has_value();
}

/**
 * @brief Verify SPI communication by reading chip ID
 */
bool verifyChipID() {
    if (!g_driver) {
        return false;
    }
    
    Serial.println(F("Reading chip identification..."));
    auto chip_id = g_driver->readChipId();
    if (!chip_id) {
        return false;
    }
    
    Serial.print(F("✅ Chip ID: 0x"));
    char buf[13];
    sprintf(buf, "%012llX", *chip_id);
    Serial.println(buf);
    
    return true;
}

/**
 * @brief Read and display comprehensive diagnostics
 */
void readAndDisplayDiagnostics() {
    // Increment read counter
    g_counters.total_reads++;
    
    // Read diagnostics
    auto diag = g_driver->readDiagnostics();
    if (!diag) {
        Serial.println(F("❌ Failed to read diagnostics"));
        return;
    }
    
    // Timestamp
    uint32_t timestamp = millis();
    
    // Check for faults
    bool has_faults = false;
    
    // Overcurrent check
    if (diag->hasOvercurrent()) {
        Serial.print(F("["));
        Serial.print(timestamp);
        Serial.println(F("] ⚠️  OVERCURRENT DETECTED"));
        Serial.println(F("  └─> One or more channels exceeded current limit"));
        Serial.println(F("  └─> Action: Check load resistance, reduce current setting"));
        
        g_counters.overcurrent_count++;
        g_counters.last_fault_time = timestamp;
        has_faults = true;
        
        // Attempt recovery
        handleOvercurrent();
    }
    
    // Overtemperature check
    if (diag->hasOvertemperature()) {
        Serial.print(F("["));
        Serial.print(timestamp);
        Serial.println(F("] 🌡️  OVERTEMPERATURE DETECTED"));
        Serial.println(F("  └─> Device junction temperature exceeded limit"));
        Serial.println(F("  └─> Action: Add cooling, reduce duty cycle"));
        
        g_counters.overtemp_count++;
        g_counters.last_fault_time = timestamp;
        has_faults = true;
        
        // Attempt recovery
        handleOvertemperature();
    }
    
    // Open load check (per channel)
    if (diag->hasOpenLoad(TEST_CHANNEL)) {
        Serial.print(F("["));
        Serial.print(timestamp);
        Serial.print(F("] 🔌  OPEN LOAD on Channel "));
        Serial.println(TEST_CHANNEL);
        Serial.println(F("  └─> No load detected or load disconnected"));
        Serial.println(F("  └─> Action: Check load connections"));
        
        g_counters.openload_count++;
        g_counters.last_fault_time = timestamp;
        has_faults = true;
    }
    
    // Short circuit check (per channel)
    if (diag->hasShortCircuit(TEST_CHANNEL)) {
        Serial.print(F("["));
        Serial.print(timestamp);
        Serial.print(F("] ⚡  SHORT CIRCUIT on Channel "));
        Serial.println(TEST_CHANNEL);
        Serial.println(F("  └─> Load or wiring shorted to ground"));
        Serial.println(F("  └─> Action: Inspect wiring, check for shorts"));
        
        g_counters.shortcircuit_count++;
        g_counters.last_fault_time = timestamp;
        has_faults = true;
        
        // Attempt recovery
        handleShortCircuit();
    }
    
    // If no faults, print status every 10 reads
    if (!has_faults && (g_counters.total_reads % 10 == 0)) {
        Serial.print(F("["));
        Serial.print(timestamp);
        Serial.print(F("] ✅  All systems normal ("));
        Serial.print(g_counters.total_reads);
        Serial.println(F(" reads)"));
    }
}

/**
 * @brief Handle overcurrent condition
 */
void handleOvercurrent() {
    Serial.println(F("  └─> Recovery: Disabling channel temporarily..."));
    g_driver->disableChannel(TEST_CHANNEL);
    delay(1000);  // Wait 1 second
    
    // Re-enable with reduced current
    uint16_t reduced_current = TEST_CURRENT_MA * 0.8;  // 80% of original
    Serial.print(F("  └─> Re-enabling with reduced current: "));
    Serial.print(reduced_current);
    Serial.println(F(" mA"));
    
    g_driver->setChannelCurrent(TEST_CHANNEL, reduced_current);
    g_driver->enableChannel(TEST_CHANNEL);
}

/**
 * @brief Handle overtemperature condition
 */
void handleOvertemperature() {
    Serial.println(F("  └─> Recovery: Disabling all channels for cooling..."));
    
    // Disable all channels
    for (uint8_t ch = 0; ch < 6; ch++) {
        g_driver->disableChannel(ch);
    }
    
    delay(5000);  // Wait 5 seconds for cooling
    
    Serial.println(F("  └─> Re-enabling test channel..."));
    g_driver->enableChannel(TEST_CHANNEL);
}

/**
 * @brief Handle short circuit condition
 */
void handleShortCircuit() {
    Serial.println(F("  └─> Recovery: Keeping channel disabled"));
    Serial.println(F("  └─> Manual intervention required!"));
    
    g_driver->disableChannel(TEST_CHANNEL);
    
    // Don't re-enable automatically - short circuit needs to be fixed
}

/**
 * @brief Print diagnostic summary
 */
void printDiagnosticSummary() {
    Serial.println();
    Serial.println(F("╔══════════════════════════════════════════════════════════════╗"));
    Serial.println(F("║              DIAGNOSTIC SUMMARY (Last 10 seconds)            ║"));
    Serial.println(F("╠══════════════════════════════════════════════════════════════╣"));
    
    Serial.print(F("║  Total Diagnostic Reads:     "));
    printPaddedNumber(g_counters.total_reads);
    Serial.println(F("║"));
    
    Serial.print(F("║  Overcurrent Events:         "));
    printPaddedNumber(g_counters.overcurrent_count);
    Serial.println(F("║"));
    
    Serial.print(F("║  Overtemperature Events:     "));
    printPaddedNumber(g_counters.overtemp_count);
    Serial.println(F("║"));
    
    Serial.print(F("║  Open Load Events:           "));
    printPaddedNumber(g_counters.openload_count);
    Serial.println(F("║"));
    
    Serial.print(F("║  Short Circuit Events:       "));
    printPaddedNumber(g_counters.shortcircuit_count);
    Serial.println(F("║"));
    
    if (g_counters.last_fault_time > 0) {
        Serial.print(F("║  Last Fault: "));
        uint32_t seconds_ago = (millis() - g_counters.last_fault_time) / 1000;
        Serial.print(seconds_ago);
        Serial.print(F(" seconds ago"));
        
        // Padding
        uint8_t padding = 37 - String(seconds_ago).length();
        for (uint8_t i = 0; i < padding; i++) {
            Serial.print(F(" "));
        }
        Serial.println(F("║"));
    }
    
    // Calculate uptime
    uint32_t uptime_seconds = millis() / 1000;
    Serial.print(F("║  System Uptime:              "));
    Serial.print(uptime_seconds);
    Serial.print(F(" seconds"));
    
    // Padding
    uint8_t padding = 31 - String(uptime_seconds).length();
    for (uint8_t i = 0; i < padding; i++) {
        Serial.print(F(" "));
    }
    Serial.println(F("║"));
    
    Serial.println(F("╚══════════════════════════════════════════════════════════════╝"));
    Serial.println();
}

/**
 * @brief Print number with padding
 */
void printPaddedNumber(uint32_t number) {
    String numStr = String(number);
    Serial.print(numStr);
    
    // Add padding (assuming field width of 36)
    uint8_t padding = 36 - numStr.length();
    for (uint8_t i = 0; i < padding; i++) {
        Serial.print(F(" "));
    }
}

/**
 * @brief Halt system with error indication
 */
void haltWithError() {
    Serial.println(F("System halted. Check connections and reset Arduino."));
    while (1) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(200);
        digitalWrite(LED_BUILTIN, LOW);
        delay(200);
    }
}

