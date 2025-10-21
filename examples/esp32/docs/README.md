# TLE92466ED ESP32 Examples Documentation

## 📚 Documentation Index

Complete documentation for TLE92466ED driver examples on ESP32-C6.

---

## 🎯 Quick Start

| Document | Purpose | When to Read |
|----------|---------|--------------|
| **[Basic Usage Test Suite](README_BASIC_USAGE.md)** | Your first example | Start here! |
| **[Configuration Guide](README_CONFIGURATION.md)** | All configuration options | Before customizing |
| **[Test Framework Reference](README_TEST_FRAMEWORK.md)** | Framework API details | Writing custom tests |

---

## 📖 Documentation Structure

### 1. [Basic Usage Test Suite](README_BASIC_USAGE.md)

**Comprehensive guide to the basic usage example**

#### Contents:
- ✅ **Test Framework Architecture** - How the test infrastructure works
- ✅ **Test Suite Structure** - All test sections and functions
- ✅ **Hardware Setup** - Complete wiring diagrams and pin configurations
- ✅ **Configuration** - Hardware and test settings
- ✅ **Building and Running** - Build commands and workflows
- ✅ **Expected Output** - Full example outputs with explanations
- ✅ **Individual Test Details** - Deep dive into each test
- ✅ **Troubleshooting** - Common issues and solutions
- ✅ **Performance Characteristics** - Timing and memory usage

**Start here if you're new to the examples!**

---

### 2. [Configuration Guide](README_CONFIGURATION.md)

**Complete reference for all configuration files and options**

#### Contents:
- ✅ **Hardware Configuration** (`TLE92466ED_Config.hpp`) - SPI pins, frequencies, current limits
- ✅ **Application Configuration** (`app_config.yml`) - Build metadata and app definitions
- ✅ **Test Section Configuration** - Compile-time test enable/disable
- ✅ **ESP-IDF SDK Configuration** (`sdkconfig`) - Framework settings
- ✅ **Component Configuration** - ESP-IDF component setup
- ✅ **Configuration Workflows** - Development and production setups
- ✅ **Troubleshooting** - Configuration-related issues

**Read this when you need to customize hardware or build settings.**

---

### 3. [Test Framework Reference](README_TEST_FRAMEWORK.md)

**Detailed API reference for the TLE92466ED Test Framework**

#### Contents:
- ✅ **GPIO14 Progress Indicator** - Visual feedback system
- ✅ **Test Result Tracking** - Automatic pass/fail counting
- ✅ **Test Execution Macros** - `RUN_TEST`, `RUN_TEST_IN_TASK`, sections
- ✅ **Usage Guide** - How to write test functions
- ✅ **Advanced Usage** - Custom contexts, stack sizes, timeouts
- ✅ **Best Practices** - Professional test organization
- ✅ **Performance Impact** - Memory and timing overhead
- ✅ **CI/CD Integration** - Automated testing

**Essential reading for creating custom tests or understanding the framework.**

---

## 🚀 Getting Started

### Prerequisites

- **Hardware**:
  - ESP32-C6-DevKitC-1 development board
  - TLE92466ED evaluation board or custom PCB
  - 12-24V power supply (2A minimum)
  - Test load (solenoid, resistor, or LED)
  - USB-C cable for programming

- **Software**:
  - ESP-IDF v5.3 or later (v5.5 recommended)
  - Python 3.8+
  - Git with submodules support
  - Serial terminal (screen, minicom, or built-in monitor)

### Quick Start Steps

1. **Clone and setup**:
   ```bash
   git clone https://github.com/N3b3x/hf-tle92466ed-driver.git
   cd hf-tle92466ed-driver/examples/esp32
   git submodule update --init --recursive
   ```

2. **Configure ESP-IDF**:
   ```bash
   . $IDF_PATH/export.sh
   idf.py set-target esp32c6
   ```

3. **Build basic example**:
   ```bash
   ./scripts/build_app.sh basic_usage Debug
   ```

4. **Flash and monitor**:
   ```bash
   ./scripts/flash_app.sh basic_usage Debug
   ./scripts/monitor_app.sh basic_usage
   ```

5. **See results**:
   - Serial output shows test execution
   - GPIO14 toggles with test progress
   - Summary printed at end

---

## 📊 Example Overview

### BasicUsageExample

**Purpose**: Validate TLE92466ED driver functionality with comprehensive test suite

