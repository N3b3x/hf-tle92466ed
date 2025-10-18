/**
 * @file TLE92466ED.cpp
 * @brief Implementation of TLE92466ED driver class
 * @author AI Generated Driver
 * @date 2025-10-18
 * @version 1.0.0
 *
 * @details
 * This file contains the complete implementation of all driver methods
 * for the TLE92466ED IC. All methods include comprehensive error checking
 * and validation.
 *
 * @copyright
 * This is free and unencumbered software released into the public domain.
 */

#include "TLE92466ED.hpp"
#include <algorithm>
#include <chrono>

using namespace std::chrono_literals;

namespace TLE92466ED {

//==============================================================================
// INITIALIZATION
//==============================================================================

DriverResult<void> Driver::init() noexcept {
    // 1. Initialize HAL
    if (auto result = hal_.init(); !result) {
        return unexpected(DriverError::HardwareError);
    }

    // 2. Wait for device power-up (minimum 1ms)
    if (auto result = hal_.delay(2ms); !result) {
        return unexpected(DriverError::HardwareError);
    }

    // 3. Verify device communication by reading device ID
    auto verify_result = verify_device_id();
    if (!verify_result) {
        return unexpected(verify_result.error());
    }
    if (!*verify_result) {
        return unexpected(DriverError::WrongDeviceID);
    }

    // 4. Apply default configuration
    if (auto result = apply_default_config(); !result) {
        return unexpected(result.error());
    }

    // 5. Clear any power-on reset flags
    if (auto result = clear_faults(); !result) {
        return unexpected(result.error());
    }

    // 6. Initialize cached state
    output_register_cache_ = 0;
    channel_state_.fill(0);

    initialized_ = true;
    return {};
}

DriverResult<void> Driver::apply_default_config() noexcept {
    // Configure CTRL1: Enable chip, enable diagnostics
    uint8_t ctrl1 = CTRL1::ENABLE | CTRL1::DIAG_EN;
    if (auto result = write_register(RegisterAddress::CTRL1, ctrl1); !result) {
        return unexpected(result.error());
    }

    // Configure CTRL2: Enable all protections
    uint8_t ctrl2 = CTRL2::DEFAULT;
    if (auto result = write_register(RegisterAddress::CTRL2, ctrl2); !result) {
        return unexpected(result.error());
    }

    // Configure default PWM frequency (1 kHz)
    if (auto result = write_register(RegisterAddress::PWM_CTRL, PWM_CTRL::DEFAULT); !result) {
        return unexpected(result.error());
    }

    // Configure all channels with default settings
    for (uint8_t ch = 0; ch < static_cast<uint8_t>(Channel::COUNT); ++ch) {
        uint8_t addr = RegisterAddress::CH0_CFG + ch;
        if (auto result = write_register(addr, CH_CFG::DEFAULT); !result) {
            return unexpected(result.error());
        }
    }

    // Ensure all outputs are off initially
    if (auto result = write_register(RegisterAddress::OUT_CTRL, OUT_CTRL::ALL_OFF); !result) {
        return unexpected(result.error());
    }

    return {};
}

//==============================================================================
// CONFIGURATION
//==============================================================================

DriverResult<void> Driver::configure_global(const GlobalConfig& config) noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    // Configure CTRL1
    uint8_t ctrl1 = CTRL1::ENABLE;
    if (config.diagnostics_enabled) ctrl1 |= CTRL1::DIAG_EN;
    if (config.pwm_mode) ctrl1 |= CTRL1::PWM_MODE;

    if (auto result = write_register(RegisterAddress::CTRL1, ctrl1); !result) {
        return unexpected(result.error());
    }

    // Configure CTRL2
    uint8_t ctrl2 = 0;
    if (config.current_limit_enabled) ctrl2 |= CTRL2::ILIM_EN;
    if (config.over_temp_enabled) ctrl2 |= CTRL2::OT_EN;
    if (config.under_voltage_enabled) ctrl2 |= CTRL2::UV_EN;
    if (config.over_voltage_enabled) ctrl2 |= CTRL2::OV_EN;
    if (config.open_load_enabled) ctrl2 |= CTRL2::OL_EN;
    if (config.short_circuit_enabled) ctrl2 |= CTRL2::SC_EN;

    if (auto result = write_register(RegisterAddress::CTRL2, ctrl2); !result) {
        return unexpected(result.error());
    }

    // Configure PWM
    uint8_t pwm_ctrl = static_cast<uint8_t>(config.pwm_frequency) << 5;
    if (config.pwm_dithering) pwm_ctrl |= PWM_CTRL::DITHER_EN;

    if (auto result = write_register(RegisterAddress::PWM_CTRL, pwm_ctrl); !result) {
        return unexpected(result.error());
    }

    return {};
}

