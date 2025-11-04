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

using namespace TLE92466ED;

/**
 * @class ESP32C6_HAL
 * @brief ESP32-C6 implementation of the TLE92466ED HAL interface
 * 
 * This class provides the platform-specific implementation for ESP32-C6,
 * handling SPI communication with proper timing and error handling.
 */
class ESP32C6_HAL : public TLE92466ED::HAL {
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
     * @return HALResult<void> Success or error
     */
    auto init() noexcept -> HALResult<void> override;

    /**
     * @brief Deinitialize the HAL
     * @return HALResult<void> Success or error
     */
    auto deinit() noexcept -> HALResult<void> override;

    /**
     * @brief Transfer 32-bit data via SPI (full-duplex)
     * @param tx_data The 32-bit data to transmit
     * @return HALResult<uint32_t> Received 32-bit data or error
     */
    auto transfer32(uint32_t tx_data) noexcept -> HALResult<uint32_t> override;

    /**
     * @brief Transfer multiple 32-bit words via SPI
     * @param tx_data Span of transmit data (32-bit words)
     * @param rx_data Span to store received data (32-bit words)
     * @return HALResult<void> Success or error
     */
    auto transfer_multi(std::span<const uint32_t> tx_data,
                       std::span<uint32_t> rx_data) noexcept -> HALResult<void> override;

    /**
     * @brief Assert (activate) chip select
     * @return HALResult<void> Success or error
     */
    auto chip_select() noexcept -> HALResult<void> override;

    /**
     * @brief Deassert (deactivate) chip select
     * @return HALResult<void> Success or error
     */
    auto chip_deselect() noexcept -> HALResult<void> override;

    /**
     * @brief Delay for specified duration
     * @param microseconds Duration to delay in microseconds
     * @return HALResult<void> Success or error
     */
    auto delay(uint32_t microseconds) noexcept -> HALResult<void> override;

    /**
     * @brief Configure SPI parameters
     * @param config New SPI configuration
     * @return HALResult<void> Success or error
     */
    auto configure(const TLE92466ED::SPIConfig& config) noexcept -> HALResult<void> override;

    /**
     * @brief Check if hardware is ready for communication
     * @return true if ready, false otherwise
     */
    bool is_ready() const noexcept override;

    /**
     * @brief Get the last error that occurred
     * @return HALError The last error code
     */
    HALError get_last_error() const noexcept override;

    /**
     * @brief Clear any pending errors
     * @return HALResult<void> Success or error
     */
    auto clear_errors() noexcept -> HALResult<void> override;

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
    HALError last_error_ = HALError::None;      ///< Last error that occurred
    
    static constexpr const char* TAG = "ESP32C6_HAL"; ///< Logging tag

    /**
     * @brief Initialize SPI bus
     * @return HALResult<void> Success or error
     */
    auto initializeSPI() noexcept -> HALResult<void>;

    /**
     * @brief Add SPI device to the bus
     * @return HALResult<void> Success or error  
     */
    auto addSPIDevice() noexcept -> HALResult<void>;
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
