/**
 * @file Arduino_HAL.hpp
 * @brief Arduino Hardware Abstraction Layer for TLE92466ED Driver
 * 
 * This HAL implementation provides Arduino platform support for the TLE92466ED
 * driver, using the Arduino SPI library for communication.
 * 
 * Compatible with:
 * - Arduino Uno/Nano (ATmega328P)
 * - Arduino Mega 2560 (ATmega2560)
 * - Arduino Due (SAM3X8E)
 * - Arduino Zero/M0 (SAMD21)
 * - Any Arduino-compatible board with SPI support
 * 
 * Hardware connections for Arduino Uno/Nano:
 * - TLE92466ED MISO -> Arduino Pin 12 (MISO)
 * - TLE92466ED MOSI -> Arduino Pin 11 (MOSI)
 * - TLE92466ED SCK  -> Arduino Pin 13 (SCK)
 * - TLE92466ED CS   -> Arduino Pin 10 (SS) or any digital pin
 * - TLE92466ED EN   -> Arduino Pin 9 (configurable)
 * - TLE92466ED RESN -> Arduino Pin 8 (configurable)
 * - TLE92466ED VDD  -> Arduino 5V
 * - TLE92466ED VIO  -> Arduino 5V (or 3.3V for 3.3V boards)
 * - TLE92466ED GND  -> Arduino GND
 * 
 * @author N3b3x
 * @date 2025-10-21
 * @version 2.0.0
 */

#pragma once

#include <Arduino.h>
#include <SPI.h>
#include "TLE92466ED_CommInterface.hpp"

/**
 * @class Arduino_HAL
 * @brief Arduino implementation of TLE92466ED HAL interface
 * 
 * This class provides SPI communication and timing functions for Arduino
 * platforms. It uses the standard Arduino SPI library and is compatible
 * with most Arduino boards.
 */
class Arduino_HAL : public TLE92466ED::CommInterface {
public:
    /**
     * @struct SPIConfig
     * @brief SPI configuration for Arduino
     */
    struct SPIConfig {
        uint8_t cs_pin = 10;        ///< Chip Select pin (SS)
        uint8_t en_pin = 9;         ///< Enable pin (optional)
        uint8_t resn_pin = 8;       ///< Reset pin (optional)
        uint32_t spi_frequency = 1000000;  ///< SPI frequency (Hz) - 1MHz default
        uint8_t spi_mode = SPI_MODE0;      ///< SPI mode (MODE0 for TLE92466ED)
        uint8_t spi_bit_order = MSBFIRST;  ///< Bit order (MSB first)
    };

    /**
     * @brief Construct Arduino HAL with configuration
     * @param config SPI configuration
     */
    explicit Arduino_HAL(const SPIConfig& config = SPIConfig())
        : m_config(config)
        , m_initialized(false)
    {
        // Constructor body - initialization happens in initialize()
    }

    /**
     * @brief Destructor - cleanup resources
     */
    ~Arduino_HAL() override {
        if (m_initialized) {
            // Set CS high
            digitalWrite(m_config.cs_pin, HIGH);
            // Disable device if EN pin is used
            if (m_config.en_pin != 255) {
                digitalWrite(m_config.en_pin, LOW);
            }
        }
    }

    /**
     * @brief Initialize the Arduino HAL
     * @return Success or error
     */
    auto initialize() noexcept -> std::expected<void, TLE92466ED_Error> override {
        if (m_initialized) {
            return {};  // Already initialized
        }

        // Configure CS pin
        pinMode(m_config.cs_pin, OUTPUT);
        digitalWrite(m_config.cs_pin, HIGH);  // CS idle high

        // Configure EN pin if specified
        if (m_config.en_pin != 255) {
            pinMode(m_config.en_pin, OUTPUT);
            digitalWrite(m_config.en_pin, HIGH);  // Enable device
        }

        // Configure RESN pin if specified
        if (m_config.resn_pin != 255) {
            pinMode(m_config.resn_pin, OUTPUT);
            digitalWrite(m_config.resn_pin, HIGH);  // Not in reset

            // Perform hardware reset
            digitalWrite(m_config.resn_pin, LOW);
            delay(10);  // Hold reset for 10ms
            digitalWrite(m_config.resn_pin, HIGH);
            delay(10);  // Wait for device to come out of reset
        }

        // Initialize SPI
        SPI.begin();

        m_initialized = true;

        return {};
    }

