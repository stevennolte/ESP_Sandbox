#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

class Config {
public:
    // Singleton pattern
    static Config& getInstance() {
        static Config instance;
        return instance;
    }

    // --- Firmware Configuration ---
    static const int FIRMWARE_VERSION = 928; // v9.28
    static constexpr const char* GITHUB_REPO = "stevennolte/ESP_Sandbox";
    static const unsigned long UPDATE_INTERVAL = 5 * 60 * 1000; // 5 minutes

    // --- Default Network Configuration ---
    static constexpr const char* DEFAULT_WIFI_SSID = "SSEI";
    static constexpr const char* DEFAULT_WIFI_PASSWORD = "Nd14il!la";
    static constexpr const char* DEFAULT_CLIENT_ID = "ESP_Default";

    // --- HTTP Configuration ---
    static const int HTTP_TIMEOUT_SHORT = 15000;  // 15 seconds
    static const int HTTP_TIMEOUT_LONG = 30000;   // 30 seconds
    static constexpr const char* USER_AGENT_TEMPLATE = "ESP32-Template-Updater";
    static constexpr const char* USER_AGENT_CHECKER = "ESP32-Template-Checker";

    // --- Hardware Configuration ---
    static const int LED_PIN = 2;         // Built-in LED
    static const int LED_CHANNEL = 0;     // PWM channel
    static const int LED_FREQ = 5000;     // PWM frequency
    static const int LED_RESOLUTION = 8;  // 8-bit resolution (0-255)
    static const int DEFAULT_LED_BRIGHTNESS = 128; // Default brightness (0-255)

    // --- Timing Configuration ---
    static const unsigned long LED_PULSE_DURATION = 50;
    static const unsigned long MAIN_LOOP_DELAY = 1000;
    static const unsigned long NETWORK_STABILIZATION_DELAY = 2000;
    static const unsigned long REBOOT_DELAY = 3000;
    static const unsigned long WIFI_CHECK_INTERVAL = 30 * 1000; // Check WiFi every 30 seconds

    // --- Network Constants ---
    static const int WIFI_MAX_ATTEMPTS = 30;
    static const int WIFI_RECONNECT_ATTEMPTS = 20;
    static const int WIFI_RETRY_DELAY = 500;

    // --- Runtime Configuration (changeable) ---
    String wifi_ssid;
    String wifi_password;
    String client_id;
    int led_brightness;
    unsigned long last_update_check;
    unsigned long last_wifi_check;

    // --- Configuration Management ---
    void begin() {
        // Initialize with defaults
        wifi_ssid = DEFAULT_WIFI_SSID;
        wifi_password = DEFAULT_WIFI_PASSWORD;
        client_id = DEFAULT_CLIENT_ID;
        led_brightness = DEFAULT_LED_BRIGHTNESS;
        last_update_check = 0;
        last_wifi_check = 0;
    }

    void loadFromPreferences() {
        Preferences prefs;
        prefs.begin("esp-config", true); // read-only
        
        client_id = prefs.getString("client_id", DEFAULT_CLIENT_ID);
        led_brightness = prefs.getInt("led_brightness", DEFAULT_LED_BRIGHTNESS);
        
        // Load WiFi credentials if saved
        String saved_ssid = prefs.getString("wifi_ssid", "");
        String saved_password = prefs.getString("wifi_password", "");
        
        prefs.end();
        
        // Update WiFi credentials if they were saved
        if (saved_ssid.length() > 0) {
            wifi_ssid = saved_ssid;
            wifi_password = saved_password;
        }
        
        Serial.printf("✓ Config loaded - Client ID: %s, LED Brightness: %d\n", 
                     client_id.c_str(), led_brightness);
        if (saved_ssid.length() > 0) {
            Serial.printf("✓ Saved WiFi: %s\n", saved_ssid.c_str());
        }
    }

    void saveClientId(const String& newClientId) {
        client_id = newClientId;
        Preferences prefs;
        prefs.begin("esp-config", false);
        prefs.putString("client_id", client_id);
        prefs.end();
        Serial.printf("✓ Client ID saved: %s\n", client_id.c_str());
    }

    void saveLedBrightness(int brightness) {
        led_brightness = brightness;
        Preferences prefs;
        prefs.begin("esp-config", false);
        prefs.putInt("led_brightness", led_brightness);
        prefs.end();
        Serial.printf("✓ LED brightness saved: %d\n", led_brightness);
    }

    void saveWiFiCredentials(const String& ssid, const String& password) {
        wifi_ssid = ssid;
        wifi_password = password;
        Preferences prefs;
        prefs.begin("esp-config", false);
        prefs.putString("wifi_ssid", wifi_ssid);
        prefs.putString("wifi_password", wifi_password);
        prefs.end();
        Serial.printf("✓ WiFi credentials saved: %s\n", wifi_ssid.c_str());
    }

    // --- Helper Methods ---
    bool shouldCheckWiFi(unsigned long currentTime) {
        return (currentTime - last_wifi_check) > WIFI_CHECK_INTERVAL;
    }

    bool shouldCheckUpdates(unsigned long currentTime) {
        return (currentTime - last_update_check) > UPDATE_INTERVAL;
    }

    void updateWiFiCheckTime(unsigned long currentTime) {
        last_wifi_check = currentTime;
    }

    void updateUpdateCheckTime(unsigned long currentTime) {
        last_update_check = currentTime;
    }

    // --- Getters for const char* compatibility ---
    const char* getWiFiSSID() const { return wifi_ssid.c_str(); }
    const char* getWiFiPassword() const { return wifi_password.c_str(); }
    const char* getClientId() const { return client_id.c_str(); }

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
};

#endif // CONFIG_H
