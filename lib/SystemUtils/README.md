# SystemUtils Library

A comprehensive utility library for ESP32 system information and hardware diagnostics.

## Overview

The SystemUtils library provides static methods to retrieve various system information including CPU temperature, memory usage, chip details, and formatting utilities for uptime and byte sizes. This library is designed as a utility class with static methods only.

## Features

### System Information
- **CPU Temperature**: Read internal temperature sensor
- **Board Type**: Get board identifier from build configuration
- **System Info**: Comprehensive system summary
- **Chip Info**: Detailed ESP32 chip specifications
- **Memory Info**: Flash and heap memory statistics

### Utility Functions
- **Uptime Formatting**: Convert milliseconds to human-readable format
- **Byte Formatting**: Convert byte counts to KB/MB with appropriate units

## Usage

```cpp
#include "SystemUtils.h"

void setup() {
    Serial.begin(115200);
    
    // Get comprehensive system information
    Serial.println("=== System Information ===");
    Serial.println(SystemUtils::getSystemInfo());
    
    // Get specific information
    Serial.print("CPU Temperature: ");
    Serial.print(SystemUtils::readCPUTemperature());
    Serial.println("°C");
    
    // Format uptime
    String uptime = SystemUtils::formatUptime(millis());
    Serial.println("Uptime: " + uptime);
    
    // Format memory sizes
    String freeHeap = SystemUtils::formatBytes(ESP.getFreeHeap());
    Serial.println("Free Heap: " + freeHeap);
}
```

## API Reference

### System Information Functions

#### `readCPUTemperature()`
- **Returns**: `float` - Temperature in Celsius
- **Description**: Reads ESP32 internal temperature sensor
- **Note**: Not highly accurate, for monitoring purposes only

#### `getBoardType()`
- **Returns**: `String` - Board type identifier
- **Description**: Returns board type from BOARD_TYPE definition or "Unknown"

#### `getSystemInfo()`
- **Returns**: `String` - Multi-line system summary
- **Description**: Comprehensive system information including hardware specs and status

#### `getChipInfo()`
- **Returns**: `String` - Multi-line chip details
- **Description**: ESP32-specific hardware information (model, revision, cores, frequency)

#### `getMemoryInfo()`
- **Returns**: `String` - Multi-line memory statistics
- **Description**: Flash and heap memory usage information

### Utility Functions

#### `formatUptime(unsigned long milliseconds)`
- **Parameters**: `milliseconds` - Time in milliseconds
- **Returns**: `String` - Formatted uptime (e.g., "1d 2h 30m 45s")
- **Description**: Converts milliseconds to human-readable uptime format

#### `formatBytes(size_t bytes)`
- **Parameters**: `bytes` - Number of bytes
- **Returns**: `String` - Formatted size with units (B, KB, MB)
- **Description**: Automatically formats byte counts with appropriate units

## Dependencies

- Arduino.h (ESP32 Arduino Core)
- ESP32 hardware abstraction layer

## Notes

- All methods are static - no instantiation required
- Temperature sensor accuracy varies between ESP32 chips
- Memory information reflects current heap state
- Uptime calculation based on millis() function

## Version

**Version**: 1.0.0  
**Author**: ESP_Sandbox Project  
**Date**: 2025
