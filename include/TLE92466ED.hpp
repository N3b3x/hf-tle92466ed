/**
 * @file TLE92466ED.hpp
 * @brief Main driver class for TLE92466ED Six-Channel High-Side Switch IC
 * @author AI Generated Driver
 * @date 2025-10-18
 * @version 1.0.0
 *
 * @details
 * Comprehensive C++20/21 driver for the Infineon TLE92466ED IC. This driver provides
 * a complete, feature-rich interface to all capabilities of the IC including:
 *
 * **Core Features:**
 * - 6 independent high-side output channels
 * - Individual channel control and monitoring
 * - Hardware-agnostic design via polymorphic HAL
 * - Modern C++20/21 features (concepts, ranges, std::expected)
 *
 * **Protection Features:**
 * - Adjustable current limiting (1A-4A per channel)
 * - Over-temperature protection and warning
 * - Under/over-voltage protection
 * - Short-circuit detection (to GND and VBAT)
 * - Open-load detection (on and off states)
 *
 * **Advanced Features:**
 * - PWM output capability on all channels (100Hz-20kHz)
 * - Configurable slew rate control
 * - Load current monitoring
 * - Comprehensive diagnostics
 * - Fault history tracking
 *
 * **Usage Example:**
 * @code{.cpp}
 * // Create HAL implementation for your platform
 * MyPlatformHAL hal;
 *
 * // Initialize driver
 * TLE92466ED::Driver driver(hal);
 * auto result = driver.init();
 *
 * // Configure channel 0
 * TLE92466ED::ChannelConfig config{
 *     .current_limit = TLE92466ED::CurrentLimit::LIMIT_2A,
 *     .pwm_enabled = true,
 *     .slew_rate = TLE92466ED::SlewRate::MEDIUM
 * };
 * driver.configure_channel(TLE92466ED::Channel::CH0, config);
 *
 * // Turn on channel 0
 * driver.set_channel(TLE92466ED::Channel::CH0, true);
 *
 * // Check for faults
 * if (auto status = driver.get_channel_status(TLE92466ED::Channel::CH0)) {
 *     if (status->has_fault) {
 *         // Handle fault condition
 *     }
 * }
 * @endcode
 *
 * @copyright
 * This is free and unencumbered software released into the public domain.
 */

#ifndef TLE92466ED_HPP
#define TLE92466ED_HPP

#include "TLE92466ED_HAL.hpp"
#include "TLE92466ED_Registers.hpp"
#include <expected>
#include <array>
#include <optional>
#include <functional>
#include <algorithm>

namespace TLE92466ED {

/**
 * @brief Driver error codes
 */
enum class DriverError : uint8_t {
    None = 0,             ///< No error
    NotInitialized,       ///< Driver not initialized
    HardwareError,        ///< HAL communication error
    InvalidChannel,       ///< Invalid channel number
    InvalidParameter,     ///< Invalid parameter value
    DeviceNotResponding,  ///< Device not responding to SPI
    WrongDeviceID,        ///< Incorrect device ID read
    RegisterError,        ///< Register read/write error
    FaultDetected,        ///< Device fault detected
    ConfigurationError,   ///< Configuration failed
    TimeoutError          ///< Operation timeout
};

/**
 * @brief Driver result type using std::expected (C++23)
 */
template<typename T>
using DriverResult = std::expected<T, DriverError>;

/**
 * @brief Channel configuration structure
 * 
 * @details
 * Contains all configurable parameters for an output channel.
 */
struct ChannelConfig {
    CurrentLimit current_limit{CurrentLimit::LIMIT_2A};  ///< Current limit setting
    SlewRate slew_rate{SlewRate::MEDIUM};                ///< Output slew rate
    bool pwm_enabled{false};                             ///< Enable PWM on channel
    bool diagnostics_enabled{true};                      ///< Enable diagnostics
    bool inverted{false};                                ///< Invert output logic
    
