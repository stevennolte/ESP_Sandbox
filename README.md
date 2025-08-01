# ESP32 IoT Device with OTA Updates

A comprehensive ESP32 IoT project featuring LED control, MQTT integration, web interface, and automatic OTA firmware updates from GitHub releases.

## Features

### Core Functionality
- **LED Indicator**: PWM-controlled brightness with web configuration
- **Temperature Monitoring**: Internal ESP32 CPU temperature via MQTT
- **Web Interface**: Responsive HTML interface for device configuration
- **MQTT Integration**: Home Assistant compatible with auto-discovery
- **OTA Updates**: Automatic firmware updates from GitHub releases

### Network Features
- **WiFi Connection**: Automatic connection with status monitoring
- **mDNS Support**: Access device via `http://[client-id].local`
- **MQTT Auto-Discovery**: Automatically finds Home Assistant server
- **Multi-Board Support**: ESP32 DevKit and Seeed Xiao ESP32S3

## Hardware Requirements

- ESP32 development board (DevKit or Seeed Xiao ESP32S3)
- DS18B20 temperature sensor (optional)
- Built-in LED (pin 2)

## Pin Configuration

| Component | Pin |
|-----------|-----|
| DS18B20 Data | GPIO 4 |
| DS18B20 Power | GPIO 21 |
| LED | GPIO 2 |

## Software Dependencies

### PlatformIO Libraries
- OneWire
- DallasTemperature
- WiFi
- ArduinoJson
- Preferences
- WebServer
- ESPmDNS
- LittleFS

### Custom Libraries
- ESPMQTTManager
- ESPOTAUpdater

## Configuration

### WiFi Settings
Update in `main.cpp`:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### MQTT Settings
```cpp
const char* mqtt_user = "YOUR_MQTT_USERNAME";
const char* mqtt_pass = "YOUR_MQTT_PASSWORD";
```

## Web Interface

Access the device via:
- IP address: `http://[device-ip]`
- mDNS: `http://[client-id].local`

### Available Endpoints
- `/` - Main configuration page
- `/set` - Update client ID (POST)
- `/brightness` - Update LED brightness (POST)
- `/reboot` - Restart device

## MQTT Topics

All topics use the format: `homeassistant/[component]/[client_id]/[entity]`

### Published Topics
- CPU Temperature: `homeassistant/sensor/[client_id]/cpu_temp/state`
- Firmware Version: `homeassistant/sensor/[client_id]/firmware/state`

### Auto-Discovery
The device automatically publishes Home Assistant discovery messages for:
- CPU temperature sensor
- Firmware version sensor

## OTA Updates

### Automatic Updates
- Checks for new firmware every 5 minutes
- Downloads from GitHub releases automatically
- Supports board-specific firmware files:
  - `firmware-esp32-devkit.bin` for ESP32 DevKit
  - `firmware-xiao-esp32s3.bin` for Seeed Xiao ESP32S3
  - `firmware.bin` as fallback

### Version Format
- Uses major.minor format (e.g., v9.5 = version 95)
- Automatically increments in GitHub Actions

## Build and Deploy

### PlatformIO
```bash
# Build for all environments
pio run

# Build specific environment
pio run -e esp32dev
pio run -e seeed_xiao_esp32s3

# Upload firmware
pio run -t upload
```

### GitHub Actions
Automatic build and release workflow:
1. Increments version number
2. Builds firmware for both board types
3. Creates GitHub release
4. Uploads firmware binaries

## File Structure

```
├── src/
│   └── main.cpp                 # Main application code
├── lib/
│   ├── ESPMQTTManager/         # MQTT management library
│   └── ESPOTAUpdater/          # OTA update library
├── data/
│   └── index.html              # Web interface template
├── platformio.ini              # PlatformIO configuration
├── firmware.json               # Version information
└── .github/workflows/          # CI/CD automation
```

## Development

### Adding New Features
1. Implement in `main.cpp` or create new library
2. Update firmware version in `main.cpp`
3. Test locally with PlatformIO
4. Commit and push - GitHub Actions handles the rest

### Debugging
- Serial output available at 115200 baud
- Web interface shows current status
- MQTT messages for monitoring

## License

This project is open source. See the repository for license details.
