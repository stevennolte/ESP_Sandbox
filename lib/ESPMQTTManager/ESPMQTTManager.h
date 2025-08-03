#ifndef ESP_MQTT_MANAGER_H
#define ESP_MQTT_MANAGER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>

class ESPMQTTManager {
public:
    // Constructor
    ESPMQTTManager(const char* username, const char* password, const char* fallbackIP = "192.168.1.12", int port = 1883);
    
    // Initialization and setup
    void begin(String clientId);
    void setTopicTemplates(String tempTopic, String cpuTempTopic, String rebootTopic, String firmwareVersionTopic);
    void setRebootCallback(void (*callback)());
    
    // Connection management
    bool connect();
    void disconnect();
    bool isConnected();
    void loop();
    
    // Server discovery
    String discoverServer();
    void updateServerIP(String newIP);
    String getCurrentServerIP();
    
    // Topic management
    void updateTopics(String clientId);
    String getTempTopic();
    String getCpuTempTopic();
    String getRebootTopic();
    String getFirmwareVersionTopic();
    
    // Publishing
    bool publishTemperature(float temperature);
    bool publishCpuTemperature(float temperature);
    bool publishFirmwareVersion(int version);
    bool publish(const char* topic, const char* payload, bool retain = false);
    
    // Subscription
    bool subscribe(const char* topic);
    bool unsubscribe(const char* topic);
    
    // Timing management
    bool shouldPublishTemperature(unsigned long currentTime);
    bool shouldPublishFirmwareVersion(unsigned long currentTime);
    bool shouldRediscoverServer(unsigned long currentTime);
    void updateLastPublishTime(unsigned long currentTime);
    void updateLastVersionPublishTime(unsigned long currentTime);
    void updateLastDiscoveryTime(unsigned long currentTime);
    
private:
    // MQTT credentials and settings
    const char* _username;
    const char* _password;
    String _serverIP;
    int _port;
    String _clientId;
    
    // Topic templates
    String _topicTemp;
    String _topicCpuTemp;
    String _topicReboot;
    String _topicFirmwareVersion;
    
    // Timing variables
    unsigned long _lastTempPublish;
    unsigned long _lastVersionPublish;
    unsigned long _lastDiscovery;
    static const unsigned long _tempPublishInterval = 10 * 1000UL; // 10 seconds
    static const unsigned long _versionPublishInterval = 1 * 60 * 1000UL; // 5 minutes
    static const unsigned long _discoveryInterval = 15 * 60 * 1000UL; // 15 minutes
    
    // MQTT client
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;
    
    // Callback function for reboot command
    void (*_rebootCallback)();
    
    // Private helper methods
    String scanForHomeAssistant();
    bool testHomeAssistantConnection(String ip);
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
    static ESPMQTTManager* _instance; // For static callback
    void handleMessage(char* topic, byte* payload, unsigned int length);
};

#endif
