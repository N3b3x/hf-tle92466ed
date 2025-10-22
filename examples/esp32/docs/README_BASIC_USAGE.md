# Basic Usage Test Suite - TLE92466ED ESP32-C6

## Overview

The Basic Usage example is a **comprehensive test suite** that demonstrates fundamental
TLE92466ED operations on ESP32-C6 using a professional FreeRTOS-based test framework.
This suite provides structured validation of driver functionality with automatic result
tracking, GPIO14 progress indicators, and detailed execution timing.

## 🎯 Purpose

- **Validate** TLE92466ED driver functionality on ESP32-C6
- **Demonstrate** best practices for driver integration
- **Provide** professional test infrastructure for development
- **Enable** CI/CD integration with structured output
- **Document** expected behavior and performance

## 🧪 Test Framework Architecture

### Professional Test Infrastructure

The example uses the **TLE92466ED Test Framework** (`TLE92466ED_TestFramework.hpp`), which provides:

#### ✅ **FreeRTOS Task-Based Execution**
- Each test runs in isolated FreeRTOS task
- Custom stack size per test (8KB default)
- Automatic semaphore synchronization
- 30-second timeout protection
- Fallback to inline execution on failure

#### ✅ **GPIO14 Progress Indicator**
- Visual feedback on oscilloscope/logic analyzer
- **Toggle**: HIGH/LOW on each test completion
- **Blink Pattern**: 5 blinks at section start/end
- Hardware-level test progression tracking

#### ✅ **Automatic Result Tracking**
- Pass/fail counting
- Microsecond-precision execution timing
- Success rate calculation
- Professional summary reports

#### ✅ **Test Section Management**
- Compile-time section enable/disable
- Section-based organization
- Custom blink patterns per section
- Structured output formatting

## 📋 Test Suite Structure

### Test Sections

#### 1. **INITIALIZATION_TESTS** (`#define ENABLE_INITIALIZATION_TESTS 1`)
Tests fundamental setup and communication:
- `test_hal_initialization` - ESP32-C6 HAL setup and SPI configuration
- `test_driver_initialization` - TLE92466ED driver instance creation
- `test_chip_id` - SPI communication verification via chip ID

#### 2. **BASIC_OPERATION_TESTS** (`#define ENABLE_BASIC_OPERATION_TESTS 1`)
Tests core driver operations:
- `test_channel_enable_disable` - Channel control operations
- `test_current_setting` - Current configuration (100mA, 500mA, 1000mA, 1500mA)
- `test_diagnostics` - Real-time diagnostics reading

#### 3. **CURRENT_CONTROL_TESTS** (`#define ENABLE_CURRENT_CONTROL_TESTS 1`)
Tests advanced current control:
- `test_current_ramping` - Smooth 0→1000mA→0mA transitions

### Test Function Signatures

All test functions follow this pattern:
```cpp
static bool test_function_name() noexcept;
```text

**Key Characteristics**:
- Return `bool` (true = passed, false = failed)
- Declared `noexcept` for embedded safety
- Self-contained and isolated
- Descriptive ESP_LOG output
- Proper error handling with `std::expected`

## 🔧 Hardware Setup

### Required Components
- **ESP32-C6-DevKitC-1** development board
- **TLE92466ED** evaluation board or custom PCB
- **12-24V power supply** for VBAT (2A minimum)
- **Test load**: Solenoid, resistor (6-120Ω), or LED with current limiting
- **Oscilloscope/Logic Analyzer** (optional, for GPIO14 monitoring)
- Connecting wires

### Wiring Diagram

```text
ESP32-C6          TLE92466ED        Function
--------          ----------        --------
GPIO2    -------> MISO              SPI Data In
GPIO7    -------> MOSI              SPI Data Out
GPIO6    -------> SCLK              SPI Clock
GPIO10   -------> CS                Chip Select (active low)
GPIO14   -------> [LED/Scope]       Test Progress Indicator
3.3V     -------> VDD               Logic Supply
GND      -------> GND               Common Ground
         
External Supply   TLE92466ED
---------------   ----------
12-24V    ------>  VBAT             Power Supply
GND       ------>  GND              Power Ground

Test Load         TLE92466ED
---------         ----------
Load+     ------>  OUT0             Channel 0 Output
Load-     ------>  GND (via sense)  Return Path
```text