    /**
     * @brief Convert configuration to register value
     * @return 8-bit register value
     */
    [[nodiscard]] constexpr uint8_t to_register() const noexcept {
        uint8_t value = 0;
        value |= (static_cast<uint8_t>(current_limit) << 6);
        value |= (pwm_enabled ? CH_CFG::PWM_EN : 0);
        value |= (diagnostics_enabled ? CH_CFG::DIAG_EN : 0);
        value |= (static_cast<uint8_t>(slew_rate) << 2);
        value |= (inverted ? CH_CFG::INV : 0);
        return value;
    }
    
    /**
     * @brief Parse configuration from register value
     * @param value 8-bit register value
     * @return Parsed configuration
     */
    [[nodiscard]] static constexpr ChannelConfig from_register(uint8_t value) noexcept {
        return ChannelConfig{
            .current_limit = static_cast<CurrentLimit>((value & CH_CFG::ILIM_MASK) >> 6),
            .slew_rate = static_cast<SlewRate>((value & CH_CFG::SLEW_MASK) >> 2),
            .pwm_enabled = (value & CH_CFG::PWM_EN) != 0,
            .diagnostics_enabled = (value & CH_CFG::DIAG_EN) != 0,
            .inverted = (value & CH_CFG::INV) != 0
        };
    }
};

/**
 * @brief Channel status structure
 * 
 * @details
 * Contains real-time status information for a channel.
 */
struct ChannelStatus {
    bool enabled{false};               ///< Channel is enabled
    bool active{false};                ///< Channel is actively driving
    bool has_fault{false};             ///< Fault condition present
    bool current_limit_active{false};  ///< Current limiting active
    bool open_load_on{false};          ///< Open load detected (output on)
    bool open_load_off{false};         ///< Open load detected (output off)
    bool short_to_vbat{false};         ///< Short circuit to VBAT
    bool short_to_gnd{false};          ///< Short circuit to GND
    uint8_t load_current{0};           ///< Load current indicator (0-15)
    
    /**
     * @brief Parse status from register value
     * @param status_byte Status register value
     * @return Parsed status
     */
    [[nodiscard]] static constexpr ChannelStatus from_register(uint8_t status_byte) noexcept {
        return ChannelStatus{
            .enabled = (status_byte & CH_STATUS::ENABLED) != 0,
            .active = (status_byte & CH_STATUS::ACTIVE) != 0,
            .has_fault = (status_byte & CH_STATUS::CH_FAULT) != 0,
            .current_limit_active = (status_byte & CH_STATUS::ILIM) != 0,
            .open_load_on = (status_byte & CH_STATUS::OL_ON) != 0,
            .open_load_off = (status_byte & CH_STATUS::OL_OFF) != 0,
            .short_to_vbat = (status_byte & CH_STATUS::SC_HS) != 0,
            .short_to_gnd = (status_byte & CH_STATUS::SC_LS) != 0
        };
    }
};

/**
 * @brief Global device status structure
 */
struct DeviceStatus {
    bool any_fault{false};           ///< Any fault condition
    bool over_temp_warning{false};   ///< Over-temperature warning
    bool over_temp_shutdown{false};  ///< Over-temperature shutdown
    bool under_voltage{false};       ///< Under-voltage fault
    bool over_voltage{false};        ///< Over-voltage fault
    bool spi_error{false};           ///< SPI communication error
    bool power_on_reset{false};      ///< Power-on reset occurred
    uint8_t temperature{0};          ///< Temperature sensor reading
    uint16_t supply_voltage{0};      ///< Supply voltage reading
    
