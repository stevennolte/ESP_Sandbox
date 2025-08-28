# ESP_Sandbox Project Requirements Document

## Project Overview

This document outlines all the requirements, dependencies, and specifications needed to generate a new project based on the ESP_Sandbox project architecture. The ESP_Sandbox is a comprehensive ESP32 IoT platform with advanced WiFi management, web interface, OTA updates, status indication, and recovery mechanisms.

## Hardware Requirements

### Primary Target Platforms
- **ESP32 Development Boards**
  - ESP32 DevKit v1 (primary target)
  - ESP32-S3 DevKit C-1 (optional)
  - Seeed Xiao ESP32S3 (optional)

### Hardware Specifications
- **Minimum Flash Memory**: 4MB (for LittleFS filesystem)
- **RAM**: Minimum 520KB (ESP32 standard)
- **WiFi**: 802.11 b/g/n support
- **GPIO Requirements**:
  - LED control pin (default: GPIO 2)
  - Optional: WS2812 addressable LED support
  - Serial communication (UART)

### Optional Hardware Features
- **Status LEDs**: Single LED or WS2812 addressable RGB LED
- **Temperature Sensor**: Built-in ESP32 temperature sensor
- **External Sensors**: GPIO-based sensor integration capability

## Software Platform Requirements

### Development Environment
- **PlatformIO** (recommended) or Arduino IDE
- **Platform**: Espressif 32 (espressif32)
- **Framework**: Arduino
- **Toolchain**: ESP-IDF compatible

### Core Dependencies (platformio.ini)
```ini
platform = espressif32
framework = arduino
board_build.filesystem = littlefs
monitor_speed = 115200

lib_deps = 
    bblanchon/ArduinoJson@^6.21.0
    adafruit/Adafruit NeoPixel@^1.10.0
```

### Standard ESP32 Libraries (Built-in)
- **WiFi.h** - WiFi connectivity management
- **WebServer.h** - HTTP server functionality
- **Preferences.h** - Non-volatile storage
- **LittleFS.h** - File system operations
- **ESPmDNS.h** - Multicast DNS services
- **HTTPClient.h** - HTTP client operations
- **Update.h** - OTA update functionality
- **FS.h** - File system interface
- **esp_task_wdt.h** - Watchdog timer
- **esp_system.h** - System utilities

## Custom Library Components

The project includes several custom libraries that must be included:

### 1. WiFiHelper Library
- **Purpose**: Comprehensive WiFi management with AP fallback
- **Dependencies**: ArduinoJson, LittleFS
- **Features**:
  - Automatic WiFi connection with fallback to Access Point mode
  - Web-based WiFi configuration interface
  - Network scanning and selection
  - Template processing for web pages
  - Connection monitoring and recovery

### 2. StatusIndicator Library
- **Purpose**: Unified status indication for different LED types
- **Dependencies**: Adafruit NeoPixel
- **Features**:
  - Support for single LEDs and WS2812 addressable LEDs
  - Automatic board detection and configuration
  - Multiple status indication patterns
  - Brightness control and color management

### 3. SystemUtils Library
- **Purpose**: System information and diagnostics
- **Features**:
  - CPU temperature monitoring
  - Memory usage reporting
  - Board type detection
  - Uptime formatting
  - Hardware information retrieval

### 4. ESPOTAUpdater Library
- **Purpose**: Over-the-air firmware updates via GitHub
- **Dependencies**: ArduinoJson, HTTPClient
- **Features**:
  - GitHub-based firmware distribution
  - Automatic update checking
  - Board-specific firmware selection
  - Progress callbacks
  - Version management

### 5. Config System
- **Purpose**: Centralized configuration management
- **Features**:
  - Singleton pattern configuration
  - Persistent storage via Preferences
  - WiFi credentials management
  - Hardware configuration constants
  - Runtime parameter adjustment

## Configuration Constants Structure

### WiFi Configuration
```cpp
namespace ConfigConstants::WiFi {
    const int MAX_ATTEMPTS = 15;
    const int RECONNECT_ATTEMPTS = 20;
    const int RETRY_DELAY = 500;
    const unsigned long CHECK_INTERVAL = 30 * 1000;
    constexpr const char* DEFAULT_SSID = "YourNetwork";
    constexpr const char* DEFAULT_PASSWORD = "YourPassword";
    constexpr const char* AP_PASSWORD = "ESP32Config";
}
```

