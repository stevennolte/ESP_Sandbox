/**
 * @file WiFiHelper.h
 * @brief Comprehensive WiFi management library for ESP32 with Access Point fallback
 * @author Steve Nolte
 * @date 2025
 * @version 1.0.0
 * 
 * This library provides a complete WiFi management solution for ESP32 projects,
 * featuring automatic fallback to Access Point mode, web interface handlers,
 * and template processing capabilities.
 */

#ifndef WIFIHELPER_H
#define WIFIHELPER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// Forward declaration to avoid circular dependency
class Config;

/**
 * @class WiFiHelper
 * @brief Static utility class for comprehensive WiFi management on ESP32
 * 
 * WiFiHelper provides a complete solution for managing WiFi connectivity in ESP32
 * applications. It supports both Station mode (connecting to existing networks)
 * and Access Point mode (creating its own network), with automatic fallback
 * between modes based on connection success.
 * 
 * Key Features:
 * - Automatic WiFi connection with configurable retry attempts
 * - Access Point fallback when Station mode fails
 * - Web interface handlers for configuration and network scanning
 * - Template variable processing for dynamic web content
 * - Recovery mode support for system reliability
 * - Comprehensive network information retrieval
 * 
 * @note This class uses static methods only and cannot be instantiated
 * @warning Requires Config class for persistent settings storage
 * 
 * @see Config
 * @see WebServer
 * 
 * Example usage:
 * @code
 * #include <WiFiHelper.h>
 * 
 * void setup() {
 *   Serial.begin(115200);
 *   
 *   // Initialize WiFi with automatic AP fallback
 *   WiFiHelper::setup();
 *   
 *   if (WiFiHelper::isConnected()) {
 *     Serial.println("Network ready: " + WiFiHelper::getConnectionInfo());
 *   }
 * }
 * @endcode
 */
class WiFiHelper {
public:
    /**
     * @name Core WiFi Management
     * @brief Primary functions for WiFi connection and management
     * @{
     */
    
    /**
     * @brief Initialize WiFi connection with automatic AP fallback
     * 
     * Attempts to connect to a configured WiFi network in Station mode.
     * If connection fails after the configured number of attempts, automatically
     * falls back to Access Point mode using the device client_id as the network name.
     * 
     * @details The setup process:
     * 1. Check if forced AP mode is enabled in configuration
     * 2. If not forced, attempt Station mode connection
     * 3. Retry connection up to MAX_ATTEMPTS times
     * 4. On failure, automatically start Access Point mode
     * 5. Configure network parameters and display connection info
     * 
     * @note Requires Config class to be properly initialized with WiFi credentials
     * @warning This function blocks until connection is established or AP mode starts
     * 
     * @see Config::getWiFiSSID()
     * @see Config::getWiFiPassword()
     * @see startAccessPoint()
     */
    static void setup();
    
    /**
     * @brief Monitor and maintain WiFi connection
     * 
     * Continuously monitors the WiFi connection status and attempts automatic
     * reconnection if the connection is lost. Should be called periodically
     * from the main loop to ensure connection reliability.
     * 
     * @details Reconnection process:
     * - Detects connection loss in Station mode
     * - Initiates reconnection attempt
     * - Retries up to RECONNECT_ATTEMPTS times
     * - Logs connection status changes
     * 
     * @note Does nothing when in Access Point mode
     * @note Non-blocking operation suitable for main loop calls
     * 
     * @see setup()
     * @see isConnected()
     */
    static void checkConnection();
    
    /** @} */ // End of Core WiFi Management
    
    /**
     * @name Access Point Functions
     * @brief Functions for managing Access Point mode
     * @{
     */
    
    /**
     * @brief Start WiFi Access Point mode
     * @return true if Access Point started successfully, false otherwise
     * 
     * Creates a WiFi Access Point using the device client_id as the network name.
     * Configures security, channel, and connection limits according to defined
     * constants. Displays connection information for user reference.
     * 
     * @details Access Point configuration:
     * - SSID: Device client_id from Config
     * - Password: Configured AP password
     * - Channel: Configurable (default: 1)
     * - Hidden: Configurable (default: false)
     * - Max connections: Configurable (default: 4)
     * 
     * @note Sets internal accessPointMode flag to true on success
     * @warning Overwrites any existing WiFi mode configuration
     * 
     * @see getAccessPointName()
     * @see isAccessPointMode()
     */
    static bool startAccessPoint();
    
