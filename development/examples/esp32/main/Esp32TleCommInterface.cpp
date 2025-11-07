/**
 * @file Esp32TleCommInterface.cpp
 * @brief ESP32 Communication Interface implementation for TLE92466ED driver
 * 
 * @author N3b3x
 * @date 2025-10-21
 * @version 2.0.0
 */

#include "Esp32TleCommInterface.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using namespace TLE92466ED;

Esp32TleCommInterface::Esp32TleCommInterface(const SPIConfig& config) : config_(config) {
    ESP_LOGI(TAG, "Esp32TleCommInterface created with SPI config:");
    ESP_LOGI(TAG, "  Host: %d", static_cast<int>(config_.host));
    ESP_LOGI(TAG, "  MISO: GPIO%d", config_.miso_pin);
    ESP_LOGI(TAG, "  MOSI: GPIO%d", config_.mosi_pin);
    ESP_LOGI(TAG, "  SCLK: GPIO%d", config_.sclk_pin);
    ESP_LOGI(TAG, "  CS: GPIO%d", config_.cs_pin);
    ESP_LOGI(TAG, "  Frequency: %d Hz", config_.frequency);
    ESP_LOGI(TAG, "  Mode: %d", config_.mode);
}

Esp32TleCommInterface::~Esp32TleCommInterface() {
    if (spi_device_ != nullptr) {
        spi_bus_remove_device(spi_device_);
        spi_device_ = nullptr;
    }
    
    if (initialized_) {
        spi_bus_free(config_.host);
        initialized_ = false;
    }
    
    ESP_LOGI(TAG, "Esp32TleCommInterface destroyed");
}

auto Esp32TleCommInterface::Init() noexcept -> CommResult<void> {
    if (initialized_) {
        ESP_LOGW(TAG, "CommInterface already initialized");
        return {};
    }

    ESP_LOGI(TAG, "Initializing Esp32TleCommInterface...");

    // Initialize GPIO pins (RESN, EN, FAULTN)
    if (auto result = initializeGPIO(); !result) {
        ESP_LOGE(TAG, "Failed to initialize GPIO pins");
        return std::unexpected(result.error());
    }

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
    ESP_LOGI(TAG, "Esp32TleCommInterface initialized successfully");
    return {};
}

auto Esp32TleCommInterface::Deinit() noexcept -> CommResult<void> {
    if (!initialized_) {
        ESP_LOGW(TAG, "CommInterface not initialized");
        return {};
    }

    ESP_LOGI(TAG, "Deinitializing Esp32TleCommInterface...");

    if (spi_device_ != nullptr) {
        spi_bus_remove_device(spi_device_);
        spi_device_ = nullptr;
    }

    spi_bus_free(config_.host);
    initialized_ = false;

    ESP_LOGI(TAG, "Esp32TleCommInterface deinitialized successfully");
    return {};
}

auto Esp32TleCommInterface::initializeGPIO() noexcept -> CommResult<void> {
    // Initialize RESN pin (output, active low - default HIGH for normal operation)
    if (config_.resn_pin >= 0) {
        gpio_config_t resn_config = {
            .pin_bit_mask = (1ULL << config_.resn_pin),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        esp_err_t ret = gpio_config(&resn_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure RESN pin (GPIO%d): %s", 
                     config_.resn_pin, esp_err_to_name(ret));
            return std::unexpected(CommError::HardwareNotReady);
        }
        // Set RESN HIGH (not in reset) by default
        gpio_set_level(static_cast<gpio_num_t>(config_.resn_pin), 1);
        ESP_LOGI(TAG, "RESN pin (GPIO%d) initialized and set HIGH", config_.resn_pin);
    }

    // Initialize EN pin (output, active high - default LOW to disable outputs)
    if (config_.en_pin >= 0) {
        gpio_config_t en_config = {
            .pin_bit_mask = (1ULL << config_.en_pin),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        esp_err_t ret = gpio_config(&en_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure EN pin (GPIO%d): %s", 
                     config_.en_pin, esp_err_to_name(ret));
            return std::unexpected(CommError::HardwareNotReady);
        }
        // Set EN LOW (disabled) by default
        gpio_set_level(static_cast<gpio_num_t>(config_.en_pin), 0);
        ESP_LOGI(TAG, "EN pin (GPIO%d) initialized and set LOW", config_.en_pin);
    }

    // Initialize FAULTN pin (input, active low)
    if (config_.faultn_pin >= 0) {
        gpio_config_t faultn_config = {
            .pin_bit_mask = (1ULL << config_.faultn_pin),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,  // Enable pull-up (active low signal)
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        esp_err_t ret = gpio_config(&faultn_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure FAULTN pin (GPIO%d): %s", 
                     config_.faultn_pin, esp_err_to_name(ret));
            return std::unexpected(CommError::HardwareNotReady);
        }
        ESP_LOGI(TAG, "FAULTN pin (GPIO%d) initialized as input", config_.faultn_pin);
    }

    return {};
}

