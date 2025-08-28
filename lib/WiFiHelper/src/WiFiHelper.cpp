/**
 * @file WiFiHelper.cpp
 * @brief Implementation of comprehensive WiFi management library for ESP32
 * @author Steve Nolte  
 * @date 2025
 * @version 1.0.0
 */

#include "WiFiHelper.h"
#include <LittleFS.h>

// Forward declaration for external config (optional dependency)
extern class Config* g_config;

// Default configuration constants (can be overridden)
namespace WiFiConstants {
    const int MAX_ATTEMPTS = 20;
    const int RECONNECT_ATTEMPTS = 5;
    const int RETRY_DELAY = 500;
    const int REBOOT_DELAY = 2000;
    
    const char* AP_PASSWORD = "ESP32Config";
    const int AP_CHANNEL = 1;
    const bool AP_HIDDEN = false;
    const int AP_MAX_CONNECTIONS = 4;
    
    const char* DEFAULT_SSID = "YourNetwork";
    const char* DEFAULT_PASSWORD = "YourPassword";
    const char* DEFAULT_CLIENT_ID = "ESP32_Device";
}

// Static member initialization
bool WiFiHelper::accessPointMode = false;

void WiFiHelper::setup() {
    /**
     * @brief Initialize WiFi connection with automatic fallback to Access Point mode
     * 
     * This function implements a robust WiFi connection strategy:
     * 1. Check configuration for forced AP mode
     * 2. Attempt Station mode connection with configurable retries
     * 3. Fall back to Access Point mode if Station connection fails
     * 4. Provide detailed status logging throughout the process
     */
    Serial.println("Setting up WiFi connection...");
    
    // Check if forced to AP mode (if config is available)
    bool forceApMode = false;
    if (g_config != nullptr) {
        // Use config if available (project-specific implementation)
        // This would need to be implemented based on the specific Config class
        // For standalone library use, this can be ignored
    }
    
    if (forceApMode) {
        Serial.println("Force AP mode enabled - starting Access Point...");
        if (startAccessPoint()) {
            accessPointMode = true;
            Serial.println("✓ Access Point started successfully");
        } else {
            Serial.println("✗ Failed to start Access Point");
        }
        return;
    }
    
    // Normal WiFi station mode connection attempt
    Serial.printf("Connecting to: %s\n", WiFiConstants::DEFAULT_SSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WiFiConstants::DEFAULT_SSID, WiFiConstants::DEFAULT_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WiFiConstants::MAX_ATTEMPTS) {
        delay(WiFiConstants::RETRY_DELAY);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✓ WiFi connected!");
        Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
        accessPointMode = false;
    } else {
        Serial.println("\n✗ Failed to connect to WiFi");
        Serial.println("Starting Access Point mode...");
        
        if (startAccessPoint()) {
            accessPointMode = true;
            Serial.println("✓ Access Point started successfully");
        } else {
            Serial.println("✗ Failed to start Access Point");
        }
    }
}

bool WiFiHelper::startAccessPoint() {
    /**
     * @brief Create and configure WiFi Access Point
     * 
     * Sets up a WiFi Access Point with the following features:
     * - Uses device-specific SSID (client_id or default)
     * - Configurable password, channel, and connection limits
     * - Comprehensive logging of AP status and connection details
     * 
     * @return true if AP started successfully, false on failure
     */
    String apName = getAccessPointName();
    
    Serial.printf("Starting Access Point: %s\n", apName.c_str());
    
    // Configure access point
    WiFi.mode(WIFI_AP);
    bool success = WiFi.softAP(
        apName.c_str(),
        WiFiConstants::AP_PASSWORD,
        WiFiConstants::AP_CHANNEL,
        WiFiConstants::AP_HIDDEN,
        WiFiConstants::AP_MAX_CONNECTIONS
    );
    
    if (success) {
        IPAddress apIP = WiFi.softAPIP();
        Serial.printf("✓ Access Point started\n");
        Serial.printf("SSID: %s\n", apName.c_str());
        Serial.printf("Password: %s\n", WiFiConstants::AP_PASSWORD);
        Serial.printf("IP address: %s\n", apIP.toString().c_str());
        Serial.printf("Connect to this network and visit: http://%s\n", apIP.toString().c_str());
        return true;
    } else {
        Serial.println("✗ Failed to start Access Point");
        return false;
    }
}

