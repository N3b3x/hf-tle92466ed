/**
 * @file esp32_tle_comm_interface.cpp
 * @brief ESP32 Communication Interface implementation for TLE92466ED driver
 * 
 * @author N3b3x
 * @date 2025-10-21
 * @version 2.0.0
 */

#include "esp32_tle_comm_interface.hpp"
#include "tle92466ed_registers.hpp"  // For CRC calculation functions
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdarg>

using namespace tle92466ed;

namespace {
    /**
     * @brief Byte-swap a 32-bit value (MSB <-> LSB)
     * @param value Input value
     * @return Byte-swapped value
     */
    [[nodiscard]] constexpr inline uint32_t byte_swap_32(uint32_t value) noexcept {
        return ((value & 0xFF000000U) >> 24) |
               ((value & 0x00FF0000U) >> 8) |
               ((value & 0x0000FF00U) << 8) |
               ((value & 0x000000FFU) << 24);
    }
}

Esp32TleCommInterface::Esp32TleCommInterface(const SPIConfig& config) noexcept : config_(config) {
    ESP_LOGI(TAG, "Esp32TleCommInterface created with SPI config:");
    ESP_LOGI(TAG, "  Host: %d", static_cast<int>(config_.host));
    ESP_LOGI(TAG, "  MISO: GPIO%d", config_.miso_pin);
    ESP_LOGI(TAG, "  MOSI: GPIO%d", config_.mosi_pin);
    ESP_LOGI(TAG, "  SCLK: GPIO%d", config_.sclk_pin);
    ESP_LOGI(TAG, "  CS: GPIO%d", config_.cs_pin);
    ESP_LOGI(TAG, "  Frequency: %d Hz", config_.frequency);
    
    // Log mode with description
    const char* mode_desc;
    switch (config_.mode) {
        case 0: mode_desc = "CPOL=0, CPHA=0"; break;
        case 1: mode_desc = "CPOL=0, CPHA=1"; break;
        case 2: mode_desc = "CPOL=1, CPHA=0"; break;
        case 3: mode_desc = "CPOL=1, CPHA=1"; break;
        default: mode_desc = "Invalid"; break;
    }
    ESP_LOGI(TAG, "  Mode: %d (%s)", config_.mode, mode_desc);
    
    if (config_.resn_pin >= 0) {
        ESP_LOGI(TAG, "  RESN: GPIO%d", config_.resn_pin);
    }
    if (config_.en_pin >= 0) {
        ESP_LOGI(TAG, "  EN: GPIO%d", config_.en_pin);
    }
    if (config_.faultn_pin >= 0) {
        ESP_LOGI(TAG, "  FAULTN: GPIO%d", config_.faultn_pin);
    }
    if (config_.drv0_pin >= 0) {
        ESP_LOGI(TAG, "  DRV0: GPIO%d", config_.drv0_pin);
    }
    if (config_.drv1_pin >= 0) {
        ESP_LOGI(TAG, "  DRV1: GPIO%d", config_.drv1_pin);
    }
}

// Destructor moved inline to header to avoid incomplete type issues with std::unique_ptr

