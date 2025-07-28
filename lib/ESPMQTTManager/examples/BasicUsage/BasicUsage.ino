#include <WiFi.h>
#include <ESPMQTTManager.h>

// WiFi credentials
const char* ssid = "your_wifi_ssid";
const char* password = "your_wifi_password";

// MQTT credentials
const char* mqtt_user = "your_mqtt_username";
const char* mqtt_pass = "your_mqtt_password";

// Create MQTT manager instance
ESPMQTTManager mqttManager(mqtt_user, mqtt_pass);

String client_id = "ESP_Example";

void setup() {
    Serial.begin(115200);
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    
    // Initialize MQTT manager
    mqttManager.begin(client_id);
    
    // Discover Home Assistant server (optional - will use fallback if not found)
    String serverIP = mqttManager.discoverServer();
    mqttManager.updateServerIP(serverIP);
    
    Serial.println("Setup complete!");
}

void loop() {
    // Ensure MQTT connection
    if (!mqttManager.isConnected()) {
        mqttManager.connect();
    }
    mqttManager.loop();
    
    unsigned long currentTime = millis();
    
    // Publish temperature every 10 seconds
    if (mqttManager.shouldPublishTemperature(currentTime)) {
        float temperature = 25.5; // Replace with actual temperature reading
        mqttManager.publishCpuTemperature(temperature);
        mqttManager.updateLastPublishTime(currentTime);
    }
    
    // Publish firmware version every 5 minutes
    if (mqttManager.shouldPublishFirmwareVersion(currentTime)) {
        int firmwareVersion = 10; // Your firmware version
        mqttManager.publishFirmwareVersion(firmwareVersion);
        mqttManager.updateLastVersionPublishTime(currentTime);
    }
    
    // Re-discover server every 15 minutes
    if (mqttManager.shouldRediscoverServer(currentTime)) {
        String newServer = mqttManager.discoverServer();
        mqttManager.updateServerIP(newServer);
        mqttManager.updateLastDiscoveryTime(currentTime);
    }
    
    delay(1000);
}
