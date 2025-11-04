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

using namespace TLE92466ED;

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

auto ESP32C6_HAL::init() noexcept -> HALResult<void> {
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

auto ESP32C6_HAL::deinit() noexcept -> HALResult<void> {
    if (!initialized_) {
        ESP_LOGW(TAG, "HAL not initialized");
        return {};
    }

    ESP_LOGI(TAG, "Deinitializing ESP32C6_HAL...");

    if (spi_device_ != nullptr) {
        spi_bus_remove_device(spi_device_);
        spi_device_ = nullptr;
    }

    spi_bus_free(config_.host);
    initialized_ = false;

    ESP_LOGI(TAG, "ESP32C6_HAL deinitialized successfully");
    return {};
}

auto ESP32C6_HAL::initializeSPI() noexcept -> HALResult<void> {
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
        .flags = SPICOMMON_BUSFLAG_MASTER
    };

    esp_err_t ret = spi_bus_initialize(config_.host, &bus_config, SPI_DMA_DISABLED);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus initialization failed: %s", esp_err_to_name(ret));
        return std::unexpected(HALError::HardwareNotReady);
    }

    ESP_LOGI(TAG, "SPI bus initialized successfully");
    return {};
}

auto ESP32C6_HAL::addSPIDevice() noexcept -> HALResult<void> {
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
        .flags = 0,  // Full-duplex mode (TLE92466ED requires simultaneous TX/RX)
        .queue_size = config_.queue_size,
        .pre_cb = nullptr,
        .post_cb = nullptr
    };

    esp_err_t ret = spi_bus_add_device(config_.host, &dev_config, &spi_device_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return std::unexpected(HALError::HardwareNotReady);
    }

    ESP_LOGI(TAG, "SPI device added successfully");
    return {};
}