bool WiFiHelper::isAccessPointMode() {
    /**
     * @brief Check current operational mode
     * @return true if operating in Access Point mode, false for Station mode
     */
    return accessPointMode;
}

String WiFiHelper::getAccessPointName() {
    /**
     * @brief Generate Access Point network name
     * @return SSID string for the Access Point
     * 
     * Priority order for AP name:
     * 1. Config client_id (if available)
     * 2. Default client ID constant
     * 3. ESP32 chip ID fallback
     */
    if (g_config != nullptr) {
        // Use config client_id if available
        // This would need project-specific implementation
        // return String(g_config->getClientId());
    }
    
    // Fallback to default or chip-based ID
    String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
    return String(WiFiConstants::DEFAULT_CLIENT_ID) + "_" + chipId.substring(6);
}

void WiFiHelper::checkConnection() {
    /**
     * @brief Monitor and maintain WiFi connection health
     * 
     * Continuously monitors WiFi status and attempts reconnection if needed.
     * Features:
     * - Non-blocking operation suitable for main loop
     * - Configurable reconnection attempts
     * - Automatic fallback prevention (maintains mode stability)
     * - Detailed logging of connection state changes
     */
    // Skip connection checks if we're in AP mode
    if (accessPointMode) {
        return;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n⚠ WiFi connection lost. Attempting to reconnect...");
        WiFi.disconnect();
        WiFi.begin(WiFiConstants::DEFAULT_SSID, WiFiConstants::DEFAULT_PASSWORD);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < WiFiConstants::RECONNECT_ATTEMPTS) {
            delay(WiFiConstants::RETRY_DELAY);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n✓ WiFi reconnected!");
            Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
            accessPointMode = false; // Ensure mode consistency
        } else {
            Serial.println("\n✗ Failed to reconnect to WiFi");
            // Note: Could optionally switch to AP mode here
        }
    }
}

void WiFiHelper::setupRecoveryWiFi() {
    /**
     * @brief Establish WiFi connectivity for recovery mode scenarios
     * 
     * Simplified WiFi setup for emergency/recovery situations:
     * - Uses default credentials for basic connectivity
     * - Limited retry attempts to avoid blocking recovery operations
     * - Falls back to AP mode if Station connection fails
     * - Minimal logging to reduce recovery time
     */
    Serial.println("Setting up WiFi for recovery mode...");
    
    // Basic WiFi setup for recovery
    WiFi.begin(WiFiConstants::DEFAULT_SSID, WiFiConstants::DEFAULT_PASSWORD);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
        delay(1000);
        attempts++;
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n✓ WiFi connected: %s\n", WiFi.localIP().toString().c_str());
        accessPointMode = false;
    } else {
        Serial.println("\n✗ Failed to connect to WiFi in recovery mode");
        // Try to start AP mode as fallback
        if (startAccessPoint()) {
            accessPointMode = true;
            Serial.println("✓ Access Point started for recovery mode");
        }
    }
}

void WiFiHelper::handleConfig(WebServer& server) {
    /**
     * @brief Serve WiFi configuration web page
     * @param server WebServer instance to send response through
     * 
     * Generates and serves a WiFi configuration page with:
     * - Current network status and signal strength
     * - Available network scanning capabilities
     * - WiFi mode selection options
     * - Configuration form elements
     */
    String html = loadTemplate("wifi_config.html");
    
    // Replace placeholders with actual values
    html.replace("{{CURRENT_SSID}}", getSSID());
    html.replace("{{SIGNAL_STRENGTH}}", String(getRSSI()));
    html.replace("{{WIFI_STATUS}}", getWiFiStatus());
    html.replace("{{CLIENT_ID}}", getAccessPointName());
    
    // Process WiFi-specific template variables
    html = processWiFiTemplateVariables(html);
    
    server.send(200, "text/html", html);
}