    /**
     * @brief Check if currently operating in Access Point mode
     * @return true if in Access Point mode, false if in Station mode
     * 
     * @note This reflects the current operational mode, not configuration
     * @see startAccessPoint()
     * @see setup()
     */
    static bool isAccessPointMode();
    
    /**
     * @brief Get the Access Point network name
     * @return String containing the AP network name (device client_id)
     * 
     * Returns the network name that will be used (or is being used) for
     * Access Point mode. This is derived from the device's client_id
     * configuration setting.
     * 
     * @note The returned name may contain the device's unique identifier
     * @see Config::getClientId()
     */
    static String getAccessPointName();
    
    /** @} */ // End of Access Point Functions
    
    /**
     * @name Recovery Mode Support
     * @brief Functions for system recovery scenarios
     * @{
     */
    
    /**
     * @brief Setup WiFi for recovery mode operation
     * 
     * Establishes basic WiFi connectivity for recovery mode scenarios,
     * such as after a watchdog reset or system panic. Uses default
     * credentials and simplified connection logic with AP fallback.
     * 
     * @details Recovery setup process:
     * 1. Attempt connection using default/emergency credentials
     * 2. Limited retry attempts (10 maximum)
     * 3. Fall back to Access Point mode if connection fails
     * 4. Minimal logging and status indication
     * 
     * @note Designed for use in constrained recovery scenarios
     * @note Uses emergency/default WiFi credentials
     * @warning May have limited functionality compared to normal setup
     * 
     * @see setup()
     */
    static void setupRecoveryWiFi();
    
    /** @} */ // End of Recovery Mode Support
    
    /**
     * @name Web Interface Handlers
     * @brief HTTP request handlers for web-based configuration
     * @{
     */
    
    /**
     * @brief Handle WiFi configuration page requests
     * @param server Reference to the WebServer instance
     * 
     * Serves the WiFi configuration page with current network status,
     * signal strength, and configuration options. Processes template
     * variables to display real-time network information.
     * 
     * @details Page content includes:
     * - Current network connection status
     * - Signal strength and network details
     * - WiFi mode selection (Station/AP)
     * - Network scanning capabilities
     * - Configuration form elements
     * 
     * @note Requires wifi_config.html template in filesystem
     * @see handleUpdate()
     * @see handleNetworkScan()
     */
    static void handleConfig(WebServer& server);
    
    /**
     * @brief Handle WiFi credential update requests
     * @param server Reference to the WebServer instance
     * 
     * Processes HTTP POST requests containing new WiFi credentials.
     * Validates input parameters, saves credentials to persistent storage,
     * and initiates system restart to apply changes.
     * 
     * @details Update process:
     * 1. Validate SSID and password parameters
     * 2. Save credentials using Config class
     * 3. Display confirmation page
     * 4. Schedule system restart
     * 
     * @note Automatically restarts the device after credential update
     * @warning System will restart after successful update
     * 
     * @see Config::saveWiFiCredentials()
     * @see handleConfig()
     */
    static void handleUpdate(WebServer& server);
    
    /**
     * @brief Handle network scanning requests
     * @param server Reference to the WebServer instance
     * 
     * Performs a WiFi network scan and returns results in JSON format.
     * Provides network information including SSID, signal strength,
     * and security status for use in web interface network selection.
     * 
     * @details JSON response format:
     * @code
     * {
     *   "networks": [
     *     {
     *       "ssid": "NetworkName",
     *       "rssi": -45,
     *       "encrypted": true
     *     },
     *     ...
     *   ]
     * }
     * @endcode
     * 
     * @note Returns JSON content type for API consumption
     * @note Clears previous scan results before new scan
     * 
     * @see handleConfig()
     */
    static void handleNetworkScan(WebServer& server);
    
    /**
     * @brief Handle WiFi mode toggle requests
     * @param server Reference to the WebServer instance
     * 
     * Processes requests to switch between Station mode and Access Point mode.
     * Updates the persistent configuration and provides user feedback.
     * Changes take effect after the next system restart.
     * 
     * @details Supported modes:
     * - "station": Connect to existing WiFi networks
     * - "ap": Create Access Point for direct device connection
     * 
     * @note Changes require system restart to take effect
     * @warning Invalid mode parameters result in HTTP 400 error
     * 
     * @see Config::saveApMode()
     * @see setup()
     */
    static void handleWiFiModeToggle(WebServer& server);
    
    /** @} */ // End of Web Interface Handlers
    
