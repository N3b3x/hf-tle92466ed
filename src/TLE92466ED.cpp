/**
 * @file TLE92466ED.cpp
 * @brief Implementation of TLE92466ED driver class
 
 * @details
 * This file contains the complete implementation of all driver methods
 * for the TLE92466ED IC. All methods include comprehensive error checking,
 * CRC calculation/verification, and validation.
 */

#include "TLE92466ED.hpp"

namespace TLE92466ED {

//==============================================================================
// INITIALIZATION
//==============================================================================

DriverResult<void> Driver::Init() noexcept {
    // 1. Initialize CommInterface (GPIO and SPI bus only)
    if (auto result = comm_.Init(); !result) {
        return std::unexpected(DriverError::HardwareError);
    }

    // 2. Perform device reset sequence
    // RESN is active low: LOW = reset, HIGH = normal operation
    // EN is active high: HIGH = enabled, LOW = disabled
    // We keep EN disabled during initialization - user must explicitly enable
    comm_.Log(LogLevel::Info, "TLE92466ED", "Performing device reset sequence...\n");
    
    // Step 1: Ensure EN is LOW (disabled) during reset
    if (auto result = SetEnable(false); !result) {
        comm_.Log(LogLevel::Warn, "TLE92466ED", 
                  "Failed to set EN pin LOW (error: %u) - continuing anyway\n",
                  static_cast<unsigned>(result.error()));
    }
    
    // Step 2: Hold device in reset (LOW)
    if (auto result = SetReset(true); !result) {
        comm_.Log(LogLevel::Error, "TLE92466ED", 
                  "Failed to hold device in reset (error: %u)\n",
                  static_cast<unsigned>(result.error()));
        return std::unexpected(DriverError::HardwareError);
    }
    comm_.Log(LogLevel::Info, "TLE92466ED", "  RESN set LOW (device in reset)\n");
    
    // Step 3: Wait for reset pulse duration (minimum 10ms per datasheet)
    if (auto result = comm_.Delay(10000); !result) {  // 10ms = 10000 microseconds
        return std::unexpected(DriverError::HardwareError);
    }
    
    // Step 4: Release reset (HIGH)
    if (auto result = SetReset(false); !result) {
        comm_.Log(LogLevel::Error, "TLE92466ED", 
                  "Failed to release device from reset (error: %u)\n",
                  static_cast<unsigned>(result.error()));
        return std::unexpected(DriverError::HardwareError);
    }
    comm_.Log(LogLevel::Info, "TLE92466ED", "  RESN set HIGH (device released from reset)\n");
    
    // Step 5: Wait for device to stabilize after reset release (minimum 10ms per datasheet)
    if (auto result = comm_.Delay(10000); !result) {  // 10ms = 10000 microseconds
        return std::unexpected(DriverError::HardwareError);
    }
    
    comm_.Log(LogLevel::Info, "TLE92466ED", "✅ Device reset sequence completed (EN remains disabled)\n");

    // 3. Read and diagnose CLK_DIV register to check clock configuration
    // This helps diagnose clock-related critical faults early
    diagnoseClockConfiguration();

    // 4. Verify device communication by reading IC version
    auto verify_result = VerifyDevice();
    if (!verify_result) {
        return std::unexpected(verify_result.error());
    }
    if (!*verify_result) {
        return std::unexpected(DriverError::WrongDeviceID);
    }

    // 5. Device starts in Config Mode after power-up
    mission_mode_ = false;

    // 6. Apply default configuration
    if (auto result = applyDefaultConfig(); !result) {
        return std::unexpected(result.error());
    }

    // 7. Clear any power-on reset flags (skip initialization check during Init)
    if (auto result = clearFaultsInternal(); !result) {
        return std::unexpected(result.error());
    }

    // 8. Initialize cached state
    channel_enable_cache_ = 0;
    channel_setpoints_.fill(0);
    crc_enabled_ = false;  // CRC starts disabled until user explicitly enables it

    initialized_ = true;
    return {};
}

DriverResult<void> Driver::applyDefaultConfig() noexcept {
    // Configure GLOBAL_CONFIG: Enable CRC, SPI watchdog, clock watchdog, 3.3V VIO
    uint16_t global_cfg = GLOBAL_CONFIG::CRC_EN | 
                          GLOBAL_CONFIG::SPI_WD_EN | 
                          GLOBAL_CONFIG::CLK_WD_EN;
    
    if (auto result = WriteRegister(CentralReg::GLOBAL_CONFIG, global_cfg, false); !result) {
        return std::unexpected(result.error());
    }

    // Update internal CRC enable state (CRC_EN is enabled in default config)
    crc_enabled_ = true;

    // Set default VBAT thresholds (UV=7V, OV=40V)
    // Use internal version that doesn't check initialization (called during Init)
    if (auto result = setVbatThresholdsInternal(7.0f, 40.0f); !result) {
        return std::unexpected(result.error());
    }

    // Configure all channels with default settings (ICC mode, 1V/us slew, disabled)
    for (uint8_t ch = 0; ch < static_cast<uint8_t>(Channel::COUNT); ++ch) {
        Channel channel = static_cast<Channel>(ch);
        uint16_t ch_base = GetChannelBase(channel);
        
        // Set mode to ICC (0x0001)
        if (auto result = WriteRegister(ch_base + ChannelReg::MODE, 
                                        static_cast<uint16_t>(ChannelMode::ICC), false); !result) {
            return std::unexpected(result.error());
        }
        
        // Set default CH_CONFIG (slew rate 2.5V/us, OL disabled)
        if (auto result = WriteRegister(ch_base + ChannelReg::CH_CONFIG, 
                                        CH_CONFIG::SLEWR_2V5_US, false); !result) {
            return std::unexpected(result.error());
        }
        
        // Set current setpoint to 0
        if (auto result = WriteRegister(ch_base + ChannelReg::SETPOINT, 0, false); !result) {
            return std::unexpected(result.error());
        }
    }

    // Reload SPI watchdog
    if (auto result = WriteRegister(CentralReg::WD_RELOAD, 1000, false); !result) {
        return std::unexpected(result.error());
    }

    return {};
}

//==========================================================================
// MODE CONTROL
//==========================================================================

DriverResult<void> Driver::EnterMissionMode() noexcept {
    if (auto result = checkInitialized(); !result) {
        return result;
    }

    comm_.Log(LogLevel::Info, "TLE92466ED", "Entering Mission Mode\n");

    // Set OP_MODE bit in CH_CTRL register
    if (auto result = ModifyRegister(CentralReg::CH_CTRL, 
                                      CH_CTRL::OP_MODE, 
                                      CH_CTRL::OP_MODE); !result) {
        return std::unexpected(result.error());
    }

    mission_mode_ = true;
    comm_.Log(LogLevel::Info, "TLE92466ED", "✅ Mission Mode entered\n");
    return {};
}

DriverResult<void> Driver::EnterConfigMode() noexcept {
    if (auto result = checkInitialized(); !result) {
        return result;
    }

    comm_.Log(LogLevel::Info, "TLE92466ED", "Entering Config Mode\n");

    // Clear OP_MODE bit in CH_CTRL register
    if (auto result = ModifyRegister(CentralReg::CH_CTRL, 
                                      CH_CTRL::OP_MODE, 
                                      0); !result) {
        return std::unexpected(result.error());
    }

    mission_mode_ = false;
    comm_.Log(LogLevel::Info, "TLE92466ED", "✅ Config Mode entered\n");
    return {};
}

//==========================================================================
// GLOBAL CONFIGURATION
//==========================================================================

DriverResult<void> Driver::ConfigureGlobal(const GlobalConfig& config) noexcept {
    if (auto result = checkInitialized(); !result) {
        return result;
    }

    // Must be in config mode to change global configuration
    if (auto result = checkConfigMode(); !result) {
        return result;
    }

    comm_.Log(LogLevel::Info, "TLE92466ED", "Configuring global settings: CRC=%s, SPI_WD=%s, CLK_WD=%s, VIO_5V=%s, "
              "UV=%.2fV, OV=%.2fV, WD_Reload=%u\n",
              config.crc_enabled ? "enabled" : "disabled",
              config.spi_watchdog_enabled ? "enabled" : "disabled",
              config.clock_watchdog_enabled ? "enabled" : "disabled",
              config.vio_5v ? "true" : "false",
              config.vbat_uv_voltage, config.vbat_ov_voltage,
              config.spi_watchdog_reload);

    // Build GLOBAL_CONFIG register value
    uint16_t global_cfg = 0;
    if (config.clock_watchdog_enabled) global_cfg |= GLOBAL_CONFIG::CLK_WD_EN;
    if (config.spi_watchdog_enabled) global_cfg |= GLOBAL_CONFIG::SPI_WD_EN;
    if (config.crc_enabled) global_cfg |= GLOBAL_CONFIG::CRC_EN;
    if (config.vio_5v) global_cfg |= GLOBAL_CONFIG::VIO_SEL;

    if (auto result = WriteRegister(CentralReg::GLOBAL_CONFIG, global_cfg); !result) {
        return std::unexpected(result.error());
    }
    
    // Update internal CRC enable state
    crc_enabled_ = config.crc_enabled;

    // Configure VBAT thresholds
    if (auto result = SetVbatThresholds(config.vbat_uv_voltage, 
                                          config.vbat_ov_voltage); !result) {
        return std::unexpected(result.error());
    }

    // Configure SPI watchdog reload
    if (config.spi_watchdog_enabled) {
        if (auto result = WriteRegister(CentralReg::WD_RELOAD, 
                                        config.spi_watchdog_reload); !result) {
            return std::unexpected(result.error());
        }
    }

    return {};
}

DriverResult<void> Driver::SetCrcEnabled(bool enabled) noexcept {
    if (auto result = checkInitialized(); !result) {
        return result;
    }

    comm_.Log(LogLevel::Info, "TLE92466ED", "Setting CRC enabled: %s\n", enabled ? "true" : "false");

    auto result = ModifyRegister(CentralReg::GLOBAL_CONFIG,
                          GLOBAL_CONFIG::CRC_EN,
                          enabled ? GLOBAL_CONFIG::CRC_EN : 0);
    
    if (result) {
        // Update internal CRC enable state only if register write succeeded
        crc_enabled_ = enabled;
        comm_.Log(LogLevel::Info, "TLE92466ED", "CRC enabled state updated: %s\n", enabled ? "true" : "false");
}

        return result;
}

DriverResult<void> Driver::SetVbatThresholds(float uv_voltage, float ov_voltage) noexcept {
    if (auto result = checkInitialized(); !result) {
        return result;
    }

    comm_.Log(LogLevel::Info, "TLE92466ED", "Setting VBAT thresholds: UV=%.2fV, OV=%.2fV\n", uv_voltage, ov_voltage);

    return setVbatThresholdsInternal(uv_voltage, ov_voltage);
}

DriverResult<void> Driver::setVbatThresholdsInternal(float uv_voltage, float ov_voltage) noexcept {
    // Validate voltage range
    if (uv_voltage < 0.0f || uv_voltage > 41.4f || 
        ov_voltage < 0.0f || ov_voltage > 41.4f) {
        return std::unexpected(DriverError::InvalidParameter);
    }

    // Calculate register values from voltage
    uint8_t uv_threshold = VBAT_THRESHOLD::CalculateFromVoltage(uv_voltage);
    uint8_t ov_threshold = VBAT_THRESHOLD::CalculateFromVoltage(ov_voltage);

    // Check if calculation was successful (non-zero values)
    if (uv_threshold == 0 && uv_voltage > 0.0f) {
        return std::unexpected(DriverError::InvalidParameter);
    }
    if (ov_threshold == 0 && ov_voltage > 0.0f) {
        return std::unexpected(DriverError::InvalidParameter);
    }

    uint16_t value = (static_cast<uint16_t>(ov_threshold) << 8) | uv_threshold;
    return WriteRegister(CentralReg::VBAT_TH, value, false); // Don't verify CRC during init
}

DriverResult<void> Driver::SetVbatThresholdsRaw(uint8_t uv_threshold, uint8_t ov_threshold) noexcept {
    if (auto result = checkInitialized(); !result) {
        return result;
    }

    float uv_voltage = VBAT_THRESHOLD::CalculateVoltage(uv_threshold);
    float ov_voltage = VBAT_THRESHOLD::CalculateVoltage(ov_threshold);
    comm_.Log(LogLevel::Info, "TLE92466ED", "Setting VBAT thresholds (raw): UV_reg=%u (%.2fV), OV_reg=%u (%.2fV)\n",
              uv_threshold, uv_voltage, ov_threshold, ov_voltage);

    uint16_t value = (static_cast<uint16_t>(ov_threshold) << 8) | uv_threshold;
    return WriteRegister(CentralReg::VBAT_TH, value);
}

//==========================================================================
// CHANNEL CONTROL
//==========================================================================

DriverResult<void> Driver::EnableChannel(Channel channel, bool enabled) noexcept {
    if (auto result = checkInitialized(); !result) {
        return result;
    }

    // Channel enable can only be changed in Mission Mode
    if (auto result = checkMissionMode(); !result) {
        comm_.Log(LogLevel::Error, "TLE92466ED", "Cannot enable/disable channel: Device must be in Mission Mode (currently in Config Mode). Call EnterMissionMode() first.\n");
        return result;
    }

    if (!isValidChannelInternal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    comm_.Log(LogLevel::Info, "TLE92466ED", "Enabling channel: Channel=%s, Enabled=%s\n",
              ToString(channel), enabled ? "true" : "false");

    uint16_t mask = CH_CTRL::ChannelMask(ToIndex(channel));

    if (enabled) {
        channel_enable_cache_ |= mask;
    } else {
        channel_enable_cache_ &= ~mask;
    }

    return ModifyRegister(CentralReg::CH_CTRL, mask, enabled ? mask : 0);
}

DriverResult<void> Driver::EnableChannels(uint8_t channel_mask) noexcept {
    if (auto result = checkInitialized(); !result) {
        return result;
    }

    if (auto result = checkMissionMode(); !result) {
        return result;
    }

    // Mask to valid channels only (bits 0-5)
    channel_mask &= CH_CTRL::ALL_CH_MASK;
    channel_enable_cache_ = channel_mask;

    comm_.Log(LogLevel::Info, "TLE92466ED", "Enabling channels: Mask=0x%02X (", channel_mask);
    bool first = true;
    for (uint8_t ch = 0; ch < 6; ++ch) {
        if (channel_mask & (1 << ch)) {
            if (!first) comm_.Log(LogLevel::Info, "TLE92466ED", ", ");
            comm_.Log(LogLevel::Info, "TLE92466ED", "%s", ToString(static_cast<Channel>(ch)));
            first = false;
        }
    }
    comm_.Log(LogLevel::Info, "TLE92466ED", ")\n");

    return ModifyRegister(CentralReg::CH_CTRL, CH_CTRL::ALL_CH_MASK, channel_mask);
}

DriverResult<void> Driver::EnableAllChannels() noexcept {
    comm_.Log(LogLevel::Info, "TLE92466ED", "Enabling all channels\n");
    return EnableChannels(CH_CTRL::ALL_CH_MASK);
}

DriverResult<void> Driver::DisableAllChannels() noexcept {
    comm_.Log(LogLevel::Info, "TLE92466ED", "Disabling all channels\n");
    return EnableChannels(0);
}

DriverResult<void> Driver::SetChannelMode(Channel channel, ChannelMode mode) noexcept {
    if (auto result = checkInitialized(); !result) {
        return result;
    }

    // Mode can only be changed in Config Mode
    if (auto result = checkConfigMode(); !result) {
        return result;
    }

    if (!isValidChannelInternal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    uint16_t ch_addr = GetChannelRegister(channel, ChannelReg::MODE);
    
    comm_.Log(LogLevel::Info, "TLE92466ED", "Setting channel mode: Channel=%s, Mode=%s (0x%04X)\n",
              ToString(channel), ToString(mode), static_cast<uint16_t>(mode));
    
    return WriteRegister(ch_addr, static_cast<uint16_t>(mode));
}

DriverResult<void> Driver::SetParallelOperation(ParallelPair pair, bool enabled) noexcept {
    if (auto result = checkInitialized(); !result) {
        return result;
    }

    // Parallel operation can only be changed in Config Mode
    if (auto result = checkConfigMode(); !result) {
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

    comm_.Log(LogLevel::Info, "TLE92466ED", "Setting parallel operation: Pair=%s, Enabled=%s\n",
              ToString(pair), enabled ? "true" : "false");

    return ModifyRegister(CentralReg::CH_CTRL, mask, enabled ? mask : 0);
}

//==========================================================================
// CURRENT CONTROL
//==========================================================================

DriverResult<void> Driver::SetCurrentSetpoint(
    Channel channel,
    uint16_t current_ma,
    bool parallel_mode) noexcept {

    if (auto result = checkInitialized(); !result) {
        return result;
    }

    if (!isValidChannelInternal(channel)) {
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
    uint16_t target = SETPOINT::CalculateTarget(current_ma, parallel_mode);
    
    // Cache the setpoint
    channel_setpoints_[ToIndex(channel)] = target;

    // Write to SETPOINT register
    uint16_t ch_addr = GetChannelRegister(channel, ChannelReg::SETPOINT);
    
    comm_.Log(LogLevel::Info, "TLE92466ED", "Setting current setpoint: Channel=%s, Current=%u mA, Target=0x%04X, Parallel=%s\n",
              ToString(channel), current_ma, target, parallel_mode ? "true" : "false");
    
    return WriteRegister(ch_addr, target);
}

DriverResult<uint16_t> Driver::GetCurrentSetpoint(
    Channel channel,
    bool parallel_mode) noexcept {

    if (auto result = checkInitialized(); !result) {
        return std::unexpected(result.error());
    }

    if (!isValidChannelInternal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    // Read SETPOINT register
    uint16_t ch_addr = GetChannelRegister(channel, ChannelReg::SETPOINT);
    auto result = ReadRegister(ch_addr);
    if (!result) {
        return std::unexpected(result.error());
    }

    // Convert to current in mA
    uint16_t target = *result & SETPOINT::TARGET_MASK;
    uint16_t current_ma = SETPOINT::CalculateCurrent(target, parallel_mode);

    return current_ma;
}

DriverResult<void> Driver::ConfigurePwmPeriod(
    Channel channel,
    float period_us) noexcept {

    if (auto result = checkInitialized(); !result) {
        return result;
    }

    if (!isValidChannelInternal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    // Validate period range
    if (period_us < 0.125f || period_us > 32640.0f) {
        return std::unexpected(DriverError::InvalidParameter);
    }

    // Calculate register values from desired period
    auto config = PERIOD::CalculateFromPeriodUs(period_us);
    
    // Check if calculation was successful (mantissa != 0)
    if (config.mantissa == 0) {
        return std::unexpected(DriverError::InvalidParameter);
    }

    // Build PERIOD register value
    uint16_t value = PERIOD::BuildRegisterValue(config);

    comm_.Log(LogLevel::Info, "TLE92466ED", "Configuring PWM period: Channel=%s, Period=%.3f us, Mantissa=%u, Exponent=%u, Register=0x%04X\n",
              ToString(channel), period_us, config.mantissa, config.exponent, value);

    uint16_t ch_addr = GetChannelRegister(channel, ChannelReg::PERIOD);
    return WriteRegister(ch_addr, value);
}

DriverResult<void> Driver::ConfigurePwmPeriodRaw(
    Channel channel,
    uint8_t period_mantissa,
    uint8_t period_exponent,
    bool low_freq_range) noexcept {

    if (auto result = checkInitialized(); !result) {
        return result;
    }

    if (!isValidChannelInternal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    // Build PERIOD register value
    uint16_t value = period_mantissa | 
                    ((period_exponent & PERIOD::EXP_VALUE_MASK) << PERIOD::EXP_SHIFT) |
                    (low_freq_range ? PERIOD::LOW_FREQ_BIT : 0);

    comm_.Log(LogLevel::Info, "TLE92466ED", "Configuring PWM period (raw): Channel=%s, Mantissa=%u, Exponent=%u, "
              "LowFreq=%s, Register=0x%04X\n",
              ToString(channel), period_mantissa, period_exponent,
              low_freq_range ? "true" : "false", value);

    uint16_t ch_addr = GetChannelRegister(channel, ChannelReg::PERIOD);
    return WriteRegister(ch_addr, value);
}

DriverResult<void> Driver::ConfigureDither(
    Channel channel,
    float amplitude_ma,
    float frequency_hz,
    bool parallel_mode) noexcept {

    if (auto result = checkInitialized(); !result) {
        return result;
    }

    if (!isValidChannelInternal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    // Validate parameters
    if (amplitude_ma < 0.0f || frequency_hz <= 0.0f) {
        return std::unexpected(DriverError::InvalidParameter);
    }

    // Check if channel is in parallel mode (if not specified, detect it)
    if (!parallel_mode) {
        auto parallel_result = isChannelParallel(channel);
        if (parallel_result) {
            parallel_mode = *parallel_result;
        }
    }

    // Calculate dither configuration from amplitude and frequency
    auto config = DITHER::CalculateFromAmplitudeFrequency(
        amplitude_ma, frequency_hz, parallel_mode);

    comm_.Log(LogLevel::Info, "TLE92466ED", "Configuring dither: Channel=%s, Amplitude=%.2f mA, Frequency=%.2f Hz, "
              "StepSize=%u, NumSteps=%u, FlatSteps=%u, Parallel=%s\n",
              ToString(channel), amplitude_ma, frequency_hz,
              config.step_size, config.num_steps, config.flat_steps, parallel_mode ? "true" : "false");

    // Configure dither registers
    return ConfigureDitherRaw(channel, config.step_size, config.num_steps, config.flat_steps);
}

DriverResult<void> Driver::ConfigureDitherRaw(
    Channel channel,
    uint16_t step_size,
    uint8_t num_steps,
    uint8_t flat_steps) noexcept {

    if (auto result = checkInitialized(); !result) {
        return result;
    }

    if (!isValidChannelInternal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    uint16_t ch_base = GetChannelBase(channel);

    comm_.Log(LogLevel::Info, "TLE92466ED", "Configuring dither (raw): Channel=%s, StepSize=%u, NumSteps=%u, FlatSteps=%u\n",
              ToString(channel), step_size, num_steps, flat_steps);

    // Configure DITHER_CTRL (step size)
    uint16_t ctrl_value = step_size & DITHER_CTRL::STEP_SIZE_MASK;
    if (auto result = WriteRegister(ch_base + ChannelReg::DITHER_CTRL, ctrl_value); !result) {
        return std::unexpected(result.error());
    }

    // Configure DITHER_STEP (steps and flat period)
    uint16_t step_value = flat_steps | (static_cast<uint16_t>(num_steps) << DITHER_STEP::STEPS_SHIFT);
    if (auto result = WriteRegister(ch_base + ChannelReg::DITHER_STEP, step_value); !result) {
        return std::unexpected(result.error());
    }

    return {};
}

DriverResult<void> Driver::ConfigureChannel(
    Channel channel,
    const ChannelConfig& config) noexcept {

    if (auto result = checkInitialized(); !result) {
        return result;
    }

    // Most configuration requires Config Mode
    if (auto result = checkConfigMode(); !result) {
        return result;
    }

    if (!isValidChannelInternal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    comm_.Log(LogLevel::Info, "TLE92466ED", "Configuring channel: %s, Mode=%s, Current=%u mA, "
              "SlewRate=%s, DiagCurrent=%s, OL_Threshold=%u\n",
              ToString(channel), ToString(config.mode), config.current_setpoint_ma,
              ToString(config.slew_rate), ToString(config.diag_current), config.open_load_threshold);

    uint16_t ch_base = GetChannelBase(channel);

    // 1. Set channel mode
    if (auto result = WriteRegister(ch_base + ChannelReg::MODE, 
                                     static_cast<uint16_t>(config.mode)); !result) {
        return std::unexpected(result.error());
    }

    // 2. Set current setpoint with parallel mode detection
    auto parallel_result = isChannelParallel(channel);
    bool is_parallel = parallel_result.value_or(false); // Default to false if can't determine
    uint16_t target = SETPOINT::CalculateTarget(config.current_setpoint_ma, is_parallel);
    if (config.auto_limit_disabled) {
        target |= SETPOINT::AUTO_LIMIT_DIS;
    }
    if (auto result = WriteRegister(ch_base + ChannelReg::SETPOINT, target); !result) {
        return std::unexpected(result.error());
    }

    // 3. Configure CH_CONFIG register
    uint16_t ch_cfg = static_cast<uint16_t>(config.slew_rate) |
                     (static_cast<uint16_t>(config.diag_current) << 2) |
                     (static_cast<uint16_t>(config.open_load_threshold & CH_CONFIG::OL_TH_VALUE_MASK) << CH_CONFIG::OL_TH_SHIFT);
    
    if (auto result = WriteRegister(ch_base + ChannelReg::CH_CONFIG, ch_cfg); !result) {
        return std::unexpected(result.error());
    }

    // 3a. Configure OLSG warning enable if requested (bit 14 of CTRL register)
    if (config.olsg_warning_enabled) {
        if (auto result = ModifyRegister(ch_base + ChannelReg::CTRL, 
                                         CH_CTRL_REG::OLSG_WARN_EN,
                                         CH_CTRL_REG::OLSG_WARN_EN); !result) {
            return std::unexpected(result.error());
        }
    }

    // 4. Configure PWM if specified
    // Note: ChannelConfig still uses low-level parameters for backward compatibility
    // New code should use ConfigurePwmPeriod(period_us) directly
    if (config.pwm_period_mantissa > 0) {
        if (auto result = ConfigurePwmPeriodRaw(channel, 
                                              config.pwm_period_mantissa,
                                              config.pwm_period_exponent,
                                              false); !result) {
            return std::unexpected(result.error());
        }
    }

    // 5. Configure dither if specified
    // Note: ChannelConfig still uses low-level parameters for backward compatibility
    // New code should use ConfigureDither(amplitude_ma, frequency_hz) directly
    if (config.dither_step_size > 0) {
        if (auto result = ConfigureDitherRaw(channel,
                                          config.dither_step_size,
                                          config.dither_steps,
                                          config.dither_flat); !result) {
            return std::unexpected(result.error());
        }
        
        // 5a. Enable deep dither if requested (bit 13 of DITHER_CTRL)
        if (config.deep_dither_enabled) {
            if (auto result = ModifyRegister(ch_base + ChannelReg::DITHER_CTRL,
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

DriverResult<DeviceStatus> Driver::GetDeviceStatus() noexcept {
    if (auto result = checkInitialized(); !result) {
        return std::unexpected(result.error());
    }

    DeviceStatus status{};

    // Read GLOBAL_DIAG0
    auto diag0_result = ReadRegister(CentralReg::GLOBAL_DIAG0);
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
    auto fb_stat_result = ReadRegister(CentralReg::FB_STAT);
    if (fb_stat_result) {
        uint16_t fb_stat = *fb_stat_result;
        status.supply_nok_internal = (fb_stat & FB_STAT::SUP_NOK_INT) != 0;
        status.supply_nok_external = (fb_stat & FB_STAT::SUP_NOK_EXT) != 0;
        status.init_done = (fb_stat & FB_STAT::INIT_DONE) != 0;
    }

    // Read CH_CTRL to get mode
    auto ch_ctrl_result = ReadRegister(CentralReg::CH_CTRL);
    if (ch_ctrl_result) {
        status.config_mode = (*ch_ctrl_result & CH_CTRL::OP_MODE) == 0;
    }

    // Read voltage feedbacks
    auto fb_voltage1_result = ReadRegister(CentralReg::FB_VOLTAGE1);
    if (fb_voltage1_result) {
        status.vbat_voltage = *fb_voltage1_result;
    }

    auto fb_voltage2_result = ReadRegister(CentralReg::FB_VOLTAGE2);
    if (fb_voltage2_result) {
        status.vio_voltage = *fb_voltage2_result;
    }

    return status;
}

DriverResult<ChannelDiagnostics> Driver::GetChannelDiagnostics(Channel channel) noexcept {
    if (auto result = checkInitialized(); !result) {
        return std::unexpected(result.error());
    }

    if (!isValidChannelInternal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    ChannelDiagnostics diag{};

    // Read DIAG_ERR register for this channel group
    auto diag_err_result = ReadRegister(CentralReg::DIAG_ERR_CHGR0 + ToIndex(channel));
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
    auto diag_warn_result = ReadRegister(CentralReg::DIAG_WARN_CHGR0 + ToIndex(channel));
    if (diag_warn_result) {
        uint16_t diag_warn = *diag_warn_result;
        diag.ot_warning = (diag_warn & (1 << 0)) != 0;
        diag.current_regulation_warning = (diag_warn & (1 << 1)) != 0;
        diag.pwm_regulation_warning = (diag_warn & (1 << 2)) != 0;
        diag.olsg_warning = (diag_warn & (1 << 3)) != 0;
    }

    // Read feedback values
    uint16_t ch_base = GetChannelBase(channel);
    
    auto fb_i_avg_result = ReadRegister(ch_base + ChannelReg::FB_I_AVG);
    if (fb_i_avg_result) {
        diag.average_current = *fb_i_avg_result;
    }

    auto fb_dc_result = ReadRegister(ch_base + ChannelReg::FB_DC);
    if (fb_dc_result) {
        diag.duty_cycle = *fb_dc_result;
    }

    auto fb_vbat_result = ReadRegister(ch_base + ChannelReg::FB_VBAT);
    if (fb_vbat_result) {
        diag.vbat_feedback = *fb_vbat_result;
    }

    // Read min/max current feedback (FB_IMIN_IMAX register)
    // Register format: [15:8] = I_MAX, [7:0] = I_MIN
    auto fb_minmax_result = ReadRegister(ch_base + ChannelReg::FB_IMIN_IMAX);
    if (fb_minmax_result) {
        uint16_t minmax = *fb_minmax_result;
        diag.min_current = minmax & DeviceID::REVISION_MASK;        // Lower 8 bits
        diag.max_current = (minmax >> 8) & DeviceID::REVISION_MASK; // Upper 8 bits
    }

    return diag;
}

DriverResult<uint16_t> Driver::GetAverageCurrent(Channel channel, bool parallel_mode) noexcept {
    if (auto result = checkInitialized(); !result) {
        return std::unexpected(result.error());
    }

    if (!isValidChannelInternal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    uint16_t ch_addr = GetChannelRegister(channel, ChannelReg::FB_I_AVG);
    auto result = ReadRegister(ch_addr);
    if (!result) {
        return std::unexpected(result.error());
    }

    // Convert raw value to mA
    // Based on datasheet: similar calculation to setpoint
    uint16_t current_ma = SETPOINT::CalculateCurrent(*result, parallel_mode);
    return current_ma;
}

DriverResult<uint16_t> Driver::GetDutyCycle(Channel channel) noexcept {
    if (auto result = checkInitialized(); !result) {
        return std::unexpected(result.error());
    }

    if (!isValidChannelInternal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    uint16_t ch_addr = GetChannelRegister(channel, ChannelReg::FB_DC);
    return ReadRegister(ch_addr);
}

DriverResult<uint16_t> Driver::GetVbatVoltage() noexcept {
    if (auto result = checkInitialized(); !result) {
        return std::unexpected(result.error());
    }

    auto result = ReadRegister(CentralReg::FB_VOLTAGE1);
    if (!result) {
        return std::unexpected(result.error());
    }

    // Convert to millivolts (formula from datasheet)
    // V_BAT measurement encoding needs datasheet formula
    return *result; // Return raw value for now
}

DriverResult<uint16_t> Driver::GetVioVoltage() noexcept {
    if (auto result = checkInitialized(); !result) {
        return std::unexpected(result.error());
    }

    auto result = ReadRegister(CentralReg::FB_VOLTAGE2);
    if (!result) {
        return std::unexpected(result.error());
    }

    return *result; // Return raw value
}

//==========================================================================
// FAULT MANAGEMENT
//==========================================================================

DriverResult<void> Driver::ClearFaults() noexcept {
    if (auto result = checkInitialized(); !result) {
        return result;
    }

    comm_.Log(LogLevel::Info, "TLE92466ED", "Clearing all fault flags\n");
    return clearFaultsInternal();
}

DriverResult<void> Driver::clearFaultsInternal() noexcept {
    // Write 1s to clear fault bits in GLOBAL_DIAG0 (rwh type - clear on write 1)
    if (auto result = WriteRegister(CentralReg::GLOBAL_DIAG0, GLOBAL_DIAG0::CLEAR_ALL); !result) {
        return std::unexpected(result.error());
    }

    // Clear GLOBAL_DIAG1
    if (auto result = WriteRegister(CentralReg::GLOBAL_DIAG1, GLOBAL_DIAG1::CLEAR_ALL); !result) {
        return std::unexpected(result.error());
    }

    // Clear GLOBAL_DIAG2
    if (auto result = WriteRegister(CentralReg::GLOBAL_DIAG2, GLOBAL_DIAG2::CLEAR_ALL); !result) {
        return std::unexpected(result.error());
    }

    return {};
}

DriverResult<bool> Driver::HasAnyFault() noexcept {
    auto status_result = GetDeviceStatus();
    if (!status_result) {
        return std::unexpected(status_result.error());
    }

    return status_result->any_fault;
}

DriverResult<void> Driver::SoftwareReset() noexcept {
    comm_.Log(LogLevel::Info, "TLE92466ED", "Performing software reset (entering config mode and disabling all channels)\n");
    // Software reset would require toggling RESN pin or power cycle
    // This IC doesn't have a software reset register
    // Return to config mode and disable all channels instead
    if (auto result = EnterConfigMode(); !result) {
        return result;
    }

    if (auto result = DisableAllChannels(); !result) {
        return result;
    }

    comm_.Log(LogLevel::Info, "TLE92466ED", "✅ Software reset completed\n");
    return {};
}

//==========================================================================
// WATCHDOG MANAGEMENT
//==========================================================================

DriverResult<void> Driver::ReloadSpiWatchdog(uint16_t reload_value) noexcept {
    if (auto result = checkInitialized(); !result) {
        return result;
    }

    comm_.Log(LogLevel::Info, "TLE92466ED", "Reloading SPI watchdog: ReloadValue=%u\n", reload_value);
    return WriteRegister(CentralReg::WD_RELOAD, reload_value);
}

//==========================================================================
// DEVICE INFORMATION
//==========================================================================

DriverResult<uint16_t> Driver::GetIcVersion() noexcept {
    if (auto result = checkInitialized(); !result) {
        return std::unexpected(result.error());
    }

    return ReadRegister(CentralReg::ICVID);
}

DriverResult<std::array<uint16_t, 3>> Driver::GetChipId() noexcept {
    if (auto result = checkInitialized(); !result) {
        return std::unexpected(result.error());
    }

    std::array<uint16_t, 3> chip_id;

    auto id0_result = ReadRegister(CentralReg::CHIPID0);
    if (!id0_result) {
        return std::unexpected(id0_result.error());
    }
    chip_id[0] = *id0_result;

    auto id1_result = ReadRegister(CentralReg::CHIPID1);
    if (!id1_result) {
        return std::unexpected(id1_result.error());
    }
    chip_id[1] = *id1_result;

    auto id2_result = ReadRegister(CentralReg::CHIPID2);
    if (!id2_result) {
        return std::unexpected(id2_result.error());
    }
    chip_id[2] = *id2_result;

    return chip_id;
}

DriverResult<bool> Driver::VerifyDevice() noexcept {
    // Read ICVID register to verify device is responding and check device type
    auto id_result = ReadRegister(CentralReg::ICVID, false); // Don't verify CRC during init
    
    if (!id_result) {
        comm_.Log(LogLevel::Error, "TLE92466ED", "Device verification failed: Failed to read ICVID register (error: %u)\n", 
                  static_cast<unsigned>(id_result.error()));
        return std::unexpected(id_result.error());
    }

    uint16_t icvid = *id_result;
    
    // Check if we got a valid response (not all zeros or all ones)
    if (icvid == 0x0000 || icvid == 0xFFFF) {
        comm_.Log(LogLevel::Error, "TLE92466ED", "Device verification failed: Invalid ICVID response (0x%04X)\n", icvid);
        return false;
    }
    
    // Validate device ID
    bool valid = DeviceID::IsValidDevice(icvid);
    
    // Extract and log device information
    uint8_t device_type = DeviceID::GetDeviceType(icvid);
    uint8_t revision = DeviceID::GetRevision(icvid);
    
    if (valid) {
        comm_.Log(LogLevel::Info, "TLE92466ED", "Device verified: ICVID=0x%04X, Type=0x%02X, Revision=0x%02X\n", 
                  icvid, device_type, revision);
    } else {
        comm_.Log(LogLevel::Warn, "TLE92466ED", "Device verification: ICVID=0x%04X (Type=0x%02X, Rev=0x%02X) - Unknown device type\n", 
                  icvid, device_type, revision);
    }
    
    return valid;
}

//==========================================================================
// REGISTER ACCESS
//==========================================================================

DriverResult<uint32_t> Driver::ReadRegister(uint16_t address, bool verify_crc) noexcept {
    if (!comm_.IsReady()) {
        return std::unexpected(DriverError::HardwareError);
    }

    // Use internal CRC enable state by default
    // verify_crc=false allows override to disable CRC verification (e.g., during init)
    // verify_crc=true allows override to force CRC verification
    bool should_verify_crc = verify_crc ? true : crc_enabled_;

    // Use CommInterface Read function (handles frame construction, CRC, and transfer)
    auto result = comm_.Read(address, should_verify_crc);
    if (!result) {
        // Map CommInterface error to driver error
        switch (result.error()) {
            case CommError::Timeout:
                return std::unexpected(DriverError::TimeoutError);
            case CommError::CRCError:
                return std::unexpected(DriverError::CRCError);
            case CommError::BusError:
            case CommError::TransferError:
                return std::unexpected(DriverError::HardwareError);
            default:
                return std::unexpected(DriverError::HardwareError);
        }
    }

    return *result;
}

DriverResult<void> Driver::WriteRegister(uint16_t address, uint16_t value, bool verify_crc) noexcept {
    if (!comm_.IsReady()) {
        return std::unexpected(DriverError::HardwareError);
    }

    // Use internal CRC enable state by default
    // verify_crc=false allows override to disable CRC verification (e.g., during init)
    // verify_crc=true allows override to force CRC verification
    bool should_verify_crc = verify_crc ? true : crc_enabled_;

    // Use CommInterface Write function (handles frame construction, CRC, and transfer)
    auto result = comm_.Write(address, value, should_verify_crc);
    if (!result) {
        // Map CommInterface error to driver error
        switch (result.error()) {
            case CommError::Timeout:
                return std::unexpected(DriverError::TimeoutError);
            case CommError::CRCError:
                return std::unexpected(DriverError::CRCError);
            case CommError::BusError:
            case CommError::TransferError:
                return std::unexpected(DriverError::HardwareError);
            default:
                return std::unexpected(DriverError::HardwareError);
        }
    }

    return {};
}

DriverResult<void> Driver::ModifyRegister(
    uint16_t address,
    uint16_t mask,
    uint16_t value) noexcept {

    // Read current value
    auto read_result = ReadRegister(address);
    if (!read_result) {
        return std::unexpected(read_result.error());
    }

    // Modify bits
    uint16_t new_value = (*read_result & ~mask) | (value & mask);

    // Write back
    return WriteRegister(address, new_value);
}

//==========================================================================
// PRIVATE METHODS
//==========================================================================

DriverResult<SPIFrame> Driver::transferFrame(const SPIFrame& tx_frame, bool verify_crc) noexcept {
    // Transfer 32-bit frame via CommInterface
    auto comm_result = comm_.Transfer32(tx_frame.word);
    if (!comm_result) {
        // Map CommInterface error to driver error
        switch (comm_result.error()) {
            case CommError::Timeout:
                return std::unexpected(DriverError::TimeoutError);
            case CommError::CRCError:
                return std::unexpected(DriverError::CRCError);
            case CommError::TransferError:
            case CommError::BusError:
                return std::unexpected(DriverError::HardwareError);
            default:
                return std::unexpected(DriverError::RegisterError);
        }
    }

    SPIFrame rx_frame;
    rx_frame.word = *comm_result;

    // Verify CRC if requested
    if (verify_crc) {
        if (!VerifyFrameCrc(rx_frame)) {
            return std::unexpected(DriverError::CRCError);
        }
    }

    // Check SPI status in reply
    if (auto result = checkSpiStatus(rx_frame); !result) {
        return std::unexpected(result.error());
    }

    return rx_frame;
}

DriverResult<void> Driver::checkSpiStatus(const SPIFrame& rx_frame) noexcept {
    // Status field only exists in 16-bit reply frames
    if (rx_frame.rx_common.reply_mode != 0x00) {
        // For non-16-bit frames, check if it's a critical fault
        if (rx_frame.rx_common.reply_mode == 0x02) {
            // Critical fault frame - extract and log fault flags
            auto fault_flags = CriticalFaultFlags::Extract(rx_frame);
            // Return hardware error for critical faults
            return std::unexpected(DriverError::HardwareError);
        }
        // 22-bit reply frames don't have status field, assume OK
        return {};
    }
    
    // 16-bit reply frame - check status field
    SPIStatus status = static_cast<SPIStatus>(rx_frame.rx_16bit.status);

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

DriverResult<bool> Driver::isChannelParallel(Channel channel) noexcept {
    if (auto result = checkInitialized(); !result) {
        return std::unexpected(result.error());
    }

    if (!isValidChannelInternal(channel)) {
        return std::unexpected(DriverError::InvalidChannel);
    }

    // Read CH_CTRL to check parallel configuration bits
    auto ctrl_result = ReadRegister(CentralReg::CH_CTRL);
    if (!ctrl_result) {
        return std::unexpected(ctrl_result.error());
    }

    uint16_t ch_ctrl = *ctrl_result;
    uint8_t ch_index = ToIndex(channel);

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

//==========================================================================
// GPIO CONTROL (Reset, Enable, Fault Status)
//==========================================================================

DriverResult<void> Driver::SetReset(bool reset) noexcept {
    comm_.Log(LogLevel::Info, "TLE92466ED", "Setting reset pin: %s\n", reset ? "LOW (in reset)" : "HIGH (released)");
    // RESN is active low: reset=true means hold in reset (GPIO LOW), reset=false means release (GPIO HIGH)
    ActiveLevel level = reset ? ActiveLevel::INACTIVE : ActiveLevel::ACTIVE;
    
    auto result = comm_.SetGpioPin(ControlPin::RESN, level);
    if (!result) {
        return std::unexpected(DriverError::HardwareError);
    }
    
    return {};
}

DriverResult<void> Driver::SetEnable(bool enable) noexcept {
    comm_.Log(LogLevel::Info, "TLE92466ED", "Setting enable pin: %s\n", enable ? "HIGH (enabled)" : "LOW (disabled)");
    // EN is active high: enable=true means enable outputs (GPIO HIGH), enable=false means disable (GPIO LOW)
    ActiveLevel level = enable ? ActiveLevel::ACTIVE : ActiveLevel::INACTIVE;
    
    auto result = comm_.SetGpioPin(ControlPin::EN, level);
    if (!result) {
        return std::unexpected(DriverError::HardwareError);
    }
    
    return {};
}

DriverResult<bool> Driver::IsFault() noexcept {
    auto result = comm_.GetGpioPin(ControlPin::FAULTN);
    if (!result) {
        return std::unexpected(DriverError::HardwareError);
    }
    
    // FAULTN is active low: ACTIVE means fault detected, INACTIVE means no fault
    return *result == ActiveLevel::ACTIVE;
}

//==========================================================================
// DIAGNOSTIC HELPERS
//==========================================================================

void Driver::diagnoseClockConfiguration() noexcept {
    // Read CLK_DIV register to check clock configuration
    // This helps diagnose clock-related critical faults early
    auto clk_div_result = ReadRegister(CentralReg::CLK_DIV, false); // Don't verify CRC during init
    
    if (!clk_div_result) {
        comm_.Log(LogLevel::Warn, "TLE92466ED", 
                  "Failed to read CLK_DIV register (error: %u) - continuing anyway\n",
                  static_cast<unsigned>(clk_div_result.error()));
        return;
    }
    
    uint16_t clk_div = static_cast<uint16_t>(*clk_div_result);
    
    // Parse CLK_DIV register fields
    bool ext_clk = (clk_div & 0x8000) != 0;           // Bit 15: EXT_CLK
    uint8_t pll_refdiv = (clk_div >> 9) & 0x3F;       // Bits 14:9: PLL_REFDIV (6 bits)
    uint16_t pll_fbdiv = clk_div & 0x01FF;            // Bits 8:0: PLL_FBDIV (9 bits)
    
    comm_.Log(LogLevel::Info, "TLE92466ED", 
              "═══════════════════════════════════════════════════════════\n");
    comm_.Log(LogLevel::Info, "TLE92466ED", 
              "CLK_DIV Register (0x0019): 0x%04X\n", clk_div);
    comm_.Log(LogLevel::Info, "TLE92466ED", 
              "  Bit 15 (EXT_CLK): %d (%s)\n", 
              ext_clk ? 1 : 0,
              ext_clk ? "External Clock (CLK-pin)" : "Internal Oscillator");
    comm_.Log(LogLevel::Info, "TLE92466ED", 
              "  Bits 14:9 (PLL_REFDIV): %d (0x%02X)\n", pll_refdiv, pll_refdiv);
    comm_.Log(LogLevel::Info, "TLE92466ED", 
              "  Bits 8:0 (PLL_FBDIV): %d (0x%03X)\n", pll_fbdiv, pll_fbdiv);
    
    // Calculate system clock frequency if external clock is used
    if (ext_clk && pll_refdiv > 0 && pll_fbdiv > 0) {
        // fSYS = fCLK * (PLL_FBDIV) / (2 * PLL_REFDIV)
        // We don't know fCLK, but we can show the divider ratio
        float divider_ratio = static_cast<float>(pll_fbdiv) / (2.0f * static_cast<float>(pll_refdiv));
        comm_.Log(LogLevel::Info, "TLE92466ED", 
                  "  PLL Divider Ratio: %.3f (fSYS = fCLK * %.3f)\n", 
                  divider_ratio, divider_ratio);
        comm_.Log(LogLevel::Info, "TLE92466ED", 
                  "  Note: fCLK is the external clock frequency on CLK-pin\n");
        
        // Show expected fSYS for common fCLK values
        comm_.Log(LogLevel::Info, "TLE92466ED", 
                  "  Expected fSYS for common fCLK values:\n");
        for (float fclk_mhz = 1.0f; fclk_mhz <= 8.0f; fclk_mhz += 0.5f) {
            float fsys_mhz = fclk_mhz * divider_ratio;
            comm_.Log(LogLevel::Info, "TLE92466ED", 
                      "    fCLK=%.1f MHz -> fSYS=%.2f MHz\n", fclk_mhz, fsys_mhz);
        }
    } else if (!ext_clk) {
        comm_.Log(LogLevel::Info, "TLE92466ED", 
                  "  Using Internal Oscillator (PLL dividers ignored)\n");
        comm_.Log(LogLevel::Info, "TLE92466ED", 
                  "  System clock fSYS is generated from internal oscillator\n");
    } else {
        comm_.Log(LogLevel::Warn, "TLE92466ED", 
                  "  ⚠️  Invalid PLL divider values (PLL_REFDIV=%d, PLL_FBDIV=%d)\n", 
                  pll_refdiv, pll_fbdiv);
        comm_.Log(LogLevel::Warn, "TLE92466ED", 
                  "  This may cause clock watchdog faults!\n");
    }
    comm_.Log(LogLevel::Info, "TLE92466ED", 
              "═══════════════════════════════════════════════════════════\n");
}

} // namespace TLE92466ED