    /**
     * @brief Transfer 32-bit SPI frame to/from TLE92466ED
     * @param tx_data Data to transmit (32 bits)
     * @return Received data (32 bits) or error
     */
    auto spiTransfer(uint32_t tx_data) noexcept -> std::expected<uint32_t, TLE92466ED_Error> override {
        if (!m_initialized) {
            return std::unexpected(TLE92466ED_Error::NOT_INITIALIZED);
        }

        // Begin SPI transaction with configured settings
        SPI.beginTransaction(SPISettings(
            m_config.spi_frequency,
            m_config.spi_bit_order,
            m_config.spi_mode
        ));

        // Assert CS (active LOW)
        digitalWrite(m_config.cs_pin, LOW);
        
        // Small delay for CS setup time
        delayMicroseconds(1);

        // Transfer 4 bytes (32 bits), MSB first
        uint32_t rx_data = 0;
        rx_data |= static_cast<uint32_t>(SPI.transfer((tx_data >> 24) & 0xFF)) << 24;
        rx_data |= static_cast<uint32_t>(SPI.transfer((tx_data >> 16) & 0xFF)) << 16;
        rx_data |= static_cast<uint32_t>(SPI.transfer((tx_data >> 8) & 0xFF)) << 8;
        rx_data |= static_cast<uint32_t>(SPI.transfer(tx_data & 0xFF));

        // Small delay before deasserting CS
        delayMicroseconds(1);

        // Deassert CS (idle HIGH)
        digitalWrite(m_config.cs_pin, HIGH);

        // End SPI transaction
        SPI.endTransaction();

        return rx_data;
    }

    /**
     * @brief Delay for specified microseconds
     * @param microseconds Delay duration in microseconds
     */
    auto delayMicroseconds(uint32_t microseconds) noexcept -> void override {
        ::delayMicroseconds(microseconds);
    }

    /**
     * @brief Check if HAL is initialized
     * @return true if initialized
     */
    auto isInitialized() const noexcept -> bool {
        return m_initialized;
    }

    /**
     * @brief Get current SPI configuration
     * @return Reference to SPI configuration
     */
    auto getConfig() const noexcept -> const SPIConfig& {
        return m_config;
    }

    /**
     * @brief Enable the TLE92466ED device (if EN pin is configured)
     */
    auto enableDevice() noexcept -> void {
        if (m_config.en_pin != 255) {
            digitalWrite(m_config.en_pin, HIGH);
        }
    }

    /**
     * @brief Disable the TLE92466ED device (if EN pin is configured)
     */
    auto disableDevice() noexcept -> void {
        if (m_config.en_pin != 255) {
            digitalWrite(m_config.en_pin, LOW);
        }
    }

    /**
     * @brief Reset the TLE92466ED device (if RESN pin is configured)
     */
    auto resetDevice() noexcept -> void {
        if (m_config.resn_pin != 255) {
            digitalWrite(m_config.resn_pin, LOW);
            delay(10);
            digitalWrite(m_config.resn_pin, HIGH);
            delay(10);
        }
    }

private:
    SPIConfig m_config;      ///< SPI configuration
    bool m_initialized;      ///< Initialization state
};

/**
 * @brief Create Arduino HAL instance with default configuration
 * @return Unique pointer to Arduino HAL
 */
inline auto createTLE92466ED_HAL() -> Arduino_HAL* {
    return new Arduino_HAL();
}

/**
 * @brief Create Arduino HAL instance with custom configuration
 * @param config SPI configuration
 * @return Unique pointer to Arduino HAL
 */
inline auto createTLE92466ED_HAL(const Arduino_HAL::SPIConfig& config) -> Arduino_HAL* {
    return new Arduino_HAL(config);
}