DriverResult<void> Driver::configure_channel(
    Channel channel,
    const ChannelConfig& config) noexcept {

    if (auto result = check_initialized(); !result) {
        return result;
    }

    if (!is_valid_channel(channel)) {
        return unexpected(DriverError::InvalidChannel);
    }

    uint8_t addr = channel_to_config_addr(channel);
    uint8_t value = config.to_register();

    if (auto result = write_register(addr, value); !result) {
        return unexpected(result.error());
    }

    // Cache the configuration
    channel_state_[to_index(channel)] = value;

    return {};
}

//==============================================================================
// OUTPUT CONTROL
//==============================================================================

DriverResult<void> Driver::set_channel(Channel channel, bool enabled) noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    if (!is_valid_channel(channel)) {
        return unexpected(DriverError::InvalidChannel);
    }

    uint8_t mask = OUT_CTRL::channel_mask(to_index(channel));

    if (enabled) {
        output_register_cache_ |= mask;
    } else {
        output_register_cache_ &= ~mask;
    }

    return write_register(RegisterAddress::OUT_CTRL, output_register_cache_);
}

DriverResult<void> Driver::set_channels(uint8_t channel_mask) noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    // Mask to valid channels only (bits 0-5)
    channel_mask &= OUT_CTRL::ALL_ON;
    output_register_cache_ = channel_mask;

    return write_register(RegisterAddress::OUT_CTRL, channel_mask);
}

DriverResult<void> Driver::toggle_channel(Channel channel) noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    if (!is_valid_channel(channel)) {
        return unexpected(DriverError::InvalidChannel);
    }

    // Read current state
    auto read_result = read_register(RegisterAddress::OUT_CTRL);
    if (!read_result) {
        return unexpected(read_result.error());
    }

    uint8_t mask = OUT_CTRL::channel_mask(to_index(channel));
    uint8_t new_state = *read_result ^ mask;  // Toggle bit

    output_register_cache_ = new_state;
    return write_register(RegisterAddress::OUT_CTRL, new_state);
}

//==============================================================================
// STATUS AND DIAGNOSTICS
//==============================================================================

DriverResult<DeviceStatus> Driver::get_device_status() noexcept {
    if (auto result = check_initialized(); !result) {
        return unexpected(result.error());
    }

    auto status_result = read_register(RegisterAddress::STATUS1);
    if (!status_result) {
        return unexpected(status_result.error());
    }

    DeviceStatus status = DeviceStatus::from_register(*status_result);

    // Read temperature if available
    if (auto temp_result = read_register(RegisterAddress::TEMP_SENSOR)) {
        status.temperature = *temp_result;
    }

    // Read supply voltage if available
    if (auto vsup_result = read_register(RegisterAddress::VSUP_MON)) {
        status.supply_voltage = *vsup_result;
    }

    return status;
}

DriverResult<ChannelStatus> Driver::get_channel_status(Channel channel) noexcept {
    if (auto result = check_initialized(); !result) {
        return unexpected(result.error());
    }

    if (!is_valid_channel(channel)) {
        return unexpected(DriverError::InvalidChannel);
    }

    uint8_t addr = channel_to_status_addr(channel);
    auto status_result = read_register(addr);
    if (!status_result) {
        return unexpected(status_result.error());
    }

    ChannelStatus status = ChannelStatus::from_register(*status_result);

    // Try to read load current from diagnostics
    if (auto diag_result = read_register(channel_to_diag_addr(channel))) {
        status.load_current = CH_DIAG::get_load_current(*diag_result);
    }

    return status;
}

DriverResult<ChannelDiagnostics> Driver::get_channel_diagnostics(Channel channel) noexcept {
    if (auto result = check_initialized(); !result) {
        return unexpected(result.error());
    }

    if (!is_valid_channel(channel)) {
        return unexpected(DriverError::InvalidChannel);
    }

    uint8_t addr = channel_to_diag_addr(channel);
    auto diag_result = read_register(addr);
    if (!diag_result) {
        return unexpected(diag_result.error());
    }

    return ChannelDiagnostics::from_register(*diag_result);
}

DriverResult<std::array<ChannelStatus, 6>> Driver::get_all_channels_status() noexcept {
    if (auto result = check_initialized(); !result) {
        return unexpected(result.error());
    }

    std::array<ChannelStatus, 6> statuses;

    for (uint8_t i = 0; i < 6; ++i) {
        auto status_result = get_channel_status(static_cast<Channel>(i));
        if (!status_result) {
            return unexpected(status_result.error());
        }
        statuses[i] = *status_result;
    }

    return statuses;
}

DriverResult<uint8_t> Driver::get_temperature() noexcept {
    if (auto result = check_initialized(); !result) {
        return unexpected(result.error());
    }

    return read_register(RegisterAddress::TEMP_SENSOR);
}

DriverResult<uint16_t> Driver::get_supply_voltage() noexcept {
    if (auto result = check_initialized(); !result) {
        return unexpected(result.error());
    }

    auto vsup_result = read_register(RegisterAddress::VSUP_MON);
    if (!vsup_result) {
        return unexpected(vsup_result.error());
    }

    // Convert to 16-bit value (implementation dependent on device)
    return static_cast<uint16_t>(*vsup_result);
}

