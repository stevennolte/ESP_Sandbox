# 📋 ESP32 Configuration System Guide

## 🏗️ Architecture Overview

The ESP32 configuration system uses a **hybrid approach** with three layers for maximum flexibility and organization:

### **1. Compile-Time Constants (`ConfigConstants` namespace)**
- **Purpose**: Fixed values that never change during runtime
- **Location**: Defined in `ConfigConstants::` namespaces
- **Usage**: Hardware pins, default values, timeouts, version info

### **2. Runtime Configuration Classes**
- **Purpose**: Values that can change during program execution
- **Storage**: Automatically saved to ESP32's NVRAM (non-volatile memory)
- **Persistence**: Survives reboots and power cycles

### **3. Main Config Class (Singleton)**
- **Purpose**: Central access point for all configuration
- **Pattern**: Single global instance accessible everywhere
- **Integration**: Combines constants and runtime configs

---

## 🔧 Layer 1: Compile-Time Constants

Access constants like this in your code:

```cpp
ConfigConstants::WiFi::MAX_ATTEMPTS        // 15
ConfigConstants::Hardware::LED_PIN         // 2  
ConfigConstants::Timing::REBOOT_DELAY      // 3000ms
ConfigConstants::Firmware::VERSION         // 928
ConfigConstants::Network::HTTP_TIMEOUT_SHORT // 15000ms
```

### Available Constant Categories:

#### **WiFi Constants**
- `MAX_ATTEMPTS` - Default WiFi connection attempts (15)
- `RECONNECT_ATTEMPTS` - Default reconnection attempts (20)
- `RETRY_DELAY` - Delay between connection attempts (500ms)
- `CHECK_INTERVAL` - WiFi status check interval (30 seconds)
- `DEFAULT_SSID` - Default network name
- `DEFAULT_PASSWORD` - Default network password

#### **Hardware Constants**
- `LED_PIN` - Built-in LED pin (2)
- `LED_CHANNEL` - PWM channel for LED (0)
- `LED_FREQ` - PWM frequency (5000 Hz)
- `LED_RESOLUTION` - PWM resolution (8-bit)
- `DEFAULT_LED_BRIGHTNESS` - Default brightness (128)

#### **Timing Constants**
- `LED_PULSE_DURATION` - LED flash duration (50ms)
- `MAIN_LOOP_DELAY` - Main loop delay (1000ms)
- `NETWORK_STABILIZATION_DELAY` - Network stabilization wait (2000ms)
- `REBOOT_DELAY` - Delay before system reboot (3000ms)

#### **Firmware Constants**
- `VERSION` - Current firmware version (928 = v9.28)
- `GITHUB_REPO` - Repository for updates ("stevennolte/ESP_Sandbox")
- `UPDATE_INTERVAL` - How often to check for updates (5 minutes)

#### **Network Constants**
- `HTTP_TIMEOUT_SHORT` - Short HTTP timeout (15 seconds)
- `HTTP_TIMEOUT_LONG` - Long HTTP timeout (30 seconds)
- `USER_AGENT_TEMPLATE` - User agent for template downloads
- `USER_AGENT_CHECKER` - User agent for update checks
- `DEFAULT_CLIENT_ID` - Default device identifier

---

## 🔄 Layer 2: Runtime Configuration Classes

### **WiFiConfig** - Network Settings
```cpp
config.wifi.ssid                    // Current WiFi network name
config.wifi.password                // Current WiFi password  
config.wifi.max_attempts            // Configurable connection attempts
config.wifi.reconnect_attempts      // Configurable reconnection attempts
config.wifi.retry_delay             // Configurable delay between attempts
config.wifi.last_check_time         // When WiFi was last checked
```

### **HardwareConfig** - Device Settings
```cpp
config.hardware.led_brightness      // LED brightness (0-255)
```

### **FirmwareConfig** - Update Management
```cpp
config.firmware.last_update_check   // When updates were last checked
```

### **NetworkConfig** - Device Identity
```cpp
config.network.client_id            // Unique device identifier
```

---

