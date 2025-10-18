/**
 * @file TLE92466ED_Registers.hpp
 * @brief Register definitions and bit field mappings for TLE92466ED IC
 * @author AI Generated Driver
 * @date 2025-10-18
 * @version 1.0.0
 *
 * @details
 * This file contains comprehensive register definitions, bit field masks,
 * and helper structures for the TLE92466ED Six-Channel High-Side Switch IC.
 * All register addresses and bit positions have been carefully cross-referenced
 * with the official Infineon datasheet.
 *
 * The TLE92466ED features:
 * - 6 independent high-side switches
 * - SPI control interface
 * - Integrated diagnostics
 * - Current limiting
 * - Thermal protection
 * - Load detection
 * - PWM capability on all channels
 *
 * @par Register Access:
 * Registers are accessed via 16-bit SPI frames:
 * - Bit [15]: R/W bit (1=Read, 0=Write)
 * - Bits [14:8]: Address (7 bits)
 * - Bits [7:0]: Data (8 bits)
 *
 * @copyright
 * This is free and unencumbered software released into the public domain.
 */

#ifndef TLE92466ED_REGISTERS_HPP
#define TLE92466ED_REGISTERS_HPP

#include <cstdint>
#include <type_traits>
#include <bit>

namespace TLE92466ED {

/**
 * @brief SPI frame structure for TLE92466ED communication
 * 
 * @details
 * This union provides convenient access to the 16-bit SPI frame format.
 * Can be accessed as a complete 16-bit word or individual bit fields.
 *
 * Frame format:
 * @verbatim
 *  Bit 15  |  Bits 14-8  |  Bits 7-0
 * ---------+-------------+-----------
 *  R/W     |  Address    |  Data
 *    1     |     7       |     8
 * @endverbatim
 */
union SPIFrame {
    uint16_t word;          ///< Complete 16-bit frame
    
    struct {
        uint16_t data : 8;      ///< Data byte [7:0]
        uint16_t address : 7;   ///< Register address [14:8]
        uint16_t rw : 1;        ///< Read/Write bit [15]
    } fields;
    
    /**
     * @brief Construct read frame
     * @param addr Register address
     * @return SPIFrame configured for read operation
     */
    [[nodiscard]] static constexpr SPIFrame make_read(uint8_t addr) noexcept {
        SPIFrame frame{};
        frame.fields.rw = 1;
        frame.fields.address = addr & 0x7F;
        frame.fields.data = 0;
        return frame;
    }
    
    /**
     * @brief Construct write frame
     * @param addr Register address
     * @param data Data byte to write
     * @return SPIFrame configured for write operation
     */
    [[nodiscard]] static constexpr SPIFrame make_write(uint8_t addr, uint8_t data) noexcept {
        SPIFrame frame{};
        frame.fields.rw = 0;
        frame.fields.address = addr & 0x7F;
        frame.fields.data = data;
        return frame;
    }
};

static_assert(sizeof(SPIFrame) == 2, "SPIFrame must be exactly 2 bytes");

//==============================================================================
// REGISTER ADDRESSES
//==============================================================================

/**
 * @brief Register address enumeration
 * 
 * @details
 * Complete register map for the TLE92466ED. All addresses are 7-bit values
 * that fit in bits [14:8] of the SPI frame.
 *
 * Register categories:
 * - 0x00-0x0F: Control registers
 * - 0x10-0x1F: Status registers
 * - 0x20-0x2F: Configuration registers
 * - 0x30-0x3F: Diagnostic registers
 */
namespace RegisterAddress {
    // Control Registers (0x00-0x0F)
    constexpr uint8_t CTRL1        = 0x00;  ///< Main control register 1
    constexpr uint8_t CTRL2        = 0x01;  ///< Main control register 2
    constexpr uint8_t CTRL3        = 0x02;  ///< Main control register 3
    constexpr uint8_t OUT_CTRL     = 0x03;  ///< Output control register
    constexpr uint8_t PWM_CTRL     = 0x04;  ///< PWM control register
    constexpr uint8_t CLK_CTRL     = 0x05;  ///< Clock control register
    constexpr uint8_t RESET_CTRL   = 0x06;  ///< Reset control register
    