**Test Sections**:
1. **INITIALIZATION_TESTS** - HAL setup, driver init, chip ID verification
2. **BASIC_OPERATION_TESTS** - Channel control, current setting, diagnostics
3. **CURRENT_CONTROL_TESTS** - Current ramping and smooth transitions

**Features**:
- ✅ FreeRTOS task-based test execution
- ✅ GPIO14 progress indicator
- ✅ Automatic result tracking
- ✅ Professional formatted output
- ✅ 30-second timeout per test
- ✅ Compile-time test section control

**Complexity**: ⭐⭐⭐☆☆

**Read**: [Basic Usage Test Suite](README_BASIC_USAGE.md)

---

### MultiChannelExample

**Purpose**: Demonstrate independent control of all 6 TLE92466ED channels

**Features** (planned):
- Sequential channel activation
- Synchronized channel control
- Wave patterns across channels
- Performance monitoring
- Advanced diagnostics

**Complexity**: ⭐⭐⭐⭐☆

**Status**: 🚧 In Development

---

## 🔧 Configuration Overview

### Hardware Configuration

**File**: `main/TLE92466ED_Config.hpp`

**What's configured**:
- SPI GPIO pins (MISO, MOSI, SCLK, CS)
- SPI frequency and mode
- Current limits (single and parallel)
- Hardware specifications (voltage, temperature)
- Test parameters

**Example**:
```cpp
struct SPIPins {
    static constexpr int MISO = 2;   // GPIO2
    static constexpr int MOSI = 7;   // GPIO7
    static constexpr int SCLK = 6;   // GPIO6
    static constexpr int CS = 10;    // GPIO10
};
```

