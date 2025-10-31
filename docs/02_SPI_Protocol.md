---
layout: default
title: "📡 SPI Protocol"
description: "32-bit SPI communication protocol with CRC-8 (SAE J1850)"
nav_order: 2
parent: "📚 Documentation"
permalink: /docs/02_SPI_Protocol/
---

# SPI Protocol

## Overview

The TLE92466ED uses a 32-bit SPI protocol with 8-bit CRC (SAE J1850) for robust
communication. This is a full-duplex, synchronous serial interface operating in
SPI Mode 0.

### Protocol Specifications

| Parameter | Value | Notes |
|-----------|-------|-------|
| Frame Size | 32 bits | Single transaction |
| CRC | 8-bit SAE J1850 | Polynomial 0x1D |
| Mode | 0 (CPOL=0, CPHA=0) | Data sampled on rising edge |
| Frequency | 100 kHz - 10 MHz | Typical: 1-2 MHz |
| Bit Order | MSB first | Most significant bit first |
| CS Polarity | Active low | Pull low during transfer |

## Frame Structure

### 32-Bit Frame Layout

```text
   Bit Position:  31  30  29  28  27  26  25  24  23  22  21  20  19  18  17  16  15 ...  0
                 ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬─────────┐
   MOSI (Write): │              CRC (8)              │    ADDR (7)   │R/W│   DATA (16)   │
                 └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴─────────┘
                 
                 ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬─────────┐
   MISO (Reply): │              CRC (8)              │RM │ STATUS(5) │R/W│   DATA (16)   │
                 └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴─────────┘

   Field Breakdown:
   ┌──────────────┬──────────┬─────────────────────────────────────────────┐
   │ Bits [31:24] │ CRC      │ 8-bit CRC checksum (SAE J1850)              │
   │ Bits [23:17] │ ADDRESS  │ 7-bit register address (MOSI)               │
   │ Bits [23:22] │ RPLY_MOD │ Reply mode indicator (MISO)                 │
   │ Bits [21:17] │ STATUS   │ 5-bit status field (MISO)                   │
   │ Bit  [16]    │ R/W      │ 1=Write, 0=Read                             │
   │ Bits [15:0]  │ DATA     │ 16-bit data payload                         │
   └──────────────┴──────────┴─────────────────────────────────────────────┘
```text

### MOSI (Master Out, Slave In) - Write Frame

```text
    Write Transaction Format:

    31                24  23        17  16  15                 0
    ┌───────────────────┬─────────────┬───┬───────────────────┐
    │     CRC (8)       │  ADDR (7)   │ 1 │    DATA (16)      │
    └───────────────────┴─────────────┴───┴───────────────────┘
         Computed            Target       Write    Value to
         checksum           register      flag     write

    Example: Write 0x1234 to register 0x0002 (GLOBAL_CONFIG)
    
    Step 1: Build frame without CRC
      Bits [23:17] = 0x01      (0x0002 >> 3 = 0x01, upper 7 bits)
      Bit  [16]    = 1         (Write operation)
      Bits [15:0]  = 0x1234    (Data to write)
      
    Step 2: Calculate CRC on bits [23:0]
      Input: 0x00 0x02 0x34 (3 bytes, address + R/W + data)
      CRC  : 0xXX (computed)
      
    Step 3: Complete frame
      [31:24] = CRC, [23:0] = 0x012341234
```text

### MOSI (Master Out, Slave In) - Read Frame

```text
    Read Transaction Format:

    31                24  23        17  16  15                 0
    ┌───────────────────┬─────────────┬───┬───────────────────┐
    │     CRC (8)       │  Don't Care │ 0 │    ADDRESS (16)   │
    └───────────────────┴─────────────┴───┴───────────────────┘
         Computed           Ignored    Read    Full register
         checksum                      flag    address

    Example: Read register 0x0100 (CH0 SETPOINT)
    
    Step 1: Build frame without CRC
      Bits [23:17] = Don't care
      Bit  [16]    = 0         (Read operation)
      Bits [15:0]  = 0x0100    (Register address)
      
    Step 2: Calculate CRC on bits [23:0]
      Input: 0x00 0x00 0x01 0x00 (3 bytes)
      CRC  : 0xXX (computed)
      
    Step 3: Complete frame
      [31:24] = CRC, [23:0] = 0x00000100
```text

