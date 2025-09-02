/**
 * @file Config.h
 * @brief Configuration management system for ESP32 applications
 * 
 * This file provides a comprehensive configuration management system that handles
 * WiFi settings, hardware configuration, firmware management, and network parameters.
 * The configuration is persistent using ESP32's Preferences library and follows
 * a singleton pattern for global access.
 * 
 * @author ESP_Sandbox Project
 * @version 9.50
 * @date 2025
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

/**
 * @namespace ConfigConstants
 * @brief Contains all configuration constants organized by functional areas
 * 
 * This namespace groups configuration constants into logical sub-namespaces
 * for WiFi, Hardware, Timing, Firmware, and Network settings. All constants
 * are compile-time defined for performance and memory efficiency.
 */
namespace ConfigConstants {
    /**
     * @namespace ConfigConstants::WiFi
     * @brief WiFi-related configuration constants
     * 
     * Contains default values and limits for WiFi connectivity, including
     * connection attempts, timeouts, and access point configuration.
     */
    namespace WiFi {
        const int MAX_ATTEMPTS = 15;                    ///< Maximum WiFi connection attempts
        const int RECONNECT_ATTEMPTS = 20;              ///< Maximum reconnection attempts
        const int RETRY_DELAY = 500;                    ///< Delay between retry attempts in milliseconds
        const unsigned long CHECK_INTERVAL = 30 * 1000; ///< WiFi status check interval (30 seconds)
        constexpr const char* DEFAULT_SSID = "SSID";    ///< Default WiFi SSID
        constexpr const char* DEFAULT_PASSWORD = "Nd14il!la"; ///< Default WiFi password
        
        // Access Point Configuration
        constexpr const char* AP_PASSWORD = "ESP32Config"; ///< Default AP password
        const int AP_CHANNEL = 1;                       ///< WiFi AP channel
        const int AP_MAX_CONNECTIONS = 4;               ///< Maximum AP connections
        const bool AP_HIDDEN = false;                   ///< AP visibility setting
        const bool DEFAULT_AP_MODE = false;             ///< Default to WiFi station mode
    }
    
    /**
     * @namespace ConfigConstants::Hardware
     * @brief Hardware-related configuration constants
     * 
     * Contains pin assignments, PWM settings, and default values for
     * hardware components like LEDs and sensors.
     */
    namespace Hardware {
        const int LED_PIN = 2;                       ///< Built-in LED pin number
        const int LED_CHANNEL = 0;                   ///< PWM channel for LED control
        const int LED_FREQ = 5000;                   ///< PWM frequency in Hz
        const int LED_RESOLUTION = 8;                ///< PWM resolution in bits
        const int DEFAULT_LED_BRIGHTNESS = 128;      ///< Default LED brightness (0-255)
    }
    
    /**
     * @namespace ConfigConstants::Timing
     * @brief Timing-related configuration constants
     * 
     * Contains timeout values, delay intervals, and timing parameters
     * for various system operations and watchdog timers.
     */
    namespace Timing {
        const unsigned long LED_PULSE_DURATION = 50;           ///< LED pulse duration in milliseconds
        const unsigned long MAIN_LOOP_DELAY = 1000;            ///< Main loop delay in milliseconds
        const unsigned long NETWORK_STABILIZATION_DELAY = 2000; ///< Network stabilization wait time
        const unsigned long REBOOT_DELAY = 3000;               ///< Delay before system reboot
        const unsigned long WATCHDOG_TIMEOUT = 30;             ///< Watchdog timeout in seconds
        const unsigned long WATCHDOG_PANIC_TIMEOUT = 5;        ///< Panic handler timeout in seconds
    }
    
    /**
     * @namespace ConfigConstants::Firmware
     * @brief Firmware update and version configuration constants
     * 
     * Contains version information, repository details, and update
     * intervals for firmware management functionality.
     */
    namespace Firmware {
        const int VERSION = 950;                                ///< Current firmware version (v9.50)
        constexpr const char* GITHUB_REPO = "stevennolte/ESP_Sandbox"; ///< GitHub repository for updates
        constexpr const char* GITHUB_BRANCH = "Minimal";       ///< Branch for template downloads
        const unsigned long UPDATE_INTERVAL = 5 * 60 * 1000;   ///< Update check interval (5 minutes)
    }
    
