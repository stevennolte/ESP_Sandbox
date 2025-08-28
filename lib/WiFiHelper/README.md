# WiFiHelper Library

A comprehensive WiFi management library for ESP32 platforms that simplifies WiFi connection handling, provides automatic fallback to Access Point mode, and includes a complete web interface for configuration.

## Features

- **🚀 Easy Setup**: Single function call to initialize WiFi with automatic fallback
- **🌐 Dual Mode**: Seamlessly switches between Station and Access Point modes
- **🔧 Web Interface**: Complete web-based configuration with modern UI
- **📡 Network Scanning**: Built-in WiFi network discovery and selection
- **⚡ Auto-Recovery**: Automatic reconnection with configurable retry logic
- **🎨 Template System**: Customizable web pages with variable replacement
- **📊 Status Monitoring**: Comprehensive connection and system status reporting
- **💾 Flexible Configuration**: Works standalone or integrates with existing config systems

## Quick Start

### Basic Usage

```cpp
#include <WiFiHelper.h>

void setup() {
    Serial.begin(115200);
    
    // Initialize WiFi with default settings
    WiFiHelper::setup();
    
    if (WiFiHelper::isConnected()) {
        Serial.println("Connected to WiFi!");
        Serial.println("IP: " + WiFiHelper::getLocalIP());
    }
}

void loop() {
    // Monitor connection health
    WiFiHelper::checkConnection();
    delay(30000); // Check every 30 seconds
}
```

### With Web Interface

```cpp
#include <WiFiHelper.h>
#include <WebServer.h>

WebServer server(80);

void setup() {
    Serial.begin(115200);
    WiFiHelper::setup();
    
    // Add WiFi management endpoints
    server.on("/wifi", []() { WiFiHelper::handleConfig(server); });
    server.on("/wifi-update", HTTP_POST, []() { WiFiHelper::handleUpdate(server); });
    server.on("/scan-networks", []() { WiFiHelper::handleNetworkScan(server); });
    
    server.begin();
}

void loop() {
    server.handleClient();
    WiFiHelper::checkConnection();
}
```

## Installation

### PlatformIO

1. Copy the `WiFiHelper` folder to your project's `lib/` directory
2. Add to your `platformio.ini`:

```ini
lib_deps = 
    ArduinoJson
    LittleFS
```

### Arduino IDE

1. Copy the `WiFiHelper` folder to your Arduino libraries directory
2. Install dependencies:
   - ArduinoJson library
   - ESP32 LittleFS library

## API Reference

### Core Functions

#### `WiFiHelper::setup()`
Initialize WiFi with default or configured settings. Attempts to connect to saved network, falls back to AP mode if connection fails.

#### `WiFiHelper::isConnected()`
Returns `true` if connected to WiFi network, `false` otherwise.

#### `WiFiHelper::checkConnection()`
Monitor and maintain WiFi connection. Call periodically to ensure connection stability.

#### `WiFiHelper::getLocalIP()`
Returns current IP address as String. Works in both Station and AP modes.

### Status Functions

#### `WiFiHelper::getWiFiStatus()`
Returns human-readable connection status string.

#### `WiFiHelper::getConnectionInfo()`
Returns detailed connection information including IP, signal strength, and network details.

#### `WiFiHelper::getRSSI()`
Returns signal strength in dBm. Returns 0 if not connected.

#### `WiFiHelper::getSSID()`
Returns current network SSID or AP name.

### Mode Management

#### `WiFiHelper::isWiFiConnected()`
Returns `true` if in Station mode and connected to WiFi.

#### `WiFiHelper::isAccessPointMode()`
Returns `true` if currently operating in Access Point mode.

#### `WiFiHelper::switchToAccessPoint()`
Manually switch to Access Point mode.

#### `WiFiHelper::switchToStation()`
Attempt to switch to Station mode with saved credentials.

### Web Interface Handlers

#### `WiFiHelper::handleConfig(WebServer& server)`
Serve WiFi configuration page with network scanning and connection options.

#### `WiFiHelper::handleUpdate(WebServer& server)`
Process WiFi credential updates from web form submissions.

#### `WiFiHelper::handleNetworkScan(WebServer& server)`
Return JSON list of available networks for dynamic web interfaces.