## 🎯 Layer 3: Main Config Class Usage

### **Getting the Config Instance**
```cpp
Config& config = Config::getInstance();  // Singleton pattern
```

### **Initialization (in setup())**
```cpp
void setup() {
    config.begin();                 // Initialize all submodules
    config.loadFromPreferences();   // Load saved settings from NVRAM
    
    // Your other setup code...
}
```

---

## 📖 How to Use the Config System

### **1. Reading Values**

```cpp
// Read compile-time constants
int maxAttempts = ConfigConstants::WiFi::MAX_ATTEMPTS;
int ledPin = ConfigConstants::Hardware::LED_PIN;

// Read runtime values  
String currentSSID = config.wifi.ssid;
int brightness = config.hardware.led_brightness;
String deviceID = config.network.client_id;

// Use in Serial output
Serial.printf("Current WiFi: %s, Brightness: %d\n", 
              config.wifi.ssid.c_str(), config.hardware.led_brightness);
```

### **2. Changing Runtime Values (Temporary)**

Changes are lost on reboot unless explicitly saved:

```cpp
// Temporary changes (lost on reboot)
config.wifi.max_attempts = 50;
config.hardware.led_brightness = 200;
config.network.client_id = "MyDevice123";
```

### **3. Changing Runtime Values (Permanent)**

These methods automatically save to NVRAM and survive reboots:

```cpp
// Permanently save WiFi credentials
config.saveWiFiCredentials("MyNetwork", "MyPassword");

// Permanently save LED brightness
config.saveLedBrightness(180);

// Permanently save device ID
config.saveClientId("ProductionDevice");

// Permanently save WiFi connection parameters
config.saveWiFiParams(50, 30, 1000);  // max_attempts, reconnect_attempts, retry_delay
```

### **4. Using in Your Code**

#### WiFi Connection Example:
```cpp
void connectWiFi() {
    Serial.printf("Connecting to %s with %d max attempts...\n", 
                  config.wifi.ssid.c_str(), config.wifi.max_attempts);
    
    WiFi.begin(config.wifi.ssid.c_str(), config.wifi.password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < config.wifi.max_attempts) {
        delay(config.wifi.retry_delay);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n✓ Connected after %d attempts\n", attempts);
    } else {
        Serial.printf("\n✗ Failed after %d attempts\n", attempts);
    }
}
```

#### LED Control Example:
```cpp
void updateLED() {
    // Use hardware constants for pin setup
    ledcSetup(ConfigConstants::Hardware::LED_CHANNEL, 
              ConfigConstants::Hardware::LED_FREQ, 
              ConfigConstants::Hardware::LED_RESOLUTION);
    
    // Use runtime brightness setting
    ledcWrite(ConfigConstants::Hardware::LED_CHANNEL, config.hardware.led_brightness);
}
```

#### Timing Example:
```cpp
void loop() {
    // Your main code here
    
    // Use configurable main loop delay
    delay(ConfigConstants::Timing::MAIN_LOOP_DELAY);
}
```

---

## 💾 Persistence & Storage

### **What Gets Saved Automatically:**
- ✅ WiFi credentials (SSID, password)
- ✅ WiFi connection parameters (max_attempts, retry_delay, etc.)
- ✅ LED brightness setting
- ✅ Device client ID
- ✅ Firmware version tracking

### **Storage Details:**
- **Location**: ESP32's NVRAM (Non-Volatile RAM)
- **Namespace**: `"esp-config"`
- **Persistence**: Survives power cycles, firmware updates, and reboots
- **Loading**: Automatic at startup via `config.loadFromPreferences()`

### **Storage Keys:**
```
wifi_ssid               // WiFi network name
wifi_password           // WiFi password
wifi_max_attempts       // Connection attempts
wifi_reconnect_attempts // Reconnection attempts  
wifi_retry_delay        // Delay between attempts
led_brightness          // LED brightness value
client_id              // Device identifier
last_firmware_version   // Firmware version tracking
```

---

## 🔍 Backward Compatibility

The system maintains compatibility with older code patterns:

