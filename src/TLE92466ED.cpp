/**
 * @file TLE92466ED.cpp
 * @brief Implementation of TLE92466ED driver class
 
 * @details
 * This file contains the complete implementation of all driver methods
 * for the TLE92466ED IC. All methods include comprehensive error checking,
 * CRC calculation/verification, and validation.
 *
 * @copyright
 * This is free and unencumbered software released into the public domain.
 */

#include "TLE92466ED.hpp"

namespace TLE92466ED {

//==============================================================================
// INITIALIZATION
//==============================================================================

DriverResult<void> Driver::init() noexcept {
    // 1. Initialize HAL
    if (auto result = hal_.init(); !result) {
        return std::unexpected(DriverError::HardwareError);
    }

    // 2. Wait for device power-up (minimum 1ms per datasheet)
    if (auto result = hal_.delay(2000); !result) {  // 2ms = 2000 microseconds
        return std::unexpected(DriverError::HardwareError);
    }

    // 3. Verify device communication by reading IC version
    auto verify_result = verify_device();
    if (!verify_result) {
        return std::unexpected(verify_result.error());
    }
    if (!*verify_result) {
        return std::unexpected(DriverError::WrongDeviceID);
    }

    // 4. Device starts in Config Mode after power-up
    mission_mode_ = false;

    // 5. Apply default configuration
    if (auto result = apply_default_config(); !result) {
        return std::unexpected(result.error());
    }

    // 6. Clear any power-on reset flags
    if (auto result = clear_faults(); !result) {
        return std::unexpected(result.error());
    }

    // 7. Initialize cached state
    channel_enable_cache_ = 0;
    channel_setpoints_.fill(0);

    initialized_ = true;
    return {};
}

DriverResult<void> Driver::apply_default_config() noexcept {
    // Configure GLOBAL_CONFIG: Enable CRC, SPI watchdog, clock watchdog, 3.3V VIO
    uint16_t global_cfg = GLOBAL_CONFIG::CRC_EN | 
                          GLOBAL_CONFIG::SPI_WD_EN | 
                          GLOBAL_CONFIG::CLK_WD_EN;
    
    if (auto result = write_register(CentralReg::GLOBAL_CONFIG, global_cfg, false); !result) {
        return std::unexpected(result.error());
    }

    // Set default VBAT thresholds (UV=7V, OV=40V approximately)
    // UV threshold: 7V / 0.16208V = ~43 (0x2B)
    // OV threshold: 40V / 0.16208V = ~247 (0xF7)
    uint16_t vbat_th = (0xF7 << 8) | 0x2B;
    if (auto result = write_register(CentralReg::VBAT_TH, vbat_th, false); !result) {
        return std::unexpected(result.error());
    }

    // Configure all channels with default settings (ICC mode, 1V/us slew, disabled)
    for (uint8_t ch = 0; ch < static_cast<uint8_t>(Channel::COUNT); ++ch) {
        Channel channel = static_cast<Channel>(ch);
        uint16_t ch_base = get_channel_base(channel);
        
        // Set mode to ICC (0x0001)
        if (auto result = write_register(ch_base + ChannelReg::MODE, 
                                        static_cast<uint16_t>(ChannelMode::ICC), false); !result) {
            return std::unexpected(result.error());
        }
        
        // Set default CH_CONFIG (slew rate 2.5V/us, OL disabled)
        if (auto result = write_register(ch_base + ChannelReg::CH_CONFIG, 
                                        CH_CONFIG::SLEWR_2V5_US, false); !result) {
            return std::unexpected(result.error());
        }
        
        // Set current setpoint to 0
        if (auto result = write_register(ch_base + ChannelReg::SETPOINT, 0, false); !result) {
            return std::unexpected(result.error());
        }
    }

    // Reload SPI watchdog
    if (auto result = write_register(CentralReg::WD_RELOAD, 1000, false); !result) {
        return std::unexpected(result.error());
    }

    return {};
}

//==========================================================================
// MODE CONTROL
//==========================================================================

DriverResult<void> Driver::enter_mission_mode() noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    // Set OP_MODE bit in CH_CTRL register
    if (auto result = modify_register(CentralReg::CH_CTRL, 
                                      CH_CTRL::OP_MODE, 
                                      CH_CTRL::OP_MODE); !result) {
        return std::unexpected(result.error());
    }

