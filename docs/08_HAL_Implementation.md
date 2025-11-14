---
layout: default
title: "🔌 HAL Implementation"
description: "Hardware abstraction layer implementation guide for platform porting"
nav_order: 8
parent: "📚 Documentation"
permalink: /docs/08_HAL_Implementation/
---

# HAL Implementation Guide

## Overview

The Hardware Abstraction Layer (HAL) provides platform-independent SPI communication
for the TLE92466ED driver. You must implement this interface for your specific hardware
platform.

### HAL Architecture

```text
    Application Code
         │
         ▼
    tle92466ed::Driver ◄──── High-level API
         │
         ▼
    tle92466ed::SpiInterface ◄──────── Abstract interface (YOU IMPLEMENT)
         │
         ▼
    Platform SPI ◄────────────  Your hardware (STM32, ESP32, etc.)
```text

## HAL Interface

### Base Class

```cpp
class HAL {
public:
    virtual ~HAL() = default;
    
    // Core methods (must implement)
    [[nodiscard]] virtual HALResult<void> init() noexcept = 0;
    [[nodiscard]] virtual HALResult<void> deinit() noexcept = 0;
    [[nodiscard]] virtual HALResult<uint32_t> transfer32(uint32_t tx_data) noexcept = 0;
    [[nodiscard]] virtual HALResult<void> transfer_multi(
        std::span<const uint32_t> tx_data,
        std::span<uint32_t> rx_data) noexcept = 0;
    [[nodiscard]] virtual HALResult<void> chip_select() noexcept = 0;
    [[nodiscard]] virtual HALResult<void> chip_deselect() noexcept = 0;
    [[nodiscard]] virtual HALResult<void> delay(uint32_t microseconds) noexcept = 0;
    [[nodiscard]] virtual HALResult<void> configure(const SPIConfig& config) noexcept = 0;
    [[nodiscard]] virtual bool is_ready() const noexcept = 0;
    [[nodiscard]] virtual HALError get_last_error() const noexcept = 0;
    [[nodiscard]] virtual HALResult<void> clear_errors() noexcept = 0;
};
```text

### HALError Enumeration

```cpp
enum class HALError : uint8_t {
    None = 0,
    BusError,
    Timeout,
    InvalidParameter,
    ChipselectError,
    TransferError,
    HardwareNotReady,
    BufferOverflow,
    CRCError,
    UnknownError
};
```text

### HALResult Type

```cpp
template<typename T>
using HALResult = std::expected<T, HALError>;
```text

## Platform Implementation Examples

### STM32 HAL Implementation

```cpp
class STM32_HAL : public tle92466ed::SpiInterface<STM32_HAL> {
public:
    STM32_HAL(SPI_HandleTypeDef* hspi, GPIO_TypeDef* cs_port, uint16_t cs_pin)
        : hspi_(hspi), cs_port_(cs_port), cs_pin_(cs_pin), initialized_(false) {}
    
    HALResult<void> init() noexcept override {
        // SPI already initialized by CubeMX
        // Configure CS pin as output
        HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_SET);
        initialized_ = true;
        return {};
    }
    
    HALResult<uint32_t> transfer32(uint32_t tx_data) noexcept override {
        if (!initialized_) {
            return std::unexpected(HALError::HardwareNotReady);
        }
        
        // Convert to bytes (MSB first)
        uint8_t tx_bytes[4] = {
            static_cast<uint8_t>((tx_data >> 24) & 0xFF),
            static_cast<uint8_t>((tx_data >> 16) & 0xFF),
            static_cast<uint8_t>((tx_data >> 8) & 0xFF),
            static_cast<uint8_t>(tx_data & 0xFF)
        };
        uint8_t rx_bytes[4] = {0};
        
        // Chip select
        HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_RESET);
        
        // Transfer
        HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(
            hspi_, tx_bytes, rx_bytes, 4, 100);
        
        // Chip deselect
        HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_SET);
        
        if (status != HAL_OK) {
            return std::unexpected(HALError::TransferError);
        }
        
        // Convert back to uint32_t
        uint32_t rx_data = (static_cast<uint32_t>(rx_bytes[0]) << 24) |
                          (static_cast<uint32_t>(rx_bytes[1]) << 16) |
                          (static_cast<uint32_t>(rx_bytes[2]) << 8) |
                          static_cast<uint32_t>(rx_bytes[3]);
        
        return rx_data;
    }
    
    HALResult<void> chip_select() noexcept override {
        HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_RESET);
        return {};
    }
    
    HALResult<void> chip_deselect() noexcept override {
        HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_SET);
        return {};
    }
    
    HALResult<void> delay(uint32_t microseconds) noexcept override {
        // Use DWT cycle counter for precise microsecond delays
        uint32_t start = DWT->CYCCNT;
        uint32_t cycles = (SystemCoreClock / 1000000) * microseconds;
        while ((DWT->CYCCNT - start) < cycles);
        return {};
    }
    
    bool is_ready() const noexcept override {
        return initialized_;
    }
    
    // ... implement other methods
    
private:
    SPI_HandleTypeDef* hspi_;
    GPIO_TypeDef* cs_port_;
    uint16_t cs_pin_;
    bool initialized_;
};
```text