```cpp
// Legacy access (still works, but deprecated)
config.wifi_ssid              // References config.wifi.ssid
config.wifi_password          // References config.wifi.password  
config.led_brightness         // References config.hardware.led_brightness
config.client_id              // References config.network.client_id

// Modern access (preferred)
config.wifi.ssid
config.wifi.password
config.hardware.led_brightness
config.network.client_id
```

---

## 🎛️ Web Interface Integration

The config system integrates seamlessly with web interfaces:

```cpp
void handleWiFiSettings() {
    if (server.hasArg("ssid") && server.hasArg("password")) {
        String ssid = server.arg("ssid");
        String password = server.arg("password");
        config.saveWiFiCredentials(ssid, password);
        server.send(200, "text/plain", "WiFi credentials updated!");
    }
}

void handleLEDSettings() {
    if (server.hasArg("brightness")) {
        int brightness = server.arg("brightness").toInt();
        if (brightness >= 0 && brightness <= 255) {
            config.saveLedBrightness(brightness);
            server.send(200, "text/plain", "LED brightness updated!");
        } else {
            server.send(400, "text/plain", "Invalid brightness value");
        }
    }
}

void handleDeviceSettings() {
    if (server.hasArg("client_id")) {
        String clientId = server.arg("client_id");
        config.saveClientId(clientId);
        server.send(200, "text/plain", "Client ID updated!");
    }
}

void handleAdvancedWiFi() {
    if (server.hasArg("max_attempts")) {
        int maxAttempts = server.arg("max_attempts").toInt();
        int reconnectAttempts = server.arg("reconnect_attempts").toInt();
        int retryDelay = server.arg("retry_delay").toInt();
        
        config.saveWiFiParams(maxAttempts, reconnectAttempts, retryDelay);
        server.send(200, "text/plain", "WiFi parameters updated!");
    }
}
```

---

## 🛠️ Adding New Configuration Options

### **Step 1: Add Constants (if needed)**
```cpp
// In ConfigConstants namespace
namespace NewFeature {
    const int DEFAULT_VALUE = 100;
    constexpr const char* DEFAULT_STRING = "DefaultText";
}
```

### **Step 2: Create Runtime Config Class**
```cpp
class NewFeatureConfig {
public:
    int runtime_value;
    String runtime_string;
    
    void begin() {
        runtime_value = ConfigConstants::NewFeature::DEFAULT_VALUE;
        runtime_string = ConfigConstants::NewFeature::DEFAULT_STRING;
    }
    
    void loadFromPreferences(Preferences& prefs) {
        runtime_value = prefs.getInt("feature_value", ConfigConstants::NewFeature::DEFAULT_VALUE);
        runtime_string = prefs.getString("feature_string", ConfigConstants::NewFeature::DEFAULT_STRING);
    }
    
    void saveValue(int newValue) {
        runtime_value = newValue;
        Preferences prefs;
        prefs.begin("esp-config", false);
        prefs.putInt("feature_value", runtime_value);
        prefs.end();
    }
};
```

### **Step 3: Add to Main Config Class**
```cpp
class Config {
public:
    // Add your new config
    NewFeatureConfig newFeature;
    
    void begin() {
        // Add to initialization
        newFeature.begin();
    }
    
    void loadFromPreferences() {
        // Add to loading
        newFeature.loadFromPreferences(prefs);
    }
};
```

---

## 📊 Configuration Status & Debugging

### **Check Current Configuration:**
```cpp
void printConfigStatus() {
    Serial.println("=== Configuration Status ===");
    
    // WiFi settings
    Serial.printf("WiFi SSID: %s\n", config.wifi.ssid.c_str());
    Serial.printf("WiFi Max Attempts: %d\n", config.wifi.max_attempts);
    Serial.printf("WiFi Retry Delay: %d ms\n", config.wifi.retry_delay);
    
    // Hardware settings
    Serial.printf("LED Brightness: %d\n", config.hardware.led_brightness);
    
    // Network settings
    Serial.printf("Client ID: %s\n", config.network.client_id.c_str());
    
    // Constants
    Serial.printf("Firmware Version: %d\n", ConfigConstants::Firmware::VERSION);
    Serial.printf("LED Pin: %d\n", ConfigConstants::Hardware::LED_PIN);
}
```