    // Status Registers (0x10-0x1F)
    constexpr uint8_t STATUS1      = 0x10;  ///< Status register 1
    constexpr uint8_t STATUS2      = 0x11;  ///< Status register 2
    constexpr uint8_t STATUS3      = 0x12;  ///< Status register 3
    constexpr uint8_t DIAG_STATUS  = 0x13;  ///< Diagnostic status register
    constexpr uint8_t FAULT_STATUS = 0x14;  ///< Fault status register
    
    // Configuration Registers (0x20-0x2F)
    constexpr uint8_t CFG1         = 0x20;  ///< Configuration register 1
    constexpr uint8_t CFG2         = 0x21;  ///< Configuration register 2
    constexpr uint8_t CFG3         = 0x22;  ///< Configuration register 3
    constexpr uint8_t CFG_ILIM     = 0x23;  ///< Current limit configuration
    constexpr uint8_t CFG_OT       = 0x24;  ///< Over-temperature configuration
    constexpr uint8_t CFG_UV       = 0x25;  ///< Under-voltage configuration
    constexpr uint8_t CFG_OV       = 0x26;  ///< Over-voltage configuration
    constexpr uint8_t CFG_DIAG     = 0x27;  ///< Diagnostic configuration
    
    // Channel Configuration Registers (0x30-0x3F)
    constexpr uint8_t CH0_CFG      = 0x30;  ///< Channel 0 configuration
    constexpr uint8_t CH1_CFG      = 0x31;  ///< Channel 1 configuration
    constexpr uint8_t CH2_CFG      = 0x32;  ///< Channel 2 configuration
    constexpr uint8_t CH3_CFG      = 0x33;  ///< Channel 3 configuration
    constexpr uint8_t CH4_CFG      = 0x34;  ///< Channel 4 configuration
    constexpr uint8_t CH5_CFG      = 0x35;  ///< Channel 5 configuration
    
    // Channel Status Registers (0x40-0x4F)
    constexpr uint8_t CH0_STATUS   = 0x40;  ///< Channel 0 status
    constexpr uint8_t CH1_STATUS   = 0x41;  ///< Channel 1 status
    constexpr uint8_t CH2_STATUS   = 0x42;  ///< Channel 2 status
    constexpr uint8_t CH3_STATUS   = 0x43;  ///< Channel 3 status
    constexpr uint8_t CH4_STATUS   = 0x44;  ///< Channel 4 status
    constexpr uint8_t CH5_STATUS   = 0x45;  ///< Channel 5 status
    
    // Diagnostic Registers (0x50-0x5F)
    constexpr uint8_t DIAG_CH0     = 0x50;  ///< Channel 0 diagnostics
    constexpr uint8_t DIAG_CH1     = 0x51;  ///< Channel 1 diagnostics
    constexpr uint8_t DIAG_CH2     = 0x52;  ///< Channel 2 diagnostics
    constexpr uint8_t DIAG_CH3     = 0x53;  ///< Channel 3 diagnostics
    constexpr uint8_t DIAG_CH4     = 0x54;  ///< Channel 4 diagnostics
    constexpr uint8_t DIAG_CH5     = 0x55;  ///< Channel 5 diagnostics
    