    mission_mode_ = true;
    return {};
}

DriverResult<void> Driver::enter_config_mode() noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    // Clear OP_MODE bit in CH_CTRL register
    if (auto result = modify_register(CentralReg::CH_CTRL, 
                                      CH_CTRL::OP_MODE, 
                                      0); !result) {
        return std::unexpected(result.error());
    }

    mission_mode_ = false;
    return {};
}

//==========================================================================
// GLOBAL CONFIGURATION
//==========================================================================

DriverResult<void> Driver::configure_global(const GlobalConfig& config) noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    // Must be in config mode to change global configuration
    if (auto result = check_config_mode(); !result) {
        return result;
    }

    // Build GLOBAL_CONFIG register value
    uint16_t global_cfg = 0;
    if (config.clock_watchdog_enabled) global_cfg |= GLOBAL_CONFIG::CLK_WD_EN;
    if (config.spi_watchdog_enabled) global_cfg |= GLOBAL_CONFIG::SPI_WD_EN;
    if (config.crc_enabled) global_cfg |= GLOBAL_CONFIG::CRC_EN;
    if (config.vio_5v) global_cfg |= GLOBAL_CONFIG::VIO_SEL;

    if (auto result = write_register(CentralReg::GLOBAL_CONFIG, global_cfg); !result) {
        return std::unexpected(result.error());
    }

    // Configure VBAT thresholds
    if (auto result = set_vbat_thresholds(config.vbat_uv_threshold, 
                                          config.vbat_ov_threshold); !result) {
        return std::unexpected(result.error());
    }

    // Configure SPI watchdog reload
    if (config.spi_watchdog_enabled) {
        if (auto result = write_register(CentralReg::WD_RELOAD, 
                                        config.spi_watchdog_reload); !result) {
            return std::unexpected(result.error());
        }
    }

    return {};
}

DriverResult<void> Driver::set_crc_enabled(bool enabled) noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    return modify_register(CentralReg::GLOBAL_CONFIG,
                          GLOBAL_CONFIG::CRC_EN,
                          enabled ? GLOBAL_CONFIG::CRC_EN : 0);
}

DriverResult<void> Driver::set_vbat_thresholds(uint8_t uv_threshold, uint8_t ov_threshold) noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    uint16_t value = (static_cast<uint16_t>(ov_threshold) << 8) | uv_threshold;
    return write_register(CentralReg::VBAT_TH, value);
}

//==========================================================================
// CHANNEL CONTROL
//==========================================================================

DriverResult<void> Driver::enable_channel(Channel channel, bool enabled) noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    // Channel enable can only be changed in Mission Mode
    if (auto result = check_mission_mode(); !result) {
        return result;
    }

    if (!is_valid_channel_internal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    uint16_t mask = CH_CTRL::channel_mask(to_index(channel));

    if (enabled) {
        channel_enable_cache_ |= mask;
    } else {
        channel_enable_cache_ &= ~mask;
    }

    return modify_register(CentralReg::CH_CTRL, mask, enabled ? mask : 0);
}

DriverResult<void> Driver::enable_channels(uint8_t channel_mask) noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    if (auto result = check_mission_mode(); !result) {
        return result;
    }

    // Mask to valid channels only (bits 0-5)
    channel_mask &= CH_CTRL::ALL_CH_MASK;
    channel_enable_cache_ = channel_mask;

    return modify_register(CentralReg::CH_CTRL, CH_CTRL::ALL_CH_MASK, channel_mask);
}

DriverResult<void> Driver::enable_all_channels() noexcept {
    return enable_channels(CH_CTRL::ALL_CH_MASK);
}

