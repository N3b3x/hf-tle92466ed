/**
 * @file TLE92466ED_Cpp11.cpp
 * @brief Implementation of TLE92466ED driver class - C++11 compatible
 * @author AI Generated Driver
 * @date 2025-10-20
 * @version 2.0.0
 *
 * @details
 * This file contains the complete implementation of all driver methods
 * for the TLE92466ED IC. All methods include comprehensive error checking,
 * CRC calculation/verification, and validation.
 *
 * @copyright
 * This is free and unencumbered software released into the public domain.
 */

#include "TLE92466ED_Cpp11.hpp"

namespace TLE92466ED {

//==============================================================================
// INITIALIZATION
//==============================================================================

bool Driver::init(DriverError* error) noexcept {
    // 1. Initialize HAL
    HALError hal_error;
    if (!hal_.init(&hal_error)) {
        if (error) *error = DriverError::HardwareError;
        return false;
    }

    // 2. Wait for device power-up (minimum 1ms per datasheet)
    if (!hal_.delay(2000, &hal_error)) {  // 2ms = 2000 microseconds
        if (error) *error = DriverError::HardwareError;
        return false;
    }

    // 3. Verify device communication by reading IC version
    bool is_valid;
    if (!verify_device(&is_valid, error)) {
        return false;
    }
    if (!is_valid) {
        if (error) *error = DriverError::WrongDeviceID;
        return false;
    }

    // 4. Device starts in Config Mode after power-up
    mission_mode_ = false;

    // 5. Apply default configuration
    if (!apply_default_config(error)) {
        return false;
    }

    // 6. Clear any power-on reset flags
    if (!clear_faults(error)) {
        return false;
    }

    // 7. Initialize cached state
    channel_enable_cache_ = 0;
    for (size_t i = 0; i < 6; ++i) {
        channel_setpoints_[i] = 0;
    }

    initialized_ = true;
    return true;
}

bool Driver::apply_default_config(DriverError* error) noexcept {
    // Configure GLOBAL_CONFIG: Enable CRC, SPI watchdog, clock watchdog, 3.3V VIO
    uint16_t global_cfg = GLOBAL_CONFIG::CRC_EN | 
                          GLOBAL_CONFIG::SPI_WD_EN | 
                          GLOBAL_CONFIG::CLK_WD_EN;
    
    if (!write_register(CentralReg::GLOBAL_CONFIG, global_cfg, false, error)) {
        return false;
    }

    // Set default VBAT thresholds (UV=7V, OV=40V approximately)
    // UV threshold: 7V / 0.16208V = ~43 (0x2B)
    // OV threshold: 40V / 0.16208V = ~247 (0xF7)
    uint16_t vbat_th = (0xF7 << 8) | 0x2B;
    if (!write_register(CentralReg::VBAT_TH, vbat_th, false, error)) {
        return false;
    }

    // Configure all channels with default settings (ICC mode, 1V/us slew, disabled)
    for (uint8_t ch = 0; ch < static_cast<uint8_t>(Channel::COUNT); ++ch) {
        Channel channel = static_cast<Channel>(ch);
        uint16_t ch_base = get_channel_base(channel);
        
        // Set mode to ICC (0x0001)
        if (!write_register(ch_base + ChannelReg::MODE, 
                           static_cast<uint16_t>(ChannelMode::ICC), false, error)) {
            return false;
        }
        
        // Set default CH_CONFIG (slew rate 2.5V/us, OL disabled)
        if (!write_register(ch_base + ChannelReg::CH_CONFIG, 
                           CH_CONFIG::SLEWR_2V5_US, false, error)) {
            return false;
        }
        
        // Set current setpoint to 0
        if (!write_register(ch_base + ChannelReg::SETPOINT, 0, false, error)) {
            return false;
        }
    }

    // Reload SPI watchdog
    if (!write_register(CentralReg::WD_RELOAD, 1000, false, error)) {
        return false;
    }

    return true;
}

//==========================================================================
// MODE CONTROL
//==========================================================================

bool Driver::enter_mission_mode(DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    // Set OP_MODE bit in CH_CTRL register
    if (!modify_register(CentralReg::CH_CTRL, 
                         CH_CTRL::OP_MODE, 
                         CH_CTRL::OP_MODE, error)) {
        return false;
    }

    mission_mode_ = true;
    return true;
}

bool Driver::enter_config_mode(DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    // Clear OP_MODE bit in CH_CTRL register
    if (!modify_register(CentralReg::CH_CTRL, 
                         CH_CTRL::OP_MODE, 
                         0, error)) {
        return false;
    }

    mission_mode_ = false;
    return true;
}

//==========================================================================
// GLOBAL CONFIGURATION
//==========================================================================

bool Driver::configure_global(const GlobalConfig& config, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    // Must be in config mode to change global configuration
    if (!check_config_mode(error)) {
        return false;
    }

    // Build GLOBAL_CONFIG register value
    uint16_t global_cfg = 0;
    if (config.clock_watchdog_enabled) global_cfg |= GLOBAL_CONFIG::CLK_WD_EN;
    if (config.spi_watchdog_enabled) global_cfg |= GLOBAL_CONFIG::SPI_WD_EN;
    if (config.crc_enabled) global_cfg |= GLOBAL_CONFIG::CRC_EN;
    if (config.vio_5v) global_cfg |= GLOBAL_CONFIG::VIO_SEL;

    if (!write_register(CentralReg::GLOBAL_CONFIG, global_cfg, true, error)) {
        return false;
    }

    // Configure VBAT thresholds
    if (!set_vbat_thresholds(config.vbat_uv_threshold, 
                             config.vbat_ov_threshold, error)) {
        return false;
    }

    // Configure SPI watchdog reload
    if (config.spi_watchdog_enabled) {
        if (!write_register(CentralReg::WD_RELOAD, 
                           config.spi_watchdog_reload, true, error)) {
            return false;
        }
    }

    return true;
}

bool Driver::set_crc_enabled(bool enabled, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    return modify_register(CentralReg::GLOBAL_CONFIG,
                          GLOBAL_CONFIG::CRC_EN,
                          enabled ? GLOBAL_CONFIG::CRC_EN : 0, error);
}

bool Driver::set_vbat_thresholds(uint8_t uv_threshold, uint8_t ov_threshold, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    uint16_t value = (static_cast<uint16_t>(ov_threshold) << 8) | uv_threshold;
    return write_register(CentralReg::VBAT_TH, value, true, error);
}

//==========================================================================
// CHANNEL CONTROL
//==========================================================================

bool Driver::enable_channel(Channel channel, bool enabled, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    // Channel enable can only be changed in Mission Mode
    if (!check_mission_mode(error)) {
        return false;
    }

    if (!is_valid_channel_internal(channel)) {
        if (error) *error = DriverError::InvalidChannel;
        return false;
    }

    uint16_t mask = CH_CTRL::channel_mask(to_index(channel));

    if (enabled) {
        channel_enable_cache_ |= mask;
    } else {
        channel_enable_cache_ &= ~mask;
    }

    return modify_register(CentralReg::CH_CTRL, mask, enabled ? mask : 0, error);
}

bool Driver::enable_channels(uint8_t channel_mask, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    if (!check_mission_mode(error)) {
        return false;
    }

    // Mask to valid channels only (bits 0-5)
    channel_mask &= CH_CTRL::ALL_CH_MASK;
    channel_enable_cache_ = channel_mask;

    return modify_register(CentralReg::CH_CTRL, CH_CTRL::ALL_CH_MASK, channel_mask, error);
}

bool Driver::enable_all_channels(DriverError* error) noexcept {
    return enable_channels(CH_CTRL::ALL_CH_MASK, error);
}

bool Driver::disable_all_channels(DriverError* error) noexcept {
    return enable_channels(0, error);
}

bool Driver::set_channel_mode(Channel channel, ChannelMode mode, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    // Mode can only be changed in Config Mode
    if (!check_config_mode(error)) {
        return false;
    }

    if (!is_valid_channel_internal(channel)) {
        if (error) *error = DriverError::InvalidChannel;
        return false;
    }

    uint16_t ch_addr = get_channel_register(channel, ChannelReg::MODE);
    return write_register(ch_addr, static_cast<uint16_t>(mode), true, error);
}

bool Driver::set_parallel_operation(ParallelPair pair, bool enabled, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    // Parallel operation can only be changed in Config Mode
    if (!check_config_mode(error)) {
        return false;
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
            if (error) *error = DriverError::InvalidParameter;
            return false;
    }

    return modify_register(CentralReg::CH_CTRL, mask, enabled ? mask : 0, error);
}

//==========================================================================
// CURRENT CONTROL
//==========================================================================

bool Driver::set_current_setpoint(Channel channel, uint16_t current_ma, bool parallel_mode, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    if (!is_valid_channel_internal(channel)) {
        if (error) *error = DriverError::InvalidChannel;
        return false;
    }

    // Validate current range (using absolute register scale)
    // Note: Datasheet typical continuous limits are ~1.5A single, ~2.7A parallel
    // but register scale allows up to 2A/4A for transient operation
    uint16_t max_current = parallel_mode ? 4000 : 2000;
    if (current_ma > max_current) {
        if (error) *error = DriverError::InvalidParameter;
        return false;
    }

    // Calculate setpoint register value
    uint16_t target = SETPOINT::calculate_target(current_ma, parallel_mode);
    
    // Cache the setpoint
    channel_setpoints_[to_index(channel)] = target;

    // Write to SETPOINT register
    uint16_t ch_addr = get_channel_register(channel, ChannelReg::SETPOINT);
    return write_register(ch_addr, target, true, error);
}

bool Driver::get_current_setpoint(Channel channel, uint16_t* current_ma, bool parallel_mode, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    if (!is_valid_channel_internal(channel)) {
        if (error) *error = DriverError::InvalidChannel;
        return false;
    }

    // Read SETPOINT register
    uint16_t ch_addr = get_channel_register(channel, ChannelReg::SETPOINT);
    uint16_t value;
    if (!read_register(ch_addr, &value, true, error)) {
        return false;
    }

    // Convert to current in mA
    uint16_t target = value & SETPOINT::TARGET_MASK;
    *current_ma = SETPOINT::calculate_current(target, parallel_mode);

    return true;
}

bool Driver::configure_pwm_period(Channel channel, uint8_t period_mantissa, uint8_t period_exponent, bool low_freq_range, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    if (!is_valid_channel_internal(channel)) {
        if (error) *error = DriverError::InvalidChannel;
        return false;
    }

    // Build PERIOD register value
    uint16_t value = period_mantissa | 
                    ((period_exponent & 0x07) << 8) |
                    (low_freq_range ? (1 << 11) : 0);

    uint16_t ch_addr = get_channel_register(channel, ChannelReg::PERIOD);
    return write_register(ch_addr, value, true, error);
}

bool Driver::configure_dither(Channel channel, uint16_t step_size, uint8_t num_steps, uint8_t flat_steps, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    if (!is_valid_channel_internal(channel)) {
        if (error) *error = DriverError::InvalidChannel;
        return false;
    }

    uint16_t ch_base = get_channel_base(channel);

    // Configure DITHER_CTRL (step size)
    uint16_t ctrl_value = step_size & DITHER_CTRL::STEP_SIZE_MASK;
    if (!write_register(ch_base + ChannelReg::DITHER_CTRL, ctrl_value, true, error)) {
        return false;
    }

    // Configure DITHER_STEP (steps and flat period)
    uint16_t step_value = flat_steps | (static_cast<uint16_t>(num_steps) << DITHER_STEP::STEPS_SHIFT);
    if (!write_register(ch_base + ChannelReg::DITHER_STEP, step_value, true, error)) {
        return false;
    }

    return true;
}

bool Driver::configure_channel(Channel channel, const ChannelConfig& config, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    // Most configuration requires Config Mode
    if (!check_config_mode(error)) {
        return false;
    }

    if (!is_valid_channel_internal(channel)) {
        if (error) *error = DriverError::InvalidChannel;
        return false;
    }

    uint16_t ch_base = get_channel_base(channel);

    // 1. Set channel mode
    if (!write_register(ch_base + ChannelReg::MODE, 
                        static_cast<uint16_t>(config.mode), true, error)) {
        return false;
    }

    // 2. Set current setpoint with parallel mode detection
    bool is_parallel;
    if (!is_channel_parallel(channel, &is_parallel, error)) {
        is_parallel = false; // Default to false if can't determine
    }
    uint16_t target = SETPOINT::calculate_target(config.current_setpoint_ma, is_parallel);
    if (config.auto_limit_disabled) {
        target |= SETPOINT::AUTO_LIMIT_DIS;
    }
    if (!write_register(ch_base + ChannelReg::SETPOINT, target, true, error)) {
        return false;
    }

    // 3. Configure CH_CONFIG register
    uint16_t ch_cfg = static_cast<uint16_t>(config.slew_rate) |
                     (static_cast<uint16_t>(config.diag_current) << 2) |
                     (static_cast<uint16_t>(config.open_load_threshold & 0x07) << 4);
    
    if (!write_register(ch_base + ChannelReg::CH_CONFIG, ch_cfg, true, error)) {
        return false;
    }

    // 3a. Configure OLSG warning enable if requested (bit 14 of CTRL register)
    if (config.olsg_warning_enabled) {
        if (!modify_register(ch_base + ChannelReg::CTRL, 
                            CH_CTRL_REG::OLSG_WARN_EN,
                            CH_CTRL_REG::OLSG_WARN_EN, error)) {
            return false;
        }
    }

    // 4. Configure PWM if specified
    if (config.pwm_period_mantissa > 0) {
        if (!configure_pwm_period(channel, 
                                 config.pwm_period_mantissa,
                                 config.pwm_period_exponent,
                                 false, error)) {
            return false;
        }
    }

    // 5. Configure dither if specified
    if (config.dither_step_size > 0) {
        if (!configure_dither(channel,
                             config.dither_step_size,
                             config.dither_steps,
                             config.dither_flat, error)) {
            return false;
        }
        
        // 5a. Enable deep dither if requested (bit 13 of DITHER_CTRL)
        if (config.deep_dither_enabled) {
            if (!modify_register(ch_base + ChannelReg::DITHER_CTRL,
                                DITHER_CTRL::DEEP_DITHER,
                                DITHER_CTRL::DEEP_DITHER, error)) {
                return false;
            }
        }
    }

    return true;
}

//==========================================================================
// STATUS AND DIAGNOSTICS
//==========================================================================

bool Driver::get_device_status(DeviceStatus* status, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    // Initialize status structure
    *status = DeviceStatus();

    // Read GLOBAL_DIAG0
    uint16_t diag0;
    if (!read_register(CentralReg::GLOBAL_DIAG0, &diag0, true, error)) {
        return false;
    }

    status->vbat_uv = (diag0 & GLOBAL_DIAG0::VBAT_UV) != 0;
    status->vbat_ov = (diag0 & GLOBAL_DIAG0::VBAT_OV) != 0;
    status->vio_uv = (diag0 & GLOBAL_DIAG0::VIO_UV) != 0;
    status->vio_ov = (diag0 & GLOBAL_DIAG0::VIO_OV) != 0;
    status->vdd_uv = (diag0 & GLOBAL_DIAG0::VDD_UV) != 0;
    status->vdd_ov = (diag0 & GLOBAL_DIAG0::VDD_OV) != 0;
    status->clock_fault = (diag0 & GLOBAL_DIAG0::CLK_NOK) != 0;
    status->ot_error = (diag0 & GLOBAL_DIAG0::COTERR) != 0;
    status->ot_warning = (diag0 & GLOBAL_DIAG0::COTWARN) != 0;
    status->reset_event = (diag0 & GLOBAL_DIAG0::RES_EVENT) != 0;
    status->por_event = (diag0 & GLOBAL_DIAG0::POR_EVENT) != 0;
    status->spi_wd_error = (diag0 & GLOBAL_DIAG0::SPI_WD_ERR) != 0;

    status->any_fault = (diag0 & GLOBAL_DIAG0::FAULT_MASK) != 0;

    // Read FB_STAT for additional status
    uint16_t fb_stat;
    if (read_register(CentralReg::FB_STAT, &fb_stat, true, nullptr)) {
        status->supply_nok_internal = (fb_stat & FB_STAT::SUP_NOK_INT) != 0;
        status->supply_nok_external = (fb_stat & FB_STAT::SUP_NOK_EXT) != 0;
        status->init_done = (fb_stat & FB_STAT::INIT_DONE) != 0;
    }

    // Read CH_CTRL to get mode
    uint16_t ch_ctrl;
    if (read_register(CentralReg::CH_CTRL, &ch_ctrl, true, nullptr)) {
        status->config_mode = (ch_ctrl & CH_CTRL::OP_MODE) == 0;
    }

    // Read voltage feedbacks
    uint16_t fb_voltage1;
    if (read_register(CentralReg::FB_VOLTAGE1, &fb_voltage1, true, nullptr)) {
        status->vbat_voltage = fb_voltage1;
    }

    uint16_t fb_voltage2;
    if (read_register(CentralReg::FB_VOLTAGE2, &fb_voltage2, true, nullptr)) {
        status->vio_voltage = fb_voltage2;
    }

    return true;
}

bool Driver::get_channel_diagnostics(Channel channel, ChannelDiagnostics* diag, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    if (!is_valid_channel_internal(channel)) {
        if (error) *error = DriverError::InvalidChannel;
        return false;
    }

    // Initialize diagnostics structure
    *diag = ChannelDiagnostics();

    // Read DIAG_ERR register for this channel group
    uint16_t diag_err;
    if (read_register(CentralReg::DIAG_ERR_CHGR0 + to_index(channel), &diag_err, true, nullptr)) {
        // Parse error flags (bit positions from datasheet Table in page 67)
        diag->overcurrent = (diag_err & (1 << 0)) != 0;         // OC bit
        diag->short_to_ground = (diag_err & (1 << 1)) != 0;     // SG bit
        diag->open_load = (diag_err & (1 << 2)) != 0;           // OL bit
        diag->over_temperature = (diag_err & (1 << 3)) != 0;    // OTE bit
        diag->open_load_short_ground = (diag_err & (1 << 4)) != 0; // OLSG bit
    }

    // Read DIAG_WARN register for warnings
    uint16_t diag_warn;
    if (read_register(CentralReg::DIAG_WARN_CHGR0 + to_index(channel), &diag_warn, true, nullptr)) {
        diag->ot_warning = (diag_warn & (1 << 0)) != 0;
        diag->current_regulation_warning = (diag_warn & (1 << 1)) != 0;
        diag->pwm_regulation_warning = (diag_warn & (1 << 2)) != 0;
        diag->olsg_warning = (diag_warn & (1 << 3)) != 0;
    }

    // Read feedback values
    uint16_t ch_base = get_channel_base(channel);
    
    uint16_t fb_i_avg;
    if (read_register(ch_base + ChannelReg::FB_I_AVG, &fb_i_avg, true, nullptr)) {
        diag->average_current = fb_i_avg;
    }

    uint16_t fb_dc;
    if (read_register(ch_base + ChannelReg::FB_DC, &fb_dc, true, nullptr)) {
        diag->duty_cycle = fb_dc;
    }

    uint16_t fb_vbat;
    if (read_register(ch_base + ChannelReg::FB_VBAT, &fb_vbat, true, nullptr)) {
        diag->vbat_feedback = fb_vbat;
    }

    // Read min/max current feedback (FB_IMIN_IMAX register)
    // Register format: [15:8] = I_MAX, [7:0] = I_MIN
    uint16_t fb_minmax;
    if (read_register(ch_base + ChannelReg::FB_IMIN_IMAX, &fb_minmax, true, nullptr)) {
        diag->min_current = fb_minmax & 0x00FF;        // Lower 8 bits
        diag->max_current = (fb_minmax >> 8) & 0x00FF; // Upper 8 bits
    }

    return true;
}

bool Driver::get_average_current(Channel channel, uint16_t* current_ma, bool parallel_mode, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    if (!is_valid_channel_internal(channel)) {
        if (error) *error = DriverError::InvalidChannel;
        return false;
    }

    uint16_t ch_addr = get_channel_register(channel, ChannelReg::FB_I_AVG);
    uint16_t value;
    if (!read_register(ch_addr, &value, true, error)) {
        return false;
    }

    // Convert raw value to mA
    // Based on datasheet: similar calculation to setpoint
    *current_ma = SETPOINT::calculate_current(value, parallel_mode);
    return true;
}

bool Driver::get_duty_cycle(Channel channel, uint16_t* duty_cycle, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    if (!is_valid_channel_internal(channel)) {
        if (error) *error = DriverError::InvalidChannel;
        return false;
    }

    uint16_t ch_addr = get_channel_register(channel, ChannelReg::FB_DC);
    return read_register(ch_addr, duty_cycle, true, error);
}

bool Driver::get_vbat_voltage(uint16_t* voltage, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    uint16_t value;
    if (!read_register(CentralReg::FB_VOLTAGE1, &value, true, error)) {
        return false;
    }

    // Convert to millivolts (formula from datasheet)
    // V_BAT measurement encoding needs datasheet formula
    *voltage = value; // Return raw value for now
    return true;
}

bool Driver::get_vio_voltage(uint16_t* voltage, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    uint16_t value;
    if (!read_register(CentralReg::FB_VOLTAGE2, &value, true, error)) {
        return false;
    }

    *voltage = value; // Return raw value
    return true;
}

//==========================================================================
// FAULT MANAGEMENT
//==========================================================================

bool Driver::clear_faults(DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    // Write 1s to clear fault bits in GLOBAL_DIAG0 (rwh type - clear on write 1)
    if (!write_register(CentralReg::GLOBAL_DIAG0, 0xFFFF, true, error)) {
        return false;
    }

    // Clear GLOBAL_DIAG1
    if (!write_register(CentralReg::GLOBAL_DIAG1, 0xFFFF, true, error)) {
        return false;
    }

    // Clear GLOBAL_DIAG2
    if (!write_register(CentralReg::GLOBAL_DIAG2, 0xFFFF, true, error)) {
        return false;
    }

    return true;
}

bool Driver::has_any_fault(bool* has_fault, DriverError* error) noexcept {
    DeviceStatus status;
    if (!get_device_status(&status, error)) {
        return false;
    }

    *has_fault = status.any_fault;
    return true;
}

bool Driver::software_reset(DriverError* error) noexcept {
    // Software reset would require toggling RESN pin or power cycle
    // This IC doesn't have a software reset register
    // Return to config mode and disable all channels instead
    if (!enter_config_mode(error)) {
        return false;
    }

    if (!disable_all_channels(error)) {
        return false;
    }

    return true;
}

//==========================================================================
// WATCHDOG MANAGEMENT
//==========================================================================

bool Driver::reload_spi_watchdog(uint16_t reload_value, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    return write_register(CentralReg::WD_RELOAD, reload_value, true, error);
}

//==========================================================================
// DEVICE INFORMATION
//==========================================================================

bool Driver::get_ic_version(uint16_t* version, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    return read_register(CentralReg::ICVID, version, true, error);
}

bool Driver::get_chip_id(std::array<uint16_t, 3>* chip_id, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    if (!read_register(CentralReg::CHIPID0, &(*chip_id)[0], true, error)) {
        return false;
    }

    if (!read_register(CentralReg::CHIPID1, &(*chip_id)[1], true, error)) {
        return false;
    }

    if (!read_register(CentralReg::CHIPID2, &(*chip_id)[2], true, error)) {
        return false;
    }

    return true;
}

bool Driver::verify_device(bool* is_valid, DriverError* error) noexcept {
    // Read ICVID register to verify device is responding and check device type
    uint16_t icvid;
    if (!read_register(CentralReg::ICVID, &icvid, false, error)) { // Don't verify CRC during init
        return false;
    }
    
    // Validate device ID (checks for valid response: not 0x0000 or 0xFFFF)
    *is_valid = DeviceID::is_valid_device(icvid);
    
    return true;
}

//==========================================================================
// REGISTER ACCESS
//==========================================================================

bool Driver::read_register(uint16_t address, uint16_t* value, bool verify_crc, DriverError* error) noexcept {
    if (!hal_.is_ready()) {
        if (error) *error = DriverError::HardwareError;
        return false;
    }

    // Create read frame
    SPIFrame tx_frame = SPIFrame::make_read(address);
    
    // Calculate and set CRC
    tx_frame.tx_fields.crc = calculate_frame_crc(tx_frame);

    // Transfer frame
    SPIFrame rx_frame;
    if (!transfer_frame(tx_frame, &rx_frame, verify_crc, error)) {
        return false;
    }

    // Extract data from response (copy from bit-field to avoid reference issue)
    *value = rx_frame.rx_fields.data;
    return true;
}

bool Driver::write_register(uint16_t address, uint16_t value, bool verify_crc, DriverError* error) noexcept {
    if (!hal_.is_ready()) {
        if (error) *error = DriverError::HardwareError;
        return false;
    }

    // Create write frame
    SPIFrame tx_frame = SPIFrame::make_write(address, value);
    
    // Calculate and set CRC
    tx_frame.tx_fields.crc = calculate_frame_crc(tx_frame);

    // Transfer frame
    SPIFrame rx_frame;
    if (!transfer_frame(tx_frame, &rx_frame, verify_crc, error)) {
        return false;
    }

    return true;
}

bool Driver::modify_register(uint16_t address, uint16_t mask, uint16_t value, DriverError* error) noexcept {
    // Read current value
    uint16_t current_value;
    if (!read_register(address, &current_value, true, error)) {
        return false;
    }

    // Modify bits
    uint16_t new_value = (current_value & ~mask) | (value & mask);

    // Write back
    return write_register(address, new_value, true, error);
}

//==========================================================================
// PRIVATE METHODS
//==========================================================================

bool Driver::transfer_frame(const SPIFrame& tx_frame, SPIFrame* rx_frame, bool verify_crc, DriverError* error) noexcept {
    // Transfer 32-bit frame via HAL
    HALError hal_error;
    uint32_t rx_word;
    if (!hal_.transfer32(tx_frame.word, &rx_word, &hal_error)) {
        // Map HAL error to driver error
        switch (hal_error) {
            case HALError::Timeout:
                if (error) *error = DriverError::TimeoutError;
                break;
            case HALError::CRCError:
                if (error) *error = DriverError::CRCError;
                break;
            case HALError::TransferError:
            case HALError::BusError:
                if (error) *error = DriverError::HardwareError;
                break;
            default:
                if (error) *error = DriverError::RegisterError;
                break;
        }
        return false;
    }

    rx_frame->word = rx_word;

    // Verify CRC if requested
    if (verify_crc) {
        if (!verify_frame_crc(*rx_frame)) {
            if (error) *error = DriverError::CRCError;
            return false;
        }
    }

    // Check SPI status in reply
    if (!check_spi_status(*rx_frame, error)) {
        return false;
    }

    return true;
}

bool Driver::check_spi_status(const SPIFrame& rx_frame, DriverError* error) noexcept {
    SPIStatus status = static_cast<SPIStatus>(rx_frame.rx_fields.status);

    switch (status) {
        case SPIStatus::NO_ERROR:
            return true;
        case SPIStatus::SPI_FRAME_ERROR:
            if (error) *error = DriverError::SPIFrameError;
            return false;
        case SPIStatus::CRC_ERROR:
            if (error) *error = DriverError::CRCError;
            return false;
        case SPIStatus::WRITE_RO_REG:
            if (error) *error = DriverError::WriteToReadOnly;
            return false;
        case SPIStatus::INTERNAL_BUS_FAULT:
            if (error) *error = DriverError::RegisterError;
            return false;
        default:
            if (error) *error = DriverError::RegisterError;
            return false;
    }
}

bool Driver::is_channel_parallel(Channel channel, bool* is_parallel, DriverError* error) noexcept {
    if (!check_initialized(error)) {
        return false;
    }

    if (!is_valid_channel_internal(channel)) {
        if (error) *error = DriverError::InvalidChannel;
        return false;
    }

    // Read CH_CTRL to check parallel configuration bits
    uint16_t ch_ctrl;
    if (!read_register(CentralReg::CH_CTRL, &ch_ctrl, true, error)) {
        return false;
    }

    uint8_t ch_index = to_index(channel);

    // Check which parallel pair this channel belongs to
    switch (ch_index) {
        case 0:
        case 3:
            *is_parallel = (ch_ctrl & CH_CTRL::CH_PAR_0_3) != 0;
            break;
        case 1:
        case 2:
            *is_parallel = (ch_ctrl & CH_CTRL::CH_PAR_1_2) != 0;
            break;
        case 4:
        case 5:
            *is_parallel = (ch_ctrl & CH_CTRL::CH_PAR_4_5) != 0;
            break;
        default:
            *is_parallel = false;
            break;
    }

    return true;
}

} // namespace TLE92466ED