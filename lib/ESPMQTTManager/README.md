# ESPMQTTManager Library

A comprehensive MQTT management library for ESP32 devices with Home Assistant auto-discovery functionality.

## Features

- **Automatic Home Assistant Discovery**: Scans the network to find Home Assistant servers
- **Connection Management**: Handles MQTT connection, reconnection, and error recovery
- **Topic Organization**: Automatically organizes topics using client ID structure
- **Timing Management**: Built-in timing for periodic publishing and server discovery
- **Customizable Callbacks**: Support for custom reboot and message handling
- **Easy Integration**: Simple API for quick integration into existing projects

## Installation

1. Copy the `ESPMQTTManager` folder to your Arduino libraries directory or PlatformIO lib folder
2. Include the library in your sketch: `#include <ESPMQTTManager.h>`

## Dependencies

- PubSubClient
- WiFi (ESP32)
- HTTPClient (ESP32)

## Basic Usage

```cpp
#include <WiFi.h>
#include <ESPMQTTManager.h>

// MQTT credentials
const char* mqtt_user = "your_username";
const char* mqtt_pass = "your_password";

// Create MQTT manager instance
ESPMQTTManager mqttManager(mqtt_user, mqtt_pass);

void setup() {
    Serial.begin(115200);
    
    // Connect to WiFi first
    WiFi.begin("your_ssid", "your_password");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    // Initialize MQTT manager
    mqttManager.begin("ESP_DeviceID");
    
    // Optional: Discover Home Assistant server
    String serverIP = mqttManager.discoverServer();
    mqttManager.updateServerIP(serverIP);
}

void loop() {
    // Ensure MQTT connection
    if (!mqttManager.isConnected()) {
        mqttManager.connect();
    }
    mqttManager.loop();
    
    unsigned long currentTime = millis();
    
    // Publish temperature periodically
    if (mqttManager.shouldPublishTemperature(currentTime)) {
        float temp = 25.5; // Your temperature reading
        mqttManager.publishCpuTemperature(temp);
        mqttManager.updateLastPublishTime(currentTime);
    }
    
    delay(1000);
}
```

## API Reference

### Constructor

```cpp
ESPMQTTManager(const char* username, const char* password, const char* fallbackIP = "192.168.1.12", int port = 1883)
```

### Initialization

- `void begin(String clientId)` - Initialize the MQTT manager with a client ID
- `void setRebootCallback(void (*callback)())` - Set custom reboot callback function

### Connection Management

- `bool connect()` - Connect to MQTT broker
- `void disconnect()` - Disconnect from MQTT broker
- `bool isConnected()` - Check if connected to MQTT broker
- `void loop()` - Must be called in main loop to handle MQTT messages

### Server Discovery

- `String discoverServer()` - Scan network for Home Assistant server
- `void updateServerIP(String newIP)` - Update MQTT server IP
- `String getCurrentServerIP()` - Get current server IP

### Topic Management

- `void updateTopics(String clientId)` - Update all topics with new client ID
- `String getTempTopic()` - Get temperature topic
- `String getCpuTempTopic()` - Get CPU temperature topic  
- `String getRebootTopic()` - Get reboot command topic
- `String getFirmwareVersionTopic()` - Get firmware version topic

### Publishing

- `bool publishTemperature(float temperature)` - Publish temperature to temp topic
- `bool publishCpuTemperature(float temperature)` - Publish CPU temperature
- `bool publishFirmwareVersion(int version)` - Publish firmware version
- `bool publish(const char* topic, const char* payload, bool retain = false)` - Generic publish

### Timing Management

- `bool shouldPublishTemperature(unsigned long currentTime)` - Check if it's time to publish temperature
- `bool shouldPublishFirmwareVersion(unsigned long currentTime)` - Check if it's time to publish firmware version
- `bool shouldRediscoverServer(unsigned long currentTime)` - Check if it's time to rediscover server
- `void updateLastPublishTime(unsigned long currentTime)` - Update last temperature publish time
- `void updateLastVersionPublishTime(unsigned long currentTime)` - Update last version publish time
- `void updateLastDiscoveryTime(unsigned long currentTime)` - Update last discovery time

## Topic Structure

The library automatically creates topics using the following structure:
- Temperature: `home/esp/{client_id}/temperature_f`
- CPU Temperature: `home/esp/{client_id}/cpu_temperature_c`
- Reboot Command: `home/esp/{client_id}/reboot`
- Firmware Version: `home/esp/{client_id}/firmware_version`

## Default Intervals

- Temperature publishing: 10 seconds
- Firmware version publishing: 5 minutes
- Server rediscovery: 15 minutes

## Home Assistant Integration

The library automatically discovers Home Assistant servers by scanning for devices listening on port 8123. When found, it uses that IP as the MQTT broker address. This works well with Home Assistant's built-in Mosquitto MQTT broker.

## License

This library is provided as-is for educational and development purposes.
