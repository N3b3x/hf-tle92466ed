/**
 * @file MultiChannelExample.ino
 * @brief Multi-channel control example for TLE92466ED driver on Arduino
 * 
 * This example demonstrates advanced multi-channel control of the TLE92466ED,
 * including independent channel operation, synchronized control, wave patterns,
 * and performance monitoring.
 * 
 * Hardware Setup (Arduino Uno/Nano):
 * ==================================
 * Same as BasicUsageExample - see BasicUsageExample.ino or README.md
 * 
 * Connect loads to multiple channels:
 * - OUT0 -> Load 1
 * - OUT1 -> Load 2
 * - OUT2 -> Load 3
 * - OUT3 -> Load 4
 * - OUT4 -> Load 5
 * - OUT5 -> Load 6
 * 
 * Features Demonstrated:
 * - Independent 6-channel control
 * - Sequential channel activation
 * - Synchronized channel operations
 * - Wave patterns across channels
 * - Channel group management
 * - Performance monitoring
 * 
 * @author N3b3x
 * @date 2025-10-21
 * @version 2.0.0
 */

#include <Arduino.h>
#include <SPI.h>
#include "../../include/TLE92466ED.hpp"
#include "../Arduino_HAL.hpp"

// Global instances
Arduino_HAL* g_hal = nullptr;
TLE92466ED* g_driver = nullptr;

// Configuration
const uint16_t BASE_CURRENT_MA = 500;        // Base current for channels
const uint32_t SEQUENCE_DELAY_MS = 300;      // Delay in sequences
const uint8_t NUM_CHANNELS = 6;              // TLE92466ED has 6 channels

// Demo mode selection
enum DemoMode {
    SEQUENTIAL,
    SYNCHRONIZED,
    WAVE_PATTERN,
    PERFORMANCE_TEST
};

DemoMode currentMode = SEQUENTIAL;
uint32_t modeStartTime = 0;
const uint32_t MODE_DURATION_MS = 10000;     // 10 seconds per mode

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
    Serial.println(F("║  TLE92466ED Multi-Channel Example - Arduino Platform        ║"));
    Serial.println(F("║              HardFOC Core Drivers v2.0.0                     ║"));
    Serial.println(F("╚══════════════════════════════════════════════════════════════╝"));
    Serial.println();
    
    delay(1000);
    
    // Initialize HAL
    Serial.println(F("=== HAL Initialization ==="));
    if (!initializeHAL()) {
        Serial.println(F("❌ HAL initialization failed!"));
        Serial.println(F("System halted. Check wiring and reset Arduino."));
        haltWithError();
    }
    Serial.println(F("✅ HAL initialized successfully"));
    Serial.println();
    
    // Initialize Driver
    Serial.println(F("=== Driver Initialization ==="));
    if (!initializeDriver()) {
        Serial.println(F("❌ Driver initialization failed!"));
        Serial.println(F("System halted. Check TLE92466ED power and connections."));
        haltWithError();
    }
    Serial.println(F("✅ Driver initialized successfully"));
    Serial.println();
    
    // Verify communication
    Serial.println(F("=== Communication Verification ==="));
    if (!verifyChipID()) {
        Serial.println(F("❌ Failed to read chip ID!"));
        Serial.println(F("System halted. Check SPI connections."));
        haltWithError();
    }
    Serial.println();
    
    // All initialization successful
    Serial.println(F("╔══════════════════════════════════════════════════════════════╗"));
    Serial.println(F("║     Initialization Complete - Starting Multi-Channel Demo   ║"));
    Serial.println(F("╚══════════════════════════════════════════════════════════════╝"));
    Serial.println();
    
    pinMode(LED_BUILTIN, OUTPUT);
    modeStartTime = millis();
}

/**
 * @brief Main loop - cycles through demo modes
 */
void loop() {
    // Check if it's time to switch modes
    if (millis() - modeStartTime >= MODE_DURATION_MS) {
        // Disable all channels before switching modes
        disableAllChannels();
        delay(500);
        
        // Switch to next mode
        currentMode = static_cast<DemoMode>((currentMode + 1) % 4);
        modeStartTime = millis();
        
        Serial.println();
        Serial.println(F("════════════════════════════════════════════════════════════"));
        printModeName(currentMode);
        Serial.println(F("════════════════════════════════════════════════════════════"));
        Serial.println();
    }
    
    // Execute current demo mode
    switch (currentMode) {
        case SEQUENTIAL:
            demonstrateSequentialControl();
            break;
        case SYNCHRONIZED:
            demonstrateSynchronizedControl();
            break;
        case WAVE_PATTERN:
            demonstrateWavePattern();
            break;
        case PERFORMANCE_TEST:
            demonstratePerformanceTest();
            break;
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
    Serial.println(F("Communication verified!"));
    
    return true;
}

/**
 * @brief Disable all channels
 */
void disableAllChannels() {
    for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
        g_driver->disableChannel(ch);
    }
}

/**
 * @brief Print demo mode name
 */
void printModeName(DemoMode mode) {
    Serial.print(F(">>> Mode: "));
    switch (mode) {
        case SEQUENTIAL:
            Serial.println(F("SEQUENTIAL CHANNEL ACTIVATION"));
            break;
        case SYNCHRONIZED:
            Serial.println(F("SYNCHRONIZED CHANNEL CONTROL"));
            break;
        case WAVE_PATTERN:
            Serial.println(F("WAVE PATTERN DEMONSTRATION"));
            break;
        case PERFORMANCE_TEST:
            Serial.println(F("PERFORMANCE MONITORING TEST"));
            break;
    }
}

