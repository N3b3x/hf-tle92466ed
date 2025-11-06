/**
 * @file TLE92466ED_HAL.hpp
 * @brief Hardware Abstraction Layer (HAL) base class for TLE92466ED driver
 
 * @details
 * This file defines the hardware abstraction layer interface for the TLE92466ED
 * Six-Channel Low-Side Solenoid Driver IC. The HAL provides a polymorphic interface
 * that allows the driver to work with any hardware platform by implementing the
 * virtual transmission functions.
 *
 * The TLE92466ED uses **32-bit SPI communication** with the following structure:
 * - MOSI: 32-bit frame (CRC[31:24] + Address[23:17] + R/W[16] + Data[15:0])
 * - MISO: 32-bit frame (CRC[31:24] + ReplyMode[23:22] + Status[21:17] + R/W[16] + Data[15:0])
 * - CS: Chip select (active low)
 * - SCLK: Serial clock (up to 10 MHz)
 * - CRC: SAE J1850 8-bit CRC
 *
 * @copyright
 * This is free and unencumbered software released into the public domain.
 */

#ifndef TLE92466ED_HAL_HPP
#define TLE92466ED_HAL_HPP

#include <cstdint>
#include <concepts>
#include <span>
#include <expected>

namespace TLE92466ED {

/**
 * @brief Error codes for HAL operations
 * 
 * This enumeration defines all possible error conditions that can occur
 * during hardware communication with the TLE92466ED IC.
 */
enum class HALError : uint8_t {
    None = 0,                 ///< No error occurred
    BusError,                 ///< SPI bus communication error
    Timeout,                  ///< Operation timed out
    InvalidParameter,         ///< Invalid parameter passed to function
    ChipselectError,          ///< Chip select control failed
    TransferError,            ///< Data transfer failed
    HardwareNotReady,         ///< Hardware not initialized or ready
    BufferOverflow,           ///< Buffer size exceeded
    CRCError,                 ///< CRC mismatch error
    UnknownError              ///< Unknown error occurred
};

/**
 * @brief Result type for HAL operations using std::expected (C++23)
 * 
 * @tparam T The success type
 * 
 * This provides a modern, safe way to return either a success value or an error.
 */
template<typename T>
using HALResult = std::expected<T, HALError>;

/**
 * @brief SPI transaction configuration
 * 
 * Defines the configuration parameters for SPI communication.
 */
struct SPIConfig {
    uint32_t frequency{1'000'000};        ///< SPI clock frequency in Hz (max 10 MHz for TLE92466ED)
    uint8_t mode{0};                      ///< SPI mode (CPOL=0, CPHA=0 for TLE92466ED)
    uint8_t bits_per_word{8};             ///< Bits per word (8-bit, transfer 4 bytes for 32-bit frame)
    bool msb_first{true};                 ///< MSB first transmission
    uint32_t timeout_ms{100}; ///< Transaction timeout in milliseconds
};

/**
 * @brief Abstract Hardware Abstraction Layer (HAL) base class
 * 
 * @details
 * This pure virtual base class defines the interface that must be implemented
 * for hardware-specific SPI communication. Users must derive from this class
 * and implement the virtual functions for their specific hardware platform
 * (e.g., STM32, ESP32, Arduino, Linux, etc.).
 *
 * The HAL uses modern C++20/23 features including:
 * - Concepts for compile-time constraints
 * - std::span for safe array access
 * - std::expected for error handling
 * - uint32_t for time management (microseconds)
 *
 * @par 32-Bit SPI Communication:
 * The TLE92466ED requires 32-bit SPI frames. Implementations must:
 * - Transfer 4 bytes (32 bits) per transaction
 * - Maintain MSB-first byte order
 * - Support full-duplex operation
 * - Calculate and verify CRC-8 (SAE J1850)
 *
 * @par Example Implementation:
 * @code{.cpp}
 * class MyPlatformHAL : public TLE92466ED::HAL {
 * public:
 *     HALResult<uint32_t> transfer32(uint32_t data) noexcept override {
 *         uint32_t result = spi_transfer_32bit(data);
 *         if (spi_error()) {
 *             return std::unexpected(HALError::TransferError);
 *         }
 *         return result;
 *     }
 *     // ... implement other virtual functions
 * };
 * @endcode
 *
 * @par Thread Safety:
 * Implementations must ensure thread-safety for multi-threaded environments.
 *
 * @par Hardware Requirements:
 * - SPI peripheral capable of 32-bit transfers (or 4x 8-bit)
 * - Minimum frequency: 100 kHz
 * - Maximum frequency: 10 MHz
 * - Support for SPI Mode 0 (CPOL=0, CPHA=0)
 * - CRC calculation capability (hardware or software)
 */
class HAL {
public:
    /**
     * @brief Virtual destructor for polymorphic behavior
     */
    virtual ~HAL() = default;