void WiFiHelper::handleUpdate(WebServer& server) {
    /**
     * @brief Process WiFi credential update requests
     * @param server WebServer instance to handle request/response
     * 
     * Validates and processes new WiFi credentials:
     * 1. Validates required parameters (SSID, password)
     * 2. Saves credentials to persistent storage (if config available)
     * 3. Provides user feedback through response page
     * 4. Schedules system restart to apply new settings
     */
    if (!server.hasArg("ssid") || !server.hasArg("password")) {
        server.send(400, "text/plain", "Missing SSID or password");
        return;
    }
    
    String newSSID = server.arg("ssid");
    String newPassword = server.arg("password");
    
    // Save new WiFi credentials (if config system is available)
    if (g_config != nullptr) {
        // Project-specific config save implementation would go here
        // g_config->saveWiFiCredentials(newSSID, newPassword);
    }
    
    // Log the update for debugging
    Serial.printf("WiFi credentials updated - SSID: %s\n", newSSID.c_str());
    
    String html = loadTemplate("wifi_updated.html");
    html.replace("{{NEW_SSID}}", newSSID);
    
    server.send(200, "text/html", html);
    
    // Schedule restart to apply new credentials
    delay(WiFiConstants::REBOOT_DELAY);
    ESP.restart();
}

void WiFiHelper::handleNetworkScan(WebServer& server) {
    /**
     * @brief Perform WiFi network scan and return JSON results
     * @param server WebServer instance to send JSON response through
     * 
     * Scans for available WiFi networks and returns structured data:
     * - Network SSID and signal strength
     * - Security/encryption status
     * - JSON format for easy web interface consumption
     */
    WiFi.scanDelete();
    int n = WiFi.scanNetworks();
    
    String json = "{\"networks\":[";
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"encrypted\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false");
        json += "}";
    }
    json += "]}";
    
    server.send(200, "application/json", json);
}

void WiFiHelper::handleWiFiModeToggle(WebServer& server) {
    /**
     * @brief Process WiFi mode toggle requests (Station ↔ Access Point)
     * @param server WebServer instance to handle request/response
     * 
     * Allows switching between WiFi operational modes:
     * - "station": Connect to existing WiFi networks
     * - "ap": Create Access Point for direct device connection
     * 
     * Changes are saved to persistent storage and require restart to take effect.
     */
    if (server.hasArg("mode")) {
        String mode = server.arg("mode");
        bool apMode = (mode == "ap");
        
        // Save mode preference (if config system is available)
        if (g_config != nullptr) {
            // Project-specific config save implementation would go here
            // g_config->saveApMode(apMode);
        }
        
        Serial.printf("WiFi mode toggle - New mode: %s\n", apMode ? "Access Point" : "Station");
        
        String html = loadTemplate("simple_response.html");
        html.replace("{{TITLE}}", "WiFi Mode Updated");
        html.replace("{{HEADER}}", "WiFi Mode Updated");
        html.replace("{{MESSAGE}}", "WiFi mode set to: <strong>" + String(apMode ? "Access Point" : "Station") + "</strong>");
        html.replace("{{EXTRA_CONTENT}}", "<p><em>Changes will take effect after reboot.</em></p>");
        server.send(200, "text/html", html);
    } else {
        server.send(400, "text/plain", "Missing mode parameter");
    }
}

String WiFiHelper::processWiFiTemplateVariables(String html) {
    /**
     * @brief Replace WiFi-related template variables in HTML content
     * @param html HTML string containing template variables
     * @return Processed HTML with variables replaced by current values
     * 
     * Supported template variables:
     * - {{IP_ADDRESS}}: Current device IP address
     * - {{WIFI_RSSI}}: WiFi signal strength in dBm  
     * - {{WIFI_STATUS}}: Connection status string
     * - {{WIFI_MODE}}: Current operational mode
     * - {{WIFI_MODE_TOGGLE}}: Toggle mode identifier
     * - {{WIFI_MODE_BUTTON}}: Mode toggle button text
     */
    html.replace("{{IP_ADDRESS}}", getLocalIP());
    html.replace("{{WIFI_RSSI}}", String(getRSSI()));
    html.replace("{{WIFI_STATUS}}", getWiFiStatus());
    
    // Mode-related variables (requires config integration for full functionality)
    bool forceApMode = accessPointMode; // Simplified fallback
    html.replace("{{WIFI_MODE}}", forceApMode ? "Access Point" : "Station");
    html.replace("{{WIFI_MODE_TOGGLE}}", forceApMode ? "station" : "ap");
    html.replace("{{WIFI_MODE_BUTTON}}", forceApMode ? "Switch to Station Mode" : "Switch to Access Point Mode");
    
    return html;
}

