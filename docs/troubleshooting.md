# Troubleshooting

This guide helps you diagnose and resolve common issues when using the TLE92466ED driver.

## Common Error Messages

### Error: Initialization Failed

**Symptoms:**
- `Init()` returns `DriverError::NotInitialized` or `DriverError::DeviceNotResponding`
- Driver not responding to SPI

**Causes:**
- SPI interface not properly initialized
- Hardware connections incorrect
- Power supply issues
- RESN pin held low

**Solutions:**
1. **Verify SPI Interface**: Ensure SPI interface is initialized before creating driver
2. **Check Connections**: Verify all SPI connections (SCK, SI, SO, CSN)
3. **Verify Power**: Check power supply voltages (VBAT: 5.5V-41V, VIO: 3.0V-5.5V)
4. **Check RESN Pin**: Ensure RESN is released (HIGH) for SPI communication
5. **Verify SPI Mode**: Ensure SPI Mode 1 (CPOL=0, CPHA=1)
6. **Check SPI Speed**: Try lower SPI speed (e.g., 1 MHz)

---

### Error: Communication Error / CRC Error

**Symptoms:**
- `DriverError::CRCError` or `DriverError::SPIFrameError` returned
- No response from device
- CRC mismatches

**Causes:**
- SPI configuration incorrect
- Signal integrity issues
- CS timing problems
- CRC calculation mismatch

**Solutions:**
1. **Check SPI Mode**: Ensure SPI Mode 1 (CPOL=0, CPHA=1)
2. **Verify Speed**: Try lower SPI speed (e.g., 1 MHz, max 10 MHz)
3. **Check CS Timing**: Verify CS assertion/deassertion timing
4. **Verify Connections**: Check all SPI connections are secure
5. **Check CRC**: Verify CRC-8 (SAE J1850) calculation matches device
6. **Check Frame Format**: Ensure 32-bit frames are used

---

### Error: Channel Not Working

**Symptoms:**
- Channel enabled but no output
- Current not flowing
- No voltage on OUTx pin

**Causes:**
- Channel not properly configured
- Channel not enabled
- Load not connected correctly
- Fault condition
- Wrong mode (Config vs Mission)

**Solutions:**
1. **Check Mode**: Ensure in Mission Mode (`EnterMissionMode()`)
2. **Check Configuration**: Verify channel configuration is correct
3. **Check Enable State**: Ensure channel is enabled via `EnableChannel()`
4. **Verify Load**: Check load connections (OUTx to load, load to VBAT)
5. **Check Faults**: Read fault status to identify issues
6. **Check Channel Mode**: Ensure channel mode is set (e.g., ICC mode)

---

### Error: Overcurrent Protection

**Symptoms:**
- `ChannelDiagnostics.overcurrent` is true
- Channel disabled automatically
- Current reading shows fault

**Causes:**
- Load current exceeds limits
- Short circuit
- Incorrect current settings
- Parallel mode not configured correctly

**Solutions:**
1. **Check Load**: Verify load is within specifications (2A max single, 4A parallel)
2. **Reduce Current**: Lower current setpoint
3. **Check for Shorts**: Verify no short circuits in wiring
4. **Clear Fault**: Read diagnostics to clear, then reconfigure
5. **Check Parallel Mode**: If using parallel, ensure `SetParallelOperation()` called

---

### Error: Open Load Detection

**Symptoms:**
- `ChannelDiagnostics.open_load` is true
- No current flow
- Load not detected

**Causes:**
- Load not connected
- Broken connection
- Load impedance too high
- Open load threshold too sensitive

**Solutions:**
1. **Check Connections**: Verify load is properly connected
2. **Check Wiring**: Inspect for broken wires
3. **Verify Load**: Ensure load impedance is appropriate
4. **Adjust Threshold**: Increase open load threshold if false positives

---

### Error: Wrong Mode

**Symptoms:**
- `DriverError::WrongMode` returned
- Operation not allowed

**Causes:**
- Attempting channel control in Config Mode
- Attempting configuration in Mission Mode
- Mode transition not completed

**Solutions:**
1. **Check Current Mode**: Use `IsMissionMode()` or `IsConfigMode()`
2. **Enter Correct Mode**: 
   - For channel control: `EnterMissionMode()`
   - For configuration: `EnterConfigMode()`
3. **Wait for Transition**: Allow time for mode transition to complete

---

### Error: Invalid Parameter

**Symptoms:**
- `DriverError::InvalidParameter` returned
- Invalid channel number
- Parameter out of range

**Causes:**
- Channel number out of range (must be 0-5)
- Current value out of range
- Invalid configuration values

**Solutions:**
1. **Check Channel Number**: Use `Channel::CH0` through `Channel::CH5`
2. **Check Current Range**: 
   - Single mode: 0-2000 mA
   - Parallel mode: 0-4000 mA
3. **Verify Parameters**: Check all configuration values are within valid ranges

---

### Error: Device Not Responding

**Symptoms:**
- `DriverError::DeviceNotResponding` returned
- No SPI response
- Timeout errors

**Causes:**
- SPI bus not working
- Device not powered
- CS line issue
- RESN pin held low

**Solutions:**
1. **Check Power**: Verify VBAT and VIO are within range
2. **Check RESN**: Ensure RESN is HIGH (not in reset)
3. **Check CS**: Verify CS line is properly controlled
4. **Check SPI Bus**: Test SPI bus with simple read/write
5. **Check Connections**: Verify all SPI connections

---

### Error: Wrong Device ID

**Symptoms:**
- `DriverError::WrongDeviceID` returned
- Device ID mismatch

**Causes:**
- Wrong device connected
- Device not fully powered up
- Communication error

**Solutions:**
1. **Verify Device**: Check that TLE92466ED is connected
2. **Wait for Power-Up**: Allow time for device power-up
3. **Check Communication**: Verify SPI communication is working
4. **Read ID Manually**: Use `GetIcVersion()` to check device ID

---

## Debugging Tips

### Enable Logging

```cpp
// Enable driver logging (if supported by your SPI interface)
hal->SetLogLevel(tle92466ed::LogLevel::Debug);
```

### Read Device Status

```cpp
if (auto status = driver.GetDeviceStatus(); status) {
    printf("Config Mode: %d\n", status->config_mode);
    printf("Init Done: %d\n", status->init_done);
    printf("Any Fault: %d\n", status->any_fault);
}
```

### Read All Faults

```cpp
if (auto faults = driver.GetAllFaults(); faults) {
    driver.PrintAllFaults();  // Prints formatted fault report
}
```

### Verify Device

```cpp
if (auto verified = driver.VerifyDevice(); verified && *verified) {
    printf("Device verified\n");
} else {
    printf("Device verification failed\n");
}
```

## Common Configuration Mistakes

1. **Forgetting to Enter Mission Mode**: Channels can only be enabled in Mission Mode
2. **Configuring in Wrong Mode**: Channel configuration requires Config Mode
3. **Not Setting Channel Mode**: Must set channel mode before setting current
4. **Parallel Mode Mismatch**: Forgetting to set `parallel_mode = true` when using parallel pairs
5. **Current Out of Range**: Setting current above 2000mA in single mode

## Next Steps

- Review [Hardware Setup](hardware_setup.md) for wiring verification
- Check [Platform Integration](platform_integration.md) for SPI interface issues
- See [Examples](examples.md) for working code samples

---

**Navigation**
⬅️ [Examples](examples.md) | [Back to Index](index.md)