    /**
     * @brief Parse device status from register value
     */
    [[nodiscard]] static constexpr DeviceStatus from_register(uint8_t status_byte) noexcept {
        return DeviceStatus{
            .any_fault = (status_byte & STATUS1::FAULT) != 0,
            .over_temp_warning = (status_byte & STATUS1::OT_WARN) != 0,
            .over_temp_shutdown = (status_byte & STATUS1::OT_SD) != 0,
            .under_voltage = (status_byte & STATUS1::UV_FAULT) != 0,
            .over_voltage = (status_byte & STATUS1::OV_FAULT) != 0,
            .spi_error = (status_byte & STATUS1::SPI_ERR) != 0,
            .power_on_reset = (status_byte & STATUS1::POR) != 0
        };
    }
};

/**
 * @brief Channel diagnostic information
 */
struct ChannelDiagnostics {
    uint8_t load_current{0};          ///< Load current indicator (0-15)
    bool open_load_history{false};    ///< Open load occurred
    bool short_circuit_history{false}; ///< Short circuit occurred
    bool over_temp_history{false};    ///< Over-temperature occurred
    bool data_valid{false};           ///< Diagnostic data is valid
    
    /**
     * @brief Parse diagnostics from register value
     */
    [[nodiscard]] static constexpr ChannelDiagnostics from_register(uint8_t diag_byte) noexcept {
        return ChannelDiagnostics{
            .load_current = CH_DIAG::get_load_current(diag_byte),
            .open_load_history = (diag_byte & CH_DIAG::OL_HISTORY) != 0,
            .short_circuit_history = (diag_byte & CH_DIAG::SC_HISTORY) != 0,
            .over_temp_history = (diag_byte & CH_DIAG::OT_HISTORY) != 0,
            .data_valid = (diag_byte & CH_DIAG::DIAG_VALID) != 0
        };
    }
};

/**
 * @brief Global driver configuration
 */
struct GlobalConfig {
    bool diagnostics_enabled{true};       ///< Enable global diagnostics
    bool pwm_mode{false};                 ///< Enable PWM mode
    bool current_limit_enabled{true};     ///< Enable current limiting
    bool over_temp_enabled{true};         ///< Enable over-temp protection
    bool under_voltage_enabled{true};     ///< Enable under-voltage protection
    bool over_voltage_enabled{true};      ///< Enable over-voltage protection
    bool open_load_enabled{true};         ///< Enable open load detection
    bool short_circuit_enabled{true};     ///< Enable short circuit detection
    PWMFrequency pwm_frequency{PWMFrequency::FREQ_1KHZ};  ///< PWM frequency
    bool pwm_dithering{false};            ///< Enable PWM dithering
};

/**
 * @brief Main TLE92466ED driver class
 * 
 * @details
 * This is the primary driver class that provides complete control over the
 * TLE92466ED IC. It manages SPI communication, configuration, monitoring,
 * and diagnostics for all six output channels.
 *
 * **Thread Safety:**
 * This class is NOT thread-safe by default. External synchronization is required
 * for multi-threaded access.
 *
 * **Resource Management:**
 * Uses RAII principles - init() must be called explicitly but cleanup is automatic.
 *
 * **Error Handling:**
 * Uses std::expected for robust error handling without exceptions.
 *
 * @par Initialization Sequence:
 * 1. Construct driver with HAL reference
 * 2. Call init() to initialize hardware and verify device
 * 3. Configure global settings with configure_global()
 * 4. Configure individual channels with configure_channel()
 * 5. Control outputs with set_channel() or set_all_channels()
 *
 * @par Diagnostic Monitoring:
 * - Continuously monitor status with get_device_status()
 * - Check individual channels with get_channel_status()
 * - Retrieve diagnostics with get_channel_diagnostics()
 * - Clear fault history with clear_faults()
 */
class Driver {
public:
    /**
     * @brief Construct driver with HAL interface
     * 
     * @param hal Reference to hardware abstraction layer
     * 
     * @pre HAL must remain valid for the lifetime of the Driver
     * @post Driver is constructed but not initialized
     */
    explicit Driver(HAL& hal) : hal_(hal), initialized_(false) {}

    /**
     * @brief Destructor - ensures clean shutdown
     */
    ~Driver() {
        if (initialized_) {
            // Best effort shutdown - ignore errors
            (void)disable_all_channels();
        }
    }

    // Prevent copying
    Driver(const Driver&) = delete;
    Driver& operator=(const Driver&) = delete;