auto ESP32C6_HAL::transfer32(uint32_t tx_data) noexcept -> HALResult<uint32_t> {
    if (!initialized_) {
        ESP_LOGE(TAG, "HAL not initialized");
        return std::unexpected(HALError::HardwareNotReady);
    }

    uint32_t rx_data = 0;
    spi_transaction_t trans = {};
    trans.length = 32;  // 32 bits
    trans.tx_buffer = &tx_data;
    trans.rx_buffer = &rx_data;

    // Print TX frame (MOSI) in detailed format per TLE92466ED datasheet
    // Frame format: [31:24]=CRC, [23:17]=Address(7), [16]=R/W, [15:0]=Data/Address
    // Note: ESP32 is little-endian, but SPI transmits MSB first (byte[3] first)
    uint8_t* tx_bytes = reinterpret_cast<uint8_t*>(&tx_data);
    uint8_t crc_tx = tx_bytes[3];              // Bits 31:24 - CRC
    uint8_t addr_byte = tx_bytes[2];           // Bits 23:16 - Address[23:17] + R/W[16]
    uint8_t data_high = tx_bytes[1];          // Bits 15:8
    uint8_t data_low = tx_bytes[0];            // Bits 7:0
    uint8_t address_upper = (addr_byte >> 1) & 0x7F;  // Bits 23:17 (7 bits)
    bool is_write = (addr_byte & 0x01) != 0;          // Bit 16
    uint16_t data_16bit = (static_cast<uint16_t>(data_high) << 8) | data_low;  // Bits 15:0
    
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════════════");
    ESP_LOGI(TAG, "SPI TX (MOSI) Frame: 0x%08X", tx_data);
    ESP_LOGI(TAG, "  Bytes [MSB->LSB]: [0x%02X] [0x%02X] [0x%02X] [0x%02X]", 
             tx_bytes[3], tx_bytes[2], tx_bytes[1], tx_bytes[0]);
    ESP_LOGI(TAG, "  ┌─ Bits 31:24: CRC = 0x%02X", crc_tx);
    ESP_LOGI(TAG, "  ├─ Bits 23:17: Address[6:0] = 0x%02X (%d)", address_upper, address_upper);
    ESP_LOGI(TAG, "  ├─ Bit  16:    R/W = %d (%s)", 
             is_write ? 1 : 0, is_write ? "Write" : "Read");
    if (is_write) {
        ESP_LOGI(TAG, "  └─ Bits 15:0:  Data = 0x%04X (%u)", data_16bit, data_16bit);
    } else {
        // For READ: lower 3 address bits are in data field
        uint8_t address_lower = data_low & 0x07;
        uint16_t full_address = (static_cast<uint16_t>(address_upper) << 3) | address_lower;
        ESP_LOGI(TAG, "  └─ Bits 15:0:  Address[2:0] = 0x%02X (Full Addr=0x%03X)", 
                 address_lower, full_address);
    }

    esp_err_t ret = spi_device_transmit(spi_device_, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transfer failed: %s", esp_err_to_name(ret));
        return std::unexpected(HALError::TransferError);
    }

    // Print RX frame (MISO) in detailed format per TLE92466ED datasheet
    // Note: On little-endian systems, bytes[3] is MSB (bits 31:24), bytes[0] is LSB (bits 7:0)
    uint8_t* rx_bytes = reinterpret_cast<uint8_t*>(&rx_data);
    uint8_t crc_rx = rx_bytes[3];                    // Bits 31:24
    uint8_t reply_status_byte = rx_bytes[2];          // Bits 23:16
    uint8_t rw_data_h = rx_bytes[1];                   // Bits 15:8
    uint8_t data_lsb_rx = rx_bytes[0];                // Bits 7:0
    
    uint8_t reply_mode = (reply_status_byte >> 6) & 0x03;  // Bits 23:22 (2 bits)
    bool rw_mirrored = (reply_status_byte & 0x01) != 0;    // Bit 16
    uint16_t data_16bit_rx = (static_cast<uint16_t>(rw_data_h) << 8) | data_lsb_rx;  // Bits 15:0
    
    const char* reply_mode_str;
    switch (reply_mode) {
        case 0x00: reply_mode_str = "16-bit Reply"; break;
        case 0x01: reply_mode_str = "22-bit Reply"; break;
        case 0x02: reply_mode_str = "Critical Fault"; break;
        default: reply_mode_str = "Unknown"; break;
    }
    
    // Status field only exists in 16-bit reply frames
    uint8_t status = 0;
    const char* status_str = "N/A";
    if (reply_mode == 0x00) {
        status = (reply_status_byte >> 1) & 0x1F;  // Bits 21:17 (5 bits) - only for 16-bit reply
        switch (status) {
            case 0x00: status_str = "No error"; break;
            case 0x01: status_str = "SPI frame error"; break;
            case 0x02: status_str = "Parity/CRC error"; break;
            case 0x03: status_str = "Write to read-only register"; break;
            case 0x04: case 0x05: case 0x06: status_str = "Internal bus fault"; break;
            default: status_str = "Unknown"; break;
        }
    }
    
    ESP_LOGI(TAG, "SPI RX (MISO) Frame: 0x%08X", rx_data);
    ESP_LOGI(TAG, "  Bytes [MSB->LSB]: [0x%02X] [0x%02X] [0x%02X] [0x%02X]", 
             rx_bytes[3], rx_bytes[2], rx_bytes[1], rx_bytes[0]);
    ESP_LOGI(TAG, "  ┌─ Bits 31:24: CRC = 0x%02X", crc_rx);
    ESP_LOGI(TAG, "  ├─ Bits 23:22: ReplyMode = %d (%s)", reply_mode, reply_mode_str);
    if (reply_mode == 0x00) {
        ESP_LOGI(TAG, "  ├─ Bits 21:17: Status = 0x%02X (%s)", status, status_str);
        ESP_LOGI(TAG, "  ├─ Bit  16:    R/W Echo = %d", rw_mirrored ? 1 : 0);
    } else {
        ESP_LOGI(TAG, "  ├─ Bit  16:    R/W Echo = %d", rw_mirrored ? 1 : 0);
    }
    
    if (reply_mode == 0x02) {
        // Critical Fault Frame - decode fault bits
        ESP_LOGI(TAG, "  └─ Bits 7:0:   Fault Flags = 0x%02X", data_lsb_rx);
        ESP_LOGI(TAG, "     ├─ Bit 7: 1V5 supply = %s", (data_lsb_rx & 0x80) ? "OK" : "NOT OK");
        ESP_LOGI(TAG, "     ├─ Bit 6: 2V5 supply = %s", (data_lsb_rx & 0x40) ? "OK" : "NOT OK");
        ESP_LOGI(TAG, "     ├─ Bit 5: BG (ADC Bandgap) = %s", (data_lsb_rx & 0x20) ? "OK" : "NOT OK");
        ESP_LOGI(TAG, "     ├─ Bit 4: CLK_TOO_SLOW = %s", (data_lsb_rx & 0x10) ? "YES" : "NO");
        ESP_LOGI(TAG, "     ├─ Bit 3: CLK_TOO_FAST = %s", (data_lsb_rx & 0x08) ? "YES" : "NO");
        ESP_LOGI(TAG, "     ├─ Bit 2: DIG_CLK_TOO_SLOW = %s", (data_lsb_rx & 0x04) ? "YES" : "NO");
        ESP_LOGI(TAG, "     ├─ Bit 1: DIG_CLK_TOO_FAST = %s", (data_lsb_rx & 0x02) ? "YES" : "NO");
        ESP_LOGI(TAG, "     └─ Bit 0: WD_REF_CLK = %s", (data_lsb_rx & 0x01) ? "MISSING" : "OK");
    } else if (reply_mode == 0x01) {
        // 22-bit Reply Frame: Bits 21:0 are data (ReplyMode is in bits 23:22)
        // Data layout per datasheet: [21:17] from bits 5:1 of byte[2], [16] from bit 0 of byte[2], [15:0] from bytes[1:0]
        // reply_status_byte[7:6] = ReplyMode(01), [5:1] = Data[21:17] (5 bits), [0] = Data[16] (1 bit)
        uint32_t data_21_17 = ((reply_status_byte & 0x3E) >> 1);  // Bits 5:1 = Data[21:17], 5 bits
        uint32_t data_16 = (reply_status_byte & 0x01);            // Bit 0 = Data[16], 1 bit
        uint32_t data_22bit = (data_21_17 << 17) | (data_16 << 16) | 
                              (static_cast<uint32_t>(rw_data_h) << 8) | data_lsb_rx;
        ESP_LOGI(TAG, "  └─ Bits 21:0:  22-bit Data = 0x%06X (%u)", data_22bit, data_22bit);
    } else {
        // 16-bit Reply Frame (default)
        ESP_LOGI(TAG, "  └─ Bits 15:0:  Data = 0x%04X (%u)", data_16bit_rx, data_16bit_rx);
    }
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════════════");

    return rx_data;
}