### Complete Pin Configuration

| Function | ESP32-C6 GPIO | TLE92466ED Pin | Configuration | Notes |
|----------|---------------|----------------|---------------|-------|
| **SPI MISO** | GPIO2 | MISO | Input | Data from TLE92466ED |
| **SPI MOSI** | GPIO7 | MOSI | Output | Data to TLE92466ED |
| **SPI SCLK** | GPIO6 | SCLK | Output | 1MHz SPI Clock |
| **SPI CS** | GPIO10 | CS | Output | Active Low |
| **Test Indicator** | GPIO14 | - | Output | Progress indicator |
| **Logic Supply** | 3.3V | VDD | Power | 3.3V ±10% |
| **Power Supply** | External | VBAT | Power | 8-28V, 2A+ |
| **Ground** | GND | GND | Ground | Common reference |

### GPIO14 Test Indicator Setup

```text
ESP32-C6 GPIO14 ──┬── LED (Anode)
                  │   └── LED (Cathode) ── 220Ω ── GND
                  │
                  └── Oscilloscope/Logic Analyzer Probe
```text

**Purpose**: Visual and measurable feedback showing test progression

## ⚙️ Configuration

### Hardware Configuration (`main/TLE92466ED_Config.hpp`)

All hardware-specific settings are centralized:

```cpp
namespace TLE92466ED_Config {

struct SPIPins {
    static constexpr int MISO = 2;   // GPIO2
    static constexpr int MOSI = 7;   // GPIO7
    static constexpr int SCLK = 6;   // GPIO6
    static constexpr int CS = 10;    // GPIO10
};

struct SPIParams {
    static constexpr int FREQUENCY = 1000000;  // 1MHz
    static constexpr int MODE = 0;             // SPI Mode 0
    static constexpr int QUEUE_SIZE = 7;
};

struct CurrentLimits {
    static constexpr uint16_t SINGLE_CHANNEL_MAX = 2000;   // 2A
    static constexpr uint16_t PARALLEL_CHANNEL_MAX = 4000; // 4A
};

struct HardwareSpecs {
    static constexpr float SUPPLY_VOLTAGE_MIN = 8.0f;    // 8V
    static constexpr float SUPPLY_VOLTAGE_MAX = 28.0f;   // 28V
    static constexpr int TEMPERATURE_MAX = 150;          // 150°C
};

} // namespace TLE92466ED_Config
```text

**To modify hardware settings**: Edit `main/TLE92466ED_Config.hpp`

### Test Configuration (`main/BasicUsageExample.cpp`)

Enable/disable test sections at compile time:

```cpp
#define ENABLE_INITIALIZATION_TESTS 1       // HAL, driver, chip ID
#define ENABLE_BASIC_OPERATION_TESTS 1      // Enable, current, diagnostics
#define ENABLE_CURRENT_CONTROL_TESTS 1      // Ramping tests
```text

**To disable a section**: Set to `0` and rebuild

### Build Configuration (`app_config.yml`)

Application metadata and build settings:

```yaml
apps:
  basic_usage:
    description: "Basic usage example for TLE92466ED"
    source_file: "BasicUsageExample.cpp"
    category: "demo"
    idf_versions: ["release/v5.5"]
    build_types: ["Debug", "Release"]
    ci_enabled: true
    featured: true
```text

**Note**: This is for build metadata only. Hardware config is in `TLE92466ED_Config.hpp`.

## 🚀 Building and Running

### Quick Start

```bash
cd examples/esp32

# Build the basic usage example
./scripts/build_app.sh basic_usage Debug

# Flash to ESP32-C6
./scripts/flash_app.sh basic_usage Debug

# Monitor output
./scripts/monitor_app.sh basic_usage
```text

### Manual Build