    /**
     * @namespace ConfigConstants::Network
     * @brief Network communication configuration constants
     * 
     * Contains HTTP timeout values, user agent strings, and default
     * client identifiers for network operations.
     */
    namespace Network {
        const int HTTP_TIMEOUT_SHORT = 15000;                      ///< Short HTTP timeout (15 seconds)
        const int HTTP_TIMEOUT_LONG = 30000;                       ///< Long HTTP timeout (30 seconds)
        constexpr const char* USER_AGENT_TEMPLATE = "ESP32-Template-Updater"; ///< User agent for template updates
        constexpr const char* USER_AGENT_CHECKER = "ESP32-Template-Checker";  ///< User agent for update checks
        constexpr const char* DEFAULT_CLIENT_ID = "ESP_Default";   ///< Default client identifier
    }
}

/**
 * @class WiFiConfig
 * @brief Manages WiFi connection configuration and persistence
 * 
 * This class handles WiFi credentials, connection parameters, and access point
 * mode settings. It provides methods for loading/saving configuration from
 * ESP32 Preferences and managing connection timing.
 * 
 * Key features:
 * - Persistent storage of WiFi credentials
 * - Configurable connection parameters
 * - Access Point mode support
 * - Connection timing management
 */
class WiFiConfig {
public:
    String ssid;                    ///< WiFi network SSID
    String password;                ///< WiFi network password
    unsigned long last_check_time;  ///< Timestamp of last WiFi status check
    bool force_ap_mode;             ///< Force access point mode on boot
    
    // Runtime configurable WiFi parameters
    int max_attempts;       ///< Maximum connection attempts
    int reconnect_attempts; ///< Maximum reconnection attempts
    int retry_delay;        ///< Delay between retry attempts in milliseconds
    
    /**
     * @brief Initialize WiFi configuration with default values
     * 
     * Sets up initial WiFi configuration using default constants
     * and resets timing and mode settings.
     */
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
    
    /**
     * @brief Load WiFi configuration from ESP32 Preferences
     * @param prefs Reference to Preferences object for data access
     * 
     * Loads saved WiFi credentials, connection parameters, and mode settings
     * from persistent storage. Falls back to defaults if no saved data exists.
     */
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
    
    /**
     * @brief Save WiFi credentials to persistent storage
     * @param newSSID New WiFi network SSID
     * @param newPassword New WiFi network password
     * 
     * Updates the current WiFi credentials and saves them to ESP32 Preferences
     * for persistence across reboots.
     */
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
    
    /**
     * @brief Save WiFi connection parameters to persistent storage
     * @param newMaxAttempts Maximum connection attempts
     * @param newReconnectAttempts Maximum reconnection attempts  
     * @param newRetryDelay Delay between retry attempts in milliseconds
     * 
     * Updates and saves the WiFi connection timing parameters to ESP32 Preferences.
     */
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
    
    /**
     * @brief Save access point mode preference to persistent storage
     * @param apMode True to force AP mode, false for station mode
     * 
     * Updates and saves the WiFi mode preference to ESP32 Preferences.
     */
    void saveApMode(bool apMode) {
        force_ap_mode = apMode;
        Preferences prefs;
        prefs.begin("esp-config", false);
        prefs.putBool("force_ap_mode", force_ap_mode);
        prefs.end();
        Serial.printf("✓ WiFi mode saved: %s\n", force_ap_mode ? "Access Point" : "Station");
    }
    
    /**
     * @brief Check if WiFi status should be checked based on timing
     * @param currentTime Current system time in milliseconds
     * @return True if enough time has passed since last check
     */
    bool shouldCheck(unsigned long currentTime) {
        return (currentTime - last_check_time) > ConfigConstants::WiFi::CHECK_INTERVAL;
    }
    
    /**
     * @brief Update the timestamp of the last WiFi check
     * @param currentTime Current system time in milliseconds
     */
    void updateCheckTime(unsigned long currentTime) {
        last_check_time = currentTime;
    }
    
    /**
     * @brief Get WiFi SSID as C-style string
     * @return Pointer to null-terminated SSID string
     */
    const char* getSSID() const { return ssid.c_str(); }
    
    /**
     * @brief Get WiFi password as C-style string  
     * @return Pointer to null-terminated password string
     */
    const char* getPassword() const { return password.c_str(); }
};

/**
 * @class HardwareConfig
 * @brief Manages hardware-related configuration settings
 * 
 * This class handles configuration for hardware components such as LEDs,
 * including brightness settings and other hardware parameters. Provides
 * persistent storage for hardware preferences.
 */
class HardwareConfig {
public:
    int led_brightness; ///< Current LED brightness value (0-255)
    