### Hardware Configuration
```cpp
namespace ConfigConstants::Hardware {
    const int LED_PIN = 2;
    const int LED_CHANNEL = 0;
    const int LED_FREQ = 5000;
    const int LED_RESOLUTION = 8;
    const int DEFAULT_LED_BRIGHTNESS = 128;
}
```

### Timing Configuration
```cpp
namespace ConfigConstants::Timing {
    const unsigned long LED_PULSE_DURATION = 50;
    const unsigned long MAIN_LOOP_DELAY = 1000;
    const unsigned long WATCHDOG_TIMEOUT = 30;
    const unsigned long REBOOT_DELAY = 3000;
}
```

### Firmware Management
```cpp
namespace ConfigConstants::Firmware {
    const int VERSION = 950; // Version number
    constexpr const char* GITHUB_REPO = "username/repository";
    constexpr const char* GITHUB_BRANCH = "main";
    const unsigned long UPDATE_INTERVAL = 5 * 60 * 1000;
}
```

## Core Functionality Requirements

### 1. WiFi Management
- **Automatic Connection**: Attempt connection to saved WiFi credentials
- **AP Fallback**: Switch to Access Point mode if connection fails
- **Web Configuration**: Browser-based WiFi setup interface
- **Network Scanning**: Discovery and selection of available networks
- **Reconnection Logic**: Automatic recovery from connection loss
- **Persistent Storage**: Save WiFi credentials to non-volatile memory

### 2. Web Server Interface
- **Configuration Pages**: WiFi setup, system configuration
- **Status Monitoring**: Real-time system status and diagnostics
- **File Management**: Upload/download files to/from device
- **Template System**: Dynamic HTML generation with variable replacement
- **API Endpoints**: JSON-based data exchange
- **Mobile Responsive**: Touch-friendly interface for mobile devices

### 3. OTA Update System
- **GitHub Integration**: Fetch firmware updates from repository
- **Version Management**: Track current and available firmware versions
- **Board Detection**: Download appropriate firmware for specific board type
- **Update Progress**: Real-time update progress reporting
- **Rollback Protection**: Recovery mechanisms for failed updates
- **Manual/Automatic**: Support both manual and automatic update modes

### 4. Status Indication
- **Connection Status**: Visual WiFi connection state
- **System Status**: Boot, running, error, and update states
- **LED Support**: Both single LED and addressable RGB LED arrays
- **Custom Patterns**: Configurable blink patterns and colors
- **Brightness Control**: User-adjustable LED brightness

### 5. System Monitoring
- **Watchdog Timer**: Software watchdog with configurable timeout
- **Recovery Mode**: Automatic recovery from system crashes
- **Temperature Monitoring**: CPU temperature sensor reading
- **Memory Tracking**: Free heap and usage monitoring
- **Uptime Tracking**: System runtime measurement
- **Debug Information**: Comprehensive system diagnostics

### 6. File System Management
- **LittleFS Integration**: Flash-based file system
- **Template Storage**: HTML templates for web interface
- **Configuration Files**: Persistent configuration storage
- **Update Management**: Template updates from GitHub
- **File Upload/Download**: Web-based file management

## Build Configuration

### PlatformIO Configuration
```ini
[platformio]
default_envs = esp32doit-devkit-v1

[env]
platform = espressif32
framework = arduino
monitor_speed = 115200
board_build.filesystem = littlefs

[env:esp32doit-devkit-v1]
board = esp32doit-devkit-v1
build_flags = 
    -DBOARD_HAS_PSRAM 
    -mfix-esp32-psram-cache-issue
    -DBOARD_TYPE=\"ESP32_DEVKIT\"
```

### Conditional Compilation Support
- **Board Type Detection**: Different configurations for different ESP32 variants
- **Feature Flags**: Enable/disable features based on hardware capabilities
- **Debug Modes**: Configurable debug output levels
- **Memory Optimization**: Conditional inclusion of features based on available resources

## Network and Communication

### WiFi Requirements
- **Station Mode**: Connect to existing WiFi networks
- **Access Point Mode**: Create temporary configuration hotspot
- **Dual Mode Support**: Seamless switching between modes
- **Multiple Network Support**: Store and attempt multiple WiFi credentials
- **Security**: Support for WPA/WPA2 encryption