```bash
cd examples/esp32

# Set target
idf.py set-target esp32c6

# Configure (optional)
idf.py menuconfig

# Build with APP_TYPE=basic_usage
idf.py -DAPP_TYPE=basic_usage build

# Flash
idf.py -p /dev/ttyUSB0 flash

# Monitor
idf.py -p /dev/ttyUSB0 monitor
```text

### Build Targets

```bash
# Debug build (optimized for debugging)
idf.py -DAPP_TYPE=basic_usage -DCMAKE_BUILD_TYPE=Debug build

# Release build (optimized for performance)
idf.py -DAPP_TYPE=basic_usage -DCMAKE_BUILD_TYPE=Release build
```text

## 📊 Expected Output

### Professional Test Header

```text
╔══════════════════════════════════════════════════════════════════════════════╗
║              TLE92466ED BASIC USAGE TEST SUITE - ESP32-C6                   ║
║                         HardFOC Core Drivers                                 ║
╚══════════════════════════════════════════════════════════════════════════════╝
Target: esp32c6
ESP-IDF Version: v5.5.0

╔══════════════════════════════════════════════════════════════════════════════╗
║ TLE92466ED TEST CONFIGURATION                                                
╠══════════════════════════════════════════════════════════════════════════════╣
║ Test sections will execute based on compile-time configuration              ║
║ GPIO14 test progression indicator: ENABLED                                   ║
╚══════════════════════════════════════════════════════════════════════════════╝
```text

### Test Execution

```text
╔══════════════════════════════════════════════════════════════════════════════╗
║ SECTION: INITIALIZATION TESTS                                                ║
╚══════════════════════════════════════════════════════════════════════════════╝
[GPIO14: Blink pattern - 5 blinks]

╔══════════════════════════════════════════════════════════════════════════════╗
║ Running (task): hal_initialization                                           ║
╚══════════════════════════════════════════════════════════════════════════════╝
I (1234) TLE92466ED_Basic: Creating HAL instance...
I (1235) ESP32C6_HAL: ESP32C6_HAL created with SPI config
I (1236) ESP32C6_HAL:   MISO: GPIO2, MOSI: GPIO7, SCLK: GPIO6, CS: GPIO10
I (1237) ESP32C6_HAL:   Frequency: 1000000 Hz, Mode: 0
I (1238) TLE92466ED_Basic: Initializing HAL...
I (1240) ESP32C6_HAL: ESP32C6_HAL initialized successfully
I (1241) TLE92466ED_Basic: ✅ HAL initialized successfully
[SUCCESS] PASSED (task): hal_initialization (12.34 ms)
Test task completed: hal_initialization
[GPIO14: Toggle HIGH]

╔══════════════════════════════════════════════════════════════════════════════╗
║ Running (task): driver_initialization                                        ║
╚══════════════════════════════════════════════════════════════════════════════╝
I (1250) TLE92466ED_Basic: Creating TLE92466ED driver instance...
I (1251) TLE92466ED_Basic: Initializing TLE92466ED driver...
I (1275) TLE92466ED_Basic: ✅ TLE92466ED driver initialized successfully
[SUCCESS] PASSED (task): driver_initialization (25.67 ms)
Test task completed: driver_initialization
[GPIO14: Toggle LOW]

╔══════════════════════════════════════════════════════════════════════════════╗
║ Running (task): chip_id                                                      ║
╚══════════════════════════════════════════════════════════════════════════════╝
I (1280) TLE92466ED_Basic: Reading chip identification...
I (1285) TLE92466ED_Basic: ✅ Chip ID: 0x123456789ABC
[SUCCESS] PASSED (task): chip_id (5.23 ms)
Test task completed: chip_id
[GPIO14: Toggle HIGH]
```text

### Test Results Summary

```text
╔══════════════════════════════════════════════════════════════════════════════╗
║                           TEST RESULTS SUMMARY                               ║
╠══════════════════════════════════════════════════════════════════════════════╣
║  Total Tests:      7                                                         ║
║  Passed:           7                                                         ║
║  Failed:           0                                                         ║
║  Success Rate:     100.00%                                                   ║
║  Total Time:       234.56 ms                                                 ║
╚══════════════════════════════════════════════════════════════════════════════╝

I (5000) TLE92466ED_Basic: ✅ ALL TESTS PASSED! System will restart in 10 seconds...
```text