### ESP32 Implementation

```cpp
class ESP32_HAL : public tle92466ed::SpiInterface<ESP32_HAL> {
public:
    ESP32_HAL(spi_host_device_t spi_host, int cs_pin)
        : spi_host_(spi_host), cs_pin_(cs_pin), spi_device_(nullptr) {}
    
    HALResult<void> init() noexcept override {
        // Configure CS pin
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << cs_pin_),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
        gpio_set_level(static_cast<gpio_num_t>(cs_pin_), 1);
        
        // Configure SPI device
        spi_device_interface_config_t dev_cfg = {
            .mode = 0,  // SPI mode 0
            .clock_speed_hz = 1000000,  // 1 MHz
            .spics_io_num = -1,  // Manual CS
            .queue_size = 1
        };
        
        esp_err_t ret = spi_bus_add_device(spi_host_, &dev_cfg, &spi_device_);
        if (ret != ESP_OK) {
            return std::unexpected(HALError::HardwareNotReady);
        }
        
        return {};
    }
    
    HALResult<uint32_t> transfer32(uint32_t tx_data) noexcept override {
        spi_transaction_t trans = {
            .length = 32,  // bits
            .tx_data = {
                static_cast<uint8_t>((tx_data >> 24) & 0xFF),
                static_cast<uint8_t>((tx_data >> 16) & 0xFF),
                static_cast<uint8_t>((tx_data >> 8) & 0xFF),
                static_cast<uint8_t>(tx_data & 0xFF)
            }
        };
        
        gpio_set_level(static_cast<gpio_num_t>(cs_pin_), 0);
        esp_err_t ret = spi_device_transmit(spi_device_, &trans);
        gpio_set_level(static_cast<gpio_num_t>(cs_pin_), 1);
        
        if (ret != ESP_OK) {
            return std::unexpected(HALError::TransferError);
        }
        
        uint32_t rx_data = (static_cast<uint32_t>(trans.rx_data[0]) << 24) |
                          (static_cast<uint32_t>(trans.rx_data[1]) << 16) |
                          (static_cast<uint32_t>(trans.rx_data[2]) << 8) |
                          static_cast<uint32_t>(trans.rx_data[3]);
        
        return rx_data;
    }
    
    HALResult<void> delay(uint32_t microseconds) noexcept override {
        esp_rom_delay_us(microseconds);
        return {};
    }
    
    // ... implement other methods
    
private:
    spi_host_device_t spi_host_;
    int cs_pin_;
    spi_device_handle_t spi_device_;
};
```text

### Linux SPI Implementation

```cpp
class LinuxSPI_HAL : public tle92466ed::SpiInterface<LinuxSPI_HAL> {
public:
    LinuxSPI_HAL(const char* device, int cs_gpio)
        : device_path_(device), cs_gpio_(cs_gpio), spi_fd_(-1) {}
    
    HALResult<void> init() noexcept override {
        // Open SPI device
        spi_fd_ = open(device_path_, O_RDWR);
        if (spi_fd_ < 0) {
            return std::unexpected(HALError::HardwareNotReady);
        }
        
        // Configure SPI mode
        uint8_t mode = SPI_MODE_0;
        if (ioctl(spi_fd_, SPI_IOC_WR_MODE, &mode) < 0) {
            close(spi_fd_);
            return std::unexpected(HALError::HardwareNotReady);
        }
        
        // Configure speed
        uint32_t speed = 1000000;  // 1 MHz
        if (ioctl(spi_fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
            close(spi_fd_);
            return std::unexpected(HALError::HardwareNotReady);
        }
        
        // Export and configure CS GPIO
        export_gpio(cs_gpio_);
        set_gpio_direction(cs_gpio_, "out");
        set_gpio_value(cs_gpio_, 1);
        
        return {};
    }
    
    HALResult<uint32_t> transfer32(uint32_t tx_data) noexcept override {
        uint8_t tx_bytes[4] = {
            static_cast<uint8_t>((tx_data >> 24) & 0xFF),
            static_cast<uint8_t>((tx_data >> 16) & 0xFF),
            static_cast<uint8_t>((tx_data >> 8) & 0xFF),
            static_cast<uint8_t>(tx_data & 0xFF)
        };
        uint8_t rx_bytes[4] = {0};
        
        struct spi_ioc_transfer transfer = {
            .tx_buf = reinterpret_cast<uint64_t>(tx_bytes),
            .rx_buf = reinterpret_cast<uint64_t>(rx_bytes),
            .len = 4,
            .speed_hz = 1000000,
            .bits_per_word = 8
        };
        
        set_gpio_value(cs_gpio_, 0);
        int ret = ioctl(spi_fd_, SPI_IOC_MESSAGE(1), &transfer);
        set_gpio_value(cs_gpio_, 1);
        
        if (ret < 0) {
            return std::unexpected(HALError::TransferError);
        }
        
        uint32_t rx_data = (static_cast<uint32_t>(rx_bytes[0]) << 24) |
                          (static_cast<uint32_t>(rx_bytes[1]) << 16) |
                          (static_cast<uint32_t>(rx_bytes[2]) << 8) |
                          static_cast<uint32_t>(rx_bytes[3]);
        
        return rx_data;
    }
    
    // ... implement other methods
    
private:
    const char* device_path_;
    int cs_gpio_;
    int spi_fd_;
    
    void export_gpio(int gpio);
    void set_gpio_direction(int gpio, const char* dir);
    void set_gpio_value(int gpio, int value);
};
```text

