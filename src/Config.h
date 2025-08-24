#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

// --- Configuration Constants ---
namespace ConfigConstants {
    // WiFi Configuration
    namespace WiFi {
        const int MAX_ATTEMPTS = 15;
        const int RECONNECT_ATTEMPTS = 20;
        const int RETRY_DELAY = 500;
        const unsigned long CHECK_INTERVAL = 30 * 1000; // 30 seconds
        constexpr const char* DEFAULT_SSID = "SSID";
        constexpr const char* DEFAULT_PASSWORD = "Nd14il!la";
        
        // Access Point Configuration
        constexpr const char* AP_PASSWORD = "ESP32Config"; // Default AP password
        const int AP_CHANNEL = 1;
        const int AP_MAX_CONNECTIONS = 4;
        const bool AP_HIDDEN = false;
        const bool DEFAULT_AP_MODE = false; // Default to WiFi station mode
    }
    
    // Hardware Configuration
    namespace Hardware {
        const int LED_PIN = 2;
        const int LED_CHANNEL = 0;
        const int LED_FREQ = 5000;
        const int LED_RESOLUTION = 8;
        const int DEFAULT_LED_BRIGHTNESS = 128;
    }
    
    // Timing Configuration
    namespace Timing {
        const unsigned long LED_PULSE_DURATION = 50;
        const unsigned long MAIN_LOOP_DELAY = 1000;
        const unsigned long NETWORK_STABILIZATION_DELAY = 2000;
        const unsigned long REBOOT_DELAY = 3000;
        const unsigned long WATCHDOG_TIMEOUT = 30; // 30 seconds watchdog timeout
        const unsigned long WATCHDOG_PANIC_TIMEOUT = 5; // 5 seconds panic handler timeout
    }
    
    // Firmware Configuration
    namespace Firmware {
        const int VERSION = 950; // v9.28
        constexpr const char* GITHUB_REPO = "stevennolte/ESP_Sandbox";
        constexpr const char* GITHUB_BRANCH = "Minimal"; // Branch for template downloads
        const unsigned long UPDATE_INTERVAL = 5 * 60 * 1000; // 5 minutes
    }
    
    // Network Configuration
    namespace Network {
        const int HTTP_TIMEOUT_SHORT = 15000;  // 15 seconds
        const int HTTP_TIMEOUT_LONG = 30000;   // 30 seconds
        constexpr const char* USER_AGENT_TEMPLATE = "ESP32-Template-Updater";
        constexpr const char* USER_AGENT_CHECKER = "ESP32-Template-Checker";
        constexpr const char* DEFAULT_CLIENT_ID = "ESP_Default";
    }
}

// --- Configuration Submodules ---
class WiFiConfig {
public:
    String ssid;
    String password;
    unsigned long last_check_time;
    bool force_ap_mode; // Force access point mode on boot
    
    // Runtime configurable WiFi parameters
    int max_attempts;
    int reconnect_attempts;
    int retry_delay;
    
    void begin() {
        ssid = ConfigConstants::WiFi::DEFAULT_SSID;
        password = ConfigConstants::WiFi::DEFAULT_PASSWORD;
        last_check_time = 0;
        force_ap_mode = ConfigConstants::WiFi::DEFAULT_AP_MODE;
        
        // Initialize with default values from constants
        max_attempts = ConfigConstants::WiFi::MAX_ATTEMPTS;
        reconnect_attempts = ConfigConstants::WiFi::RECONNECT_ATTEMPTS;
        retry_delay = ConfigConstants::WiFi::RETRY_DELAY;
    }
    
    void loadFromPreferences(Preferences& prefs) {
        String saved_ssid = prefs.getString("wifi_ssid", "");
        String saved_password = prefs.getString("wifi_password", "");
        
        // Load AP mode preference
        force_ap_mode = prefs.getBool("force_ap_mode", ConfigConstants::WiFi::DEFAULT_AP_MODE);
        
        // Load WiFi parameters from preferences
        max_attempts = prefs.getInt("wifi_max_attempts", ConfigConstants::WiFi::MAX_ATTEMPTS);
        reconnect_attempts = prefs.getInt("wifi_reconnect_attempts", ConfigConstants::WiFi::RECONNECT_ATTEMPTS);
        retry_delay = prefs.getInt("wifi_retry_delay", ConfigConstants::WiFi::RETRY_DELAY);
        
        if (saved_ssid.length() > 0) {
            ssid = saved_ssid;
            password = saved_password;
            Serial.printf("✓ WiFi credentials loaded: %s\n", ssid.c_str());
        }
        
        Serial.printf("✓ WiFi mode preference loaded: %s\n", force_ap_mode ? "Access Point" : "Station");
    }
    
    void saveCredentials(const String& newSSID, const String& newPassword) {
        ssid = newSSID;
        password = newPassword;
        Preferences prefs;
        prefs.begin("esp-config", false);
        prefs.putString("wifi_ssid", ssid);
        prefs.putString("wifi_password", password);
        prefs.end();
        Serial.printf("✓ WiFi credentials saved: %s\n", ssid.c_str());
    }
    
    void saveWiFiParams(int newMaxAttempts, int newReconnectAttempts, int newRetryDelay) {
        max_attempts = newMaxAttempts;
        reconnect_attempts = newReconnectAttempts;
        retry_delay = newRetryDelay;
        
        Preferences prefs;
        prefs.begin("esp-config", false);
        prefs.putInt("wifi_max_attempts", max_attempts);
        prefs.putInt("wifi_reconnect_attempts", reconnect_attempts);
        prefs.putInt("wifi_retry_delay", retry_delay);
        prefs.end();
        
        Serial.printf("✓ WiFi parameters saved: max_attempts=%d, reconnect_attempts=%d, retry_delay=%d\n", 
                     max_attempts, reconnect_attempts, retry_delay);
    }
    