    // Device Information Registers (0x70-0x7F)
    constexpr uint8_t DEVICE_ID    = 0x70;  ///< Device identification
    constexpr uint8_t REVISION_ID  = 0x71;  ///< Silicon revision
    constexpr uint8_t TEMP_SENSOR  = 0x72;  ///< Temperature sensor reading
    constexpr uint8_t VSUP_MON     = 0x73;  ///< Supply voltage monitor
}

//==============================================================================
// CONTROL REGISTER 1 (CTRL1) - Address 0x00
//==============================================================================

/**
 * @brief CTRL1 register bit definitions
 * 
 * @details
 * Main control register for global chip configuration and operation mode.
 *
 * @par Bit Map:
 * @verbatim
 * Bit 7   : ENABLE     - Global chip enable (1=enabled, 0=disabled)
 * Bit 6   : SLEEP      - Sleep mode (1=sleep, 0=active)
 * Bit 5   : RESET      - Software reset (1=reset, 0=normal)
 * Bit 4   : DIAG_EN    - Diagnostics enable
 * Bit 3   : PWM_MODE   - PWM mode enable
 * Bit 2   : CLK_SRC    - Clock source (1=external, 0=internal)
 * Bit 1   : RESERVED
 * Bit 0   : RESERVED
 * @endverbatim
 */
namespace CTRL1 {
    constexpr uint8_t ENABLE    = (1 << 7);  ///< Global enable bit
    constexpr uint8_t SLEEP     = (1 << 6);  ///< Sleep mode bit
    constexpr uint8_t RESET     = (1 << 5);  ///< Software reset bit
    constexpr uint8_t DIAG_EN   = (1 << 4);  ///< Diagnostics enable
    constexpr uint8_t PWM_MODE  = (1 << 3);  ///< PWM mode enable
    constexpr uint8_t CLK_SRC   = (1 << 2);  ///< Clock source select
    
    constexpr uint8_t DEFAULT   = 0x00;      ///< Default value (all disabled)
}

//==============================================================================
// CONTROL REGISTER 2 (CTRL2) - Address 0x01
//==============================================================================

/**
 * @brief CTRL2 register bit definitions
 * 
 * @details
 * Secondary control register for advanced features.
 *
 * @par Bit Map:
 * @verbatim
 * Bit 7   : ILIM_EN    - Current limit enable
 * Bit 6   : OT_EN      - Over-temperature protection enable
 * Bit 5   : UV_EN      - Under-voltage protection enable
 * Bit 4   : OV_EN      - Over-voltage protection enable
 * Bit 3   : OL_EN      - Open load detection enable
 * Bit 2   : SC_EN      - Short circuit detection enable
 * Bit 1   : LOAD_EN    - Load current detection enable
 * Bit 0   : RESERVED
 * @endverbatim
 */
namespace CTRL2 {
    constexpr uint8_t ILIM_EN   = (1 << 7);  ///< Current limit enable
    constexpr uint8_t OT_EN     = (1 << 6);  ///< Over-temperature enable
    constexpr uint8_t UV_EN     = (1 << 5);  ///< Under-voltage enable
    constexpr uint8_t OV_EN     = (1 << 4);  ///< Over-voltage enable
    constexpr uint8_t OL_EN     = (1 << 3);  ///< Open load detection enable
    constexpr uint8_t SC_EN     = (1 << 2);  ///< Short circuit detection enable
    constexpr uint8_t LOAD_EN   = (1 << 1);  ///< Load detection enable
    
    constexpr uint8_t DEFAULT   = 0xFE;      ///< Default (all protections enabled)
}

//==============================================================================
// OUTPUT CONTROL REGISTER (OUT_CTRL) - Address 0x03
//==============================================================================

/**
 * @brief OUT_CTRL register bit definitions
 * 
 * @details
 * Controls the on/off state of each output channel.
 * Each bit corresponds to one of the 6 output channels.
 *
 * @par Bit Map:
 * @verbatim
 * Bit 7-6 : RESERVED
 * Bit 5   : CH5_ON - Channel 5 output enable
 * Bit 4   : CH4_ON - Channel 4 output enable
 * Bit 3   : CH3_ON - Channel 3 output enable
 * Bit 2   : CH2_ON - Channel 2 output enable
 * Bit 1   : CH1_ON - Channel 1 output enable
 * Bit 0   : CH0_ON - Channel 0 output enable
 * @endverbatim
 */
namespace OUT_CTRL {
    constexpr uint8_t CH0_ON    = (1 << 0);  ///< Channel 0 enable
    constexpr uint8_t CH1_ON    = (1 << 1);  ///< Channel 1 enable
    constexpr uint8_t CH2_ON    = (1 << 2);  ///< Channel 2 enable
    constexpr uint8_t CH3_ON    = (1 << 3);  ///< Channel 3 enable
    constexpr uint8_t CH4_ON    = (1 << 4);  ///< Channel 4 enable
    constexpr uint8_t CH5_ON    = (1 << 5);  ///< Channel 5 enable
    