## Implementation Checklist

### Required Methods

- [x] `init()` - Initialize SPI peripheral and GPIO
- [x] `deinit()` - Clean up resources
- [x] `transfer32()` - **Critical**: 32-bit full-duplex transfer
- [x] `transfer_multi()` - Multiple transfers (can call transfer32 in loop)
- [x] `chip_select()` - Assert CS (low)
- [x] `chip_deselect()` - Deassert CS (high)
- [x] `delay()` - Microsecond delay
- [x] `configure()` - Update SPI settings
- [x] `is_ready()` - Check initialization status
- [x] `get_last_error()` - Return last error
- [x] `clear_errors()` - Reset error state

### SPI Requirements

```text
Parameter           Requirement
─────────────────── ─────────────────────────
Mode                0 (CPOL=0, CPHA=0)
Frequency           100 kHz - 10 MHz
Bit Order           MSB first
Frame Size          32 bits (4 bytes)
CS Polarity         Active low
Full-Duplex         Yes
```text

### Timing Requirements

```text
Parameter           Min     Max     Unit
───────────────── ─────── ─────── ─────
CS Setup Time       50      -       ns
CS Hold Time        50      -       ns  
CS Inactive Time    100     -       ns
Data Setup Time     20      -       ns
Data Hold Time      20      -       ns
```text

## Testing Your HAL

### Basic Test

```cpp
// Create your HAL
MyPlatformHAL hal;

// Test initialization
auto init_result = hal.init();
assert(init_result.has_value());
assert(hal.is_ready());

// Test chip select
auto cs_result = hal.chip_select();
assert(cs_result.has_value());

auto ds_result = hal.chip_deselect();
assert(ds_result.has_value());

// Test transfer (loopback if available)
uint32_t test_data = 0x12345678;
auto transfer_result = hal.transfer32(test_data);
assert(transfer_result.has_value());

// Test delay
auto delay_result = hal.delay(100);
assert(delay_result.has_value());
```text

### Integration Test

```cpp
// Create driver with your HAL
MyPlatformHAL hal;
tle92466ed::Driver driver(hal);

// Test full initialization
auto result = driver.init();
if (!result) {
    log("Init failed: %d", static_cast<int>(result.error()));
    return;
}

// Read device ID
auto id_result = driver.get_ic_version();
if (id_result) {
    log("Device ID: 0x%04X", *id_result);
}

// Test register access
auto reg_result = driver.read_register(0x0003);
if (reg_result) {
    log("GLOBAL_DIAG0: 0x%04X", *reg_result);
}
```text

## Common Pitfalls

### 1. Byte Order

**Problem**: Data appears scrambled

**Solution**: Ensure MSB-first transmission
```cpp
// Correct: MSB first
tx_bytes[0] = (data >> 24) & 0xFF;  // MSB
tx_bytes[1] = (data >> 16) & 0xFF;
tx_bytes[2] = (data >> 8) & 0xFF;
tx_bytes[3] = data & 0xFF;          // LSB
```text

### 2. CS Timing

**Problem**: Communication errors

**Solution**: Ensure proper CS timing
```cpp
// Correct sequence
gpio_set(CS, LOW);
delay_ns(50);  // CS setup time
spi_transfer(...);
delay_ns(50);  // CS hold time
gpio_set(CS, HIGH);
delay_ns(100); // CS inactive time
```text

### 3. SPI Mode

**Problem**: No response from device

**Solution**: Verify SPI Mode 0 (CPOL=0, CPHA=0)
```cpp
// Ensure:
// - Clock idle state: LOW
// - Data sampled on: RISING edge
// - Data shifted on: FALLING edge
```text

### 4. Exception Safety

**Problem**: Exceptions thrown in noexcept functions

**Solution**: Catch all exceptions and return errors
```cpp
HALResult<uint32_t> transfer32(uint32_t data) noexcept override {
    try {
        // ... SPI operations
        return result;
    } catch (...) {
        return std::unexpected(HALError::TransferError);
    }
}
```text

## Platform-Specific Notes

### STM32
- Use DMA for better performance
- Enable DWT cycle counter for precise delays
- Consider using HAL timeout values

### ESP32
- Use dedicated SPI host (HSPI or VSPI)
- FreeRTOS delays acceptable for ms-range
- Use esp_rom_delay_us for µs precision

### Arduino
- Use SPI.beginTransaction() for settings
- SPI.transfer() for byte-by-byte
- delayMicroseconds() for timing

### Linux
- Use spidev for user-space access
- ioctl() for configuration
- GPIO sysfs or libgpiod for CS

---

**Navigation**: [← Driver API](07_Driver_API.md) | [Next: Usage Examples →](09_Usage_examples.md)