    // Allow moving
    Driver(Driver&&) noexcept = default;
    Driver& operator=(Driver&&) noexcept = default;

    //==========================================================================
    // INITIALIZATION AND CONFIGURATION
    //==========================================================================

    /**
     * @brief Initialize the driver and hardware
     * 
     * @details
     * Performs complete initialization sequence:
     * 1. Initialize HAL (SPI peripheral)
     * 2. Verify device communication
     * 3. Read and verify device ID
     * 4. Apply default configuration
     * 5. Clear any power-on faults
     *
     * @return DriverResult<void> Success or error code
     * @retval DriverError::HardwareError HAL initialization failed
     * @retval DriverError::DeviceNotResponding No SPI response
     * @retval DriverError::WrongDeviceID Device ID mismatch
     *
     * @par Example:
     * @code{.cpp}
     * Driver driver(hal);
     * if (auto result = driver.init(); !result) {
     *     // Handle initialization error
     *     std::cerr << "Init failed" << std::endl;
     *     return;
     * }
     * @endcode
     */
    [[nodiscard]] DriverResult<void> init() noexcept;

    /**
     * @brief Configure global device settings
     * 
     * @param config Global configuration structure
     * @return DriverResult<void> Success or error
     * @retval DriverError::NotInitialized Driver not initialized
     * @retval DriverError::ConfigurationError Configuration failed
     *
     * @par Example:
     * @code{.cpp}
     * GlobalConfig config{
     *     .pwm_mode = true,
     *     .pwm_frequency = PWMFrequency::FREQ_2KHZ
     * };
     * driver.configure_global(config);
     * @endcode
     */
    [[nodiscard]] DriverResult<void> configure_global(const GlobalConfig& config) noexcept;

    /**
     * @brief Configure individual channel
     * 
     * @param channel Channel to configure (CH0-CH5)
     * @param config Channel configuration
     * @return DriverResult<void> Success or error
     * @retval DriverError::InvalidChannel Invalid channel number
     * @retval DriverError::NotInitialized Driver not initialized
     *
     * @par Example:
     * @code{.cpp}
     * ChannelConfig cfg{
     *     .current_limit = CurrentLimit::LIMIT_3A,
     *     .slew_rate = SlewRate::FAST,
     *     .pwm_enabled = true
     * };
     * driver.configure_channel(Channel::CH0, cfg);
     * @endcode
     */
    [[nodiscard]] DriverResult<void> configure_channel(
        Channel channel, 
        const ChannelConfig& config) noexcept;

    //==========================================================================
    // OUTPUT CONTROL
    //==========================================================================

    /**
     * @brief Set individual channel on/off state
     * 
     * @param channel Channel to control
     * @param enabled true to enable output, false to disable
     * @return DriverResult<void> Success or error
     *
     * @par Example:
     * @code{.cpp}
     * // Turn on channel 0
     * driver.set_channel(Channel::CH0, true);
     * 
     * // Turn off channel 1
     * driver.set_channel(Channel::CH1, false);
     * @endcode
     */
    [[nodiscard]] DriverResult<void> set_channel(Channel channel, bool enabled) noexcept;

    /**
     * @brief Set multiple channels using bitmask
     * 
     * @param channel_mask Bitmask where bit N controls channel N
     * @return DriverResult<void> Success or error
     *
     * @par Example:
     * @code{.cpp}
     * // Turn on channels 0, 2, and 4
     * driver.set_channels(0b010101);
     * @endcode
     */
    [[nodiscard]] DriverResult<void> set_channels(uint8_t channel_mask) noexcept;

    /**
     * @brief Enable all channels
     * 
     * @return DriverResult<void> Success or error
     */
    [[nodiscard]] DriverResult<void> enable_all_channels() noexcept {
        return set_channels(OUT_CTRL::ALL_ON);
    }

