/**
 * @file ESP32C6_HAL.cpp
 * @brief ESP32-C6 Hardware Abstraction Layer implementation for TLE92466ED driver
 * 
 * @author N3b3x
 * @date 2025-10-21
 * @version 2.0.0
 */

#include "ESP32C6_HAL.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

ESP32C6_HAL::ESP32C6_HAL(const SPIConfig& config) : config_(config) {
    ESP_LOGI(TAG, "ESP32C6_HAL created with SPI config:");
    ESP_LOGI(TAG, "  Host: %d", static_cast<int>(config_.host));
    ESP_LOGI(TAG, "  MISO: GPIO%d", config_.miso_pin);
    ESP_LOGI(TAG, "  MOSI: GPIO%d", config_.mosi_pin);
    ESP_LOGI(TAG, "  SCLK: GPIO%d", config_.sclk_pin);
    ESP_LOGI(TAG, "  CS: GPIO%d", config_.cs_pin);
    ESP_LOGI(TAG, "  Frequency: %d Hz", config_.frequency);
    ESP_LOGI(TAG, "  Mode: %d", config_.mode);
}

ESP32C6_HAL::~ESP32C6_HAL() {
    if (spi_device_ != nullptr) {
        spi_bus_remove_device(spi_device_);
        spi_device_ = nullptr;
    }
    
    if (initialized_) {
        spi_bus_free(config_.host);
        initialized_ = false;
    }
    
    ESP_LOGI(TAG, "ESP32C6_HAL destroyed");
}

auto ESP32C6_HAL::initialize() noexcept -> std::expected<void, HALError> {
    if (initialized_) {
        ESP_LOGW(TAG, "HAL already initialized");
        return {};
    }

    ESP_LOGI(TAG, "Initializing ESP32C6_HAL...");

    // Initialize SPI bus
    if (auto result = initializeSPI(); !result) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus");
        return std::unexpected(result.error());
    }

    // Add SPI device
    if (auto result = addSPIDevice(); !result) {
        ESP_LOGE(TAG, "Failed to add SPI device");
        spi_bus_free(config_.host);
        return std::unexpected(result.error());
    }

    initialized_ = true;
    ESP_LOGI(TAG, "ESP32C6_HAL initialized successfully");
    return {};
}

auto ESP32C6_HAL::initializeSPI() noexcept -> std::expected<void, HALError> {
    spi_bus_config_t bus_config = {
        .mosi_io_num = config_.mosi_pin,
        .miso_io_num = config_.miso_pin,
        .sclk_io_num = config_.sclk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = 32,  // TLE92466ED uses 32-bit frames
        .flags = SPICOMMON_BUSFLAG_MASTER,
        .isr_cpu_id = INTR_CPU_ID_AUTO,
        .intr_flags = 0
    };

    esp_err_t ret = spi_bus_initialize(config_.host, &bus_config, SPI_DMA_DISABLED);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus initialization failed: %s", esp_err_to_name(ret));
        return std::unexpected(HALError::InitializationFailed);
    }

    ESP_LOGI(TAG, "SPI bus initialized successfully");
    return {};
}

auto ESP32C6_HAL::addSPIDevice() noexcept -> std::expected<void, HALError> {
    spi_device_interface_config_t dev_config = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = static_cast<uint8_t>(config_.mode),
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .duty_cycle_pos = 128,  // 50% duty cycle
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz = config_.frequency,
        .input_delay_ns = 0,
        .spics_io_num = config_.cs_pin,
        .flags = SPI_DEVICE_HALFDUPLEX,  // TLE92466ED uses half-duplex
        .queue_size = config_.queue_size,
        .pre_cb = nullptr,
        .post_cb = nullptr
    };

    esp_err_t ret = spi_bus_add_device(config_.host, &dev_config, &spi_device_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return std::unexpected(HALError::InitializationFailed);
    }

    ESP_LOGI(TAG, "SPI device added successfully");
    return {};
}

auto ESP32C6_HAL::spiTransfer(std::span<const uint8_t> txData, 
                              std::span<uint8_t> rxData) noexcept 
    -> std::expected<void, HALError> {
    
    if (!initialized_) {
        ESP_LOGE(TAG, "HAL not initialized");
        return std::unexpected(HALError::NotInitialized);
    }

    if (txData.size() != 4 || rxData.size() != 4) {
        ESP_LOGE(TAG, "Invalid transfer size. TLE92466ED requires 4-byte transfers");
        return std::unexpected(HALError::InvalidParameter);
    }

    ESP_LOGD(TAG, "SPI Transfer - TX: 0x%02X%02X%02X%02X", 
             txData[0], txData[1], txData[2], txData[3]);

    spi_transaction_t trans = {};
    trans.length = 32;  // 32 bits
    trans.tx_buffer = txData.data();
    trans.rx_buffer = rxData.data();

    esp_err_t ret = spi_device_transmit(spi_device_, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transfer failed: %s", esp_err_to_name(ret));
        return std::unexpected(HALError::CommunicationError);
    }

    ESP_LOGD(TAG, "SPI Transfer - RX: 0x%02X%02X%02X%02X", 
             rxData[0], rxData[1], rxData[2], rxData[3]);

    return {};
}

void ESP32C6_HAL::delayMicroseconds(uint32_t us) noexcept {
    if (us == 0) {
        return;
    }

    if (us < 1000) {
        // For delays less than 1ms, use esp_timer for accuracy
        int64_t start_time = esp_timer_get_time();
        while ((esp_timer_get_time() - start_time) < us) {
            // Busy wait for short delays
        }
    } else {
        // For longer delays, use FreeRTOS delay to yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(us / 1000));
        
        // Handle remaining microseconds
        uint32_t remaining_us = us % 1000;
        if (remaining_us > 0) {
            int64_t start_time = esp_timer_get_time();
            while ((esp_timer_get_time() - start_time) < remaining_us) {
                // Busy wait for remainder
            }
        }
    }
}