//==============================================================================
// FAULT MANAGEMENT
//==============================================================================

DriverResult<void> Driver::clear_faults() noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    // Write to status registers to clear latched faults
    // (typical clear-on-write behavior)
    if (auto result = write_register(RegisterAddress::STATUS1, 0xFF); !result) {
        return unexpected(result.error());
    }

    if (auto result = write_register(RegisterAddress::STATUS2, 0xFF); !result) {
        return unexpected(result.error());
    }

    if (auto result = write_register(RegisterAddress::FAULT_STATUS, 0xFF); !result) {
        return unexpected(result.error());
    }

    return {};
}

DriverResult<bool> Driver::has_any_fault() noexcept {
    auto status_result = get_device_status();
    if (!status_result) {
        return unexpected(status_result.error());
    }

    if (status_result->any_fault) {
        return true;
    }

    // Also check individual channels
    for (uint8_t i = 0; i < 6; ++i) {
        auto ch_status = get_channel_status(static_cast<Channel>(i));
        if (ch_status && ch_status->has_fault) {
            return true;
        }
    }

    return false;
}

DriverResult<void> Driver::software_reset() noexcept {
    if (!hal_.is_ready()) {
        return unexpected(DriverError::HardwareError);
    }

    // Write reset bit to CTRL1
    if (auto result = write_register(RegisterAddress::CTRL1, CTRL1::RESET); !result) {
        return unexpected(result.error());
    }

    // Wait for reset to complete
    if (auto result = hal_.delay(5ms); !result) {
        return unexpected(DriverError::HardwareError);
    }

    // Mark as uninitialized - requires re-initialization
    initialized_ = false;

    return {};
}

//==============================================================================
// ADVANCED FEATURES
//==============================================================================

DriverResult<void> Driver::enter_sleep_mode() noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    // Set sleep bit in CTRL1
    return modify_register(RegisterAddress::CTRL1, CTRL1::SLEEP, CTRL1::SLEEP);
}

DriverResult<void> Driver::exit_sleep_mode() noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    // Clear sleep bit in CTRL1
    return modify_register(RegisterAddress::CTRL1, CTRL1::SLEEP, 0);
}

DriverResult<void> Driver::set_pwm_frequency(PWMFrequency frequency) noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    uint8_t freq_value = static_cast<uint8_t>(frequency) << 5;
    return modify_register(RegisterAddress::PWM_CTRL, PWM_CTRL::FREQ_MASK, freq_value);
}

DriverResult<bool> Driver::verify_device_id() noexcept {
    auto id_result = read_register(RegisterAddress::DEVICE_ID);
    if (!id_result) {
        return unexpected(id_result.error());
    }

    return (*id_result == DeviceID::TLE92466ED);
}

//==============================================================================
// REGISTER ACCESS
//==============================================================================

DriverResult<uint8_t> Driver::read_register(uint8_t address) noexcept {
    if (!hal_.is_ready()) {
        return unexpected(DriverError::HardwareError);
    }

    // Create read frame
    SPIFrame frame = SPIFrame::make_read(address);

    // Transfer via HAL
    auto result = transfer_checked(frame.word);
    if (!result) {
        return unexpected(result.error());
    }

    // Extract data from response
    SPIFrame response;
    response.word = *result;

    return response.fields.data;
}

DriverResult<void> Driver::write_register(uint8_t address, uint8_t value) noexcept {
    if (!hal_.is_ready()) {
        return unexpected(DriverError::HardwareError);
    }

    // Create write frame
    SPIFrame frame = SPIFrame::make_write(address, value);

    // Transfer via HAL
    auto result = transfer_checked(frame.word);
    if (!result) {
        return unexpected(result.error());
    }

    return {};
}

DriverResult<void> Driver::modify_register(
    uint8_t address,
    uint8_t mask,
    uint8_t value) noexcept {

    // Read current value
    auto read_result = read_register(address);
    if (!read_result) {
        return unexpected(read_result.error());
    }

    // Modify bits
    uint8_t new_value = (*read_result & ~mask) | (value & mask);

    // Write back
    return write_register(address, new_value);
}

//==============================================================================
// PRIVATE METHODS
//==============================================================================

DriverResult<uint16_t> Driver::transfer_checked(uint16_t frame) noexcept {
    auto result = hal_.transfer(frame);

    if (!result) {
        // Map HAL error to driver error
        switch (result.error()) {
            case HALError::Timeout:
                return unexpected(DriverError::TimeoutError);
            case HALError::TransferError:
            case HALError::BusError:
                return unexpected(DriverError::HardwareError);
            default:
                return unexpected(DriverError::RegisterError);
        }
    }

    return *result;
}

} // namespace TLE92466ED