    /**
     * @brief Disable all channels
     * 
     * @return DriverResult<void> Success or error
     */
    [[nodiscard]] DriverResult<void> disable_all_channels() noexcept {
        return set_channels(OUT_CTRL::ALL_OFF);
    }

    /**
     * @brief Toggle channel state
     * 
     * @param channel Channel to toggle
     * @return DriverResult<void> Success or error
     */
    [[nodiscard]] DriverResult<void> toggle_channel(Channel channel) noexcept;

    //==========================================================================
    // STATUS AND DIAGNOSTICS
    //==========================================================================

    /**
     * @brief Get global device status
     * 
     * @return DriverResult<DeviceStatus> Device status or error
     *
     * @par Example:
     * @code{.cpp}
     * if (auto status = driver.get_device_status()) {
     *     if (status->any_fault) {
     *         std::cout << "Device fault detected" << std::endl;
     *     }
     *     if (status->over_temp_warning) {
     *         std::cout << "Temperature warning" << std::endl;
     *     }
     * }
     * @endcode
     */
    [[nodiscard]] DriverResult<DeviceStatus> get_device_status() noexcept;

    /**
     * @brief Get individual channel status
     * 
     * @param channel Channel to query
     * @return DriverResult<ChannelStatus> Channel status or error
     *
     * @par Example:
     * @code{.cpp}
     * if (auto status = driver.get_channel_status(Channel::CH0)) {
     *     if (status->open_load_on) {
     *         std::cout << "Open load detected" << std::endl;
     *     }
     *     if (status->short_to_gnd) {
     *         std::cout << "Short to ground!" << std::endl;
     *     }
     * }
     * @endcode
     */
    [[nodiscard]] DriverResult<ChannelStatus> get_channel_status(Channel channel) noexcept;

    /**
     * @brief Get channel diagnostic information
     * 
     * @param channel Channel to query
     * @return DriverResult<ChannelDiagnostics> Diagnostics or error
     *
     * @par Example:
     * @code{.cpp}
     * if (auto diag = driver.get_channel_diagnostics(Channel::CH0)) {
     *     std::cout << "Load current: " 
     *               << static_cast<int>(diag->load_current) << std::endl;
     * }
     * @endcode
     */
    [[nodiscard]] DriverResult<ChannelDiagnostics> get_channel_diagnostics(
        Channel channel) noexcept;

    /**
     * @brief Get all channels status at once
     * 
     * @return DriverResult<std::array<ChannelStatus, 6>> Status array or error
     *
     * @par Example:
     * @code{.cpp}
     * if (auto statuses = driver.get_all_channels_status()) {
     *     for (size_t i = 0; i < statuses->size(); ++i) {
     *         if ((*statuses)[i].has_fault) {
     *             std::cout << "Channel " << i << " fault" << std::endl;
     *         }
     *     }
     * }
     * @endcode
     */
    [[nodiscard]] DriverResult<std::array<ChannelStatus, 6>> get_all_channels_status() noexcept;

    /**
     * @brief Read device temperature
     * 
     * @return DriverResult<uint8_t> Temperature value or error
     * @note Temperature encoding is device-specific, refer to datasheet
     */
    [[nodiscard]] DriverResult<uint8_t> get_temperature() noexcept;

    /**
     * @brief Read supply voltage
     * 
     * @return DriverResult<uint16_t> Voltage value or error
     * @note Voltage encoding is device-specific, refer to datasheet
     */
    [[nodiscard]] DriverResult<uint16_t> get_supply_voltage() noexcept;

    //==========================================================================
    // FAULT MANAGEMENT
    //==========================================================================

    /**
     * @brief Clear all fault flags
     * 
     * @details
     * Clears latched fault conditions and history flags.
     * Does not clear active faults (e.g., ongoing short circuit).
     *
     * @return DriverResult<void> Success or error
     */
    [[nodiscard]] DriverResult<void> clear_faults() noexcept;

    /**
     * @brief Check if any channel has a fault
     * 
     * @return DriverResult<bool> true if any fault exists
     */
    [[nodiscard]] DriverResult<bool> has_any_fault() noexcept;