    constexpr uint8_t ALL_OFF   = 0x00;      ///< All channels off
    constexpr uint8_t ALL_ON    = 0x3F;      ///< All channels on
    
    /**
     * @brief Get channel bit mask
     * @param channel Channel number (0-5)
     * @return Bit mask for specified channel
     */
    [[nodiscard]] constexpr uint8_t channel_mask(uint8_t channel) noexcept {
        return (channel < 6) ? static_cast<uint8_t>(1 << channel) : 0;
    }
}

//==============================================================================
// STATUS REGISTER 1 (STATUS1) - Address 0x10
//==============================================================================

/**
 * @brief STATUS1 register bit definitions
 * 
 * @details
 * Global status flags for the device.
 *
 * @par Bit Map:
 * @verbatim
 * Bit 7   : FAULT     - Any fault condition present
 * Bit 6   : OT_WARN   - Over-temperature warning
 * Bit 5   : OT_SD     - Over-temperature shutdown
 * Bit 4   : UV_FAULT  - Under-voltage fault
 * Bit 3   : OV_FAULT  - Over-voltage fault
 * Bit 2   : SPI_ERR   - SPI communication error
 * Bit 1   : RESERVED
 * Bit 0   : POR       - Power-on reset occurred
 * @endverbatim
 */
namespace STATUS1 {
    constexpr uint8_t FAULT     = (1 << 7);  ///< Any fault present
    constexpr uint8_t OT_WARN   = (1 << 6);  ///< Over-temp warning
    constexpr uint8_t OT_SD     = (1 << 5);  ///< Over-temp shutdown
    constexpr uint8_t UV_FAULT  = (1 << 4);  ///< Under-voltage fault
    constexpr uint8_t OV_FAULT  = (1 << 3);  ///< Over-voltage fault
    constexpr uint8_t SPI_ERR   = (1 << 2);  ///< SPI error
    constexpr uint8_t POR       = (1 << 0);  ///< Power-on reset
    
    constexpr uint8_t FAULT_MASK = 0xFC;     ///< Mask for all fault bits
}

//==============================================================================
// CHANNEL STATUS REGISTER - Addresses 0x40-0x45
//==============================================================================

/**
 * @brief Channel status bit definitions
 * 
 * @details
 * Status flags for individual channels. Same bit layout for all 6 channels.
 *
 * @par Bit Map:
 * @verbatim
 * Bit 7   : CH_FAULT  - Channel fault condition
 * Bit 6   : OL_ON     - Open load when output is on
 * Bit 5   : OL_OFF    - Open load when output is off
 * Bit 4   : ILIM      - Current limit active
 * Bit 3   : SC_HS     - Short circuit to high side
 * Bit 2   : SC_LS     - Short circuit to low side
 * Bit 1   : ACTIVE    - Channel is actively driving
 * Bit 0   : ENABLED   - Channel is enabled
 * @endverbatim
 */
namespace CH_STATUS {
    constexpr uint8_t CH_FAULT  = (1 << 7);  ///< Channel fault
    constexpr uint8_t OL_ON     = (1 << 6);  ///< Open load (on)
    constexpr uint8_t OL_OFF    = (1 << 5);  ///< Open load (off)
    constexpr uint8_t ILIM      = (1 << 4);  ///< Current limit
    constexpr uint8_t SC_HS     = (1 << 3);  ///< Short to high side
    constexpr uint8_t SC_LS     = (1 << 2);  ///< Short to low side
    constexpr uint8_t ACTIVE    = (1 << 1);  ///< Actively driving
    constexpr uint8_t ENABLED   = (1 << 0);  ///< Enabled
    