### MISO (Master In, Slave Out) - Reply Frame Types

#### Type 1: 16-Bit Reply Frame (Standard)

```text
    31                24  23 22  21        17  16  15                 0
    ┌───────────────────┬──┬──┬────────────┬───┬───────────────────┐
    │     CRC (8)       │ 0│ 0│ STATUS (5) │R/W│    DATA (16)      │
    └───────────────────┴──┴──┴────────────┴───┴───────────────────┘
         Computed        Reply  Status      Echo   Register
         checksum        Mode   flags       R/W    contents

    Reply Mode [23:22] = 00b: Standard 16-bit data
    Status [21:17]: Error/status indication
    R/W [16]: Echoes request R/W bit
    Data [15:0]: Register value or last write acknowledgment
```text

#### Type 2: 22-Bit Reply Frame (Extended Data)

```text
    31                24  23 22  21                             0
    ┌───────────────────┬──┬──┬──────────────────────────────────┐
    │     CRC (8)       │ 0│ 1│       EXTENDED DATA (22)        │
    └───────────────────┴──┴──┴──────────────────────────────────┘
         Computed        Reply     Extended feedback data
         checksum        Mode      (e.g., current measurements)

    Reply Mode [23:22] = 01b: Extended 22-bit data
    Used for: High-resolution feedback registers
```text

#### Type 3: Critical Fault Frame

```text
    31                24  23 22  21                             0
    ┌───────────────────┬──┬──┬──────────────────────────────────┐
    │     CRC (8)       │ 1│ 0│        Don't Care               │
    └───────────────────┴──┴──┴──────────────────────────────────┘
         Computed        Reply     Undefined
         checksum        Mode

    Reply Mode [23:22] = 10b: Critical fault condition
    Indicates: Severe hardware fault, device in safe state
```text

## SPI Status Codes

### Status Field [21:17] Encoding

```text
    Status Bits: 5-bit field in MISO reply

    ┌──────┬─────────────────────────────────────────────────┐
    │ Code │ Meaning                                         │
    ├──────┼─────────────────────────────────────────────────┤
    │00000 │ No error - transaction successful               │
    │00001 │ SPI frame error - invalid frame format          │
    │00010 │ Parity/CRC error - checksum mismatch            │
    │00011 │ Write to read-only register - access denied     │
    │00100 │ Internal bus fault - hardware issue             │
    │00101 │ Internal bus fault - alternate code             │
    │00110 │ Internal bus fault - alternate code             │
    │xxxxx │ Other codes reserved                            │
    └──────┴─────────────────────────────────────────────────┘

    Priority: Lower encoding = higher priority
    (If multiple errors, lowest code is reported)
```text

## CRC Calculation

### SAE J1850 CRC-8 Algorithm

```text
    Polynomial: 0x1D (x^8 + x^4 + x^3 + x^2 + 1)
    Init Value: 0xFF
    Final XOR:  0xFF
    Bit Order:  MSB first

    Algorithm Pseudocode:
    ┌────────────────────────────────────────────────────────┐
    │ function calculate_crc8(data[], length):               │
    │     crc = 0xFF              // Initialize              │
    │     for each byte in data:                             │
    │         crc = crc XOR byte  // Mix in data byte        │
    │         for bit = 0 to 7:   // Process 8 bits          │
    │             if crc & 0x80:  // Check MSB               │
    │                 crc = (crc << 1) XOR 0x1D              │
    │             else:                                       │
    │                 crc = (crc << 1)                       │
    │     return crc XOR 0xFF     // Final inversion         │
    └────────────────────────────────────────────────────────┘
```text

### CRC Calculation Example

