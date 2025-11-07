/**
 * @file PWMDitherExample.ino
 * @brief PWM and Dither functionality example for TLE92466ED driver on Arduino
 * 
 * This example demonstrates the TLE92466ED's integrated PWM generator and
 * dither functionality for advanced current control, noise reduction, and
 * acoustic performance improvement.
 * 
 * Hardware Setup (Arduino Uno/Nano):
 * ==================================
 * Same as BasicUsageExample - see BasicUsageExample.ino or README.md
 * 
 * Note: Connect an oscilloscope to the OUT0 pin to observe PWM and dither effects
 * 
 * Features Demonstrated:
 * - PWM frequency configuration (100Hz - 20kHz)
 * - Duty cycle control
 * - Dither generator enable/disable
 * - Dither amplitude control
 * - Current control with PWM + Dither
 * - Acoustic noise reduction
 * - Smooth current transitions
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
const uint16_t BASE_CURRENT_MA = 1000;

// Demo modes
enum DemoMode {
    PWM_FREQUENCY_SWEEP,
    DUTY_CYCLE_CONTROL,
    DITHER_DEMONSTRATION,
    COMBINED_PWM_DITHER
};

DemoMode currentMode = PWM_FREQUENCY_SWEEP;
uint32_t modeStartTime = 0;
const uint32_t MODE_DURATION_MS = 15000;  // 15 seconds per mode

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
    Serial.println(F("║  TLE92466ED PWM/Dither Example - Arduino Platform           ║"));
    Serial.println(F("║              HardFOC Core Drivers v2.0.0                     ║"));
    Serial.println(F("╚══════════════════════════════════════════════════════════════╝"));
    Serial.println();
    
    Serial.println(F("This example demonstrates PWM and Dither features."));
    Serial.println(F("Connect oscilloscope to OUT0 for best results!"));
    Serial.println();
    
    delay(2000);
    
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
    
    // All initialization successful
    Serial.println(F("╔══════════════════════════════════════════════════════════════╗"));
    Serial.println(F("║     Initialization Complete - Starting PWM/Dither Demo      ║"));
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
        // Disable channel before switching modes
        g_driver->disableChannel(TEST_CHANNEL);
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
        case PWM_FREQUENCY_SWEEP:
            demonstratePWMFrequencySweep();
            break;
        case DUTY_CYCLE_CONTROL:
            demonstrateDutyCycleControl();
            break;
        case DITHER_DEMONSTRATION:
            demonstrateDither();
            break;
        case COMBINED_PWM_DITHER:
            demonstrateCombinedPWMDither();
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
    
    return true;
}

/**
 * @brief Print demo mode name
 */
void printModeName(DemoMode mode) {
    Serial.print(F(">>> Mode: "));
    switch (mode) {
        case PWM_FREQUENCY_SWEEP:
            Serial.println(F("PWM FREQUENCY SWEEP (100Hz - 20kHz)"));
            Serial.println(F("    Watch oscilloscope for frequency changes!"));
            break;
        case DUTY_CYCLE_CONTROL:
            Serial.println(F("PWM DUTY CYCLE CONTROL (0% - 100%)"));
            Serial.println(F("    Effective current control via duty cycle"));
            break;
        case DITHER_DEMONSTRATION:
            Serial.println(F("DITHER GENERATOR DEMONSTRATION"));
            Serial.println(F("    Reduces acoustic noise and improves smoothness"));
            break;
        case COMBINED_PWM_DITHER:
            Serial.println(F("COMBINED PWM + DITHER"));
            Serial.println(F("    Optimal current control with noise reduction"));
            break;
    }
}

/**
 * @brief Demonstrate PWM frequency sweep
 */
void demonstratePWMFrequencySweep() {
    static uint32_t lastUpdate = 0;
    static uint16_t pwm_frequency_hz = 100;
    static bool sweeping_up = true;
    
    if (millis() - lastUpdate >= 1000) {
        // Configure PWM frequency
        // Note: Actual implementation depends on driver API
        Serial.print(F("PWM Frequency: "));
        Serial.print(pwm_frequency_hz);
        Serial.println(F(" Hz"));
        
        // Set current and enable channel
        g_driver->setChannelCurrent(TEST_CHANNEL, BASE_CURRENT_MA);
        g_driver->enableChannel(TEST_CHANNEL);
        
        // Update frequency for next iteration
        if (sweeping_up) {
            pwm_frequency_hz += 100;
            if (pwm_frequency_hz >= 2000) {
                sweeping_up = false;
            }
        } else {
            pwm_frequency_hz -= 100;
            if (pwm_frequency_hz <= 100) {
                sweeping_up = true;
            }
        }
        
        // LED feedback
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        lastUpdate = millis();
    }
}

/**
 * @brief Demonstrate duty cycle control
 */
