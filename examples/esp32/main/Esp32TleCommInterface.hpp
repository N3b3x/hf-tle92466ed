/**
 * @file Esp32TleCommInterface.hpp
 * @brief ESP32 Communication Interface implementation for TLE92466ED driver
 * 
 * This file provides the ESP32-specific implementation of the TLE92466ED CommInterface.
 * It handles SPI communication and timing functions required by the driver.
 * 
 * @author N3b3x
 * @date 2025-10-21
 * @version 2.0.0
 */

#pragma once

#include "TLE92466ED_CommInterface.hpp"
#include "TLE92466ED_TestConfig.hpp"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <memory>

using namespace TLE92466ED;

/**
 * @class Esp32TleCommInterface
 * @brief ESP32 implementation of the TLE92466ED CommInterface
 * 
 * This class provides the platform-specific implementation for ESP32,
 * handling SPI communication with proper timing and error handling.
 */
class Esp32TleCommInterface : public TLE92466ED::CommInterface {
public:
    /**
     * @brief SPI configuration structure for ESP32
     */
    struct SPIConfig {
        spi_host_device_t host = SPI2_HOST;     ///< SPI host (SPI2_HOST for ESP32-C6)
        int16_t miso_pin = 2;                   ///< MISO pin (GPIO2, -1 = not configured)
        int16_t mosi_pin = 7;                   ///< MOSI pin (GPIO7, -1 = not configured)
        int16_t sclk_pin = 6;                   ///< SCLK pin (GPIO6, -1 = not configured)
        int16_t cs_pin = 10;                    ///< CS pin (GPIO10, -1 = not configured)
        int16_t resn_pin = -1;                  ///< RESN pin (active low, -1 = not configured)
        int16_t en_pin = -1;                    ///< EN pin (active high, -1 = not configured)
        int16_t faultn_pin = -1;                ///< FAULTN pin (active low, -1 = not configured)
        int16_t drv0_pin = -1;                  ///< DRV0 pin (external drive control, -1 = not configured)
        int16_t drv1_pin = -1;                  ///< DRV1 pin (external drive control, -1 = not configured)
        uint32_t frequency = 1000000;           ///< SPI frequency (1MHz)
        uint8_t mode = 1;                       ///< SPI mode (1 = CPOL=0, CPHA=1)
        uint8_t queue_size = 1;                 ///< Transaction queue size
        uint8_t cs_ena_pretrans = 1;            ///< CS asserted N clock cycles before transaction
        uint8_t cs_ena_posttrans = 1;           ///< CS held N clock cycles after transaction
    };

    /**
     * @brief Constructor with default SPI configuration
     */
    Esp32TleCommInterface() : Esp32TleCommInterface(SPIConfig{}) {}

    /**
     * @brief Constructor with custom SPI configuration
     * @param config SPI configuration parameters
     */
    explicit Esp32TleCommInterface(const SPIConfig& config);

    /**
     * @brief Destructor - cleans up SPI resources
     */
    ~Esp32TleCommInterface() override;

    /**
     * @brief Initialize the CommInterface (must be called before use)
     * @return CommResult<void> Success or error
     */
    auto Init() noexcept -> CommResult<void> override;

    /**
     * @brief Deinitialize the CommInterface
     * @return CommResult<void> Success or error
     */
    auto Deinit() noexcept -> CommResult<void> override;

    /**
     * @brief Transfer 32-bit data via SPI (full-duplex)
     * @param tx_data The 32-bit data to transmit
     * @return CommResult<uint32_t> Received 32-bit data or error
     */
    auto Transfer32(uint32_t tx_data) noexcept -> CommResult<uint32_t> override;

    /**
     * @brief Transfer multiple 32-bit words via SPI
     * @param tx_data Span of transmit data (32-bit words)
     * @param rx_data Span to store received data (32-bit words)
     * @return CommResult<void> Success or error
     */
    auto TransferMulti(std::span<const uint32_t> tx_data,
                       std::span<uint32_t> rx_data) noexcept -> CommResult<void> override;

    /**
     * @brief Delay for specified duration
     * @param microseconds Duration to delay in microseconds
     * @return CommResult<void> Success or error
     */
    auto Delay(uint32_t microseconds) noexcept -> CommResult<void> override;