### **Reset to Defaults:**
```cpp
void resetConfiguration() {
    Serial.println("Resetting configuration to defaults...");
    
    // Clear preferences
    Preferences prefs;
    prefs.begin("esp-config", false);
    prefs.clear();
    prefs.end();
    
    // Reinitialize
    config.begin();
    
    Serial.println("Configuration reset complete!");
}
```

---

## 🚀 Key Benefits

1. **📁 Organized**: Related settings grouped logically by category
2. **💾 Persistent**: Important settings survive reboots and power cycles
3. **⚡ Flexible**: Mix of compile-time constants and runtime variables
4. **🔒 Type-Safe**: Strong typing prevents configuration errors
5. **🎯 Centralized**: Single point of access for all settings
6. **🔧 Extensible**: Easy to add new configuration categories
7. **🌐 Web-Friendly**: Integrates seamlessly with web interfaces
8. **🐛 Debug-Friendly**: Clear structure makes troubleshooting easier
9. **🔄 Backward Compatible**: Legacy code continues to work
10. **⚙️ Professional**: Enterprise-grade configuration management

---

## 🎯 Best Practices

### **Do's:**
- ✅ Use `ConfigConstants::` for values that never change
- ✅ Use `config.submodule.property` for runtime values
- ✅ Use `config.saveX()` methods for permanent changes
- ✅ Call `config.begin()` and `config.loadFromPreferences()` in setup()
- ✅ Group related constants in logical namespaces
- ✅ Validate user input before saving to config

### **Don'ts:**
- ❌ Don't modify constants at runtime (they're const for a reason)
- ❌ Don't forget to save important changes with `config.saveX()` methods
- ❌ Don't access config before calling `config.begin()`
- ❌ Don't use raw preference keys, use the config system instead
- ❌ Don't mix up temporary changes vs. permanent saves

---

## 📝 Example: Complete Implementation

```cpp
#include "Config.h"

Config& config = Config::getInstance();

void setup() {
    Serial.begin(115200);
    
    // Initialize configuration system
    config.begin();
    config.loadFromPreferences();
    
    // Print current settings
    Serial.printf("Device: %s\n", config.network.client_id.c_str());
    Serial.printf("WiFi: %s\n", config.wifi.ssid.c_str());
    Serial.printf("LED Brightness: %d\n", config.hardware.led_brightness);
    
    // Connect to WiFi using config
    connectWiFi();
    
    // Setup LED using config
    setupLED();
}

void connectWiFi() {
    WiFi.begin(config.wifi.ssid.c_str(), config.wifi.password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < config.wifi.max_attempts) {
        delay(config.wifi.retry_delay);
        Serial.print(".");
        attempts++;
    }
    
    Serial.printf("\nWiFi %s after %d attempts\n", 
                  WiFi.status() == WL_CONNECTED ? "connected" : "failed", attempts);
}

void setupLED() {
    ledcSetup(ConfigConstants::Hardware::LED_CHANNEL, 
              ConfigConstants::Hardware::LED_FREQ, 
              ConfigConstants::Hardware::LED_RESOLUTION);
    ledcAttachPin(ConfigConstants::Hardware::LED_PIN, 
                  ConfigConstants::Hardware::LED_CHANNEL);
}

void loop() {
    // Pulse LED with configured brightness
    ledcWrite(ConfigConstants::Hardware::LED_CHANNEL, config.hardware.led_brightness);
    delay(ConfigConstants::Timing::LED_PULSE_DURATION);
    ledcWrite(ConfigConstants::Hardware::LED_CHANNEL, 0);
    
    // Main loop delay
    delay(ConfigConstants::Timing::MAIN_LOOP_DELAY);
}
```

This configuration system provides a robust, professional foundation for your ESP32 IoT projects! 🚀