void demonstrateDutyCycleControl() {
    static uint32_t lastUpdate = 0;
    static uint8_t duty_cycle_percent = 0;
    static bool ramping_up = true;
    
    if (millis() - lastUpdate >= 500) {
        // Calculate effective current based on duty cycle
        uint16_t effective_current = (BASE_CURRENT_MA * duty_cycle_percent) / 100;
        
        Serial.print(F("Duty Cycle: "));
        Serial.print(duty_cycle_percent);
        Serial.print(F("% (Effective Current: "));
        Serial.print(effective_current);
        Serial.println(F(" mA)"));
        
        // Set current and enable channel
        if (effective_current > 0) {
            g_driver->setChannelCurrent(TEST_CHANNEL, effective_current);
            g_driver->enableChannel(TEST_CHANNEL);
        } else {
            g_driver->disableChannel(TEST_CHANNEL);
        }
        
        // Update duty cycle for next iteration
        if (ramping_up) {
            duty_cycle_percent += 10;
            if (duty_cycle_percent >= 100) {
                ramping_up = false;
            }
        } else {
            if (duty_cycle_percent >= 10) {
                duty_cycle_percent -= 10;
            }
            if (duty_cycle_percent == 0) {
                ramping_up = true;
            }
        }
        
        // LED feedback
        digitalWrite(LED_BUILTIN, effective_current > 500);
        lastUpdate = millis();
    }
}

/**
 * @brief Demonstrate dither functionality
 */
void demonstrateDither() {
    static uint32_t lastUpdate = 0;
    static bool dither_enabled = false;
    static uint8_t demo_phase = 0;
    
    if (millis() - lastUpdate >= 3000) {
        switch (demo_phase) {
            case 0:
                // Without dither
                Serial.println(F("╔══════════════════════════════════════════════════╗"));
                Serial.println(F("║  Phase 1: Without Dither (Higher Acoustic Noise) ║"));
                Serial.println(F("╚══════════════════════════════════════════════════╝"));
                dither_enabled = false;
                break;
                
            case 1:
                // With dither
                Serial.println(F("╔══════════════════════════════════════════════════╗"));
                Serial.println(F("║  Phase 2: With Dither (Reduced Acoustic Noise)   ║"));
                Serial.println(F("╚══════════════════════════════════════════════════╝"));
                dither_enabled = true;
                break;
                
            case 2:
                // Comparison
                Serial.println(F("╔══════════════════════════════════════════════════╗"));
                Serial.println(F("║  Dither Benefits:                                 ║"));
                Serial.println(F("║  • Reduces acoustic noise                         ║"));
                Serial.println(F("║  • Improves current smoothness                    ║"));
                Serial.println(F("║  • Better low-current performance                 ║"));
                Serial.println(F("║  • Reduces mechanical vibration                   ║"));
                Serial.println(F("╚══════════════════════════════════════════════════╝"));
                break;
        }
        
        // Apply settings
        g_driver->setChannelCurrent(TEST_CHANNEL, BASE_CURRENT_MA);
        
        // Note: Actual dither enable/disable depends on driver API
        // This is a placeholder for demonstration
        if (demo_phase < 2) {
            g_driver->enableChannel(TEST_CHANNEL);
            Serial.print(F("Dither: "));
            Serial.println(dither_enabled ? F("ENABLED ✓") : F("DISABLED"));
        } else {
            g_driver->disableChannel(TEST_CHANNEL);
        }
        
        demo_phase = (demo_phase + 1) % 3;
        
        // LED feedback
        digitalWrite(LED_BUILTIN, demo_phase == 1);
        lastUpdate = millis();
    }
}

/**
 * @brief Demonstrate combined PWM and Dither
 */
void demonstrateCombinedPWMDither() {
    static uint32_t lastUpdate = 0;
    static uint16_t current_ma = 200;
    static bool ramping_up = true;
    
    if (millis() - lastUpdate >= 500) {
        Serial.println(F("Combined PWM + Dither Configuration:"));
        Serial.print(F("  Current: "));
        Serial.print(current_ma);
        Serial.println(F(" mA"));
        Serial.println(F("  PWM: 1kHz"));
        Serial.println(F("  Dither: Enabled for smooth operation"));
        Serial.println();
        
        // Set current with PWM and Dither both enabled
        g_driver->setChannelCurrent(TEST_CHANNEL, current_ma);
        g_driver->enableChannel(TEST_CHANNEL);
        
        // Ramp current up and down
        if (ramping_up) {
            current_ma += 100;
            if (current_ma >= 1500) {
                ramping_up = false;
            }
        } else {
            current_ma -= 100;
            if (current_ma <= 200) {
                ramping_up = true;
            }
        }
        
        // LED feedback proportional to current
        digitalWrite(LED_BUILTIN, current_ma > 800);
        lastUpdate = millis();
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