    /**
     * @name Template Processing
     * @brief Functions for dynamic web content generation
     * @{
     */
    
    /**
     * @brief Process WiFi-related template variables in HTML content
     * @param html HTML content containing template variables
     * @return Processed HTML with variables replaced by actual values
     * 
     * Replaces WiFi-specific template placeholders with current network
     * information. Supports both Station and Access Point mode variables.
     * 
     * @details Supported template variables:
     * - `{{IP_ADDRESS}}`: Current device IP address
     * - `{{WIFI_RSSI}}`: WiFi signal strength in dBm
     * - `{{WIFI_STATUS}}`: Connection status string
     * - `{{WIFI_MODE}}`: Current operational mode
     * - `{{WIFI_MODE_TOGGLE}}`: Toggle mode identifier
     * - `{{WIFI_MODE_BUTTON}}`: Mode toggle button text
     * 
     * @note Variables are replaced with appropriate values for current mode
     * @note Gracefully handles missing or invalid variables
     * 
     * @example
     * @code
     * String html = "<p>IP: {{IP_ADDRESS}}</p>";
     * html = WiFiHelper::processWiFiTemplateVariables(html);
     * // Result: "<p>IP: 192.168.1.100</p>"
     * @endcode
     * 
     * @see getLocalIP()
     * @see getWiFiStatus()
     * @see getRSSI()
     */
    static String processWiFiTemplateVariables(String html);
    
    /**
     * @brief Get formatted WiFi status information
     * @return Multi-line string containing current WiFi status details
     * 
     * Provides comprehensive WiFi status information formatted for display
     * or logging purposes. Content varies based on current operational mode.
     * 
     * @details Status information includes:
     * - Station Mode: SSID, IP, signal strength, connection status
     * - Access Point Mode: AP name, IP, connected client count
     * 
     * @note Information is formatted for human readability
     * @see getConnectionInfo()
     * @see getDebugInfo()
     */
    static String getWiFiStatusInfo();
    
    /** @} */ // End of Template Processing
    
    /**
     * @name Network Information
     * @brief Functions for retrieving network status and configuration
     * @{
     */
    
    /**
     * @brief Check if network connection is available
     * @return true if connected (Station mode) or AP is active, false otherwise
     * 
     * Determines if the device has network connectivity available for
     * applications. Returns true for both successful WiFi connections
     * and active Access Point mode.
     * 
     * @note In AP mode, returns true even without internet connectivity
     * @see isWiFiConnected()
     * @see isAccessPointMode()
     */
    static bool isConnected();
    
    /**
     * @brief Get human-readable connection information
     * @return String describing current network connection status
     * 
     * Provides a concise, human-readable description of the current
     * network connection including mode, network name, IP address,
     * and relevant metrics.
     * 
     * @details Example outputs:
     * - "WiFi: MyNetwork (192.168.1.100) RSSI: -45dBm"
     * - "Access Point: ESP32_Device (192.168.4.1) Clients: 2"
     * - "WiFi: Disconnected"
     * 
     * @note Format varies based on current operational mode
     * @see printStatus()
     * @see getWiFiStatusInfo()
     */
    static String getConnectionInfo();
    
    /**
     * @brief Print current connection status to Serial
     * 
     * Convenience function to output current connection information
     * to the Serial console for debugging and monitoring purposes.
     * 
     * @note Uses getConnectionInfo() for status details
     * @see getConnectionInfo()
     */
    static void printStatus();
    
    /**
     * @brief Get comprehensive debug information for web interface
     * @return HTML-formatted debug information for web display
     * 
     * Generates detailed network debugging information formatted as HTML
     * for display in web-based debug interfaces. Includes all relevant
     * network parameters and status indicators.
     * 
     * @details Debug information includes:
     * - Network mode and status
     * - IP configuration (IP, gateway, DNS)
     * - Hardware information (MAC address)
     * - Signal strength and connection metrics
     * - Access Point specific details when applicable
     * 
     * @note Output includes CSS classes for styling
     * @note Content varies based on operational mode
     * 
     * @see getWiFiStatusInfo()
     * @see getConnectionInfo()
     */
    static String getDebugInfo();
    
    /** @} */ // End of Network Information
    
    /**
     * @name Low-Level Network Access
     * @brief Direct access to network parameters and status
     * @{
     */
    