#### `WiFiHelper::handleWiFiModeToggle(WebServer& server)`
Handle requests to switch between Station and Access Point modes.

### Template Processing

#### `WiFiHelper::processWiFiTemplateVariables(String html)`
Replace template variables in HTML strings with current WiFi status and information.

**Available Variables:**
- `{{WIFI_STATUS}}` - Current connection status
- `{{WIFI_SSID}}` - Current network name
- `{{WIFI_IP}}` - Current IP address
- `{{WIFI_RSSI}}` - Signal strength
- `{{WIFI_MAC}}` - Device MAC address
- `{{AP_IP}}` - Access Point IP address
- `{{CLIENT_COUNT}}` - Number of connected clients (AP mode)

## Configuration

### Default Settings

WiFiHelper includes built-in defaults that work out of the box:

```cpp
namespace WiFiConstants {
    const char* DEFAULT_SSID = "YourNetwork";
    const char* DEFAULT_PASSWORD = "YourPassword";
    const char* DEFAULT_CLIENT_ID = "ESP32_Device";
    const char* AP_SSID = "ESP32_Setup";
    const char* AP_PASSWORD = "setup123";
    const IPAddress AP_IP = IPAddress(192, 168, 4, 1);
    const int MAX_WIFI_RETRIES = 3;
    const int WIFI_TIMEOUT_MS = 10000;
}
```

### Custom Configuration

For advanced usage, WiFiHelper can integrate with external configuration systems:

```cpp
// Define your config class
class MyConfig {
public:
    const char* getWiFiSSID() const { return "MyNetwork"; }
    const char* getWiFiPassword() const { return "MyPassword"; }
    const char* getClientId() const { return "MyESP32"; }
};

// Make it available to WiFiHelper (modify library as needed)
MyConfig config;
MyConfig* g_config = &config;
```

## Examples

The library includes comprehensive examples:

### BasicWiFiSetup.ino
- Simple WiFi connection with web interface
- Status LED integration
- Network scanning and selection
- Template-based web pages

### AdvancedWiFiConfig.ino
- Persistent configuration storage
- Advanced web interface with AJAX updates
- Integration with Preferences library
- Factory reset functionality
- Live status monitoring

## Web Interface

WiFiHelper provides a complete web interface accessible at the device's IP address:

### Main Features
- **Network Scanner**: Discover and connect to available WiFi networks
- **Mode Switching**: Toggle between Station and Access Point modes
- **Status Monitoring**: Real-time connection and system information
- **Configuration Management**: Update WiFi credentials and settings

### Customization

Web pages use a template system that allows easy customization:

```cpp
String html = WiFiHelper::loadTemplate("my_page.html");
html = WiFiHelper::processWiFiTemplateVariables(html);
server.send(200, "text/html", html);
```

## Troubleshooting

### Common Issues

**Connection Fails**
- Verify SSID and password are correct
- Check signal strength with `WiFiHelper::getRSSI()`
- Review debug output with `WiFiHelper::getDebugInfo()`

**Web Interface Not Accessible**
- Confirm device IP with `WiFiHelper::getLocalIP()`
- Check if device is in AP mode: `WiFiHelper::isAccessPointMode()`
- Verify web server is started and handling requests

**Frequent Disconnections**
- Increase retry count in configuration
- Check router compatibility and signal strength
- Monitor connection with `WiFiHelper::checkConnection()`

### Debug Information

Use the debug functions for troubleshooting:

```cpp
Serial.println(WiFiHelper::getDebugInfo());
Serial.println(WiFiHelper::getWiFiStatusInfo());
Serial.println(WiFiHelper::getConnectionInfo());
```

## Dependencies

- **ESP32 Arduino Core**: 2.0.0 or newer
- **ArduinoJson**: 6.0.0 or newer
- **LittleFS**: Included with ESP32 core

## Hardware Requirements

- ESP32 development board
- Minimum 4MB flash memory (for LittleFS storage)
- WiFi antenna (built-in on most development boards)

## License

MIT License - see LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## Version History

### v1.0.0
- Initial release
- Complete WiFi management functionality
- Web interface with network scanning
- Template processing system
- Comprehensive documentation and examples

---

**WiFiHelper Library** - Simplifying ESP32 WiFi management one connection at a time! 🚀
