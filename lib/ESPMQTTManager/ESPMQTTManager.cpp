#include "ESPMQTTManager.h"

// Static instance for callback
ESPMQTTManager* ESPMQTTManager::_instance = nullptr;

ESPMQTTManager::ESPMQTTManager(const char* username, const char* password, const char* fallbackIP, int port)
    : _username(username), _password(password), _serverIP(fallbackIP), _port(port),
      _mqttClient(_wifiClient), _rebootCallback(nullptr),
      _lastTempPublish(0), _lastVersionPublish(0), _lastDiscovery(0) {
    _instance = this;
}

void ESPMQTTManager::begin(String clientId) {
    _clientId = clientId;
    updateTopics(clientId);
    _mqttClient.setServer(_serverIP.c_str(), _port);
    _mqttClient.setCallback(mqttCallback);
}

void ESPMQTTManager::setTopicTemplates(String tempTopic, String cpuTempTopic, String rebootTopic, String firmwareVersionTopic) {
    _topicTemp = tempTopic;
    _topicCpuTemp = cpuTempTopic;
    _topicReboot = rebootTopic;
    _topicFirmwareVersion = firmwareVersionTopic;
}

void ESPMQTTManager::setRebootCallback(void (*callback)()) {
    _rebootCallback = callback;
}

bool ESPMQTTManager::connect() {
    if (_mqttClient.connected()) {
        return true;
    }
    
    while (!_mqttClient.connected()) {
        if (_mqttClient.connect(_clientId.c_str(), _username, _password)) {
            _mqttClient.subscribe(_topicReboot.c_str());
            Serial.println("MQTT connected with Client ID: " + _clientId);
            Serial.println("MQTT server: " + _serverIP);
            return true;
        } else {
            Serial.print("MQTT connection failed, rc=");
            Serial.print(_mqttClient.state());
            Serial.println(" retrying in 5 seconds");
            delay(5000);
        }
    }
    return false;
}

void ESPMQTTManager::disconnect() {
    _mqttClient.disconnect();
}

bool ESPMQTTManager::isConnected() {
    return _mqttClient.connected();
}

void ESPMQTTManager::loop() {
    _mqttClient.loop();
}

String ESPMQTTManager::discoverServer() {
    Serial.println("Searching for Home Assistant server...");
    
    // Scan network for Home Assistant
    String foundIP = scanForHomeAssistant();
    if (foundIP != "") {
        return foundIP;
    }
    
    Serial.println("Home Assistant server not found, using fallback IP");
    return _serverIP; // Return the current fallback IP
}

void ESPMQTTManager::updateServerIP(String newIP) {
    if (newIP != _serverIP) {
        Serial.println("MQTT server changed from " + _serverIP + " to " + newIP);
        _serverIP = newIP;
        _mqttClient.disconnect();
        _mqttClient.setServer(_serverIP.c_str(), _port);
    }
}

String ESPMQTTManager::getCurrentServerIP() {
    return _serverIP;
}

void ESPMQTTManager::updateTopics(String clientId) {
    _clientId = clientId;
    _topicTemp = "home/esp/" + clientId + "/temperature_f";
    _topicCpuTemp = "home/esp/" + clientId + "/cpu_temperature_c";
    _topicReboot = "home/esp/" + clientId + "/reboot";
    _topicFirmwareVersion = "home/esp/" + clientId + "/firmware_version";
}

String ESPMQTTManager::getTempTopic() {
    return _topicTemp;
}

String ESPMQTTManager::getCpuTempTopic() {
    return _topicCpuTemp;
}

String ESPMQTTManager::getRebootTopic() {
    return _topicReboot;
}

String ESPMQTTManager::getFirmwareVersionTopic() {
    return _topicFirmwareVersion;
}

bool ESPMQTTManager::publishTemperature(float temperature) {
    String tempStr = String(temperature, 1); // 1 decimal place
    
    if (_mqttClient.publish(_topicTemp.c_str(), tempStr.c_str())) {
        Serial.printf("Published temperature: %s°F to topic: %s\n", tempStr.c_str(), _topicTemp.c_str());
        return true;
    } else {
        Serial.println("Failed to publish temperature");
        return false;
    }
}

