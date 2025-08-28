/**
 * @file SimpleConnection.ino
 * @brief Minimal example showing basic WiFiHelper usage
 * @author WiFiHelper Library
 * @version 1.0.0
 * 
 * This is the simplest possible WiFiHelper example that demonstrates:
 * - Basic WiFi connection
 * - Connection monitoring
 * - Status reporting
 * 
 * Perfect for getting started or when you just need basic WiFi connectivity
 * without a web interface.
 */

#include <WiFiHelper.h>

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== WiFiHelper Simple Connection Example ===");
    
    // Initialize WiFi - that's it!
    Serial.println("Setting up WiFi...");
    WiFiHelper::setup();
    
    // Check if we're connected
    if (WiFiHelper::isConnected()) {
        Serial.println("✓ WiFi connected successfully!");
        Serial.println("📶 " + WiFiHelper::getConnectionInfo());
    } else {
        Serial.println("✗ WiFi connection failed");
        Serial.println("📡 Device is now in Access Point mode");
        Serial.println("🔗 Connect to: " + WiFiHelper::getSSID());
        Serial.println("🌐 Configuration at: http://" + WiFiHelper::getLocalIP());
    }
    
    Serial.println("\n=== Ready ===");
}

void loop() {
    static unsigned long lastCheck = 0;
    static unsigned long lastStatus = 0;
    
    // Check connection health every 30 seconds
    if (millis() - lastCheck > 30000) {
        WiFiHelper::checkConnection();
        lastCheck = millis();
    }
    
    // Print status every 60 seconds
    if (millis() - lastStatus > 60000) {
        Serial.println("Status: " + WiFiHelper::getWiFiStatus());
        if (WiFiHelper::isWiFiConnected()) {
            Serial.println("Signal: " + String(WiFiHelper::getRSSI()) + " dBm");
        }
        lastStatus = millis();
    }
    
    // Your application code goes here
    
    delay(1000);
}