DriverResult<void> Driver::disable_all_channels() noexcept {
    return enable_channels(0);
}

DriverResult<void> Driver::set_channel_mode(Channel channel, ChannelMode mode) noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    // Mode can only be changed in Config Mode
    if (auto result = check_config_mode(); !result) {
        return result;
    }

    if (!is_valid_channel_internal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    uint16_t ch_addr = get_channel_register(channel, ChannelReg::MODE);
    return write_register(ch_addr, static_cast<uint16_t>(mode));
}

DriverResult<void> Driver::set_parallel_operation(ParallelPair pair, bool enabled) noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    // Parallel operation can only be changed in Config Mode
    if (auto result = check_config_mode(); !result) {
        return result;
    }

    uint16_t mask = 0;
    switch (pair) {
        case ParallelPair::CH0_CH3:
            mask = CH_CTRL::CH_PAR_0_3;
            break;
        case ParallelPair::CH1_CH2:
            mask = CH_CTRL::CH_PAR_1_2;
            break;
        case ParallelPair::CH4_CH5:
            mask = CH_CTRL::CH_PAR_4_5;
            break;
        default:
            return std::unexpected(DriverError::InvalidParameter);
    }

    return modify_register(CentralReg::CH_CTRL, mask, enabled ? mask : 0);
}

//==========================================================================
// CURRENT CONTROL
//==========================================================================