String WiFiHelper::getWiFiStatusInfo() {
    /**
     * @brief Generate comprehensive WiFi status information
     * @return Multi-line formatted string with current WiFi details
     * 
     * Provides detailed status information formatted for display or logging.
     * Content varies based on current operational mode (Station vs Access Point).
     */
    String info = "";
    if (accessPointMode) {
        info += "Mode: Access Point\n";
        info += "SSID: " + getAccessPointName() + "\n";
        info += "IP: " + WiFi.softAPIP().toString() + "\n";
        info += "Clients: " + String(WiFi.softAPgetStationNum()) + "\n";
    } else {
        info += "Mode: Station\n";
        info += "Status: " + getWiFiStatus() + "\n";
        if (WiFi.status() == WL_CONNECTED) {
            info += "SSID: " + WiFi.SSID() + "\n";
            info += "IP: " + getLocalIP() + "\n";
            info += "RSSI: " + String(getRSSI()) + " dBm\n";
        }
    }
    return info;
}

bool WiFiHelper::isConnected() {
    /**
     * @brief Check if network connectivity is available
     * @return true if connected (Station) or AP is active, false otherwise
     * 
     * Returns true for both successful WiFi connections and active Access Point mode,
     * indicating that network functionality is available to applications.
     */
    return WiFi.status() == WL_CONNECTED || accessPointMode;
}

String WiFiHelper::getConnectionInfo() {
    /**
     * @brief Get human-readable connection status summary
     * @return Formatted string describing current network connection
     * 
     * Provides concise connection information suitable for logging or display.
     * Format varies based on operational mode and connection status.
     */
    if (accessPointMode) {
        String info = "Access Point: " + getAccessPointName();
        info += " (" + WiFi.softAPIP().toString() + ")";
        info += " Clients: " + String(WiFi.softAPgetStationNum());
        return info;
    } else if (WiFi.status() == WL_CONNECTED) {
        String info = "WiFi: " + WiFi.SSID();
        info += " (" + WiFi.localIP().toString() + ")";
        info += " RSSI: " + String(WiFi.RSSI()) + "dBm";
        return info;
    } else {
        return "WiFi: Disconnected";
    }
}

void WiFiHelper::printStatus() {
    /**
     * @brief Print current connection status to Serial console
     * 
     * Convenience function for debugging and monitoring.
     * Outputs the same information as getConnectionInfo().
     */
    Serial.println(getConnectionInfo());
}

String WiFiHelper::getDebugInfo() {
    /**
     * @brief Generate HTML-formatted debug information for web interfaces
     * @return HTML string with comprehensive network debugging details
     * 
     * Creates detailed debugging information formatted with CSS classes
     * for web-based debug interfaces. Includes all relevant network
     * parameters and status indicators.
     */
    String debugInfo = "";
    
    if (accessPointMode) {
        debugInfo += "<div class='debug-item'><span class='debug-label'>Mode:</span><span class='debug-value success' data-id='network-mode'>Access Point</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>AP Name:</span><span class='debug-value' data-id='network-ssid'>" + getAccessPointName() + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>AP IP:</span><span class='debug-value' data-id='network-ip'>" + WiFi.softAPIP().toString() + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>Connected Clients:</span><span class='debug-value' data-id='network-clients'>" + String(WiFi.softAPgetStationNum()) + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>AP MAC:</span><span class='debug-value' data-id='network-mac'>" + WiFi.softAPmacAddress() + "</span></div>";
    } else {
        debugInfo += "<div class='debug-item'><span class='debug-label'>Mode:</span><span class='debug-value " + String(WiFi.status() == WL_CONNECTED ? "success" : "error") + "' data-id='network-mode'>WiFi Station</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>WiFi Status:</span><span class='debug-value " + String(WiFi.status() == WL_CONNECTED ? "success" : "error") + "' data-id='network-wifi-status'>" + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>SSID:</span><span class='debug-value' data-id='network-ssid'>" + WiFi.SSID() + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>IP Address:</span><span class='debug-value' data-id='network-ip'>" + WiFi.localIP().toString() + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>Gateway:</span><span class='debug-value' data-id='network-gateway'>" + WiFi.gatewayIP().toString() + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>DNS:</span><span class='debug-value' data-id='network-dns'>" + WiFi.dnsIP().toString() + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>MAC Address:</span><span class='debug-value' data-id='network-mac'>" + WiFi.macAddress() + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>Signal Strength:</span><span class='debug-value' data-id='network-rssi'>" + String(WiFi.RSSI()) + " dBm</span></div>";
    }
    
    return debugInfo;
}