    /**
     * @brief Initialize the hardware interface
     * 
     * @details
     * This function should initialize the SPI peripheral, configure GPIO pins,
     * and prepare the hardware for communication. It should be called before
     * any other HAL functions.
     *
     * @return HALResult<void> Success or error code
     * @retval HALError::None Initialization successful
     * @retval HALError::HardwareNotReady Hardware initialization failed
     * @retval HALError::InvalidParameter Invalid configuration
     */
    [[nodiscard]] virtual HALResult<void> init() noexcept = 0;

    /**
     * @brief Deinitialize the hardware interface
     * 
     * @details
     * Releases hardware resources and disables the SPI peripheral. Should be
     * called when the driver is no longer needed.
     *
     * @return HALResult<void> Success or error code
     */
    [[nodiscard]] virtual HALResult<void> deinit() noexcept = 0;

    /**
     * @brief Transfer 32-bit data via SPI (full-duplex)
     * 
     * @details
     * Performs a full-duplex SPI transaction, simultaneously sending and
     * receiving 32 bits of data. This is the primary communication method
     * for the TLE92466ED.
     *
     * The TLE92466ED requires 32-bit SPI frames with the following format:
     * - Bits [31:24]: CRC-8 (SAE J1850)
     * - Bits [23:17]: Register address (7 bits of 10-bit address)
     * - Bit [16]: R/W (1=Write, 0=Read)
     * - Bits [15:0]: Data (16 bits)
     *
     * @param[in] tx_data The 32-bit data to transmit
     * @return HALResult<uint32_t> Received 32-bit data or error
     * @retval HALError::TransferError SPI transfer failed
     * @retval HALError::Timeout Transfer timeout
     *
     * @par Timing Requirements:
     * - CS must be held low during entire 32-bit transfer
     * - Minimum CS inactive time between transfers: 100ns
     * - Data sampled on rising edge (CPHA=0)
     *
     * @note CRC calculation is handled by the driver layer, not HAL
     */
    [[nodiscard]] virtual HALResult<uint32_t> transfer32(uint32_t tx_data) noexcept = 0;

    /**
     * @brief Transfer multiple 32-bit words via SPI
     * 
     * @details
     * Performs multiple consecutive SPI transfers efficiently. Useful for
     * reading or writing multiple registers in sequence.
     *
     * @param[in] tx_data Span of transmit data (32-bit words)
     * @param[out] rx_data Span to store received data (32-bit words)
     * @return HALResult<void> Success or error code
     * @retval HALError::InvalidParameter Buffer size mismatch
     * @retval HALError::TransferError Transfer failed
     *
     * @pre tx_data.size() == rx_data.size()
     * @pre Both spans must be valid for the duration of the transfer
     */
    [[nodiscard]] virtual HALResult<void> transfer_multi(
        std::span<const uint32_t> tx_data,
        std::span<uint32_t> rx_data) noexcept = 0;

    /**
     * @brief Assert (activate) chip select
     * 
     * @details
     * Pulls the CS line low to select the TLE92466ED for communication.
     * Must be called before SPI transfers in manual CS mode.
     *
     * @return HALResult<void> Success or error code
     * @retval HALError::ChipselectError CS control failed
     *
     * @note Some implementations may handle CS automatically in transfer32()
     */
    [[nodiscard]] virtual HALResult<void> chip_select() noexcept = 0;