DriverResult<void> Driver::set_current_setpoint(
    Channel channel,
    uint16_t current_ma,
    bool parallel_mode) noexcept {

    if (auto result = check_initialized(); !result) {
        return result;
    }

    if (!is_valid_channel_internal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    // Validate current range (using absolute register scale)
    // Note: Datasheet typical continuous limits are ~1.5A single, ~2.7A parallel
    // but register scale allows up to 2A/4A for transient operation
    uint16_t max_current = parallel_mode ? 4000 : 2000;
    if (current_ma > max_current) {
        return std::unexpected(DriverError::InvalidParameter);
    }

    // Calculate setpoint register value
    uint16_t target = SETPOINT::calculate_target(current_ma, parallel_mode);
    
    // Cache the setpoint
    channel_setpoints_[to_index(channel)] = target;

    // Write to SETPOINT register
    uint16_t ch_addr = get_channel_register(channel, ChannelReg::SETPOINT);
    return write_register(ch_addr, target);
}

DriverResult<uint16_t> Driver::get_current_setpoint(
    Channel channel,
    bool parallel_mode) noexcept {

    if (auto result = check_initialized(); !result) {
        return std::unexpected(result.error());
    }

    if (!is_valid_channel_internal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    // Read SETPOINT register
    uint16_t ch_addr = get_channel_register(channel, ChannelReg::SETPOINT);
    auto result = read_register(ch_addr);
    if (!result) {
        return std::unexpected(result.error());
    }

    // Convert to current in mA
    uint16_t target = *result & SETPOINT::TARGET_MASK;
    uint16_t current_ma = SETPOINT::calculate_current(target, parallel_mode);

    return current_ma;
}

DriverResult<void> Driver::configure_pwm_period(
    Channel channel,
    uint8_t period_mantissa,
    uint8_t period_exponent,
    bool low_freq_range) noexcept {

    if (auto result = check_initialized(); !result) {
        return result;
    }

    if (!is_valid_channel_internal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    // Build PERIOD register value
    uint16_t value = period_mantissa | 
                    ((period_exponent & 0x07) << 8) |
                    (low_freq_range ? (1 << 11) : 0);

    uint16_t ch_addr = get_channel_register(channel, ChannelReg::PERIOD);
    return write_register(ch_addr, value);
}

DriverResult<void> Driver::configure_dither(
    Channel channel,
    uint16_t step_size,
    uint8_t num_steps,
    uint8_t flat_steps) noexcept {

    if (auto result = check_initialized(); !result) {
        return result;
    }

    if (!is_valid_channel_internal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    uint16_t ch_base = get_channel_base(channel);

    // Configure DITHER_CTRL (step size)
    uint16_t ctrl_value = step_size & DITHER_CTRL::STEP_SIZE_MASK;
    if (auto result = write_register(ch_base + ChannelReg::DITHER_CTRL, ctrl_value); !result) {
        return std::unexpected(result.error());
    }

    // Configure DITHER_STEP (steps and flat period)
    uint16_t step_value = flat_steps | (static_cast<uint16_t>(num_steps) << DITHER_STEP::STEPS_SHIFT);
    if (auto result = write_register(ch_base + ChannelReg::DITHER_STEP, step_value); !result) {
        return std::unexpected(result.error());
    }

    return {};
}

DriverResult<void> Driver::configure_channel(
    Channel channel,
    const ChannelConfig& config) noexcept {

    if (auto result = check_initialized(); !result) {
        return result;
    }

    // Most configuration requires Config Mode
    if (auto result = check_config_mode(); !result) {
        return result;
    }

    if (!is_valid_channel_internal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    uint16_t ch_base = get_channel_base(channel);

    // 1. Set channel mode
    if (auto result = write_register(ch_base + ChannelReg::MODE, 
                                     static_cast<uint16_t>(config.mode)); !result) {
        return std::unexpected(result.error());
    }

    // 2. Set current setpoint with parallel mode detection
    auto parallel_result = is_channel_parallel(channel);
    bool is_parallel = parallel_result.value_or(false); // Default to false if can't determine
    uint16_t target = SETPOINT::calculate_target(config.current_setpoint_ma, is_parallel);
    if (config.auto_limit_disabled) {
        target |= SETPOINT::AUTO_LIMIT_DIS;
    }
    if (auto result = write_register(ch_base + ChannelReg::SETPOINT, target); !result) {
        return std::unexpected(result.error());
    }

    // 3. Configure CH_CONFIG register
    uint16_t ch_cfg = static_cast<uint16_t>(config.slew_rate) |
                     (static_cast<uint16_t>(config.diag_current) << 2) |
                     (static_cast<uint16_t>(config.open_load_threshold & 0x07) << 4);
    
    if (auto result = write_register(ch_base + ChannelReg::CH_CONFIG, ch_cfg); !result) {
        return std::unexpected(result.error());
    }

    // 3a. Configure OLSG warning enable if requested (bit 14 of CTRL register)
    if (config.olsg_warning_enabled) {
        if (auto result = modify_register(ch_base + ChannelReg::CTRL, 
                                         CH_CTRL_REG::OLSG_WARN_EN,
                                         CH_CTRL_REG::OLSG_WARN_EN); !result) {
            return std::unexpected(result.error());
        }
    }

    // 4. Configure PWM if specified
    if (config.pwm_period_mantissa > 0) {
        if (auto result = configure_pwm_period(channel, 
                                              config.pwm_period_mantissa,
                                              config.pwm_period_exponent,
                                              false); !result) {
            return std::unexpected(result.error());
        }
    }

    // 5. Configure dither if specified
    if (config.dither_step_size > 0) {
        if (auto result = configure_dither(channel,
                                          config.dither_step_size,
                                          config.dither_steps,
                                          config.dither_flat); !result) {
            return std::unexpected(result.error());
        }
        
        // 5a. Enable deep dither if requested (bit 13 of DITHER_CTRL)
        if (config.deep_dither_enabled) {
            if (auto result = modify_register(ch_base + ChannelReg::DITHER_CTRL,
                                             DITHER_CTRL::DEEP_DITHER,
                                             DITHER_CTRL::DEEP_DITHER); !result) {
                return std::unexpected(result.error());
            }
        }
    }

    return {};
}

//==========================================================================
// STATUS AND DIAGNOSTICS
//==========================================================================

DriverResult<DeviceStatus> Driver::get_device_status() noexcept {
    if (auto result = check_initialized(); !result) {
        return std::unexpected(result.error());
    }

    DeviceStatus status{};

    // Read GLOBAL_DIAG0
    auto diag0_result = read_register(CentralReg::GLOBAL_DIAG0);
    if (!diag0_result) {
        return std::unexpected(diag0_result.error());
    }

    uint16_t diag0 = *diag0_result;
    status.vbat_uv = (diag0 & GLOBAL_DIAG0::VBAT_UV) != 0;
    status.vbat_ov = (diag0 & GLOBAL_DIAG0::VBAT_OV) != 0;
    status.vio_uv = (diag0 & GLOBAL_DIAG0::VIO_UV) != 0;
    status.vio_ov = (diag0 & GLOBAL_DIAG0::VIO_OV) != 0;
    status.vdd_uv = (diag0 & GLOBAL_DIAG0::VDD_UV) != 0;
    status.vdd_ov = (diag0 & GLOBAL_DIAG0::VDD_OV) != 0;
    status.clock_fault = (diag0 & GLOBAL_DIAG0::CLK_NOK) != 0;
    status.ot_error = (diag0 & GLOBAL_DIAG0::COTERR) != 0;
    status.ot_warning = (diag0 & GLOBAL_DIAG0::COTWARN) != 0;
    status.reset_event = (diag0 & GLOBAL_DIAG0::RES_EVENT) != 0;
    status.por_event = (diag0 & GLOBAL_DIAG0::POR_EVENT) != 0;
    status.spi_wd_error = (diag0 & GLOBAL_DIAG0::SPI_WD_ERR) != 0;

    status.any_fault = (diag0 & GLOBAL_DIAG0::FAULT_MASK) != 0;

    // Read FB_STAT for additional status
    auto fb_stat_result = read_register(CentralReg::FB_STAT);
    if (fb_stat_result) {
        uint16_t fb_stat = *fb_stat_result;
        status.supply_nok_internal = (fb_stat & FB_STAT::SUP_NOK_INT) != 0;
        status.supply_nok_external = (fb_stat & FB_STAT::SUP_NOK_EXT) != 0;
        status.init_done = (fb_stat & FB_STAT::INIT_DONE) != 0;
    }

    // Read CH_CTRL to get mode
    auto ch_ctrl_result = read_register(CentralReg::CH_CTRL);
    if (ch_ctrl_result) {
        status.config_mode = (*ch_ctrl_result & CH_CTRL::OP_MODE) == 0;
    }

    // Read voltage feedbacks
    auto fb_voltage1_result = read_register(CentralReg::FB_VOLTAGE1);
    if (fb_voltage1_result) {
        status.vbat_voltage = *fb_voltage1_result;
    }

    auto fb_voltage2_result = read_register(CentralReg::FB_VOLTAGE2);
    if (fb_voltage2_result) {
        status.vio_voltage = *fb_voltage2_result;
    }

    return status;
}

DriverResult<ChannelDiagnostics> Driver::get_channel_diagnostics(Channel channel) noexcept {
    if (auto result = check_initialized(); !result) {
        return std::unexpected(result.error());
    }

    if (!is_valid_channel_internal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    ChannelDiagnostics diag{};

    // Read DIAG_ERR register for this channel group
    auto diag_err_result = read_register(CentralReg::DIAG_ERR_CHGR0 + to_index(channel));
    if (diag_err_result) {
        uint16_t diag_err = *diag_err_result;
        // Parse error flags (bit positions from datasheet Table in page 67)
        diag.overcurrent = (diag_err & (1 << 0)) != 0;         // OC bit
        diag.short_to_ground = (diag_err & (1 << 1)) != 0;     // SG bit
        diag.open_load = (diag_err & (1 << 2)) != 0;           // OL bit
        diag.over_temperature = (diag_err & (1 << 3)) != 0;    // OTE bit
        diag.open_load_short_ground = (diag_err & (1 << 4)) != 0; // OLSG bit
    }

    // Read DIAG_WARN register for warnings
    auto diag_warn_result = read_register(CentralReg::DIAG_WARN_CHGR0 + to_index(channel));
    if (diag_warn_result) {
        uint16_t diag_warn = *diag_warn_result;
        diag.ot_warning = (diag_warn & (1 << 0)) != 0;
        diag.current_regulation_warning = (diag_warn & (1 << 1)) != 0;
        diag.pwm_regulation_warning = (diag_warn & (1 << 2)) != 0;
        diag.olsg_warning = (diag_warn & (1 << 3)) != 0;
    }

    // Read feedback values
    uint16_t ch_base = get_channel_base(channel);
    
    auto fb_i_avg_result = read_register(ch_base + ChannelReg::FB_I_AVG);
    if (fb_i_avg_result) {
        diag.average_current = *fb_i_avg_result;
    }

    auto fb_dc_result = read_register(ch_base + ChannelReg::FB_DC);
    if (fb_dc_result) {
        diag.duty_cycle = *fb_dc_result;
    }

    auto fb_vbat_result = read_register(ch_base + ChannelReg::FB_VBAT);
    if (fb_vbat_result) {
        diag.vbat_feedback = *fb_vbat_result;
    }

    // Read min/max current feedback (FB_IMIN_IMAX register)
    // Register format: [15:8] = I_MAX, [7:0] = I_MIN
    auto fb_minmax_result = read_register(ch_base + ChannelReg::FB_IMIN_IMAX);
    if (fb_minmax_result) {
        uint16_t minmax = *fb_minmax_result;
        diag.min_current = minmax & 0x00FF;        // Lower 8 bits
        diag.max_current = (minmax >> 8) & 0x00FF; // Upper 8 bits
    }

    return diag;
}

DriverResult<uint16_t> Driver::get_average_current(Channel channel, bool parallel_mode) noexcept {
    if (auto result = check_initialized(); !result) {
        return std::unexpected(result.error());
    }

    if (!is_valid_channel_internal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    uint16_t ch_addr = get_channel_register(channel, ChannelReg::FB_I_AVG);
    auto result = read_register(ch_addr);
    if (!result) {
        return std::unexpected(result.error());
    }

    // Convert raw value to mA
    // Based on datasheet: similar calculation to setpoint
    uint16_t current_ma = SETPOINT::calculate_current(*result, parallel_mode);
    return current_ma;
}

DriverResult<uint16_t> Driver::get_duty_cycle(Channel channel) noexcept {
    if (auto result = check_initialized(); !result) {
        return std::unexpected(result.error());
    }

    if (!is_valid_channel_internal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    uint16_t ch_addr = get_channel_register(channel, ChannelReg::FB_DC);
    return read_register(ch_addr);
}

DriverResult<uint16_t> Driver::get_vbat_voltage() noexcept {
    if (auto result = check_initialized(); !result) {
        return std::unexpected(result.error());
    }

    auto result = read_register(CentralReg::FB_VOLTAGE1);
    if (!result) {
        return std::unexpected(result.error());
    }

    // Convert to millivolts (formula from datasheet)
    // V_BAT measurement encoding needs datasheet formula
    return *result; // Return raw value for now
}

DriverResult<uint16_t> Driver::get_vio_voltage() noexcept {
    if (auto result = check_initialized(); !result) {
        return std::unexpected(result.error());
    }

    auto result = read_register(CentralReg::FB_VOLTAGE2);
    if (!result) {
        return std::unexpected(result.error());
    }

    return *result; // Return raw value
}

//==========================================================================
// FAULT MANAGEMENT
//==========================================================================

DriverResult<void> Driver::clear_faults() noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    // Write 1s to clear fault bits in GLOBAL_DIAG0 (rwh type - clear on write 1)
    if (auto result = write_register(CentralReg::GLOBAL_DIAG0, 0xFFFF); !result) {
        return std::unexpected(result.error());
    }

    // Clear GLOBAL_DIAG1
    if (auto result = write_register(CentralReg::GLOBAL_DIAG1, 0xFFFF); !result) {
        return std::unexpected(result.error());
    }

    // Clear GLOBAL_DIAG2
    if (auto result = write_register(CentralReg::GLOBAL_DIAG2, 0xFFFF); !result) {
        return std::unexpected(result.error());
    }

    return {};
}

DriverResult<bool> Driver::has_any_fault() noexcept {
    auto status_result = get_device_status();
    if (!status_result) {
        return std::unexpected(status_result.error());
    }

    return status_result->any_fault;
}

DriverResult<void> Driver::software_reset() noexcept {
    // Software reset would require toggling RESN pin or power cycle
    // This IC doesn't have a software reset register
    // Return to config mode and disable all channels instead
    if (auto result = enter_config_mode(); !result) {
        return result;
    }

    if (auto result = disable_all_channels(); !result) {
        return result;
    }

    return {};
}

//==========================================================================
// WATCHDOG MANAGEMENT
//==========================================================================

DriverResult<void> Driver::reload_spi_watchdog(uint16_t reload_value) noexcept {
    if (auto result = check_initialized(); !result) {
        return result;
    }

    return write_register(CentralReg::WD_RELOAD, reload_value);
}

//==========================================================================
// DEVICE INFORMATION
//==========================================================================

DriverResult<uint16_t> Driver::get_ic_version() noexcept {
    if (auto result = check_initialized(); !result) {
        return std::unexpected(result.error());
    }

    return read_register(CentralReg::ICVID);
}

DriverResult<std::array<uint16_t, 3>> Driver::get_chip_id() noexcept {
    if (auto result = check_initialized(); !result) {
        return std::unexpected(result.error());
    }

    std::array<uint16_t, 3> chip_id;

    auto id0_result = read_register(CentralReg::CHIPID0);
    if (!id0_result) {
        return std::unexpected(id0_result.error());
    }
    chip_id[0] = *id0_result;

    auto id1_result = read_register(CentralReg::CHIPID1);
    if (!id1_result) {
        return std::unexpected(id1_result.error());
    }
    chip_id[1] = *id1_result;

    auto id2_result = read_register(CentralReg::CHIPID2);
    if (!id2_result) {
        return std::unexpected(id2_result.error());
    }
    chip_id[2] = *id2_result;

    return chip_id;
}

DriverResult<bool> Driver::verify_device() noexcept {
    // Read ICVID register to verify device is responding and check device type
    auto id_result = read_register(CentralReg::ICVID, false); // Don't verify CRC during init
    if (!id_result) {
        return std::unexpected(id_result.error());
    }

    uint16_t icvid = *id_result;
    
    // Validate device ID (checks for valid response: not 0x0000 or 0xFFFF)
    bool valid = DeviceID::is_valid_device(icvid);
    
    // Log device information for debugging (would require logger interface)
    // uint8_t device_type = DeviceID::get_device_type(icvid);
    // uint8_t revision = DeviceID::get_revision(icvid);
    
    return valid;
}

//==========================================================================
// REGISTER ACCESS
//==========================================================================

DriverResult<uint16_t> Driver::read_register(uint16_t address, bool verify_crc) noexcept {
    if (!hal_.is_ready()) {
        return std::unexpected(DriverError::HardwareError);
    }

    // Create read frame
    SPIFrame tx_frame = SPIFrame::make_read(address);
    
    // Calculate and set CRC
    tx_frame.tx_fields.crc = calculate_frame_crc(tx_frame);

    // Transfer frame
    auto rx_result = transfer_frame(tx_frame, verify_crc);
    if (!rx_result) {
        return std::unexpected(rx_result.error());
    }

    // Extract data from response (copy from bit-field to avoid reference issue)
    uint16_t data = rx_result->rx_fields.data;
    return data;
}

DriverResult<void> Driver::write_register(uint16_t address, uint16_t value, bool verify_crc) noexcept {
    if (!hal_.is_ready()) {
        return std::unexpected(DriverError::HardwareError);
    }

    // Create write frame
    SPIFrame tx_frame = SPIFrame::make_write(address, value);
    
    // Calculate and set CRC
    tx_frame.tx_fields.crc = calculate_frame_crc(tx_frame);

    // Transfer frame
    auto rx_result = transfer_frame(tx_frame, verify_crc);
    if (!rx_result) {
        return std::unexpected(rx_result.error());
    }

    return {};
}

DriverResult<void> Driver::modify_register(
    uint16_t address,
    uint16_t mask,
    uint16_t value) noexcept {

    // Read current value
    auto read_result = read_register(address);
    if (!read_result) {
        return std::unexpected(read_result.error());
    }

    // Modify bits
    uint16_t new_value = (*read_result & ~mask) | (value & mask);

    // Write back
    return write_register(address, new_value);
}

//==========================================================================
// PRIVATE METHODS
//==========================================================================

DriverResult<SPIFrame> Driver::transfer_frame(const SPIFrame& tx_frame, bool verify_crc) noexcept {
    // Transfer 32-bit frame via HAL
    auto hal_result = hal_.transfer32(tx_frame.word);
    if (!hal_result) {
        // Map HAL error to driver error
        switch (hal_result.error()) {
            case HALError::Timeout:
                return std::unexpected(DriverError::TimeoutError);
            case HALError::CRCError:
                return std::unexpected(DriverError::CRCError);
            case HALError::TransferError:
            case HALError::BusError:
                return std::unexpected(DriverError::HardwareError);
            default:
                return std::unexpected(DriverError::RegisterError);
        }
    }

    SPIFrame rx_frame;
    rx_frame.word = *hal_result;

    // Verify CRC if requested
    if (verify_crc) {
        if (!verify_frame_crc(rx_frame)) {
            return std::unexpected(DriverError::CRCError);
        }
    }

    // Check SPI status in reply
    if (auto result = check_spi_status(rx_frame); !result) {
        return std::unexpected(result.error());
    }

    return rx_frame;
}

DriverResult<void> Driver::check_spi_status(const SPIFrame& rx_frame) noexcept {
    SPIStatus status = static_cast<SPIStatus>(rx_frame.rx_fields.status);

    switch (status) {
        case SPIStatus::NO_ERROR:
            return {};
        case SPIStatus::SPI_FRAME_ERROR:
            return std::unexpected(DriverError::SPIFrameError);
        case SPIStatus::CRC_ERROR:
            return std::unexpected(DriverError::CRCError);
        case SPIStatus::WRITE_RO_REG:
            return std::unexpected(DriverError::WriteToReadOnly);
        case SPIStatus::INTERNAL_BUS_FAULT:
            return std::unexpected(DriverError::RegisterError);
        default:
            return std::unexpected(DriverError::RegisterError);
    }
}

DriverResult<bool> Driver::is_channel_parallel(Channel channel) noexcept {
    if (auto result = check_initialized(); !result) {
        return std::unexpected(result.error());
    }

    if (!is_valid_channel_internal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    // Read CH_CTRL to check parallel configuration bits
    auto ctrl_result = read_register(CentralReg::CH_CTRL);
    if (!ctrl_result) {
        return std::unexpected(ctrl_result.error());
    }

    uint16_t ch_ctrl = *ctrl_result;
    uint8_t ch_index = to_index(channel);

    // Check which parallel pair this channel belongs to
    switch (ch_index) {
        case 0:
        case 3:
            return (ch_ctrl & CH_CTRL::CH_PAR_0_3) != 0;
        case 1:
        case 2:
            return (ch_ctrl & CH_CTRL::CH_PAR_1_2) != 0;
        case 4:
        case 5:
            return (ch_ctrl & CH_CTRL::CH_PAR_4_5) != 0;
        default:
            return false;
    }
}

} // namespace TLE92466ED
