#include "WiFiHelper.h"
#include <LittleFS.h>

// Get config instance
extern Config& config;

void WiFiHelper::setup() {
    Serial.println("Setting up WiFi connection...");
    Serial.printf("Connecting to: %s\n", config.getWiFiSSID());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.getWiFiSSID(), config.getWiFiPassword());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < ConfigConstants::WiFi::MAX_ATTEMPTS) {
        delay(ConfigConstants::WiFi::RETRY_DELAY);
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✓ WiFi connected!");
        Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
    } else {
        Serial.println("\n✗ Failed to connect to WiFi");
        Serial.println("Please check your WiFi credentials in the web interface");
    }
}

void WiFiHelper::checkConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n⚠ WiFi connection lost. Attempting to reconnect...");
        WiFi.disconnect();
        WiFi.begin(config.getWiFiSSID(), config.getWiFiPassword());
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < ConfigConstants::WiFi::RECONNECT_ATTEMPTS) {
            delay(ConfigConstants::WiFi::RETRY_DELAY);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n✓ WiFi reconnected!");
            Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
        } else {
            Serial.println("\n✗ Failed to reconnect to WiFi");
        }
    }
}

void WiFiHelper::handleConfig(WebServer& server) {
    String html = loadTemplate("wifi_config.html");
    
    // Replace placeholders with actual values
    html.replace("{{CURRENT_SSID}}", WiFi.SSID());
    html.replace("{{SIGNAL_STRENGTH}}", String(WiFi.RSSI()));
    html.replace("{{WIFI_STATUS}}", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    
    server.send(200, "text/html", html);
}

void WiFiHelper::handleUpdate(WebServer& server) {
    if (!server.hasArg("ssid") || !server.hasArg("password")) {
        server.send(400, "text/plain", "Missing SSID or password");
        return;
    }
    
    String newSSID = server.arg("ssid");
    String newPassword = server.arg("password");
    
    // Save new WiFi credentials using config
    config.saveWiFiCredentials(newSSID, newPassword);
    
    String html = loadTemplate("wifi_updated.html");
    html.replace("{{NEW_SSID}}", newSSID);
    
    server.send(200, "text/html", html);
    
    delay(ConfigConstants::Timing::REBOOT_DELAY);
    ESP.restart();
}

void WiFiHelper::handleNetworkScan(WebServer& server) {
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

bool WiFiHelper::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiHelper::getConnectionInfo() {
    if (!isConnected()) {
        return "WiFi: Disconnected";
    }
    
    String info = "WiFi: " + WiFi.SSID();
    info += " (" + WiFi.localIP().toString() + ")";
    info += " RSSI: " + String(WiFi.RSSI()) + "dBm";
    return info;
}

void WiFiHelper::printStatus() {
    Serial.println(getConnectionInfo());
}

String WiFiHelper::getDebugInfo() {
    String debugInfo = "";
    debugInfo += "<div class='debug-item'><span class='debug-label'>WiFi Status:</span><span class='debug-value " + String(WiFi.status() == WL_CONNECTED ? "success" : "error") + "' data-id='network-wifi-status'>" + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "</span></div>";
    debugInfo += "<div class='debug-item'><span class='debug-label'>SSID:</span><span class='debug-value' data-id='network-ssid'>" + WiFi.SSID() + "</span></div>";
    debugInfo += "<div class='debug-item'><span class='debug-label'>IP Address:</span><span class='debug-value' data-id='network-ip'>" + WiFi.localIP().toString() + "</span></div>";
    debugInfo += "<div class='debug-item'><span class='debug-label'>Gateway:</span><span class='debug-value' data-id='network-gateway'>" + WiFi.gatewayIP().toString() + "</span></div>";
    debugInfo += "<div class='debug-item'><span class='debug-label'>DNS:</span><span class='debug-value' data-id='network-dns'>" + WiFi.dnsIP().toString() + "</span></div>";
    debugInfo += "<div class='debug-item'><span class='debug-label'>MAC Address:</span><span class='debug-value' data-id='network-mac'>" + WiFi.macAddress() + "</span></div>";
    debugInfo += "<div class='debug-item'><span class='debug-label'>Signal Strength:</span><span class='debug-value' data-id='network-rssi'>" + String(WiFi.RSSI()) + " dBm</span></div>";
    return debugInfo;
}

String WiFiHelper::loadTemplate(const char* templatePath) {
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