    /**
     * @brief Get current device IP address
     * @return String containing IP address (Station IP or AP IP)
     * 
     * Returns the appropriate IP address based on current operational mode:
     * - Station mode: Assigned IP from DHCP or static configuration
     * - Access Point mode: AP interface IP address (typically 192.168.4.1)
     * 
     * @note Returns empty string if no IP is assigned
     * @see getGatewayIP()
     * @see getDNSIP()
     */
    static String getLocalIP();
    
    /**
     * @brief Get WiFi signal strength
     * @return Signal strength in dBm, or 0 if not applicable
     * 
     * Returns the received signal strength indicator (RSSI) for the current
     * WiFi connection. Only meaningful in Station mode when connected to
     * an external access point.
     * 
     * @details RSSI interpretation:
     * - -30 dBm: Excellent signal
     * - -50 dBm: Very good signal  
     * - -70 dBm: Good signal
     * - -80 dBm: Weak signal
     * - -90 dBm: Very weak signal
     * 
     * @note Returns 0 in Access Point mode (not applicable)
     * @note Only valid when WiFi is connected in Station mode
     */
    static int getRSSI();
    
    /**
     * @brief Get current WiFi connection status
     * @return String describing connection status
     * 
     * Returns a human-readable string indicating the current WiFi status:
     * - "Connected": Station mode with active WiFi connection
     * - "Disconnected": Station mode without WiFi connection
     * - "Access Point": Operating in AP mode
     * 
     * @see isWiFiConnected()
     * @see isAccessPointMode()
     */
    static String getWiFiStatus();
    
    /**
     * @brief Get current network SSID
     * @return String containing network name
     * 
     * Returns the SSID (network name) for the current connection:
     * - Station mode: SSID of connected WiFi network
     * - Access Point mode: Name of created access point
     * 
     * @note Returns device's AP name when in Access Point mode
     * @see getAccessPointName()
     */
    static String getSSID();
    
    /**
     * @brief Get gateway IP address
     * @return String containing gateway IP address
     * 
     * Returns the network gateway IP address:
     * - Station mode: Router/gateway IP from DHCP
     * - Access Point mode: Device's own IP (acting as gateway)
     * 
     * @note In AP mode, the device acts as the gateway
     * @see getLocalIP()
     * @see getDNSIP()
     */
    static String getGatewayIP();
    
    /**
     * @brief Get DNS server IP address
     * @return String containing DNS server IP address
     * 
     * Returns the DNS server IP address:
     * - Station mode: DNS server provided by DHCP
     * - Access Point mode: Device's own IP (acting as DNS)
     * 
     * @note In AP mode, the device provides basic DNS services
     * @see getGatewayIP()
     * @see getLocalIP()
     */
    static String getDNSIP();
    
    /**
     * @brief Get device MAC address
     * @return String containing MAC address
     * 
     * Returns the MAC address for the appropriate network interface:
     * - Station mode: WiFi station interface MAC
     * - Access Point mode: WiFi AP interface MAC
     * 
     * @note MAC address format: XX:XX:XX:XX:XX:XX
     * @note Different interfaces may have different MAC addresses
     */
    static String getMACAddress();
    
    /**
     * @brief Check if WiFi is connected in Station mode
     * @return true if connected to WiFi network, false otherwise
     * 
     * Specifically checks for active WiFi connection in Station mode.
     * Unlike isConnected(), this returns false when in Access Point mode.
     * 
     * @note Returns false in Access Point mode regardless of client connections
     * @see isConnected()
     * @see isAccessPointMode()
     */
    static bool isWiFiConnected();
    
    /** @} */ // End of Low-Level Network Access

private:
    /**
     * @brief Load HTML template from filesystem
     * @param templatePath Relative path to template file
     * @return String containing template content or error message
     * 
     * @note Template files should be located in /templates/ directory
     * @note Returns error HTML if template file is not found
     */
    static String loadTemplate(const char* templatePath);
    
    /**
     * @brief Internal flag tracking Access Point mode status
     * @note true when operating in AP mode, false for Station mode
     */
    static bool accessPointMode;
    
    /**
     * @brief Private constructor to prevent instantiation
     * @note This class provides only static methods
     */
    WiFiHelper() = delete;
    
    /**
     * @brief Disabled copy constructor
     * @note This class cannot be copied
     */
    WiFiHelper(const WiFiHelper&) = delete;
    
    /**
     * @brief Disabled assignment operator
     * @note This class cannot be assigned
     */
    WiFiHelper& operator=(const WiFiHelper&) = delete;
};

#endif // WIFIHELPER_H
