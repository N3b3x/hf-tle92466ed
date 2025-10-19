/**
 * @file TLE92466ED_HAL.hpp
 * @brief Hardware Abstraction Layer (HAL) base class for TLE92466ED driver
 * @author AI Generated Driver
 * @date 2025-10-18
 * @version 1.0.0
 *
 * @details
 * This file defines the hardware abstraction layer interface for the TLE92466ED
 * Six-Channel High-Side Switch IC. The HAL provides a polymorphic interface that
 * allows the driver to work with any hardware platform by implementing the
 * virtual transmission functions.
 *
 * The TLE92466ED uses SPI communication with a 16-bit frame structure:
 * - MOSI: Data from microcontroller to IC
 * - MISO: Data from IC to microcontroller
 * - CS: Chip select (active low)
 * - SCLK: Serial clock
 *
 * @copyright
 * This is free and unencumbered software released into the public domain.
 */

#ifndef TLE92466ED_HAL_HPP
#define TLE92466ED_HAL_HPP

#include <cstdint>
#include <concepts>
#include <span>
#include <chrono>
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
 * Defines the configuration parameters for a single SPI transaction.
 */
struct SPIConfig {
    uint32_t frequency{1'000'000};        ///< SPI clock frequency in Hz (max 10 MHz for TLE92466ED)
    uint8_t mode{0};                      ///< SPI mode (CPOL=0, CPHA=0 for TLE92466ED)
    uint8_t bits_per_word{16};            ///< Bits per word (16-bit for TLE92466ED)
    bool msb_first{true};                 ///< MSB first transmission
    std::chrono::milliseconds timeout{100}; ///< Transaction timeout
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
 * The HAL uses modern C++20/21 features including:
 * - Concepts for compile-time constraints
 * - std::span for safe array access
 * - std::expected for error handling
 * - std::chrono for time management
 *
 * @par Example Implementation for a specific platform:
 * @code{.cpp}
 * class MyPlatformHAL : public TLE92466ED::HAL {
 * public:
 *     HALResult<void> init() override {
 *         // Initialize SPI peripheral
 *         spi_init();
 *         return {};
 *     }
 *
 *     HALResult<uint16_t> transfer(uint16_t data) override {
 *         uint16_t result = spi_transfer_16bit(data);
 *         if (spi_error()) {
 *             return std::unexpected(HALError::TransferError);
 *         }
 *         return result;
 *     }
 *
 *     // ... implement other virtual functions
 * };
 * @endcode
 *
 * @par Thread Safety:
 * Implementations must ensure thread-safety for multi-threaded environments.
 *
 * @par Hardware Requirements:
 * - SPI peripheral capable of 16-bit transfers
 * - Minimum frequency: 100 kHz
 * - Maximum frequency: 10 MHz
 * - Support for SPI Mode 0 (CPOL=0, CPHA=0)
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
     *
     * @par Example:
     * @code{.cpp}
     * auto result = hal->init();
     * if (!result) {
     *     // Handle initialization error
     *     std::cout << "Init failed" << std::endl;
     * }
     * @endcode
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
     * @brief Transfer 16-bit data via SPI (full-duplex)
     * 
     * @details
     * Performs a full-duplex SPI transaction, simultaneously sending and
     * receiving 16 bits of data. This is the primary communication method
     * for the TLE92466ED.
     *
     * The TLE92466ED requires 16-bit SPI frames with the following format:
     * - Bit 15: R/W (1=Read, 0=Write)
     * - Bits 14-8: Register address
     * - Bits 7-0: Data
     *
     * @param[in] tx_data The 16-bit data to transmit
     * @return HALResult<uint16_t> Received 16-bit data or error
     * @retval HALError::TransferError SPI transfer failed
     * @retval HALError::Timeout Transfer timeout
     *
     * @par Timing Requirements:
     * - CS must be held low during entire 16-bit transfer
     * - Minimum CS inactive time between transfers: 100ns
     * - Data sampled on rising edge (CPHA=0)
     *
     * @par Example:
     * @code{.cpp}
     * auto result = hal->transfer(0x8000);  // Read command
     * if (result) {
     *     uint16_t response = result.value();
     *     // Process response
     * }
     * @endcode
     */
    [[nodiscard]] virtual HALResult<uint16_t> transfer(uint16_t tx_data) noexcept = 0;

    /**
     * @brief Transfer multiple 16-bit words via SPI
     * 
     * @details
     * Performs multiple consecutive SPI transfers efficiently. Useful for
     * reading or writing multiple registers in sequence.
     *
     * @param[in] tx_data Span of transmit data
     * @param[out] rx_data Span to store received data
     * @return HALResult<void> Success or error code
     * @retval HALError::InvalidParameter Buffer size mismatch
     * @retval HALError::TransferError Transfer failed
     *
     * @pre tx_data.size() == rx_data.size()
     * @pre Both spans must be valid for the duration of the transfer
     *
     * @par Example:
     * @code{.cpp}
     * std::array<uint16_t, 3> tx = {0x8000, 0x8001, 0x8002};
     * std::array<uint16_t, 3> rx{};
     * auto result = hal->transfer_multi(tx, rx);
     * @endcode
     */
    [[nodiscard]] virtual HALResult<void> transfer_multi(
        std::span<const uint16_t> tx_data,
        std::span<uint16_t> rx_data) noexcept = 0;

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
     * @note Some implementations may handle CS automatically in transfer()
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
     * @param[in] duration Duration to delay
     * @return HALResult<void> Success or error code
     *
     * @par Timing Requirements:
     * - Reset pulse width: minimum 1μs
     * - Power-up delay: minimum 1ms
     *
     * @par Example:
     * @code{.cpp}
     * using namespace std::chrono_literals;
     * hal->delay(1ms);  // 1 millisecond delay
     * @endcode
     */
    [[nodiscard]] virtual HALResult<void> delay(std::chrono::microseconds duration) noexcept = 0;

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
     * - Frame size: 16 bits
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
     *
     * @par Example:
     * @code{.cpp}
     * if (hal->is_ready()) {
     *     hal->transfer(0x0000);
     * }
     * @endcode
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
 *
 * @par Example:
 * @code{.cpp}
 * template<HALInterface T>
 * class MyDriver {
 *     T& hal;
 * public:
 *     explicit MyDriver(T& h) : hal(h) {}
 * };
 * @endcode
 */
template<typename T>
concept HALInterface = std::is_base_of_v<HAL, T> && requires(T hal, uint16_t data, SPIConfig cfg) {
    { hal.init() } -> std::same_as<HALResult<void>>;
    { hal.transfer(data) } -> std::same_as<HALResult<uint16_t>>;
    { hal.chip_select() } -> std::same_as<HALResult<void>>;
    { hal.chip_deselect() } -> std::same_as<HALResult<void>>;
    { hal.is_ready() } -> std::same_as<bool>;
    { hal.configure(cfg) } -> std::same_as<HALResult<void>>;
};

} // namespace TLE92466ED

#endif // TLE92466ED_HAL_HPP