    /**
     * @brief Software reset of the device
     * 
     * @details
     * Performs a software reset. All registers return to default values.
     * init() must be called again after reset.
     *
     * @return DriverResult<void> Success or error
     */
    [[nodiscard]] DriverResult<void> software_reset() noexcept;

    //==========================================================================
    // ADVANCED FEATURES
    //==========================================================================

    /**
     * @brief Enter sleep mode
     * 
     * @details
     * Puts device into low-power sleep mode. All outputs are disabled.
     * SPI communication remains active.
     *
     * @return DriverResult<void> Success or error
     */
    [[nodiscard]] DriverResult<void> enter_sleep_mode() noexcept;

    /**
     * @brief Exit sleep mode
     * 
     * @return DriverResult<void> Success or error
     */
    [[nodiscard]] DriverResult<void> exit_sleep_mode() noexcept;

    /**
     * @brief Configure PWM frequency for all channels
     * 
     * @param frequency PWM frequency setting
     * @return DriverResult<void> Success or error
     */
    [[nodiscard]] DriverResult<void> set_pwm_frequency(PWMFrequency frequency) noexcept;

    /**
     * @brief Verify device ID matches expected value
     * 
     * @return DriverResult<bool> true if ID matches
     */
    [[nodiscard]] DriverResult<bool> verify_device_id() noexcept;

    /**
     * @brief Check if driver is initialized
     * 
     * @return true if initialized and ready
     */
    [[nodiscard]] bool is_initialized() const noexcept {
        return initialized_;
    }

    //==========================================================================
    // REGISTER ACCESS (Advanced)
    //==========================================================================

    /**
     * @brief Read register directly
     * 
     * @param address Register address (7-bit)
     * @return DriverResult<uint8_t> Register value or error
     *
     * @warning Direct register access bypasses driver state management
     */
    [[nodiscard]] DriverResult<uint8_t> read_register(uint8_t address) noexcept;

    /**
     * @brief Write register directly
     * 
     * @param address Register address (7-bit)
     * @param value Value to write
     * @return DriverResult<void> Success or error
     *
     * @warning Direct register access bypasses driver state management
     */
    [[nodiscard]] DriverResult<void> write_register(uint8_t address, uint8_t value) noexcept;

    /**
     * @brief Modify register bits
     * 
     * @param address Register address
     * @param mask Bit mask for modification
     * @param value New bit values
     * @return DriverResult<void> Success or error
     *
     * @details Reads register, modifies specified bits, writes back
     */
    [[nodiscard]] DriverResult<void> modify_register(
        uint8_t address, 
        uint8_t mask, 
        uint8_t value) noexcept;

private:
    //==========================================================================
    // PRIVATE METHODS
    //==========================================================================

    /**
     * @brief Transfer SPI frame and check for errors
     */
    [[nodiscard]] DriverResult<uint16_t> transfer_checked(uint16_t frame) noexcept;

    /**
     * @brief Validate channel number
     */
    [[nodiscard]] constexpr bool is_valid_channel(Channel channel) const noexcept {
        return to_index(channel) < static_cast<uint8_t>(Channel::COUNT);
    }

    /**
     * @brief Check if driver is initialized (for internal use)
     */
    [[nodiscard]] DriverResult<void> check_initialized() const noexcept {
        if (!initialized_) {
            return std::unexpected(DriverError::NotInitialized);
        }
        return {};
    }

    /**
     * @brief Apply default configuration after initialization
     */
    [[nodiscard]] DriverResult<void> apply_default_config() noexcept;

    //==========================================================================
    // MEMBER VARIABLES
    //==========================================================================

    HAL& hal_;                              ///< Hardware abstraction layer
    bool initialized_;                      ///< Initialization status
    std::array<uint8_t, 6> channel_state_;  ///< Cached channel states
    uint8_t output_register_cache_;         ///< Cached output control register
};

} // namespace TLE92466ED

#endif // TLE92466ED_HPP