auto Esp32TleCommInterface::initializeSPI() noexcept -> CommResult<void> {
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
        return std::unexpected(CommError::HardwareNotReady);
    }

    ESP_LOGI(TAG, "SPI bus initialized successfully");
    return {};
}

auto Esp32TleCommInterface::addSPIDevice() noexcept -> CommResult<void> {
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
        return std::unexpected(CommError::HardwareNotReady);
    }

    ESP_LOGI(TAG, "SPI device added successfully");
    return {};
}

auto Esp32TleCommInterface::Transfer32(uint32_t tx_data) noexcept -> CommResult<uint32_t> {
    if (!initialized_) {
        ESP_LOGE(TAG, "CommInterface not initialized");
        return std::unexpected(CommError::HardwareNotReady);
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
    if (is_write) {
        // For WRITE: Bits 23:17 contain address upper 7 bits, Bits 15:0 contain data
        ESP_LOGI(TAG, "  ├─ Bits 23:17: Address[9:3] = 0x%02X (%d)", address_upper, address_upper);
        ESP_LOGI(TAG, "  ├─ Bit  16:    R/W = %d (%s)", 
                    is_write ? 1 : 0, is_write ? "Write" : "Read");
        ESP_LOGI(TAG, "  └─ Bits 15:0:  Data = 0x%04X (%u)", data_16bit, data_16bit);
    } else {
        // For READ: Bits 23:17 are don't care, Bits 15:0 contain 7-bit address
        ESP_LOGI(TAG, "  ├─ Bits 23:17: Don't care = 0x%02X", address_upper);
        ESP_LOGI(TAG, "  ├─ Bit  16:    R/W = %d (%s)", 
                    is_write ? 1 : 0, is_write ? "Write" : "Read");
        uint8_t read_addr_7bit = data_16bit & 0x7F;  // 7-bit address in bits 6:0
        ESP_LOGI(TAG, "  └─ Bits 15:0:  Read Address (7-bit) = 0x%04X (value: 0x%02X)", 
                    data_16bit, read_addr_7bit);
    }

    esp_err_t ret = spi_device_transmit(spi_device_, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transfer failed: %s", esp_err_to_name(ret));
        return std::unexpected(CommError::TransferError);
    }

    // Print RX frame (MISO) in detailed format per TLE92466ED datasheet
    // Use the SPIFrame union structures for proper parsing
    SPIFrame rx_frame;
    rx_frame.word = rx_data;
    
    uint8_t* rx_bytes = reinterpret_cast<uint8_t*>(&rx_data);
    uint8_t reply_mode = rx_frame.rx_common.reply_mode;
    
    const char* reply_mode_str;
    switch (reply_mode) {
        case 0x00: reply_mode_str = "16-bit Reply"; break;
        case 0x01: reply_mode_str = "22-bit Reply"; break;
        case 0x02: reply_mode_str = "Critical Fault"; break;
        default: reply_mode_str = "Unknown"; break;
    }
    
    ESP_LOGI(TAG, "SPI RX (MISO) Frame: 0x%08X", rx_data);
    ESP_LOGI(TAG, "  Bytes [MSB->LSB]: [0x%02X] [0x%02X] [0x%02X] [0x%02X]", 
             rx_bytes[3], rx_bytes[2], rx_bytes[1], rx_bytes[0]);
    ESP_LOGI(TAG, "  ┌─ Bits 31:24: %s = 0x%02X", 
             (reply_mode == 0x02) ? "Don't care" : "CRC", rx_bytes[3]);
    ESP_LOGI(TAG, "  ├─ Bits 23:22: ReplyMode = %d (%s)", reply_mode, reply_mode_str);
    
    if (reply_mode == 0x00) {
        // 16-bit Reply Frame
        uint8_t status = rx_frame.rx_16bit.status;
        bool rw_echo = rx_frame.rx_16bit.rw_echo != 0;
        uint16_t data = rx_frame.rx_16bit.data;
        uint8_t crc = rx_frame.rx_16bit.crc;
        
        const char* status_str;
        switch (status) {
            case 0x00: status_str = "No error"; break;
            case 0x01: status_str = "SPI frame error"; break;
            case 0x02: status_str = "Parity/CRC error"; break;
            case 0x03: status_str = "Write to read-only register"; break;
            case 0x04: case 0x05: case 0x06: status_str = "Internal bus fault"; break;
            default: status_str = "Unknown"; break;
        }
        
        ESP_LOGI(TAG, "  ├─ Bits 21:17: Status = 0x%02X (%s)", status, status_str);
        ESP_LOGI(TAG, "  ├─ Bit  16:    R/W Echo = %d", rw_echo ? 1 : 0);
        ESP_LOGI(TAG, "  └─ Bits 15:0:  Data = 0x%04X (%u)", data, data);
        
    } else if (reply_mode == 0x01) {
        // 22-bit Reply Frame
        uint32_t data_22bit = rx_frame.rx_22bit.data;  // Already masked to 22 bits by bit-field
        uint8_t crc = rx_frame.rx_22bit.crc;
        
        ESP_LOGI(TAG, "  └─ Bits 21:0:  22-bit Data = 0x%06X (%u)", data_22bit, data_22bit);
        
    } else if (reply_mode == 0x02) {
        // Critical Fault Frame
        auto fault_flags = CriticalFaultFlags::Extract(rx_frame);
        
        ESP_LOGI(TAG, "  └─ Bits 7:0:   Fault Flags = 0x%02X", rx_frame.rx_fault.fault_flags);
        ESP_LOGI(TAG, "     ├─ Bit 7: 1V5 supply = %s", fault_flags.supply_1v5_ok ? "OK" : "NOT OK");
        ESP_LOGI(TAG, "     ├─ Bit 6: 2V5 supply = %s", fault_flags.supply_2v5_ok ? "OK" : "NOT OK");
        ESP_LOGI(TAG, "     ├─ Bit 5: BG (ADC Bandgap) = %s", fault_flags.adc_bandgap_ok ? "OK" : "NOT OK");
        ESP_LOGI(TAG, "     ├─ Bit 4: CLK_TOO_SLOW = %s", fault_flags.clk_too_slow ? "YES" : "NO");
        ESP_LOGI(TAG, "     ├─ Bit 3: CLK_TOO_FAST = %s", fault_flags.clk_too_fast ? "YES" : "NO");
        ESP_LOGI(TAG, "     ├─ Bit 2: DIG_CLK_TOO_SLOW = %s", fault_flags.dig_clk_too_slow ? "YES" : "NO");
        ESP_LOGI(TAG, "     ├─ Bit 1: DIG_CLK_TOO_FAST = %s", fault_flags.dig_clk_too_fast ? "YES" : "NO");
        ESP_LOGI(TAG, "     └─ Bit 0: WD_REF_CLK = %s", fault_flags.wd_ref_clk_missing ? "MISSING" : "OK");
    }
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════════════");

    return rx_data;
}

auto Esp32TleCommInterface::TransferMulti(std::span<const uint32_t> tx_data,
                                std::span<uint32_t> rx_data) noexcept -> CommResult<void> {
    if (!initialized_) {
        ESP_LOGE(TAG, "CommInterface not initialized");
        return std::unexpected(CommError::HardwareNotReady);
    }

    if (tx_data.size() != rx_data.size()) {
        ESP_LOGE(TAG, "Buffer size mismatch: tx=%zu, rx=%zu", tx_data.size(), rx_data.size());
        return std::unexpected(CommError::InvalidParameter);
    }

    for (size_t i = 0; i < tx_data.size(); ++i) {
        if (auto result = Transfer32(tx_data[i]); !result) {
            return std::unexpected(result.error());
        } else {
            rx_data[i] = *result;
        }
    }

    return {};
}

auto Esp32TleCommInterface::Delay(uint32_t microseconds) noexcept -> CommResult<void> {
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

auto Esp32TleCommInterface::Configure(const TLE92466ED::SPIConfig& config) noexcept -> CommResult<void> {
    // Note: ESP-IDF SPI configuration requires reinitialization
    // For now, we'll just update our internal config
    // In a real implementation, you might want to deinit and reinit
    ESP_LOGW(TAG, "SPI configuration update requested - not fully implemented");
    return {};
}

bool Esp32TleCommInterface::IsReady() const noexcept {
    return initialized_ && (spi_device_ != nullptr);
}

CommError Esp32TleCommInterface::GetLastError() const noexcept {
    return last_error_;
}

auto Esp32TleCommInterface::ClearErrors() noexcept -> CommResult<void> {
    last_error_ = CommError::None;
    return {};
}

auto Esp32TleCommInterface::SetGpioPin(ControlPin pin, ActiveLevel level) noexcept -> CommResult<void> {
    if (!IsReady()) {
        last_error_ = CommError::HardwareNotReady;
        return std::unexpected(CommError::HardwareNotReady);
    }

    int gpio_pin = -1;
    
    // Map ControlPin to GPIO pin number
    switch (pin) {
        case ControlPin::RESN:
            gpio_pin = config_.resn_pin;
            break;
        case ControlPin::EN:
            gpio_pin = config_.en_pin;
            break;
        case ControlPin::FAULTN:
            // FAULTN is read-only, cannot be set
            last_error_ = CommError::InvalidParameter;
            return std::unexpected(CommError::InvalidParameter);
    }

    // Check if pin is configured
    if (gpio_pin < 0) {
        ESP_LOGE(TAG, "GPIO pin not configured for %s", 
                 pin == ControlPin::RESN ? "RESN" : "EN");
        last_error_ = CommError::InvalidParameter;
        return std::unexpected(CommError::InvalidParameter);
    }

    // Convert ActiveLevel (logical active/inactive) to GPIO level
    // RESN is active low: ACTIVE = not in reset = GPIO HIGH, INACTIVE = in reset = GPIO LOW
    // EN is active high: ACTIVE = enabled = GPIO HIGH, INACTIVE = disabled = GPIO LOW
    int gpio_level;
    if (pin == ControlPin::RESN) {
        // RESN: ACTIVE = GPIO HIGH, INACTIVE = GPIO LOW
        gpio_level = (level == ActiveLevel::ACTIVE) ? 1 : 0;
    } else {
        // EN: ACTIVE = GPIO HIGH, INACTIVE = GPIO LOW
        gpio_level = (level == ActiveLevel::ACTIVE) ? 1 : 0;
    }

    // Set GPIO level
    esp_err_t ret = gpio_set_level(static_cast<gpio_num_t>(gpio_pin), gpio_level);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set GPIO%d level: %s", gpio_pin, esp_err_to_name(ret));
        last_error_ = CommError::BusError;
        return std::unexpected(CommError::BusError);
    }

    ESP_LOGD(TAG, "Set %s pin (GPIO%d) to %s (GPIO %s)", 
             pin == ControlPin::RESN ? "RESN" : "EN",
             gpio_pin,
             level == ActiveLevel::ACTIVE ? "ACTIVE" : "INACTIVE",
             gpio_level ? "HIGH" : "LOW");

    return {};
}

auto Esp32TleCommInterface::GetGpioPin(ControlPin pin) noexcept -> CommResult<ActiveLevel> {
    if (!IsReady()) {
        last_error_ = CommError::HardwareNotReady;
        return std::unexpected(CommError::HardwareNotReady);
    }

    // Only FAULTN can be read
    if (pin != ControlPin::FAULTN) {
        ESP_LOGE(TAG, "Cannot read %s pin (only FAULTN can be read)",
                 pin == ControlPin::RESN ? "RESN" : "EN");
        last_error_ = CommError::InvalidParameter;
        return std::unexpected(CommError::InvalidParameter);
    }

    int gpio_pin = config_.faultn_pin;
    
    // Check if pin is configured
    if (gpio_pin < 0) {
        ESP_LOGE(TAG, "FAULTN GPIO pin not configured");
        last_error_ = CommError::InvalidParameter;
        return std::unexpected(CommError::InvalidParameter);
    }

    // Read GPIO level
    int gpio_level = gpio_get_level(static_cast<gpio_num_t>(gpio_pin));
    
    // FAULTN is active low:
    // GPIO LOW (0) = fault detected = ActiveLevel::ACTIVE
    // GPIO HIGH (1) = no fault = ActiveLevel::INACTIVE
    ActiveLevel level = (gpio_level == 0) ? ActiveLevel::ACTIVE : ActiveLevel::INACTIVE;

    ESP_LOGD(TAG, "Read FAULTN pin (GPIO%d): %s (GPIO %s, %s)", 
             gpio_pin,
             level == ActiveLevel::ACTIVE ? "ACTIVE (FAULT)" : "INACTIVE (NO FAULT)",
             gpio_level ? "HIGH" : "LOW",
             gpio_level == 0 ? "fault detected" : "no fault");

    return level;
}