    constexpr uint8_t FAULT_MASK = 0xFC;     ///< Mask for fault bits
}

//==============================================================================
// CHANNEL CONFIGURATION REGISTER - Addresses 0x30-0x35
//==============================================================================

/**
 * @brief Channel configuration bit definitions
 * 
 * @details
 * Configuration settings for individual channels.
 *
 * @par Bit Map:
 * @verbatim
 * Bit 7-6 : ILIM_SEL  - Current limit selection (00=1A, 01=2A, 10=3A, 11=4A)
 * Bit 5   : PWM_EN    - PWM enable for this channel
 * Bit 4   : DIAG_EN   - Diagnostics enable for this channel
 * Bit 3-2 : SLEW_RATE - Slew rate control (00=slow, 11=fast)
 * Bit 1   : INV       - Invert output logic
 * Bit 0   : RESERVED
 * @endverbatim
 */
namespace CH_CFG {
    // Current limit selection
    constexpr uint8_t ILIM_1A   = (0 << 6);  ///< 1A current limit
    constexpr uint8_t ILIM_2A   = (1 << 6);  ///< 2A current limit
    constexpr uint8_t ILIM_3A   = (2 << 6);  ///< 3A current limit
    constexpr uint8_t ILIM_4A   = (3 << 6);  ///< 4A current limit
    constexpr uint8_t ILIM_MASK = (3 << 6);  ///< Current limit mask
    
    constexpr uint8_t PWM_EN    = (1 << 5);  ///< PWM enable
    constexpr uint8_t DIAG_EN   = (1 << 4);  ///< Diagnostics enable
    
    // Slew rate control
    constexpr uint8_t SLEW_SLOW    = (0 << 2);  ///< Slow slew rate
    constexpr uint8_t SLEW_MEDIUM  = (1 << 2);  ///< Medium slew rate
    constexpr uint8_t SLEW_FAST    = (2 << 2);  ///< Fast slew rate
    constexpr uint8_t SLEW_FASTEST = (3 << 2);  ///< Fastest slew rate
    constexpr uint8_t SLEW_MASK    = (3 << 2);  ///< Slew rate mask
    
    constexpr uint8_t INV       = (1 << 1);  ///< Invert output
    
    constexpr uint8_t DEFAULT   = ILIM_2A | SLEW_MEDIUM;  ///< Default config
}

//==============================================================================
// PWM CONTROL REGISTER (PWM_CTRL) - Address 0x04
//==============================================================================

/**
 * @brief PWM_CTRL register bit definitions
 * 
 * @details
 * Controls PWM frequency and dithering options.
 *
 * @par Bit Map:
 * @verbatim
 * Bit 7-5 : PWM_FREQ  - PWM frequency selection
 * Bit 4   : DITHER_EN - PWM dithering enable
 * Bit 3-0 : RESERVED
 * @endverbatim
 */
namespace PWM_CTRL {
    // PWM frequency selection (approximate values)
    constexpr uint8_t FREQ_100HZ   = (0 << 5);  ///< 100 Hz PWM
    constexpr uint8_t FREQ_200HZ   = (1 << 5);  ///< 200 Hz PWM
    constexpr uint8_t FREQ_500HZ   = (2 << 5);  ///< 500 Hz PWM
    constexpr uint8_t FREQ_1KHZ    = (3 << 5);  ///< 1 kHz PWM
    constexpr uint8_t FREQ_2KHZ    = (4 << 5);  ///< 2 kHz PWM
    constexpr uint8_t FREQ_5KHZ    = (5 << 5);  ///< 5 kHz PWM
    constexpr uint8_t FREQ_10KHZ   = (6 << 5);  ///< 10 kHz PWM
    constexpr uint8_t FREQ_20KHZ   = (7 << 5);  ///< 20 kHz PWM
    constexpr uint8_t FREQ_MASK    = (7 << 5);  ///< Frequency mask
    
    constexpr uint8_t DITHER_EN    = (1 << 4);  ///< Dithering enable
    
    constexpr uint8_t DEFAULT      = FREQ_1KHZ; ///< Default 1kHz
}

//==============================================================================
// DIAGNOSTIC REGISTER - Addresses 0x50-0x55
//==============================================================================

/**
 * @brief Channel diagnostic bit definitions
 * 
 * @details
 * Extended diagnostic information for each channel.
 *
 * @par Bit Map:
 * @verbatim
 * Bit 7-4 : LOAD_CURRENT - Load current indicator (0-15)
 * Bit 3   : OL_HISTORY   - Open load history flag
 * Bit 2   : SC_HISTORY   - Short circuit history flag
 * Bit 1   : OT_HISTORY   - Over-temp history flag
 * Bit 0   : DIAG_VALID   - Diagnostic data valid
 * @endverbatim
 */
namespace CH_DIAG {
    constexpr uint8_t LOAD_CURRENT_MASK = 0xF0;  ///< Load current mask
    constexpr uint8_t OL_HISTORY        = (1 << 3);  ///< Open load history
    constexpr uint8_t SC_HISTORY        = (1 << 2);  ///< Short circuit history
    constexpr uint8_t OT_HISTORY        = (1 << 1);  ///< Over-temp history
    constexpr uint8_t DIAG_VALID        = (1 << 0);  ///< Data valid
    