auto Esp32TleCommInterface::Init() noexcept -> CommResult<void> {
    if (initialized_) {
        ESP_LOGW(TAG, "CommInterface already initialized");
        return {};
    }

    ESP_LOGI(TAG, "Initializing Esp32TleCommInterface...");

    // Initialize GPIO pins (RESN, EN, FAULTN, DRV0, DRV1)
    // Note: GPIO initialization only - no device-specific sequences here
    // The Driver class will handle reset/enable sequences
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
        // Set RESN LOW (in reset) initially - will be released by reset sequence
        gpio_set_level(static_cast<gpio_num_t>(config_.resn_pin), 0);
        ESP_LOGI(TAG, "RESN pin (GPIO%d) initialized and set LOW (in reset)", config_.resn_pin);
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
        // Set EN LOW (disabled) initially - will be enabled during initialization sequence
        gpio_set_level(static_cast<gpio_num_t>(config_.en_pin), 0);
        ESP_LOGI(TAG, "EN pin (GPIO%d) initialized and set LOW (will be enabled during init)", config_.en_pin);
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

    // Initialize DRV0 pin (output, for external drive control)
    if (config_.drv0_pin >= 0) {
        gpio_config_t drv0_config = {
            .pin_bit_mask = (1ULL << config_.drv0_pin),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        esp_err_t ret = gpio_config(&drv0_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure DRV0 pin (GPIO%d): %s", 
                     config_.drv0_pin, esp_err_to_name(ret));
            return std::unexpected(CommError::HardwareNotReady);
        }
        // Set DRV0 LOW by default (inactive)
        gpio_set_level(static_cast<gpio_num_t>(config_.drv0_pin), 0);
        ESP_LOGI(TAG, "DRV0 pin (GPIO%d) initialized and set LOW", config_.drv0_pin);
    }

    // Initialize DRV1 pin (output, for external drive control)
    if (config_.drv1_pin >= 0) {
        gpio_config_t drv1_config = {
            .pin_bit_mask = (1ULL << config_.drv1_pin),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        esp_err_t ret = gpio_config(&drv1_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure DRV1 pin (GPIO%d): %s", 
                     config_.drv1_pin, esp_err_to_name(ret));
            return std::unexpected(CommError::HardwareNotReady);
        }
        // Set DRV1 LOW by default (inactive)
        gpio_set_level(static_cast<gpio_num_t>(config_.drv1_pin), 0);
        ESP_LOGI(TAG, "DRV1 pin (GPIO%d) initialized and set LOW", config_.drv1_pin);
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
    // Verify mode is correct: Mode 1 = CPOL=0, CPHA=1
    // ESP-IDF SPI mode encoding:
    // Mode 0: CPOL=0, CPHA=0 (clock idle LOW, sample on RISING edge)
    // Mode 1: CPOL=0, CPHA=1 (clock idle LOW, sample on FALLING edge) <- TLE92466ED requires this
    // Mode 2: CPOL=1, CPHA=0 (clock idle HIGH, sample on FALLING edge)
    // Mode 3: CPOL=1, CPHA=1 (clock idle HIGH, sample on RISING edge)
    
    uint8_t spi_mode = static_cast<uint8_t>(config_.mode);
    
    // Ensure mode is in valid range (0-3)
    if (spi_mode > 3) {
        ESP_LOGE(TAG, "Invalid SPI mode %d, must be 0-3. Using Mode 1.", spi_mode);
        spi_mode = 1;
    }
    
    // Log SPI mode details for verification
    const char* mode_desc;
    switch (spi_mode) {
        case 0: mode_desc = "CPOL=0, CPHA=0"; break;
        case 1: mode_desc = "CPOL=0, CPHA=1"; break;
        case 2: mode_desc = "CPOL=1, CPHA=0"; break;
        case 3: mode_desc = "CPOL=1, CPHA=1"; break;
        default: mode_desc = "Invalid"; break;
    }
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════════════");
    ESP_LOGI(TAG, "Configuring SPI device:");
    ESP_LOGI(TAG, "  Mode: %d (%s)", spi_mode, mode_desc);
    ESP_LOGI(TAG, "  Expected for TLE92466ED: CPOL=0 (clock idle LOW)");
    ESP_LOGI(TAG, "                        CPHA=1 (data sampled on FALLING edge)");
    ESP_LOGI(TAG, "  Config value: %d", config_.mode);
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════════════");
    
    spi_device_interface_config_t dev_config = {};
    dev_config.command_bits = 0;
    dev_config.address_bits = 0;
    dev_config.dummy_bits = 0;
    dev_config.mode = spi_mode;
    dev_config.clock_source = SPI_CLK_SRC_DEFAULT;
    dev_config.duty_cycle_pos = 128;  // 50% duty cycle
    
    // CRITICAL: CS timing requirements per TLE92466ED datasheet
    // tCSS (CS setup): CS must be asserted (LOW) at least 50ns BEFORE first SCK edge
    // tCSH (CS hold): CS must remain asserted at least 50ns AFTER last SCK edge
    // tCSI (CS inactive): CS must be deasserted (HIGH) at least 100ns between transactions
    //
    // Values are configured in TLE92466ED_TestConfig.hpp SPIParams
    // At 1MHz SPI: clock period = 1000ns
    // cs_ena_pretrans = 2 means CS goes LOW 2 clock cycles (2000ns) before first SCK edge
    // This provides 2000ns setup time, well above the 50ns minimum requirement
    // This accounts for GPIO delay, wire delay, and ensures reliable communication
    dev_config.cs_ena_pretrans = config_.cs_ena_pretrans;   // From config (default: 2 clock cycles)
    dev_config.cs_ena_posttrans = config_.cs_ena_posttrans; // From config (default: 2 clock cycles)
    dev_config.clock_speed_hz = config_.frequency;
    dev_config.input_delay_ns = 0;
    dev_config.spics_io_num = config_.cs_pin;
    
    // Calculate actual CS timing in nanoseconds for verification
    uint32_t clock_period_ns = 1'000'000'000 / config_.frequency;  // Period in nanoseconds
    uint32_t cs_setup_ns = dev_config.cs_ena_pretrans * clock_period_ns;
    uint32_t cs_hold_ns = dev_config.cs_ena_posttrans * clock_period_ns;
    
    ESP_LOGI(TAG, "CS Timing Configuration (from TLE92466ED_TestConfig.hpp):");
    ESP_LOGI(TAG, "  CS Setup (cs_ena_pretrans): %d clock cycles = %lu ns", 
             dev_config.cs_ena_pretrans, cs_setup_ns);
    ESP_LOGI(TAG, "    Required: >= 50ns | Actual: %lu ns | %s", 
             cs_setup_ns, (cs_setup_ns >= 50) ? "✅ OK" : "❌ TOO SHORT");
    ESP_LOGI(TAG, "  CS Hold (cs_ena_posttrans): %d clock cycles = %lu ns", 
             dev_config.cs_ena_posttrans, cs_hold_ns);
    ESP_LOGI(TAG, "    Required: >= 50ns | Actual: %lu ns | %s", 
             cs_hold_ns, (cs_hold_ns >= 50) ? "✅ OK" : "❌ TOO SHORT");
    dev_config.flags = 0;  // Full-duplex mode (TLE92466ED requires simultaneous TX/RX)
    dev_config.queue_size = config_.queue_size;
    dev_config.pre_cb = nullptr;
    dev_config.post_cb = nullptr;

    esp_err_t ret = spi_bus_add_device(config_.host, &dev_config, &spi_device_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return std::unexpected(CommError::HardwareNotReady);
    }
    
    ESP_LOGI(TAG, "SPI device added successfully with Mode %d (%s)", spi_mode, mode_desc);
    return {};
}

auto Esp32TleCommInterface::Transfer32(uint32_t tx_data) noexcept -> CommResult<uint32_t> {
    if (!initialized_) {
        ESP_LOGE(TAG, "CommInterface not initialized");
        return std::unexpected(CommError::HardwareNotReady);
    }

    // CRITICAL: ESP32-C6 is a little-endian chip, which means the least significant byte (LSB)
    // of uint32_t variables is stored at the smallest address. Hence, bits [7:0] are sent first,
    // followed by bits [15:8], [23:16], and [31:24].
    //
    // We construct frames with MSB at bit 31 (e.g., 0x91000040 = CRC=0x91 at bits [31:24], data=0x40 at bits [15:0])
    // In little-endian memory: byte[0]=0x40, byte[1]=0x00, byte[2]=0x00, byte[3]=0x91
    // ESP-IDF SPI driver with length=32 transmits LSB-first: 0x40, 0x00, 0x00, 0x91 (WRONG - we need MSB first!)
    // We need: 0x91, 0x00, 0x00, 0x40 (MSB first - CORRECT!)
    // Solution: Byte-swap the data so MSB is transmitted first
    uint32_t tx_data_swapped = byte_swap_32(tx_data);
    
    uint32_t rx_data_raw = 0;
    spi_transaction_t trans = {};
    trans.length = 32;  // 32 bits = 4 bytes
    trans.tx_buffer = &tx_data_swapped;  // Use byte-swapped data for transmission
    trans.rx_buffer = &rx_data_raw;

#if ESP32_TLE_COMM_ENABLE_DETAILED_SPI_LOGGING
    // Print TX frame (MOSI) in detailed format per TLE92466ED datasheet
    // Frame format: [31:24]=CRC, [23:17]=Address(7), [16]=R/W, [15:0]=Data/Address
    uint8_t* tx_bytes_orig = reinterpret_cast<uint8_t*>(&tx_data);  // Original for parsing
    uint8_t* tx_bytes_swapped = reinterpret_cast<uint8_t*>(&tx_data_swapped);  // Swapped for transmission
    uint8_t crc_tx = tx_bytes_orig[3];              // Bits 31:24 - CRC
    uint8_t addr_byte = tx_bytes_orig[2];           // Bits 23:16 - Address[23:17] + R/W[16]
    uint8_t data_high = tx_bytes_orig[1];          // Bits 15:8
    uint8_t data_low = tx_bytes_orig[0];            // Bits 7:0
    uint8_t address_upper = (addr_byte >> 1) & 0x7F;  // Bits 23:17 (7 bits)
    bool is_write = (addr_byte & 0x01) != 0;          // Bit 16
    uint16_t data_16bit = (static_cast<uint16_t>(data_high) << 8) | data_low;  // Bits 15:0
    
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════════════");
    ESP_LOGI(TAG, "SPI TX (MOSI) Frame: 0x%08X (original)", tx_data);
    ESP_LOGI(TAG, "  Bytes [as stored in memory - little-endian]: [0x%02X] [0x%02X] [0x%02X] [0x%02X]", 
             tx_bytes_orig[0], tx_bytes_orig[1], tx_bytes_orig[2], tx_bytes_orig[3]);  // byte[0] is LSB, byte[3] is MSB
    ESP_LOGI(TAG, "  Frame (byte-swapped for TX): 0x%08X", tx_data_swapped);
    ESP_LOGI(TAG, "  Bytes [MSB->LSB on wire]: [0x%02X] [0x%02X] [0x%02X] [0x%02X]", 
             tx_bytes_swapped[3], tx_bytes_swapped[2], tx_bytes_swapped[1], tx_bytes_swapped[0]);
    // Calculate and verify TX CRC
    SPIFrame tx_frame_for_crc;
    tx_frame_for_crc.word = tx_data;
    uint8_t tx_crc_received = tx_frame_for_crc.tx_fields.crc;
    tx_frame_for_crc.tx_fields.crc = 0;  // Clear CRC for calculation
    uint8_t tx_crc_calculated = CalculateFrameCrc(tx_frame_for_crc);
    bool tx_crc_match = (tx_crc_received == tx_crc_calculated);
    
    ESP_LOGI(TAG, "  ┌─ Bits 31:24: CRC = 0x%02X", crc_tx);
    ESP_LOGI(TAG, "     Calculated CRC: 0x%02X | Received CRC: 0x%02X | %s", 
             tx_crc_calculated, tx_crc_received, tx_crc_match ? "✅ MATCH" : "❌ MISMATCH");
    if (is_write) {
        // For WRITE: Bits 23:17 contain address upper 7 bits, Bits 15:0 contain data
        ESP_LOGI(TAG, "  ├─ Bits 23:17: Address[9:3] = 0x%02X (%d)", address_upper, address_upper);
        ESP_LOGI(TAG, "  ├─ Bit  16:    R/W = %d (%s)", 
                    is_write ? 1 : 0, is_write ? "Write" : "Read");
        ESP_LOGI(TAG, "  └─ Bits 15:0:  Data = 0x%04X (%u)", data_16bit, data_16bit);
    } else {
        // For READ: Bits 23:17 are don't care, Bits 15:0 contain 16-bit address
        ESP_LOGI(TAG, "  ├─ Bits 23:17: Don't care = 0x%02X", address_upper);
        ESP_LOGI(TAG, "  ├─ Bit  16:    R/W = %d (%s)", 
                    is_write ? 1 : 0, is_write ? "Write" : "Read");
        ESP_LOGI(TAG, "  └─ Bits 15:0:  Read Address (16-bit) = 0x%04X", data_16bit);
    }
#endif // ESP32_TLE_COMM_ENABLE_DETAILED_SPI_LOGGING

    esp_err_t ret = spi_device_transmit(spi_device_, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transfer failed: %s", esp_err_to_name(ret));
        return std::unexpected(CommError::TransferError);
    }

    // CRITICAL: ESP32-C6 is a little-endian chip. The SPI driver receives MSB-first on the wire,
    // but stores it in little-endian format. We need to byte-swap it back for proper bitfield extraction.
    uint32_t rx_data = byte_swap_32(rx_data_raw);
    
#if ESP32_TLE_COMM_ENABLE_DETAILED_SPI_LOGGING
    // Print RX frame (MISO) in detailed format per TLE92466ED datasheet
    SPIFrame rx_frame;
    rx_frame.word = rx_data;  // Use byte-swapped data for bitfield extraction
    uint8_t reply_mode = rx_frame.rx_common.reply_mode;
    
    const char* reply_mode_str;
    switch (reply_mode) {
        case 0x00: reply_mode_str = "16-bit Reply"; break;
        case 0x01: reply_mode_str = "22-bit Reply"; break;
        case 0x02: reply_mode_str = "Critical Fault"; break;
        default: reply_mode_str = "Unknown"; break;
    }
    
    uint8_t* rx_bytes_raw = reinterpret_cast<uint8_t*>(&rx_data_raw);
    uint8_t* rx_bytes_swapped = reinterpret_cast<uint8_t*>(&rx_data);
    
    ESP_LOGI(TAG, "SPI RX (MISO) Frame: 0x%08X (raw from SPI)", rx_data_raw);
    ESP_LOGI(TAG, "  Bytes [as stored in memory - little-endian]: [0x%02X] [0x%02X] [0x%02X] [0x%02X]", 
             rx_bytes_raw[0], rx_bytes_raw[1], rx_bytes_raw[2], rx_bytes_raw[3]);
    ESP_LOGI(TAG, "  Frame (byte-swapped for parsing): 0x%08X", rx_data);
    ESP_LOGI(TAG, "  Bytes [MSB->LSB on wire]: [0x%02X] [0x%02X] [0x%02X] [0x%02X]", 
             rx_bytes_swapped[3], rx_bytes_swapped[2], rx_bytes_swapped[1], rx_bytes_swapped[0]);
    
    // Calculate and verify RX CRC (only for non-fault frames)
    bool rx_crc_valid = false;
    uint8_t rx_crc_received = 0;
    uint8_t rx_crc_calculated = 0;
    if (reply_mode != 0x02) {
        // For 16-bit and 22-bit reply frames, verify CRC
        rx_crc_received = rx_frame.rx_common.crc;
        SPIFrame rx_frame_for_crc;
        rx_frame_for_crc.word = rx_data;
        rx_frame_for_crc.rx_common.crc = 0;  // Clear CRC for calculation
        rx_crc_calculated = CalculateFrameCrc(rx_frame_for_crc);
        rx_crc_valid = VerifyFrameCrc(rx_frame);
    } else {
        // For Critical Fault frames, bits [31:24] are "Don't Care" (not CRC)
        // But we can still calculate what CRC would be on the data bytes for debugging
        SPIFrame rx_frame_for_crc;
        rx_frame_for_crc.word = rx_data;
        rx_frame_for_crc.rx_common.crc = 0;  // Clear "Don't Care" field for calculation
        rx_crc_calculated = CalculateFrameCrc(rx_frame_for_crc);
        rx_crc_received = rx_bytes_swapped[3];  // The "Don't Care" value
        // Note: This is just for informational purposes - Critical Fault frames don't have CRC
    }
    
    ESP_LOGI(TAG, "  ┌─ Bits 31:24: %s = 0x%02X", 
             (reply_mode == 0x02) ? "Don't care" : "CRC", rx_bytes_swapped[3]);
    if (reply_mode != 0x02) {
        ESP_LOGI(TAG, "     Calculated CRC: 0x%02X | Received CRC: 0x%02X | %s", 
                 rx_crc_calculated, rx_crc_received, rx_crc_valid ? "✅ MATCH" : "❌ MISMATCH");
    } else {
        ESP_LOGI(TAG, "     (Note: Critical Fault frames don't have CRC - calculated on data: 0x%02X, Don't Care field: 0x%02X)", 
                 rx_crc_calculated, rx_crc_received);
    }
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
#endif // ESP32_TLE_COMM_ENABLE_DETAILED_SPI_LOGGING

    // Return the reconstructed data (MSB at bit 31) so the driver layer can parse it correctly
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

auto Esp32TleCommInterface::Configure(const tle92466ed::SPIConfig& config) noexcept -> CommResult<void> {
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

void Esp32TleCommInterface::Log(LogLevel level, const char* tag, const char* format, va_list args) noexcept {
    // Map LogLevel to ESP-IDF log level
    esp_log_level_t esp_level;
    switch (level) {
        case LogLevel::Error:
            esp_level = ESP_LOG_ERROR;
            break;
        case LogLevel::Warn:
            esp_level = ESP_LOG_WARN;
            break;
        case LogLevel::Info:
            esp_level = ESP_LOG_INFO;
            break;
        case LogLevel::Debug:
            esp_level = ESP_LOG_DEBUG;
            break;
        case LogLevel::Verbose:
            esp_level = ESP_LOG_VERBOSE;
            break;
        default:
            esp_level = ESP_LOG_INFO;
            break;
    }
    
    // Use esp_log_writev which accepts va_list
    esp_log_writev(esp_level, tag, format, args);
}

auto CreateEsp32TleCommInterface() noexcept -> std::unique_ptr<Esp32TleCommInterface> {
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