### GPIO14 Signal Pattern

```text
Time    │ GPIO14 │ Event
────────┼────────┼──────────────────────────────
0ms     │   LOW  │ Boot
100ms   │ BLINK  │ Section start (5 blinks)
        │   ↑↓   │ 
200ms   │  HIGH  │ Test 1 complete
300ms   │   LOW  │ Test 2 complete
400ms   │  HIGH  │ Test 3 complete
500ms   │ BLINK  │ Section 2 start (5 blinks)
600ms   │   LOW  │ Test 4 complete
...
```text

## 🧪 Individual Test Details

### 1. `test_hal_initialization()`

**Purpose**: Validate ESP32-C6 HAL creation and SPI initialization

**Operations**:
1. Create ESP32C6_HAL instance with configuration from `TLE92466ED_Config.hpp`
2. Initialize SPI peripheral (SPI2_HOST)
3. Configure GPIO pins for SPI
4. Set up SPI parameters (1MHz, Mode 0)

**Expected Result**: ✅ PASS
- HAL instance created successfully
- SPI bus initialized
- GPIO pins configured
- Ready for communication

**Failure Modes**:
- SPI bus already initialized
- GPIO pins in use
- Invalid SPI configuration
- Hardware fault

---

### 2. `test_driver_initialization()`

**Purpose**: Create TLE92466ED driver instance and initialize IC

**Operations**:
1. Create TLE92466ED driver instance with HAL reference
2. Execute driver initialization sequence
3. Configure TLE92466ED registers
4. Set up default parameters

**Expected Result**: ✅ PASS
- Driver instance created
- IC initialized successfully
- Registers configured
- Ready for operations

**Failure Modes**:
- HAL not initialized
- SPI communication failure
- TLE92466ED not powered
- IC initialization timeout

---

### 3. `test_chip_id()`

**Purpose**: Verify SPI communication by reading 48-bit chip ID

**Operations**:
1. Read chip identification register
2. Verify CRC-8 (SAE J1850)
3. Validate chip ID format

**Expected Result**: ✅ PASS
- Chip ID read successfully
- 48-bit value displayed (e.g., `0x123456789ABC`)
- CRC verified

**Failure Modes**:
- SPI communication error
- CRC mismatch
- Invalid chip ID
- TLE92466ED not responding

---

### 4. `test_channel_enable_disable()`

**Purpose**: Validate channel control operations

**Operations**:
1. Enable channel 0
2. Wait 1 second
3. Disable channel 0
4. Verify register writes

**Expected Result**: ✅ PASS
- Channel enabled successfully
- Channel disabled successfully
- No communication errors

**Failure Modes**:
- Register write failure
- SPI timeout
- Invalid channel number

---

### 5. `test_current_setting()`

**Purpose**: Test current configuration at multiple setpoints

**Operations**:
Test current settings at:
- 100mA
- 500mA
- 1000mA
- 1500mA

**Expected Result**: ✅ PASS
- All current values set successfully
- No register errors
- 500ms delay between steps

**Failure Modes**:
- Current out of range (0-2000mA)
- Register write failure
- Invalid channel

---

### 6. `test_diagnostics()`

**Purpose**: Verify diagnostics reading and fault detection

**Operations**:
1. Read diagnostics registers
2. Check for faults:
   - Overcurrent
   - Overtemperature
   - Open load
   - Short circuit
3. Report status

**Expected Result**: ✅ PASS (no faults expected)
- Diagnostics read successfully
- "All systems normal" (assuming proper load)

**Possible Warnings** (not test failures):
- ⚠️  Overcurrent detected (load too high)
- 🌡️  Overtemperature detected (thermal issue)
- 🔌  Open load (no load connected)
- ⚡  Short circuit (load shorted)

**Failure Modes**:
- Cannot read diagnostics
- SPI communication error

---

### 7. `test_current_ramping()`

**Purpose**: Demonstrate smooth current transitions