auto ESP32C6_HAL::transfer_multi(std::span<const uint32_t> tx_data,
                                 std::span<uint32_t> rx_data) noexcept -> HALResult<void> {
    if (!initialized_) {
        ESP_LOGE(TAG, "HAL not initialized");
        return std::unexpected(HALError::HardwareNotReady);
    }

    if (tx_data.size() != rx_data.size()) {
        ESP_LOGE(TAG, "Buffer size mismatch: tx=%zu, rx=%zu", tx_data.size(), rx_data.size());
        return std::unexpected(HALError::InvalidParameter);
    }

    for (size_t i = 0; i < tx_data.size(); ++i) {
        if (auto result = transfer32(tx_data[i]); !result) {
            return std::unexpected(result.error());
        } else {
            rx_data[i] = *result;
        }
    }

    return {};
}

auto ESP32C6_HAL::chip_select() noexcept -> HALResult<void> {
    // ESP-IDF SPI driver handles CS automatically, but we can implement manual control if needed
    // For now, CS is handled automatically by the SPI device
    return {};
}

auto ESP32C6_HAL::chip_deselect() noexcept -> HALResult<void> {
    // ESP-IDF SPI driver handles CS automatically, but we can implement manual control if needed
    // For now, CS is handled automatically by the SPI device
    return {};
}

auto ESP32C6_HAL::delay(uint32_t microseconds) noexcept -> HALResult<void> {
    if (microseconds == 0) {
        return {};
    }

    if (microseconds < 1000) {
        // For delays less than 1ms, use esp_timer for accuracy
        int64_t start_time = esp_timer_get_time();
        while ((esp_timer_get_time() - start_time) < microseconds) {
            // Busy wait for short delays
        }
    } else {
        // For longer delays, use FreeRTOS delay to yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(microseconds / 1000));
        
        // Handle remaining microseconds
        uint32_t remaining_us = microseconds % 1000;
        if (remaining_us > 0) {
            int64_t start_time = esp_timer_get_time();
            while ((esp_timer_get_time() - start_time) < remaining_us) {
                // Busy wait for remainder
            }
        }
    }

    return {};
}

auto ESP32C6_HAL::configure(const TLE92466ED::SPIConfig& config) noexcept -> HALResult<void> {
    // Note: ESP-IDF SPI configuration requires reinitialization
    // For now, we'll just update our internal config
    // In a real implementation, you might want to deinit and reinit
    ESP_LOGW(TAG, "SPI configuration update requested - not fully implemented");
    return {};
}

bool ESP32C6_HAL::is_ready() const noexcept {
    return initialized_ && (spi_device_ != nullptr);
}

HALError ESP32C6_HAL::get_last_error() const noexcept {
    return last_error_;
}

auto ESP32C6_HAL::clear_errors() noexcept -> HALResult<void> {
    last_error_ = HALError::None;
    return {};
}
