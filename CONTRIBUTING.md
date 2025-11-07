---
layout: default
title: "🤝 Contributing"
description: "Guidelines and information for contributing to the HardFOC TLE92466ED Driver"
nav_order: 5
parent: "🔧 HardFOC TLE92466ED Driver"
permalink: /CONTRIBUTING/
---

# 🤝 Contributing to HardFOC TLE92466ED Driver

Thank you for your interest in contributing to the HardFOC TLE92466ED Driver!
This document provides guidelines and information for contributors.

## 📋 **Code Standards**

### 🎯 **Coding Style and Best Practices for HardFOC Development**

- **C++23 Standard Compliance** - All code must be compatible with C++23
- **Consistent Naming** - Follow the established naming conventions:
  - Classes: `PascalCase` (e.g., `TLE92466ED`, `HAL`)
  - Functions: `PascalCase` (e.g., `SetCurrent`, `GetDiagnostics`)
  - Variables: `snake_case` with trailing underscore for members (e.g., `channel_enable_`, `current_setting_`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `TLE92466ED_MAX_CURRENT`)
  - Types: Hardware-agnostic types where applicable

### 🏗️ **Architecture Guidelines**

- **Hardware Abstraction** - Use hardware-agnostic HAL interfaces
- **Error Handling** - All functions use `std::expected` for error handling
- **Safety** - Use `noexcept` where appropriate for safety-critical code
- **Dependencies** - Keep dependencies minimal (freestanding where possible)

## 🧪 **Testing**

### 🔧 **Unit Tests and Hardware Validation Requirements**

- **Unit Tests** - Write comprehensive unit tests for all new functionality
- **Hardware Testing** - Test on actual TLE92466ED hardware with ESP32
- **Integration Tests** - Verify compatibility with existing HardFOC systems
- **Performance Tests** - Ensure real-time performance requirements are met
- **Safety Tests** - Validate safety features and error handling

## 📖 **Documentation**

### 📚 **Documentation Standards and Updates**

- **API Documentation** - Update documentation for all public interfaces
- **User Guides** - Create or update guides for new features
- **Example Code** - Provide working examples for solenoid control applications
- **Architecture Documentation** - Document design decisions and patterns

## 🐛 **Bug Reports**

### 🔍 **How to Report Bugs Effectively**

When reporting bugs, please include:

1. **Hardware Information**: TLE92466ED board, ESP32 version, solenoid configuration
2. **Environment Details**: ESP-IDF version, compiler version, operating system
3. **Reproduction Steps**: Minimal code example, configuration settings
4. **Hardware Configuration**: Connected peripherals, pin assignments
5. **Debugging Information**: Error messages, log output, stack traces

## ✨ **Feature Requests**

### 🚀 **Proposing New Features and Enhancements**

When proposing new features:

1. **Use Case** - Describe the specific solenoid control use case
2. **Technical Specification** - Provide detailed technical requirements
3. **API Design** - Propose the interface design following established patterns
4. **Implementation Plan** - Outline the implementation approach
5. **Testing Strategy** - Describe how the feature will be tested

## 🔄 **Development Workflow**

### 📋 **Step-by-Step Development Process**

1. **Fork the Repository**
2. **Create a Feature Branch**
3. **Implement Your Changes with HardFOC-Specific Tests**
4. **Document Your Changes with HardFOC Examples**
5. **Submit a Pull Request**

## 📋 **Code Quality Standards for HardFOC**

- **C++23 Compliance** - Code compiles without warnings
- **HardFOC Compatibility** - Tested on HardFOC boards
- **Error Handling** - All error conditions handled appropriately using std::expected
- **Documentation** - All public APIs documented
- **Tests** - Adequate test coverage provided
- **Performance** - Real-time requirements met

---

## 🚀 Thank You for Contributing to HardFOC

Your contributions help make HardFOC motor controller boards more accessible and powerful for everyone.