**Operations**:
1. Enable channel 0
2. Ramp UP: 0mA → 1000mA (100mA steps, 300ms each)
3. Ramp DOWN: 1000mA → 0mA (100mA steps, 300ms each)
4. Disable channel 0

**Expected Result**: ✅ PASS
- Smooth ramping completed
- All current values set successfully
- Channel disabled at end

**Failure Modes**:
- Current setting failure
- Channel enable/disable failure
- Timeout during ramping

## 🐛 Troubleshooting

### Common Issues

#### 1. **All Tests Fail** ❌

**Symptom**: Every test returns FAILED

**Root Causes**:
- TLE92466ED not powered (check VBAT: 12-24V)
- SPI wiring incorrect
- Common ground missing
- ESP32-C6 not programmed correctly

**Solutions**:
```bash
# Check wiring
- VBAT: 12-24V present
- VDD: 3.3V present
- GND: Common ground between ESP32-C6, TLE92466ED, and power supply
- SPI pins: MISO, MOSI, SCLK, CS connected correctly

# Re-flash
idf.py -p /dev/ttyUSB0 erase-flash
idf.py -p /dev/ttyUSB0 flash
```text

---

#### 2. **HAL Initialization Fails** ❌

**Symptom**: `test_hal_initialization` fails

**Root Causes**:
- SPI2 bus already in use
- GPIO pins conflicting
- Invalid SPI configuration

**Solutions**:
1. Check `sdkconfig` for SPI conflicts
2. Verify GPIO pins are not used elsewhere
3. Try different GPIO pins (modify `TLE92466ED_Config.hpp`)
4. Check ESP32-C6 hardware

---

#### 3. **Driver Initialization Fails** ❌

**Symptom**: `test_driver_initialization` fails

**Root Causes**:
- SPI communication not working
- TLE92466ED not responding
- Power supply issue

**Solutions**:
1. Verify SPI with logic analyzer/oscilloscope
2. Check VBAT voltage (must be 8-28V)
3. Check VDD voltage (must be 3.3V ±10%)
4. Verify TLE92466ED is not damaged

---

#### 4. **Chip ID Read Fails** ❌

**Symptom**: `test_chip_id` fails, "Failed to read chip ID"

**Root Causes**:
- SPI communication error
- TLE92466ED not powered
- CS pin not working

**Solutions**:
1. Measure SPI signals with oscilloscope:
   - SCLK: Should show 1MHz clock during transactions
   - MOSI: Should show data output
   - MISO: Should show data input
   - CS: Should go LOW during transactions
2. Check power supply voltages
3. Verify chip is not in reset or fault state

---

#### 5. **Diagnostics Show Faults** ⚠️

**Symptom**: Tests pass but diagnostics show warnings

**Overcurrent** ⚠️:
- Load drawing too much current
- Check load resistance (should be ≥6Ω for 2A max)
- Verify VBAT voltage

**Overtemperature** 🌡️:
- IC too hot (>150°C junction temp)
- Add heatsink
- Reduce duty cycle
- Lower ambient temperature

**Open Load** 🔌:
- No load connected (expected if testing without load)
- Loose connection
- Load resistance too high

**Short Circuit** ⚡:
- Load or wiring shorted to ground
- Check continuity
- Inspect for solder bridges

---

#### 6. **Current Ramping Fails** ❌

**Symptom**: `test_current_ramping` fails midway

**Root Causes**:
- Load specification issue
- Power supply can't deliver current
- Thermal shutdown during ramp

**Solutions**:
1. Use proper test load (resistor: 6-120Ω, 5W+)
2. Ensure power supply can deliver 2A+
3. Monitor diagnostics during ramp
4. Add cooling if needed

---

### Debug Configuration

#### Enable Verbose Logging

In `sdkconfig`:
```ini
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y
CONFIG_ESP_SYSTEM_PANIC_PRINT_HALT=y
```text

Or via menuconfig:
```bash
idf.py menuconfig
# Component config -> Log output -> Default log verbosity -> Debug
```text

#### SPI Signal Analysis

