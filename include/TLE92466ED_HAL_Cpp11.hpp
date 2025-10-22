/**
 * @file TLE92466ED_HAL_Cpp11.hpp
 * @brief Hardware Abstraction Layer (HAL) base class for TLE92466ED driver - C++11 compatible
 * @author AI Generated Driver
 * @date 2025-10-20
 * @version 2.0.0
 *
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

#ifndef TLE92466ED_HAL_CPP11_HPP
#define TLE92466ED_HAL_CPP11_HPP

#include <cstdint>

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
 * @brief SPI transaction configuration
 * 
 * Defines the configuration parameters for SPI communication.
 */
struct SPIConfig {
    uint32_t frequency;        ///< SPI clock frequency in Hz (max 10 MHz for TLE92466ED)
    uint8_t mode;              ///< SPI mode (CPOL=0, CPHA=0 for TLE92466ED)
    uint8_t bits_per_word;     ///< Bits per word (8-bit, transfer 4 bytes for 32-bit frame)
    bool msb_first;            ///< MSB first transmission
    uint32_t timeout_ms;       ///< Transaction timeout in milliseconds
    
    // C++11 constructor
    SPIConfig() : frequency(1000000), mode(0), bits_per_word(8), msb_first(true), timeout_ms(100) {}
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
 * The HAL uses C++11 features and is designed for embedded systems:
 * - Simple error handling with error codes
 * - noexcept functions for embedded safety
 * - Minimal dependencies
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
 *     bool transfer32(uint32_t tx_data, uint32_t* rx_data, HALError* error = nullptr) noexcept override {
 *         uint32_t result = spi_transfer_32bit(tx_data);
 *         if (spi_error()) {
 *             if (error) *error = HALError::TransferError;
 *             return false;
 *         }
 *         *rx_data = result;
 *         return true;
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
    virtual ~HAL() {}

    /**
     * @brief Initialize the hardware interface
     * 
     * @details
     * This function should initialize the SPI peripheral, configure GPIO pins,
     * and prepare the hardware for communication. It should be called before
     * any other HAL functions.
     *
     * @param[out] error Optional pointer to receive error code
     * @return true if successful, false on error
     * @retval HALError::None Initialization successful
     * @retval HALError::HardwareNotReady Hardware initialization failed
     * @retval HALError::InvalidParameter Invalid configuration
     */
    virtual bool init(HALError* error = nullptr) noexcept = 0;

    /**
     * @brief Deinitialize the hardware interface
     * 
     * @details
     * Releases hardware resources and disables the SPI peripheral. Should be
     * called when the driver is no longer needed.
     *
     * @param[out] error Optional pointer to receive error code
     * @return true if successful, false on error
     */
    virtual bool deinit(HALError* error = nullptr) noexcept = 0;

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
     * @param[out] rx_data Pointer to store received 32-bit data
     * @param[out] error Optional pointer to receive error code
     * @return true if successful, false on error
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
    virtual bool transfer32(uint32_t tx_data, uint32_t* rx_data, HALError* error = nullptr) noexcept = 0;

    /**
     * @brief Transfer multiple 32-bit words via SPI
     * 
     * @details
     * Performs multiple consecutive SPI transfers efficiently. Useful for
     * reading or writing multiple registers in sequence.
     *
     * @param[in] tx_data Pointer to transmit data (32-bit words)
     * @param[out] rx_data Pointer to store received data (32-bit words)
     * @param[in] count Number of 32-bit words to transfer
     * @param[out] error Optional pointer to receive error code
     * @return true if successful, false on error
     * @retval HALError::InvalidParameter Invalid parameters
     * @retval HALError::TransferError Transfer failed
     *
     * @pre tx_data and rx_data must be valid for count words
     */
    virtual bool transfer_multi(const uint32_t* tx_data, uint32_t* rx_data, size_t count, HALError* error = nullptr) noexcept = 0;

    /**
     * @brief Assert (activate) chip select
     * 
     * @details
     * Pulls the CS line low to select the TLE92466ED for communication.
     * Must be called before SPI transfers in manual CS mode.
     *
     * @param[out] error Optional pointer to receive error code
     * @return true if successful, false on error
     * @retval HALError::ChipselectError CS control failed
     *
     * @note Some implementations may handle CS automatically in transfer32()
     */
    virtual bool chip_select(HALError* error = nullptr) noexcept = 0;

    /**
     * @brief Deassert (deactivate) chip select
     * 
     * @details
     * Pulls the CS line high to deselect the TLE92466ED after communication.
     * Must be called after SPI transfers in manual CS mode.
     *
     * @param[out] error Optional pointer to receive error code
     * @return true if successful, false on error
     * @retval HALError::ChipselectError CS control failed
     */
    virtual bool chip_deselect(HALError* error = nullptr) noexcept = 0;

    /**
     * @brief Delay for specified duration
     * 
     * @details
     * Provides a hardware-specific delay implementation. Required for timing
     * constraints such as reset pulse width and power-up delays.
     *
     * @param[in] microseconds Duration to delay in microseconds
     * @param[out] error Optional pointer to receive error code
     * @return true if successful, false on error
     *
     * @par Timing Requirements:
     * - Reset pulse width: minimum 1µs
     * - Power-up delay: minimum 1ms
     */
    virtual bool delay(uint32_t microseconds, HALError* error = nullptr) noexcept = 0;

    /**
     * @brief Configure SPI parameters
     * 
     * @details
     * Updates the SPI configuration. Can be called at runtime to adjust
     * communication parameters.
     *
     * @param[in] config New SPI configuration
     * @param[out] error Optional pointer to receive error code
     * @return true if successful, false on error
     * @retval HALError::InvalidParameter Invalid configuration
     *
     * @par TLE92466ED SPI Requirements:
     * - Frequency: 100 kHz - 10 MHz
     * - Mode: 0 (CPOL=0, CPHA=0)
     * - Bit order: MSB first
     * - Frame size: 32 bits (4 bytes)
     */
    virtual bool configure(const SPIConfig& config, HALError* error = nullptr) noexcept = 0;

    /**
     * @brief Check if hardware is ready for communication
     * 
     * @details
     * Verifies that the hardware interface is initialized and ready for
     * SPI transactions.
     *
     * @return true if ready, false otherwise
     */
    virtual bool is_ready() const noexcept = 0;

    /**
     * @brief Get the last error that occurred
     * 
     * @details
     * Retrieves the most recent error code. Useful for debugging and
     * error recovery.
     *
     * @return HALError The last error code
     */
    virtual HALError get_last_error() const noexcept = 0;

    /**
     * @brief Clear any pending errors
     * 
     * @details
     * Resets the error state. Should be called after handling an error
     * condition and before retrying operations.
     *
     * @param[out] error Optional pointer to receive error code
     * @return true if successful, false on error
     */
    virtual bool clear_errors(HALError* error = nullptr) noexcept = 0;

protected:
    /**
     * @brief Protected constructor to prevent direct instantiation
     * 
     * @details
     * This class can only be instantiated through derived classes.
     */
    HAL() {}

    // Prevent copying
    HAL(const HAL&);
    HAL& operator=(const HAL&);
};

} // namespace TLE92466ED

#endif // TLE92466ED_HAL_CPP11_HPP