/**
 * @brief Demonstrate sequential channel activation
 */
void demonstrateSequentialControl() {
    static uint8_t activeChannel = 0;
    static uint32_t lastUpdate = 0;
    
    if (millis() - lastUpdate >= SEQUENCE_DELAY_MS) {
        // Disable previous channel
        if (activeChannel > 0) {
            g_driver->disableChannel(activeChannel - 1);
        } else {
            g_driver->disableChannel(NUM_CHANNELS - 1);
        }
        
        // Configure and enable current channel
        g_driver->setChannelCurrent(activeChannel, BASE_CURRENT_MA);
        g_driver->enableChannel(activeChannel);
        
        Serial.print(F("Channel ")); Serial.print(activeChannel);
        Serial.print(F(" enabled (")); Serial.print(BASE_CURRENT_MA);
        Serial.println(F(" mA)"));
        
        // LED feedback
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        
        // Move to next channel
        activeChannel = (activeChannel + 1) % NUM_CHANNELS;
        lastUpdate = millis();
    }
}

/**
 * @brief Demonstrate synchronized channel control
 */
void demonstrateSynchronizedControl() {
    static uint32_t lastUpdate = 0;
    static bool allEnabled = false;
    
    if (millis() - lastUpdate >= SEQUENCE_DELAY_MS * 2) {
        if (!allEnabled) {
            // Enable all channels with same current
            Serial.println(F("Enabling all channels simultaneously..."));
            for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
                g_driver->setChannelCurrent(ch, BASE_CURRENT_MA);
                g_driver->enableChannel(ch);
            }
            Serial.print(F("✅ All 6 channels enabled at "));
            Serial.print(BASE_CURRENT_MA);
            Serial.println(F(" mA"));
            allEnabled = true;
        } else {
            // Disable all channels
            Serial.println(F("Disabling all channels..."));
            for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
                g_driver->disableChannel(ch);
            }
            Serial.println(F("✅ All channels disabled"));
            allEnabled = false;
        }
        
        // LED feedback
        digitalWrite(LED_BUILTIN, allEnabled);
        lastUpdate = millis();
    }
}

/**
 * @brief Demonstrate wave pattern across channels
 */
void demonstrateWavePattern() {
    static uint32_t lastUpdate = 0;
    static float phase = 0.0;
    
    if (millis() - lastUpdate >= SEQUENCE_DELAY_MS / 2) {
        Serial.print(F("Wave phase: ")); Serial.println(phase, 2);
        
        // Create sine wave pattern across channels
        for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
            float channelPhase = phase + (ch * 3.14159 / 3.0);  // 60° phase shift
            float amplitude = (sin(channelPhase) + 1.0) / 2.0;   // 0.0 to 1.0
            
            uint16_t current = static_cast<uint16_t>(amplitude * BASE_CURRENT_MA);
            
            if (current > 50) {  // Minimum threshold
                g_driver->setChannelCurrent(ch, current);
                g_driver->enableChannel(ch);
                
                Serial.print(F("  CH")); Serial.print(ch);
                Serial.print(F(": ")); Serial.print(current);
                Serial.println(F(" mA"));
            } else {
                g_driver->disableChannel(ch);
            }
        }
        
        // Advance phase
        phase += 0.3;
        if (phase > 6.28318) {  // 2π
            phase -= 6.28318;
        }
        
        // LED feedback
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        lastUpdate = millis();
        Serial.println();
    }
}

/**
 * @brief Demonstrate performance monitoring
 */
void demonstratePerformanceTest() {
    static uint32_t testStartTime = 0;
    static uint32_t operationCount = 0;
    static bool testRunning = false;
    static uint8_t testPhase = 0;
    
    if (!testRunning) {
        // Start new test
        Serial.println(F("╔══════════════════════════════════════════════════════════════╗"));
        Serial.println(F("║              Performance Monitoring Test                     ║"));
        Serial.println(F("╚══════════════════════════════════════════════════════════════╝"));
        Serial.println();
        
        testStartTime = millis();
        operationCount = 0;
        testRunning = true;
        testPhase = 0;
    }
    
    uint32_t elapsedTime = millis() - testStartTime;
    
    if (elapsedTime < 5000) {
        // Rapid channel switching test
        uint8_t ch = testPhase % NUM_CHANNELS;
        
        g_driver->setChannelCurrent(ch, BASE_CURRENT_MA);
        g_driver->enableChannel(ch);
        delayMicroseconds(100);
        g_driver->disableChannel(ch);
        
        operationCount++;
        testPhase++;
        
        // LED feedback at slower rate
        if (testPhase % 100 == 0) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        }
    } else {
        // Print results
        Serial.println(F("Performance Test Results:"));
        Serial.print(F("  Total Operations: ")); Serial.println(operationCount);
        Serial.print(F("  Test Duration: ")); Serial.print(elapsedTime);
        Serial.println(F(" ms"));
        Serial.print(F("  Operations/Second: "));
        Serial.println((operationCount * 1000UL) / elapsedTime);
        Serial.println();
        
        // Reset for next test
        testRunning = false;
        delay(2000);
    }
}

/**
 * @brief Halt system with error indication
 */
void haltWithError() {
    while (1) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(200);
        digitalWrite(LED_BUILTIN, LOW);
        delay(200);
    }
}