### Web Server Requirements
- **HTTP Server**: Standard HTTP/1.1 support
- **MIME Types**: Support for HTML, CSS, JavaScript, JSON, and binary files
- **File Serving**: Static file serving from LittleFS
- **Form Processing**: POST data handling for configuration
- **CORS Support**: Cross-origin resource sharing for API access

### External Communication
- **GitHub API**: Access to repository releases and files
- **HTTP Client**: Outbound HTTP requests for updates
- **JSON Processing**: Parse and generate JSON data
- **SSL/TLS**: Secure communication for GitHub API

## Security Considerations

### Access Control
- **AP Mode Security**: Default password for configuration access point
- **Web Interface**: Basic authentication for sensitive operations
- **HTTPS Support**: SSL/TLS encryption for external communications
- **Input Validation**: Sanitize all user inputs

### Data Protection
- **Credential Storage**: Encrypted storage of WiFi passwords
- **Firmware Verification**: Validate firmware before installation
- **Recovery Mechanisms**: Protection against malicious updates
- **Factory Reset**: Complete configuration reset capability

## Development Guidelines

### Code Structure
- **Modular Design**: Separate libraries for distinct functionality
- **Singleton Pattern**: Single instance configuration management
- **Callback System**: Event-driven programming model
- **Error Handling**: Comprehensive error checking and recovery
- **Documentation**: Full Doxygen documentation for all public APIs

### Memory Management
- **Stack Usage**: Minimize stack usage in recursive functions
- **Heap Management**: Careful dynamic memory allocation
- **String Handling**: Efficient string operations
- **Buffer Management**: Fixed-size buffers where possible

### Performance Requirements
- **Boot Time**: Target < 10 seconds from power-on to operational
- **Web Response**: HTTP requests should complete within 5 seconds
- **WiFi Connection**: Establish connection within 30 seconds
- **Update Speed**: OTA updates should utilize available bandwidth efficiently

## Testing and Validation

### Functional Testing
- **WiFi Connection**: Test with multiple network types and configurations
- **Web Interface**: Validate all web pages and API endpoints
- **OTA Updates**: Test update process with various firmware versions
- **Recovery**: Validate watchdog and recovery mechanisms
- **File System**: Test file operations and template management

### Performance Testing
- **Memory Usage**: Monitor heap usage under various conditions
- **Network Performance**: Measure throughput and latency
- **Stability**: Long-term operation testing
- **Load Testing**: Multiple concurrent web connections

### Hardware Testing
- **Board Compatibility**: Test on all supported ESP32 variants
- **LED Functionality**: Validate status indication on different LED types
- **Temperature Monitoring**: Verify sensor readings
- **GPIO Operations**: Test all hardware interfaces

## Deployment Requirements

### Production Checklist
- [ ] Update default WiFi credentials
- [ ] Configure GitHub repository for OTA updates
- [ ] Set production firmware version number
- [ ] Configure board-specific build flags
- [ ] Validate all template files in data directory
- [ ] Test factory reset functionality
- [ ] Verify watchdog timer operation
- [ ] Confirm LED status indication
- [ ] Test web interface on target hardware

### Documentation Requirements
- **User Manual**: End-user operation instructions
- **API Documentation**: Complete function reference
- **Configuration Guide**: Setup and customization instructions
- **Troubleshooting Guide**: Common issues and solutions
- **Hardware Setup**: Wiring diagrams and component requirements

## Version Control and Release Management

### Repository Structure
```
project/
├── platformio.ini          # Build configuration
├── src/
│   ├── main.cpp            # Main application
│   └── Config.h            # Configuration management
├── lib/                    # Custom libraries
│   ├── WiFiHelper/
│   ├── StatusIndicator/
│   ├── SystemUtils/
│   └── ESPOTAUpdater/
├── data/                   # LittleFS files
│   └── index.html          # Web interface templates
├── include/                # Header files (if needed)
├── test/                   # Unit tests
└── docs/                   # Documentation
```

### Release Process
1. **Version Numbering**: Semantic versioning (MAJOR.MINOR.PATCH)
2. **GitHub Releases**: Tagged releases with firmware binaries
3. **Board-Specific Builds**: Separate binaries for each supported board
4. **Update Packages**: Include both firmware and template updates
5. **Release Notes**: Detailed changelog and upgrade instructions

This comprehensive requirements document provides all the necessary information to recreate or generate a new project based on the ESP_Sandbox architecture. Each component is designed to be modular and reusable, enabling rapid development of ESP32-based IoT devices with robust WiFi management, web interfaces, and remote update capabilities.