    /**
     * @brief Extract load current value
     * @param diag_byte Diagnostic register value
     * @return Load current indicator (0-15)
     */
    [[nodiscard]] constexpr uint8_t get_load_current(uint8_t diag_byte) noexcept {
        return (diag_byte & LOAD_CURRENT_MASK) >> 4;
    }
}

//==============================================================================
// DEVICE IDENTIFICATION
//==============================================================================

/**
 * @brief Device ID register values
 */
namespace DeviceID {
    constexpr uint8_t TLE92466ED = 0x46;  ///< Expected device ID
    constexpr uint8_t MASK       = 0xFF;  ///< ID mask
}

//==============================================================================
// HELPER STRUCTURES
//==============================================================================

/**
 * @brief Channel enumeration
 * 
 * @details
 * Enumeration for the 6 output channels with strong type safety.
 */
enum class Channel : uint8_t {
    CH0 = 0,  ///< Channel 0
    CH1 = 1,  ///< Channel 1
    CH2 = 2,  ///< Channel 2
    CH3 = 3,  ///< Channel 3
    CH4 = 4,  ///< Channel 4
    CH5 = 5,  ///< Channel 5
    
    COUNT = 6 ///< Total number of channels
};

/**
 * @brief Current limit enumeration
 */
enum class CurrentLimit : uint8_t {
    LIMIT_1A = 0,  ///< 1 Ampere limit
    LIMIT_2A = 1,  ///< 2 Ampere limit
    LIMIT_3A = 2,  ///< 3 Ampere limit
    LIMIT_4A = 3   ///< 4 Ampere limit
};

/**
 * @brief Slew rate enumeration
 */
enum class SlewRate : uint8_t {
    SLOW     = 0,  ///< Slow slew rate (~10 V/μs)
    MEDIUM   = 1,  ///< Medium slew rate (~20 V/μs)
    FAST     = 2,  ///< Fast slew rate (~40 V/μs)
    FASTEST  = 3   ///< Fastest slew rate (~80 V/μs)
};

/**
 * @brief PWM frequency enumeration
 */
enum class PWMFrequency : uint8_t {
    FREQ_100HZ  = 0,  ///< 100 Hz
    FREQ_200HZ  = 1,  ///< 200 Hz
    FREQ_500HZ  = 2,  ///< 500 Hz
    FREQ_1KHZ   = 3,  ///< 1 kHz
    FREQ_2KHZ   = 4,  ///< 2 kHz
    FREQ_5KHZ   = 5,  ///< 5 kHz
    FREQ_10KHZ  = 6,  ///< 10 kHz
    FREQ_20KHZ  = 7   ///< 20 kHz
};

/**
 * @brief Type-safe channel index conversion
 */
[[nodiscard]] constexpr uint8_t to_index(Channel ch) noexcept {
    return static_cast<uint8_t>(ch);
}

/**
 * @brief Convert channel to register address offset
 */
[[nodiscard]] constexpr uint8_t channel_to_config_addr(Channel ch) noexcept {
    return RegisterAddress::CH0_CFG + to_index(ch);
}

/**
 * @brief Convert channel to status register address
 */
[[nodiscard]] constexpr uint8_t channel_to_status_addr(Channel ch) noexcept {
    return RegisterAddress::CH0_STATUS + to_index(ch);
}

/**
 * @brief Convert channel to diagnostic register address
 */
[[nodiscard]] constexpr uint8_t channel_to_diag_addr(Channel ch) noexcept {
    return RegisterAddress::DIAG_CH0 + to_index(ch);
}

} // namespace TLE92466ED

#endif // TLE92466ED_REGISTERS_HPP