    /**
     * @brief Deassert (deactivate) chip select
     * 
     * @details
     * Pulls the CS line high to deselect the TLE92466ED after communication.
     * Must be called after SPI transfers in manual CS mode.
     *
     * @return HALResult<void> Success or error code
     * @retval HALError::ChipselectError CS control failed
     */
    [[nodiscard]] virtual HALResult<void> chip_deselect() noexcept = 0;

    /**
     * @brief Delay for specified duration
     * 
     * @details
     * Provides a hardware-specific delay implementation. Required for timing
     * constraints such as reset pulse width and power-up delays.
     *
     * @param[in] microseconds Duration to delay in microseconds
     * @return HALResult<void> Success or error code
     *
     * @par Timing Requirements:
     * - Reset pulse width: minimum 1µs
     * - Power-up delay: minimum 1ms
     */
    [[nodiscard]] virtual HALResult<void> delay(uint32_t microseconds) noexcept = 0;

    /**
     * @brief Configure SPI parameters
     * 
     * @details
     * Updates the SPI configuration. Can be called at runtime to adjust
     * communication parameters.
     *
     * @param[in] config New SPI configuration
     * @return HALResult<void> Success or error code
     * @retval HALError::InvalidParameter Invalid configuration
     *
     * @par TLE92466ED SPI Requirements:
     * - Frequency: 100 kHz - 10 MHz
     * - Mode: 0 (CPOL=0, CPHA=0)
     * - Bit order: MSB first
     * - Frame size: 32 bits (4 bytes)
     */
    [[nodiscard]] virtual HALResult<void> configure(const SPIConfig& config) noexcept = 0;

    /**
     * @brief Check if hardware is ready for communication
     * 
     * @details
     * Verifies that the hardware interface is initialized and ready for
     * SPI transactions.
     *
     * @return true if ready, false otherwise
     */
    [[nodiscard]] virtual bool is_ready() const noexcept = 0;

    /**
     * @brief Get the last error that occurred
     * 
     * @details
     * Retrieves the most recent error code. Useful for debugging and
     * error recovery.
     *
     * @return HALError The last error code
     */
    [[nodiscard]] virtual HALError get_last_error() const noexcept = 0;

    /**
     * @brief Clear any pending errors
     * 
     * @details
     * Resets the error state. Should be called after handling an error
     * condition and before retrying operations.
     *
     * @return HALResult<void> Success or error code
     */
    [[nodiscard]] virtual HALResult<void> clear_errors() noexcept = 0;

protected:
    /**
     * @brief Protected constructor to prevent direct instantiation
     * 
     * @details
     * This class can only be instantiated through derived classes.
     */
    HAL() = default;

    /**
     * @brief Prevent copying
     */
    HAL(const HAL&) = delete;
    HAL& operator=(const HAL&) = delete;

    /**
     * @brief Allow moving
     */
    HAL(HAL&&) noexcept = default;
    HAL& operator=(HAL&&) noexcept = default;
};

/**
 * @brief Concept to verify a type implements the HAL interface
 * 
 * @tparam T Type to check
 * 
 * @details
 * This C++20 concept ensures at compile-time that a class properly
 * implements the HAL interface. Provides better error messages than
 * traditional template constraints.
 */
template<typename T>
concept HALInterface = std::is_base_of_v<HAL, T> && requires(T hal, uint32_t data, SPIConfig cfg) {
    { hal.init() } -> std::same_as<HALResult<void>>;
    { hal.transfer32(data) } -> std::same_as<HALResult<uint32_t>>;
    { hal.chip_select() } -> std::same_as<HALResult<void>>;
    { hal.chip_deselect() } -> std::same_as<HALResult<void>>;
    { hal.is_ready() } -> std::same_as<bool>;
    { hal.configure(cfg) } -> std::same_as<HALResult<void>>;
};

} // namespace TLE92466ED

#endif // TLE92466ED_HAL_HPP