    /**
     * @brief Initialize hardware configuration with default values
     */
    void begin() {
        led_brightness = ConfigConstants::Hardware::DEFAULT_LED_BRIGHTNESS;
    }
    
    /**
     * @brief Load hardware configuration from ESP32 Preferences
     * @param prefs Reference to Preferences object for data access
     */
    void loadFromPreferences(Preferences& prefs) {
        led_brightness = prefs.getInt("led_brightness", ConfigConstants::Hardware::DEFAULT_LED_BRIGHTNESS);
        Serial.printf("✓ LED brightness loaded: %d\n", led_brightness);
    }
    
    /**
     * @brief Save LED brightness setting to persistent storage
     * @param brightness New brightness value (0-255)
     */
    void saveBrightness(int brightness) {
        led_brightness = brightness;
        Preferences prefs;
        prefs.begin("esp-config", false);
        prefs.putInt("led_brightness", led_brightness);
        prefs.end();
        Serial.printf("✓ LED brightness saved: %d\n", led_brightness);
    }
};

/**
 * @class FirmwareConfig
 * @brief Manages firmware update timing and version tracking
 * 
 * This class handles timing for firmware update checks and provides
 * methods to determine when the next update check should occur.
 */
class FirmwareConfig {
public:
    unsigned long last_update_check; ///< Timestamp of last firmware update check
    
    /**
     * @brief Initialize firmware configuration
     */
    void begin() {
        last_update_check = 0;
    }
    
    /**
     * @brief Check if firmware update should be checked based on timing
     * @param currentTime Current system time in milliseconds
     * @return True if enough time has passed since last update check
     */
    bool shouldCheckUpdates(unsigned long currentTime) {
        return (currentTime - last_update_check) > ConfigConstants::Firmware::UPDATE_INTERVAL;
    }
    
    /**
     * @brief Update the timestamp of the last firmware update check
     * @param currentTime Current system time in milliseconds
     */
    void updateCheckTime(unsigned long currentTime) {
        last_update_check = currentTime;
    }
};

/**
 * @class NetworkConfig
 * @brief Manages network-related configuration settings
 * 
 * This class handles client identification and other network parameters
 * for HTTP communications and device identification.
 */
class NetworkConfig {
public:
    String client_id; ///< Unique client identifier for network communications
    
    /**
     * @brief Initialize network configuration with default values
     */
    void begin() {
        client_id = ConfigConstants::Network::DEFAULT_CLIENT_ID;
    }
    
    /**
     * @brief Load network configuration from ESP32 Preferences
     * @param prefs Reference to Preferences object for data access
     */
    void loadFromPreferences(Preferences& prefs) {
        client_id = prefs.getString("client_id", ConfigConstants::Network::DEFAULT_CLIENT_ID);
        Serial.printf("✓ Client ID loaded: %s\n", client_id.c_str());
    }
    
    /**
     * @brief Save client ID to persistent storage
     * @param newClientId New client identifier string
     */
    void saveClientId(const String& newClientId) {
        client_id = newClientId;
        Preferences prefs;
        prefs.begin("esp-config", false);
        prefs.putString("client_id", client_id);
        prefs.end();
        Serial.printf("✓ Client ID saved: %s\n", client_id.c_str());
    }
    
    /**
     * @brief Get client ID as C-style string
     * @return Pointer to null-terminated client ID string
     */
    const char* getClientId() const { return client_id.c_str(); }
};

/**
 * @class Config
 * @brief Main configuration management class using singleton pattern
 * 
 * This is the primary configuration class that coordinates all configuration
 * submodules (WiFi, Hardware, Firmware, Network). It implements the singleton
 * pattern to ensure global access to configuration settings throughout the
 * application.
 * 
 * Features:
 * - Singleton pattern for global access
 * - Unified interface to all configuration modules
 * - Backward compatibility with legacy property access
 * - Centralized preference loading/saving
 * - Timing management for various operations
 * 
 * Usage:
 * @code
 * Config& config = Config::getInstance();
 * config.begin();
 * config.loadFromPreferences();
 * @endcode
 */
class Config {
public:
    // Configuration submodules
    WiFiConfig wifi;        ///< WiFi configuration module
    HardwareConfig hardware; ///< Hardware configuration module
    FirmwareConfig firmware; ///< Firmware configuration module
    NetworkConfig network;   ///< Network configuration module
    
    /**
     * @brief Get singleton instance of Config class
     * @return Reference to the single Config instance
     * 
     * Implements the singleton pattern to ensure only one Config instance
     * exists throughout the application lifecycle.
     */
    static Config& getInstance() {
        static Config instance;
        return instance;
    }
    