```text
    Example: Calculate CRC for write to GLOBAL_CONFIG
    
    Frame contents (before CRC):
      Address [23:17] = 0x01
      R/W [16]        = 1
      Data [15:0]     = 0x4005
      
    Bytes to CRC (bits [23:0] = 3 bytes):
      Byte 0: 0x02      (bits [23:16])
      Byte 1: 0x40      (bits [15:8])
      Byte 2: 0x05      (bits [7:0])
      
    Step-by-step:
      crc = 0xFF
      
      Process 0x02:
        crc = 0xFF XOR 0x02 = 0xFD
        [bit processing yields]
        crc = 0xC5
        
      Process 0x40:
        crc = 0xC5 XOR 0x40 = 0x85
        [bit processing yields]
        crc = 0x7A
        
      Process 0x05:
        crc = 0x7A XOR 0x05 = 0x7F
        [bit processing yields]
        crc = 0x23
        
      Final XOR:
        crc = 0x23 XOR 0xFF = 0xDC
        
    Result: CRC = 0xDC
    Complete frame: 0xDC024005
```text

## Communication Timing

### SPI Timing Diagram

```text
    CSN     ─────────┐                                         ┌──────
                     └─────────────────────────────────────────┘
                       tCSS                           tCSH
                     
    SCK     ─────────┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌───────
                     │  │  │  │  │  │  │  │  │  │  │  │  │  │
                     └──┘  └──┘  └──┘  └──┘  └──┘  └──┘  └──┘
                      tSCKH tSCKL      32 clock cycles
                       
    SI/SO   ─────────┤ B31│ B30│ B29│ ... │ B2 │ B1 │ B0 ├──────
                     └────┴────┴────┴─────┴────┴────┴────┘
                      tSU  tH
    
    Timing Parameters:
    ┌────────────────────┬──────┬─────────┬──────┬──────┐
    │ Parameter          │ Min  │ Typical │ Max  │ Unit │
    ├────────────────────┼──────┼─────────┼──────┼──────┤
    │ tSCK (Clock period)│ 100  │   -     │  -   │  ns  │
    │ tCSS (CS setup)    │  50  │   -     │  -   │  ns  │
    │ tCSH (CS hold)     │  50  │   -     │  -   │  ns  │
    │ tSU (Data setup)   │  20  │   -     │  -   │  ns  │
    │ tH (Data hold)     │  20  │   -     │  -   │  ns  │
    │ tCSI (CS inactive) │ 100  │   -     │  -   │  ns  │
    └────────────────────┴──────┴─────────┴──────┴──────┘
```text

### Transaction Sequence

```text
    Complete Read/Write Sequence:

    Transaction 1: Write Command
    ───────────────────────────────────────────────────────────
    MCU → IC:  [WRITE CMD: 0xXX + CRC + ADDR + DATA]
    IC → MCU:  [RESPONSE: Previous register state]
    
    Transaction 2: Read Back (optional verify)
    ───────────────────────────────────────────────────────────
    MCU → IC:  [READ CMD: 0xXX + CRC + ADDR]
    IC → MCU:  [RESPONSE: Current register state]
    
    Timing:
    CSN  ┐   ┌───┐   ┌───
         └───┘   └───┘
          T1      T2
          
    Notes:
    - Response to write contains previous value
    - Read returns current value from addressed register
    - Minimum 100ns between transactions
```text

## Transaction Examples

### Example 1: Initialize Global Configuration

