/**
 * @file BasicUsage.ino
 * @brief StatusIndicator Library - Basic Usage Example
 * @author ESP_Sandbox Project
 * @version 1.0.0
 * @date 2025-08-26
 * 
 * This example demonstrates the basic functionality of the StatusIndicator library.
 * It cycles through different status types and modes to show the various capabilities.
 * 
 * The library automatically detects whether your ESP32 has a simple LED or RGB capabilities.
 * 
 * @note Always call indicator.update() in your main loop for animations to work properly.
 */

#include <StatusIndicator.h>

/// @brief StatusIndicator instance with automatic board detection
StatusIndicator indicator;

/**
 * @brief Arduino setup function
 * 
 * Initializes serial communication and the StatusIndicator library.
 * Prints detected configuration information for debugging.
 */
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== StatusIndicator Library Example ===");
    
    // Initialize the status indicator
    indicator.begin(128);  // Medium brightness
    
    // Print detected configuration
    String ledTypeStr = (indicator.getLEDType() == LEDType::WS2812_LED) ? "WS2812 RGB LED" : "Single LED";
    Serial.printf("Detected LED type: %s\n", ledTypeStr.c_str());
    
    Serial.println("Starting status demonstration...\n");
}

/**
 * @brief Arduino main loop function
 * 
 * Continuously updates the status indicator and cycles through different
 * status demonstrations every 3 seconds. This showcases the various
 * animation modes and status types available in the library.
 */
void loop() {
    // Always call update() for animations to work
    indicator.update();
    
    // Demonstrate different statuses
    static unsigned long lastChange = 0;
    static int currentDemo = 0;
    
    if (millis() - lastChange >= 3000) {  // Change every 3 seconds
        switch (currentDemo) {
            case 0:
                Serial.println("Showing NORMAL status (green pulsing)");
                indicator.showNormal();
                break;
                
            case 1:
                Serial.println("Showing CONNECTING status (cyan breathing)");
                indicator.showConnecting();
                break;
                
            case 2:
                Serial.println("Showing SUCCESS status (green solid)");
                indicator.showSuccess();
                break;
                
            case 3:
                Serial.println("Showing WARNING status (orange slow blink)");
                indicator.showWarning();
                break;
                
            case 4:
                Serial.println("Showing ERROR status (red fast blink)");
                indicator.showError();
                break;
                
            case 5:
                Serial.println("Showing INFO status (blue solid)");
                indicator.showInfo();
                break;
                
            case 6:
                Serial.println("Showing RECOVERY status (red fast blink)");
                indicator.showRecovery();
                break;
                
            case 7:
                if (indicator.getLEDType() == LEDType::WS2812_LED) {
                    Serial.println("Showing custom WS2812 colors (cycling through RGB)");
                    static int colorDemo = 0;
                    switch (colorDemo % 3) {
                        case 0: indicator.setRGBColor(255, 0, 0); break;    // Red
                        case 1: indicator.setRGBColor(0, 255, 0); break;    // Green  
                        case 2: indicator.setRGBColor(0, 0, 255); break;    // Blue
                    }
                    colorDemo++;
                } else {
                    Serial.println("Showing BREATHE effect");
                    indicator.setStatusWithMode(StatusType::NORMAL, IndicatorMode::BREATHE);
                }
                break;
                
            case 8:
                Serial.println("Turning OFF");
                indicator.off();
                break;
                
            default:
                currentDemo = -1;  // Reset cycle
                break;
        }
        
        currentDemo++;
        lastChange = millis();
        
        // Print current status
        indicator.printStatus();
        Serial.println();
    }
    
    delay(10);  // Small delay for smooth operation
}