    /**
     * @brief Initialize all configuration modules
     * 
     * Calls the begin() method on all configuration submodules to set up
     * their default values and initial state.
     */
    void begin() {
        wifi.begin();
        hardware.begin();
        firmware.begin();
        network.begin();
    }
    
    /**
     * @brief Load all configuration from ESP32 Preferences
     * 
     * Opens the preferences storage and loads configuration for all
     * submodules from persistent storage. This should be called after
     * begin() to restore saved settings.
     */
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
    /**
     * @brief Get WiFi SSID (backward compatibility method)
     * @return Pointer to null-terminated SSID string
     */
    const char* getWiFiSSID() const { return wifi.getSSID(); }
    
    /**
     * @brief Get WiFi password (backward compatibility method)
     * @return Pointer to null-terminated password string
     */
    const char* getWiFiPassword() const { return wifi.getPassword(); }
    
    /**
     * @brief Get client ID (backward compatibility method)
     * @return Pointer to null-terminated client ID string
     */
    const char* getClientId() const { return network.getClientId(); }
    
    /**
     * @brief Save client ID (convenience method)
     * @param newClientId New client identifier string
     */
    void saveClientId(const String& newClientId) { network.saveClientId(newClientId); }
    
    /**
     * @brief Save LED brightness (convenience method)  
     * @param brightness New brightness value (0-255)
     */
    void saveLedBrightness(int brightness) { hardware.saveBrightness(brightness); }
    
    /**
     * @brief Save WiFi credentials (convenience method)
     * @param ssid WiFi network SSID
     * @param password WiFi network password
     */
    void saveWiFiCredentials(const String& ssid, const String& password) { 
        wifi.saveCredentials(ssid, password); 
    }
    
    /**
     * @brief Save WiFi connection parameters (convenience method)
     * @param maxAttempts Maximum connection attempts
     * @param reconnectAttempts Maximum reconnection attempts
     * @param retryDelay Delay between retry attempts
     */
    void saveWiFiParams(int maxAttempts, int reconnectAttempts, int retryDelay) {
        wifi.saveWiFiParams(maxAttempts, reconnectAttempts, retryDelay);
    }
    
    /**
     * @brief Save access point mode preference (convenience method)
     * @param apMode True to force AP mode, false for station mode
     */
    void saveApMode(bool apMode) {
        wifi.saveApMode(apMode);
    }
    
    // Timing helper methods
    /**
     * @brief Check if WiFi status should be checked (convenience method)
     * @param currentTime Current system time in milliseconds
     * @return True if WiFi check is due
     */
    bool shouldCheckWiFi(unsigned long currentTime) { return wifi.shouldCheck(currentTime); }
    
    /**
     * @brief Check if firmware updates should be checked (convenience method)
     * @param currentTime Current system time in milliseconds  
     * @return True if update check is due
     */
    bool shouldCheckUpdates(unsigned long currentTime) { return firmware.shouldCheckUpdates(currentTime); }
    
    /**
     * @brief Update WiFi check timestamp (convenience method)
     * @param currentTime Current system time in milliseconds
     */
    void updateWiFiCheckTime(unsigned long currentTime) { wifi.updateCheckTime(currentTime); }
    
    /**
     * @brief Update firmware check timestamp (convenience method)
     * @param currentTime Current system time in milliseconds
     */
    void updateUpdateCheckTime(unsigned long currentTime) { firmware.updateCheckTime(currentTime); }
    
    // Legacy property access for backward compatibility
    String& wifi_ssid = wifi.ssid;                         ///< Legacy reference to WiFi SSID
    String& wifi_password = wifi.password;                 ///< Legacy reference to WiFi password  
    String& client_id = network.client_id;                 ///< Legacy reference to client ID
    int& led_brightness = hardware.led_brightness;         ///< Legacy reference to LED brightness
    unsigned long& last_update_check = firmware.last_update_check; ///< Legacy reference to last update check
    unsigned long& last_wifi_check = wifi.last_check_time; ///< Legacy reference to last WiFi check

private:
    /**
     * @brief Private constructor for singleton pattern
     */
    Config() = default;
    
    /**
     * @brief Private destructor for singleton pattern
     */
    ~Config() = default;
    
    /**
     * @brief Deleted copy constructor for singleton pattern
     */
    Config(const Config&) = delete;
    
    /**
     * @brief Deleted assignment operator for singleton pattern
     */
    Config& operator=(const Config&) = delete;
};

#endif // CONFIG_H