    void saveApMode(bool apMode) {
        force_ap_mode = apMode;
        Preferences prefs;
        prefs.begin("esp-config", false);
        prefs.putBool("force_ap_mode", force_ap_mode);
        prefs.end();
        Serial.printf("✓ WiFi mode saved: %s\n", force_ap_mode ? "Access Point" : "Station");
    }
    
    bool shouldCheck(unsigned long currentTime) {
        return (currentTime - last_check_time) > ConfigConstants::WiFi::CHECK_INTERVAL;
    }
    
    void updateCheckTime(unsigned long currentTime) {
        last_check_time = currentTime;
    }
    
    const char* getSSID() const { return ssid.c_str(); }
    const char* getPassword() const { return password.c_str(); }
};

class HardwareConfig {
public:
    int led_brightness;
    
    void begin() {
        led_brightness = ConfigConstants::Hardware::DEFAULT_LED_BRIGHTNESS;
    }
    
    void loadFromPreferences(Preferences& prefs) {
        led_brightness = prefs.getInt("led_brightness", ConfigConstants::Hardware::DEFAULT_LED_BRIGHTNESS);
        Serial.printf("✓ LED brightness loaded: %d\n", led_brightness);
    }
    
    void saveBrightness(int brightness) {
        led_brightness = brightness;
        Preferences prefs;
        prefs.begin("esp-config", false);
        prefs.putInt("led_brightness", led_brightness);
        prefs.end();
        Serial.printf("✓ LED brightness saved: %d\n", led_brightness);
    }
};

class FirmwareConfig {
public:
    unsigned long last_update_check;
    
    void begin() {
        last_update_check = 0;
    }
    
    bool shouldCheckUpdates(unsigned long currentTime) {
        return (currentTime - last_update_check) > ConfigConstants::Firmware::UPDATE_INTERVAL;
    }
    
    void updateCheckTime(unsigned long currentTime) {
        last_update_check = currentTime;
    }
};

class NetworkConfig {
public:
    String client_id;
    
    void begin() {
        client_id = ConfigConstants::Network::DEFAULT_CLIENT_ID;
    }
    
    void loadFromPreferences(Preferences& prefs) {
        client_id = prefs.getString("client_id", ConfigConstants::Network::DEFAULT_CLIENT_ID);
        Serial.printf("✓ Client ID loaded: %s\n", client_id.c_str());
    }
    
    void saveClientId(const String& newClientId) {
        client_id = newClientId;
        Preferences prefs;
        prefs.begin("esp-config", false);
        prefs.putString("client_id", client_id);
        prefs.end();
        Serial.printf("✓ Client ID saved: %s\n", client_id.c_str());
    }
    
    const char* getClientId() const { return client_id.c_str(); }
};

// --- Main Configuration Class ---
class Config {
public:
    // Configuration submodules
    WiFiConfig wifi;
    HardwareConfig hardware;
    FirmwareConfig firmware;
    NetworkConfig network;
    
    // Singleton pattern
    static Config& getInstance() {
        static Config instance;
        return instance;
    }
    
    void begin() {
        wifi.begin();
        hardware.begin();
        firmware.begin();
        network.begin();
    }
    
    void loadFromPreferences() {
        Preferences prefs;
        prefs.begin("esp-config", true); // read-only
        
        wifi.loadFromPreferences(prefs);
        hardware.loadFromPreferences(prefs);
        network.loadFromPreferences(prefs);
        
        prefs.end();
        
        Serial.println("✓ All configuration loaded from preferences");
    }
    
    // Convenience methods for backward compatibility
    const char* getWiFiSSID() const { return wifi.getSSID(); }
    const char* getWiFiPassword() const { return wifi.getPassword(); }
    const char* getClientId() const { return network.getClientId(); }
    
    void saveClientId(const String& newClientId) { network.saveClientId(newClientId); }
    void saveLedBrightness(int brightness) { hardware.saveBrightness(brightness); }
    void saveWiFiCredentials(const String& ssid, const String& password) { 
        wifi.saveCredentials(ssid, password); 
    }
    void saveWiFiParams(int maxAttempts, int reconnectAttempts, int retryDelay) {
        wifi.saveWiFiParams(maxAttempts, reconnectAttempts, retryDelay);
    }
    void saveApMode(bool apMode) {
        wifi.saveApMode(apMode);
    }
    
    // Timing helper methods
    bool shouldCheckWiFi(unsigned long currentTime) { return wifi.shouldCheck(currentTime); }
    bool shouldCheckUpdates(unsigned long currentTime) { return firmware.shouldCheckUpdates(currentTime); }
    
    void updateWiFiCheckTime(unsigned long currentTime) { wifi.updateCheckTime(currentTime); }
    void updateUpdateCheckTime(unsigned long currentTime) { firmware.updateCheckTime(currentTime); }
    
    // Legacy property access for backward compatibility
    String& wifi_ssid = wifi.ssid;
    String& wifi_password = wifi.password;
    String& client_id = network.client_id;
    int& led_brightness = hardware.led_brightness;
    unsigned long& last_update_check = firmware.last_update_check;
    unsigned long& last_wifi_check = wifi.last_check_time;

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
};

#endif // CONFIG_H