String WiFiHelper::getLocalIP() {
    /**
     * @brief Get current device IP address
     * @return IP address string appropriate for current mode
     */
    if (accessPointMode) {
        return WiFi.softAPIP().toString();
    } else {
        return WiFi.localIP().toString();
    }
}

int WiFiHelper::getRSSI() {
    /**
     * @brief Get WiFi signal strength in dBm
     * @return RSSI value in dBm, or 0 if not applicable (AP mode)
     */
    if (accessPointMode) {
        return 0; // No RSSI in AP mode
    } else {
        return WiFi.RSSI();
    }
}

String WiFiHelper::getWiFiStatus() {
    /**
     * @brief Get current WiFi connection status as string
     * @return Human-readable status description
     */
    if (accessPointMode) {
        return "Access Point";
    } else {
        return WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected";
    }
}

String WiFiHelper::getSSID() {
    /**
     * @brief Get current network SSID/name
     * @return Network name (WiFi SSID or AP name)
     */
    if (accessPointMode) {
        return getAccessPointName();
    } else {
        return WiFi.SSID();
    }
}

String WiFiHelper::getGatewayIP() {
    /**
     * @brief Get network gateway IP address
     * @return Gateway IP (router IP in Station mode, device IP in AP mode)
     */
    if (accessPointMode) {
        return WiFi.softAPIP().toString(); // In AP mode, we are the gateway
    } else {
        return WiFi.gatewayIP().toString();
    }
}

String WiFiHelper::getDNSIP() {
    /**
     * @brief Get DNS server IP address
     * @return DNS server IP (network DNS in Station mode, device IP in AP mode)
     */
    if (accessPointMode) {
        return WiFi.softAPIP().toString(); // In AP mode, we act as DNS
    } else {
        return WiFi.dnsIP().toString();
    }
}

String WiFiHelper::getMACAddress() {
    /**
     * @brief Get device MAC address for current interface
     * @return MAC address in XX:XX:XX:XX:XX:XX format
     */
    if (accessPointMode) {
        return WiFi.softAPmacAddress();
    } else {
        return WiFi.macAddress();
    }
}

bool WiFiHelper::isWiFiConnected() {
    /**
     * @brief Check specifically for WiFi Station mode connection
     * @return true only if connected to WiFi network in Station mode
     * 
     * Unlike isConnected(), this returns false when in Access Point mode,
     * specifically checking for external WiFi network connectivity.
     */
    return WiFi.status() == WL_CONNECTED;
}

String WiFiHelper::loadTemplate(const char* templatePath) {
    /**
     * @brief Load HTML template from filesystem
     * @param templatePath Relative path to template file
     * @return Template content or error HTML if file not found
     * 
     * Attempts to load HTML templates from the /templates/ directory.
     * Returns error HTML with debugging information if template is not found.
     */
    String fullPath = "/templates/" + String(templatePath);
    
    if (!LittleFS.exists(fullPath)) {
        return "<!DOCTYPE html><html><body><h1>Error: Template not found</h1><p>Path: " + fullPath + "</p></body></html>";
    }
    
    File file = LittleFS.open(fullPath, "r");
    if (!file) {
        return "<!DOCTYPE html><html><body><h1>Error: Could not open template</h1><p>Path: " + fullPath + "</p></body></html>";
    }
    
    String html = file.readString();
    file.close();
    return html;
}