```text
    Objective: Enable CRC, watchdogs, 3.3V VIO
    
    Register: GLOBAL_CONFIG (0x0002)
    Value: 0x4005 (CLK_WD_EN | SPI_WD_EN | CRC_EN)
    
    Transaction Breakdown:
    ┌────────────────────────────────────────────────────────┐
    │ Step 1: Build MOSI frame                               │
    │   Address: 0x0002 → [23:17] = 0x00, [16] = 1 (write)  │
    │   Data: 0x4005                                         │
    │   Bytes for CRC: [0x02, 0x40, 0x05]                   │
    │   CRC: 0xDC (calculated)                               │
    │   Complete: 0xDC024005                                 │
    │                                                        │
    │ Step 2: SPI Transfer                                   │
    │   MOSI: 0xDC024005                                     │
    │   MISO: 0xXXXXXXXX (previous register value + status) │
    │                                                        │
    │ Step 3: Verify (optional)                              │
    │   Read back GLOBAL_CONFIG to confirm                   │
    └────────────────────────────────────────────────────────┘
```text

### Example 2: Read Channel Status

```text
    Objective: Read CH0 current setpoint
    
    Register: CH0_SETPOINT (0x0100)
    
    Transaction Breakdown:
    ┌────────────────────────────────────────────────────────┐
    │ Step 1: Build MOSI frame                               │
    │   Address: 0x0100 → [15:0] = 0x0100, [16] = 0 (read)  │
    │   Bytes for CRC: [0x00, 0x01, 0x00]                   │
    │   CRC: 0xXX (calculated)                               │
    │   Complete: 0xXX000100                                 │
    │                                                        │
    │ Step 2: SPI Transfer                                   │
    │   MOSI: 0xXX000100                                     │
    │   MISO: 0xYY000567 (register value = 0x0567)          │
    │           └─┬─┘                                        │
    │            CRC                                         │
    │                                                        │
    │ Step 3: Verify CRC & Extract Data                      │
    │   Status [21:17]: Check for errors                     │
    │   Data [15:0]: 0x0567 = current setpoint              │
    └────────────────────────────────────────────────────────┘
```text

### Example 3: Error Handling

```text
    Scenario: Write to read-only register
    
    Transaction:
      MOSI: Write to ICVID (0x0200, read-only)
      MISO: Status = 0b00011 (Write to RO register)
      
    Response Handling:
    ┌────────────────────────────────────────────────────────┐
    │ Parse MISO frame:                                       │
    │   [31:24]: CRC → Verify                                │
    │   [23:22]: Reply Mode = 00 (normal)                    │
    │   [21:17]: Status = 0b00011 → ERROR!                   │
    │   [16]:    R/W echo                                     │
    │   [15:0]:  Undefined (ignore)                          │
    │                                                        │
    │ Error Response:                                         │
    │   - Retry not recommended                              │
    │   - Check register address                             │
    │   - Verify register is writable                        │
    └────────────────────────────────────────────────────────┘
```text

## Best Practices

### Transaction Management

1. **CRC Verification**
   ```
   Always verify:
   - Outgoing CRC before transmission
   - Incoming CRC after reception
   - Reject frames with CRC mismatch
   ```text

2. **Status Checking**
   ```text
   After every transaction:
   - Parse status field [21:17]
   - Handle errors appropriately
   - Log unexpected status codes
   ```

3. **Timing Compliance**
   ```text
   Ensure:
   - Minimum CS inactive time (100ns)
   - Proper setup/hold times
   - Clock frequency within limits
   ```

4. **Error Recovery**
   ```text
   On error:
   - Check CRC first
   - Verify register address
   - Retry with exponential backoff (max 3)
   - Reset communication if persistent
   ```

### Performance Optimization

```text
Recommended SPI Frequency:
┌─────────────────────┬──────────────────────────────────┐
│ Application         │ Frequency                        │
├─────────────────────┼──────────────────────────────────┤
│ Debug/Development   │ 100-500 kHz (easier to debug)    │
│ Normal Operation    │ 1-2 MHz (good balance)           │
│ High Performance    │ 5-10 MHz (maximum throughput)    │
└─────────────────────┴──────────────────────────────────┘

Transaction Rate:
- 32 bits @ 1 MHz = 32 µs per transaction
- +100ns CS gap = ~33 µs total
- Max rate: ~30,000 transactions/second
```text

---

**Navigation**: [← IC Overview](01_IC_Overview.md) | [Next: Register Map →](03_Register_Map.md)
