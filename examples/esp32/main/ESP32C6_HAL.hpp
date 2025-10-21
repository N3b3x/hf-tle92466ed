/**
 * @file ESP32C6_HAL.hpp
 * @brief ESP32-C6 Hardware Abstraction Layer for TLE92466ED driver
 * 
 * This file provides the ESP32-C6 specific implementation of the TLE92466ED HAL interface.
 * It handles SPI communication and timing functions required by the driver.
 * 
 * @author N3b3x
 * @date 2025-10-21
 * @version 2.0.0
 */

#pragma once

#include "TLE92466ED_HAL.hpp"
#include "TLE92466ED_Config.hpp"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <memory>

/**
 * @class ESP32C6_HAL
 * @brief ESP32-C6 implementation of the TLE92466ED HAL interface
 * 
 * This class provides the platform-specific implementation for ESP32-C6,
 * handling SPI communication with proper timing and error handling.
 */
class ESP32C6_HAL : public TLE92466ED_HAL {
public:
    /**
     * @brief SPI configuration structure for ESP32-C6
     */
    struct SPIConfig {
        spi_host_device_t host = SPI2_HOST;     ///< SPI host (SPI2_HOST for ESP32-C6)
        int miso_pin = 2;                       ///< MISO pin (GPIO2)
        int mosi_pin = 7;                       ///< MOSI pin (GPIO7)  
        int sclk_pin = 6;                       ///< SCLK pin (GPIO6)
        int cs_pin = 10;                        ///< CS pin (GPIO10)
        int frequency = 1000000;                ///< SPI frequency (1MHz)
        int mode = 0;                           ///< SPI mode (0 = CPOL=0, CPHA=0)
        int queue_size = 1;                     ///< Transaction queue size
    };

    /**
     * @brief Constructor with default SPI configuration
     */
    ESP32C6_HAL() : ESP32C6_HAL(SPIConfig{}) {}

    /**
     * @brief Constructor with custom SPI configuration
     * @param config SPI configuration parameters
     */
    explicit ESP32C6_HAL(const SPIConfig& config);

    /**
     * @brief Destructor - cleans up SPI resources
     */
    ~ESP32C6_HAL() override;

    /**
     * @brief Initialize the HAL (must be called before use)
     * @return std::expected<void, HALError> Success or error
     */
    auto initialize() noexcept -> std::expected<void, HALError>;

    /**
     * @brief Perform SPI transfer (32-bit frames for TLE92466ED)
     * @param txData Transmit data buffer
     * @param rxData Receive data buffer  
     * @return std::expected<void, HALError> Success or error
     */
    auto spiTransfer(std::span<const uint8_t> txData, 
                     std::span<uint8_t> rxData) noexcept 
        -> std::expected<void, HALError> override;

    /**
     * @brief Microsecond delay implementation
     * @param us Delay in microseconds
     */
    void delayMicroseconds(uint32_t us) noexcept override;

    /**
     * @brief Get the current SPI configuration
     * @return Current SPI configuration
     */
    auto getConfig() const noexcept -> const SPIConfig& { return config_; }

    /**
     * @brief Check if HAL is initialized
     * @return true if initialized, false otherwise
     */
    auto isInitialized() const noexcept -> bool { return initialized_; }

private:
    SPIConfig config_;                          ///< SPI configuration
    spi_device_handle_t spi_device_ = nullptr; ///< SPI device handle
    bool initialized_ = false;                  ///< Initialization state
    
    static constexpr const char* TAG = "ESP32C6_HAL"; ///< Logging tag

    /**
     * @brief Initialize SPI bus
     * @return std::expected<void, HALError> Success or error
     */
    auto initializeSPI() noexcept -> std::expected<void, HALError>;

    /**
     * @brief Add SPI device to the bus
     * @return std::expected<void, HALError> Success or error  
     */
    auto addSPIDevice() noexcept -> std::expected<void, HALError>;
};

/**
 * @brief Create a configured ESP32C6_HAL instance for TLE92466ED
 * @return Unique pointer to configured HAL instance
 * 
 * This function uses the configuration from TLE92466ED_Config.hpp
 */
inline auto createTLE92466ED_HAL() -> std::unique_ptr<ESP32C6_HAL> {
    using namespace TLE92466ED_Config;
    
    ESP32C6_HAL::SPIConfig config;
    
    // Configuration from TLE92466ED_Config.hpp
    config.host = SPI2_HOST;
    config.miso_pin = SPIPins::MISO;
    config.mosi_pin = SPIPins::MOSI;
    config.sclk_pin = SPIPins::SCLK;
    config.cs_pin = SPIPins::CS;
    config.frequency = SPIParams::FREQUENCY;
    config.mode = SPIParams::MODE;
    config.queue_size = SPIParams::QUEUE_SIZE;
    
    return std::make_unique<ESP32C6_HAL>(config);
}