    /**
     * @brief Configure SPI parameters
     * @param config New SPI configuration
     * @return CommResult<void> Success or error
     */
    auto Configure(const TLE92466ED::SPIConfig& config) noexcept -> CommResult<void> override;

    /**
     * @brief Check if hardware is ready for communication
     * @return true if ready, false otherwise
     */
    bool IsReady() const noexcept override;

    /**
     * @brief Get the last error that occurred
     * @return CommError The last error code
     */
    CommError GetLastError() const noexcept override;

    /**
     * @brief Clear any pending errors
     * @return CommResult<void> Success or error
     */
    auto ClearErrors() noexcept -> CommResult<void> override;

    /**
     * @brief Get the current SPI configuration
     * @return Current SPI configuration
     */
    auto getConfig() const noexcept -> const SPIConfig& { return config_; }

    /**
     * @brief Set GPIO control pin level
     * @param pin Control pin to set (RESN or EN)
     * @param level Active level (HIGH or LOW)
     * @return CommResult<void> Success or error
     */
    auto SetGpioPin(ControlPin pin, ActiveLevel level) noexcept -> CommResult<void> override;

    /**
     * @brief Get GPIO control pin level
     * @param pin Control pin to read (FAULTN)
     * @return CommResult<ActiveLevel> Pin level or error
     */
    auto GetGpioPin(ControlPin pin) noexcept -> CommResult<ActiveLevel> override;

    /**
     * @brief Log a message with specified severity level and tag (ESP_LOG implementation)
     * @param level Log severity level
     * @param tag Tag/component name for the log message
     * @param format Format string (printf-style)
     * @param ... Variable arguments for format string
     */
    void Log(LogLevel level, const char* tag, const char* format, ...) noexcept override;

    /**
     * @brief Check if CommInterface is initialized
     * @return true if initialized, false otherwise
     */
    auto isInitialized() const noexcept -> bool { return initialized_; }

private:
    SPIConfig config_;                          ///< SPI configuration
    spi_device_handle_t spi_device_ = nullptr;  ///< SPI device handle
    bool initialized_ = false;                  ///< Initialization state
    CommError last_error_ = CommError::None;    ///< Last error that occurred
    
    static constexpr const char* TAG = "Esp32TleComm"; ///< Logging tag

    /**
     * @brief Initialize GPIO pins (RESN, EN, FAULTN)
     * @return CommResult<void> Success or error
     */
    auto initializeGPIO() noexcept -> CommResult<void>;

    /**
     * @brief Initialize SPI bus
     * @return CommResult<void> Success or error
     */
    auto initializeSPI() noexcept -> CommResult<void>;

    /**
     * @brief Add SPI device to the bus
     * @return CommResult<void> Success or error  
     */
    auto addSPIDevice() noexcept -> CommResult<void>;
};

/**
 * @brief Create a configured Esp32TleCommInterface instance for TLE92466ED
 * @return Unique pointer to configured CommInterface instance
 * 
 * This function uses the configuration from TLE92466ED_TestConfig.hpp
 */
inline auto CreateEsp32TleCommInterface() -> std::unique_ptr<Esp32TleCommInterface> {
    using namespace TLE92466ED_TestConfig;
    
    Esp32TleCommInterface::SPIConfig config;
    
    // SPI pins from TLE92466ED_TestConfig.hpp
    config.host = SPI2_HOST;
    config.miso_pin = SPIPins::MISO;
    config.mosi_pin = SPIPins::MOSI;
    config.sclk_pin = SPIPins::SCLK;
    config.cs_pin = SPIPins::CS;
    
    // Control GPIO pins from TLE92466ED_TestConfig.hpp
    config.resn_pin = ControlPins::RESN;
    config.en_pin = ControlPins::EN;
    config.faultn_pin = ControlPins::FAULTN;
    config.drv0_pin = ControlPins::DRV0;
    config.drv1_pin = ControlPins::DRV1;
    
    // SPI parameters from TLE92466ED_TestConfig.hpp
    config.frequency = SPIParams::FREQUENCY;
    config.mode = SPIParams::MODE;
    config.queue_size = SPIParams::QUEUE_SIZE;
    config.cs_ena_pretrans = SPIParams::CS_ENA_PRETRANS;
    config.cs_ena_posttrans = SPIParams::CS_ENA_POSTTRANS;
    
    return std::make_unique<Esp32TleCommInterface>(config);
}