**Read**: [Configuration Guide - Hardware Configuration](README_CONFIGURATION.md#1-hardware-configuration-maintle92466ed_confighpp)

---

### Build Configuration

**File**: `app_config.yml`

**What's configured**:
- Available applications (basic_usage, multi_channel, etc.)
- Build types (Debug, Release)
- ESP-IDF version compatibility
- CI/CD settings
- Documentation links

**Example**:
```yaml
apps:
  basic_usage:
    description: "Basic usage example for TLE92466ED"
    source_file: "BasicUsageExample.cpp"
    build_types: ["Debug", "Release"]
```

**Read**: [Configuration Guide - Application Configuration](README_CONFIGURATION.md#2-application-configuration-app_configyml)

---

### Test Configuration

**File**: Each example source file (e.g., `BasicUsageExample.cpp`)

**What's configured**:
- Test section enable/disable flags
- Compile-time test selection

**Example**:
```cpp
#define ENABLE_INITIALIZATION_TESTS 1     // Enable
#define ENABLE_BASIC_OPERATION_TESTS 1    // Enable
#define ENABLE_CURRENT_CONTROL_TESTS 0    // Disable
```

**Read**: [Configuration Guide - Test Section Configuration](README_CONFIGURATION.md#3-test-section-configuration)

---

## 🧪 Test Framework Features

### Professional Test Infrastructure

The examples use a **production-grade test framework** with:

#### ✅ FreeRTOS Task-Based Execution
- Each test runs in isolated task
- Custom stack size per test (default: 8KB)
- Automatic semaphore synchronization
- 30-second timeout protection

#### ✅ GPIO14 Progress Indicator
- **Visual feedback** on oscilloscope/logic analyzer
- **Toggle**: HIGH/LOW on test completion
- **Blink patterns**: 5 blinks at section boundaries
- Hardware-level progress tracking

#### ✅ Automatic Result Tracking
- Pass/fail counting
- Microsecond-precision execution timing
- Success rate calculation
- Professional formatted summaries

#### ✅ Test Section Management
- Compile-time section enable/disable
- Organized test grouping
- Custom blink patterns per section

**Read**: [Test Framework Reference](README_TEST_FRAMEWORK.md)

---

## 📈 Example Output

### Test Execution

```
╔══════════════════════════════════════════════════════════════════╗
║         TLE92466ED BASIC USAGE TEST SUITE - ESP32-C6            ║
╚══════════════════════════════════════════════════════════════════╝
Target: esp32c6
ESP-IDF Version: v5.5.0

╔══════════════════════════════════════════════════════════════════╗
║ SECTION: INITIALIZATION TESTS                                    ║
╚══════════════════════════════════════════════════════════════════╝
[GPIO14: Blink pattern - 5 blinks]

╔══════════════════════════════════════════════════════════════════╗
║ Running (task): hal_initialization                               ║
╚══════════════════════════════════════════════════════════════════╝
I (1234) TLE92466ED_Basic: Creating HAL instance...
I (1235) TLE92466ED_Basic: ✅ HAL initialized successfully
[SUCCESS] PASSED (task): hal_initialization (12.34 ms)
[GPIO14: Toggle HIGH]

... (more tests) ...

╔══════════════════════════════════════════════════════════════════╗
║                    TEST RESULTS SUMMARY                          ║
╠══════════════════════════════════════════════════════════════════╣
║  Total Tests:      7                                             ║
║  Passed:           7                                             ║
║  Failed:           0                                             ║
║  Success Rate:     100.00%                                       ║
║  Total Time:       234.56 ms                                     ║
╚══════════════════════════════════════════════════════════════════╝

I (5000) TLE92466ED_Basic: ✅ ALL TESTS PASSED!
```

---

## 🐛 Troubleshooting Quick Reference

### Common Issues

| Issue | Quick Fix | Full Guide |
|-------|-----------|------------|
| **All tests fail** | Check VBAT power (12-24V) and wiring | [Basic Usage - Troubleshooting](README_BASIC_USAGE.md#-troubleshooting) |
| **HAL init fails** | Verify GPIO pins in `TLE92466ED_Config.hpp` | [Configuration - GPIO Conflict](README_CONFIGURATION.md#gpio-conflict) |
| **Chip ID read fails** | Check SPI wiring, CS pin | [Basic Usage - Chip ID Fails](README_BASIC_USAGE.md#4-chip-id-read-fails-) |
| **Stack overflow** | Increase stack size in `RUN_TEST_IN_TASK` | [Configuration - Stack Overflow](README_CONFIGURATION.md#stack-overflow) |
| **Build errors** | Clean and rebuild: `idf.py fullclean && idf.py build` | [Configuration - Build Type Issues](README_CONFIGURATION.md#build-type-issues) |

---

## 📦 Project Structure

```
examples/esp32/
├── app_config.yml                  # Build metadata
├── CMakeLists.txt                  # Main CMake configuration
├── sdkconfig                       # ESP-IDF SDK configuration
│
├── main/                           # Application source
│   ├── CMakeLists.txt             # Main component build
│   ├── TLE92466ED_Config.hpp      # Hardware configuration
│   ├── TLE92466ED_TestFramework.hpp  # Test infrastructure
│   ├── ESP32C6_HAL.hpp/.cpp       # ESP32-C6 HAL implementation
│   ├── BasicUsageExample.cpp      # Basic test suite
│   └── MultiChannelExample.cpp    # Multi-channel tests
│
├── components/                     # ESP-IDF components
│   └── tle92466ed-espidf/         # TLE92466ED driver component
│       ├── CMakeLists.txt
│       ├── idf_component.yml
│       └── README.md
│
├── docs/                           # Documentation
│   ├── README.md                  # This file
│   ├── README_BASIC_USAGE.md      # Basic usage guide
│   ├── README_CONFIGURATION.md    # Configuration reference
│   └── README_TEST_FRAMEWORK.md   # Test framework API
│
└── scripts/                        # Build/flash scripts (submodule)
    ├── build_app.sh
    ├── flash_app.sh
    └── monitor_app.sh
```

---

## 🔗 Related Documentation

### Driver Documentation
- [Driver Overview](../../../README.md) - Main driver README
- [Driver API Reference](../../../docs/07_Driver_API.md) - Complete API docs
- [HAL Implementation Guide](../../../docs/08_HAL_Implementation.md) - Porting guide
- [Register Map](../../../docs/03_Register_Map.md) - TLE92466ED registers
- [Diagnostics](../../../docs/06_Diagnostics.md) - Fault detection

### ESP32 Resources
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [ESP32-C6 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32-c6_technical_reference_manual_en.pdf)
- [FreeRTOS Documentation](https://www.freertos.org/Documentation/RTOS_book.html)

### TLE92466ED Resources
- [TLE92466ED Datasheet](../../../Datasheet/)
- [Infineon Product Page](https://www.infineon.com/cms/en/product/power/motor-control-ics/intelligent-motor-control-ics/multi-half-bridge-ics/tle92466ed/)

---

## 🎓 Learning Path

### Beginner Path

1. **Read**: [Basic Usage Test Suite](README_BASIC_USAGE.md)
2. **Build**: Basic usage example (Debug mode)
3. **Run**: Flash to ESP32-C6 and observe output
4. **Modify**: Change current values in tests
5. **Experiment**: Try with different loads

### Intermediate Path

1. **Read**: [Configuration Guide](README_CONFIGURATION.md)
2. **Customize**: Modify `TLE92466ED_Config.hpp` for your hardware
3. **Configure**: Adjust test sections and parameters
4. **Optimize**: Try Release build, measure performance
5. **Debug**: Use oscilloscope to observe SPI and GPIO14

### Advanced Path

1. **Read**: [Test Framework Reference](README_TEST_FRAMEWORK.md)
2. **Extend**: Add custom test functions
3. **Port**: Implement HAL for different MCU (ESP32-S3, etc.)
4. **Integrate**: Set up CI/CD pipeline with test parsing
5. **Optimize**: Profile and optimize for your application

---

## 💡 Tips and Best Practices

### Development Tips

1. **Start with Debug builds** for detailed logging
2. **Use GPIO14 indicator** to track progress without serial
3. **Monitor SPI signals** with logic analyzer during bring-up
4. **Test with known-good load** (resistor) before real solenoids
5. **Enable all test sections** initially, disable as needed

### Configuration Tips

1. **Keep hardware config centralized** in `TLE92466ED_Config.hpp`
2. **Use compile-time flags** for test sections (faster than runtime)
3. **Document custom settings** in code comments
4. **Version control sdkconfig.defaults** for team consistency
5. **Use Release builds** for final performance testing

### Testing Tips

1. **Run full test suite** after any changes
2. **Check GPIO14 patterns** on oscilloscope for test flow
3. **Analyze execution times** to find performance issues
4. **Test with realistic loads** matching your application
5. **Monitor diagnostics** for fault conditions

---

## 🆘 Getting Help

### Documentation Issues

If documentation is unclear or incomplete:
1. Check the specific guide (Basic Usage, Configuration, Test Framework)
2. Search for keywords in all docs
3. Review related driver documentation
4. Open GitHub issue with details

### Build/Configuration Issues

If you have build or configuration problems:
1. Check [Configuration Guide - Troubleshooting](README_CONFIGURATION.md#troubleshooting-configuration)
2. Verify ESP-IDF version (v5.3+)
3. Clean and rebuild: `idf.py fullclean && idf.py build`
4. Check `sdkconfig` for conflicts

### Hardware/Test Issues

If tests fail or hardware doesn't work:
1. Check [Basic Usage - Troubleshooting](README_BASIC_USAGE.md#-troubleshooting)
2. Verify wiring against pin configuration
3. Measure power supplies (VBAT, VDD)
4. Test SPI communication with logic analyzer
5. Try lower SPI frequency (100kHz) for debugging

### Support Channels

- **GitHub Issues**: Bug reports, feature requests
- **GitHub Discussions**: Questions, general help
- **Driver Documentation**: Technical reference

---

## 📊 Example Statistics

### BasicUsageExample

| Metric | Value |
|--------|-------|
| **Total Tests** | 7 |
| **Test Sections** | 3 |
| **Typical Execution Time** | 200-250ms |
| **Flash Size** | ~55KB |
| **RAM Usage** | ~12KB |
| **Stack per Task** | 8KB |

### Test Breakdown

| Section | Tests | Time |
|---------|-------|------|
| **Initialization** | 3 | ~40ms |
| **Basic Operations** | 3 | ~100ms |
| **Current Control** | 1 | ~70ms |

---

## 🎉 Success Criteria

You've successfully set up the examples when:

- ✅ Build completes without errors
- ✅ Flash and monitor show output
- ✅ All tests pass (100% success rate)
- ✅ GPIO14 toggles during execution
- ✅ Test summary shows timing information
- ✅ System restarts automatically after tests

Example success output:
```
╔══════════════════════════════════════════════════════════════════╗
║                    TEST RESULTS SUMMARY                          ║
╠══════════════════════════════════════════════════════════════════╣
║  Total Tests:      7                                             ║
║  Passed:           7                                             ║
║  Failed:           0                                             ║
║  Success Rate:     100.00%                                       ║
║  Total Time:       234.56 ms                                     ║
╚══════════════════════════════════════════════════════════════════╝

✅ ALL TESTS PASSED! System will restart in 10 seconds...
```

---

**Documentation Version**: 1.0.0  
**Last Updated**: 2025-10-21  
**Status**: ✅ Complete

**Next**: Start with [Basic Usage Test Suite](README_BASIC_USAGE.md) →