bool ESPMQTTManager::publishCpuTemperature(float temperature) {
    String tempStr = String(temperature, 1); // 1 decimal place
    
    if (_mqttClient.publish(_topicCpuTemp.c_str(), tempStr.c_str())) {
        Serial.printf("Published CPU temperature: %s°C to topic: %s\n", tempStr.c_str(), _topicCpuTemp.c_str());
        return true;
    } else {
        Serial.println("Failed to publish CPU temperature");
        return false;
    }
}

bool ESPMQTTManager::publishFirmwareVersion(int version) {
    String versionStr = String(version);
    
    if (_mqttClient.publish(_topicFirmwareVersion.c_str(), versionStr.c_str(), true)) { // Retain message
        Serial.printf("Published firmware version: %s to topic: %s\n", versionStr.c_str(), _topicFirmwareVersion.c_str());
        return true;
    } else {
        Serial.println("Failed to publish firmware version");
        return false;
    }
}

bool ESPMQTTManager::publish(const char* topic, const char* payload, bool retain) {
    return _mqttClient.publish(topic, payload, retain);
}

bool ESPMQTTManager::subscribe(const char* topic) {
    return _mqttClient.subscribe(topic);
}

bool ESPMQTTManager::unsubscribe(const char* topic) {
    return _mqttClient.unsubscribe(topic);
}

bool ESPMQTTManager::shouldPublishTemperature(unsigned long currentTime) {
    return (currentTime - _lastTempPublish > _tempPublishInterval);
}

bool ESPMQTTManager::shouldPublishFirmwareVersion(unsigned long currentTime) {
    return (currentTime - _lastVersionPublish > _versionPublishInterval);
}

bool ESPMQTTManager::shouldRediscoverServer(unsigned long currentTime) {
    return (currentTime - _lastDiscovery > _discoveryInterval);
}

void ESPMQTTManager::updateLastPublishTime(unsigned long currentTime) {
    _lastTempPublish = currentTime;
}

void ESPMQTTManager::updateLastVersionPublishTime(unsigned long currentTime) {
    _lastVersionPublish = currentTime;
}

void ESPMQTTManager::updateLastDiscoveryTime(unsigned long currentTime) {
    _lastDiscovery = currentTime;
}

String ESPMQTTManager::scanForHomeAssistant() {
    IPAddress localIP = WiFi.localIP();
    String subnet = String(localIP[0]) + "." + String(localIP[1]) + "." + String(localIP[2]) + ".";
    
    Serial.println("Scanning network for Home Assistant on port 8123...");
    
    // Scan a limited range to avoid taking too long
    for (int i = 1; i <= 254; i += 10) { // Check every 10th IP to speed up
        String testIP = subnet + String(i);
        if (testHomeAssistantConnection(testIP)) {
            Serial.printf("Found Home Assistant at: %s\n", testIP.c_str());
            return testIP;
        }
        
        // Check a few IPs around common router ranges
        if (i == 1) {
            // Check common router/server IPs
            int commonIPs[] = {2, 3, 4, 5, 10, 19, 20, 100, 101, 254};
            for (int j = 0; j < 10; j++) {
                String commonIP = subnet + String(commonIPs[j]);
                if (testHomeAssistantConnection(commonIP)) {
                    Serial.printf("Found Home Assistant at: %s\n", commonIP.c_str());
                    return commonIP;
                }
            }
        }
    }
    
    Serial.println("Network scan completed, no Home Assistant found");
    return "";
}

bool ESPMQTTManager::testHomeAssistantConnection(String ip) {
    HTTPClient http;
    http.setTimeout(2000); // 2 second timeout
    http.begin("http://" + ip + ":8123/api/");
    
    int httpCode = http.GET();
    http.end();
    
    // Home Assistant API returns 401 Unauthorized when accessed without token
    // This confirms HA is running on this IP
    return (httpCode == 401 || httpCode == 200);
}

void ESPMQTTManager::mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (_instance) {
        _instance->handleMessage(topic, payload, length);
    }
}

void ESPMQTTManager::handleMessage(char* topic, byte* payload, unsigned int length) {
    if (String(topic) == _topicReboot) {
        Serial.println("Reboot command received via MQTT!");
        if (_rebootCallback) {
            _rebootCallback();
        } else {
            ESP.restart();
        }
    }
}