Use logic analyzer to verify:
- **SCLK**: 1MHz square wave during transactions
- **CS**: Active LOW during transactions
- **MOSI/MISO**: Data patterns visible
- **Timing**: Mode 0 (CPOL=0, CPHA=0)

#### GPIO14 Monitoring

Attach oscilloscope to GPIO14:
- Should toggle after each test
- Should show blink patterns at section boundaries
- Indicates test progression even if serial output fails

## 📈 Performance Characteristics

### Typical Execution Times

| Test | Expected Time | Max Time |
|------|--------------|----------|
| HAL Initialization | 10-15 ms | 50 ms |
| Driver Initialization | 20-30 ms | 100 ms |
| Chip ID Read | 5-10 ms | 20 ms |
| Channel Enable/Disable | 15-25 ms | 50 ms |
| Current Setting (4 values) | 40-60 ms | 100 ms |
| Diagnostics Read | 10-15 ms | 30 ms |
| Current Ramping | 60-80 ms | 150 ms |
| **Total Suite** | **200-250 ms** | **500 ms** |

### Memory Usage

| Resource | Usage | Notes |
|----------|-------|-------|
| **Flash** | ~55KB | Driver + HAL + test framework |
| **RAM (static)** | ~12KB | Driver instance + buffers |
| **Stack (per task)** | 8KB | Configurable in `RUN_TEST_IN_TASK` |
| **Heap (dynamic)** | ~4KB | HAL instance + std::unique_ptr |

### SPI Performance

- **Clock Frequency**: 1MHz (configurable 100kHz - 8MHz)
- **Transaction Time**: ~32μs per 32-bit frame
- **Register Read**: ~100μs (including overhead)
- **Register Write**: ~80μs (including overhead)

## 🔗 Related Documentation

### ESP32 Examples
- [Multi-Channel Test Suite](README_MULTI_CHANNEL.md) - Multiple channel control tests
- [Test Framework Reference](../main/TLE92466ED_TestFramework.hpp) - Framework API
- [Hardware Configuration](../main/TLE92466ED_Config.hpp) - Pin and parameter config
- [ESP32-C6 HAL](../main/ESP32C6_HAL.hpp) - HAL implementation details

### Driver Documentation
- [Driver API Reference](../../../docs/07_Driver_API.md) - Complete API documentation
- [HAL Implementation Guide](../../../docs/08_HAL_Implementation.md) - Porting to new platforms
- [Register Map](../../../docs/03_Register_Map.md) - TLE92466ED register details
- [Diagnostics](../../../docs/06_Diagnostics.md) - Fault detection and handling

### Build System
- [App Configuration](../app_config.yml) - Build metadata
- [CMake Setup](../CMakeLists.txt) - Build system configuration
- [Component Structure](../components/tle92466ed-espidf/) - ESP-IDF component

## 🎓 Learning Path

### Beginner
1. ✅ **Start Here**: Basic Usage Test Suite
2. Run tests and observe GPIO14 indicators
3. Modify test current values
4. Try with different loads

### Intermediate
1. Modify `TLE92466ED_Config.hpp` for custom hardware
2. Add new test functions
3. Configure test sections
4. Analyze SPI signals with oscilloscope

### Advanced
1. Implement custom HAL for different MCU
2. Extend test framework for CI/CD
3. Create application-specific tests
4. Optimize performance and memory

## 📊 CI/CD Integration

### Structured Output

The test suite produces structured output suitable for parsing:

```text
[SUCCESS] PASSED (task): test_name (12.34 ms)
[FAILED] FAILED (task): test_name (45.67 ms)
```text

### Exit Codes

- **0**: All tests passed
- **Non-zero**: Tests failed (after restart)

### Parsing Results

```bash
# Extract pass/fail count
grep "Total Tests:" output.log
grep "Success Rate:" output.log

# Check for failures
grep "\[FAILED\]" output.log && echo "Tests failed!" || echo "All passed!"
```text

---

**Example**: Basic Usage Test Suite  
**Platform**: ESP32-C6  
**Framework**: FreeRTOS + Test Framework  
**Complexity**: ⭐⭐⭐☆☆  
**Status**: ✅ Production Ready
