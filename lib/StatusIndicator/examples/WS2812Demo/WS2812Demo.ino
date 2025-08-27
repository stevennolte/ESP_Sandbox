/**
 * @file WS2812Demo.ino
 * @brief StatusIndicator Library - WS2812 Addressable RGB LED Demo
 * @author ESP_Sandbox Project
 * @version 1.0.0
 * @date 2025-08-26
 * 
 * This example demonstrates WS2812 addressable RGB LED functionality with the StatusIndicator library.
 * It's specifically designed for ESP32-S3-DevKitC-1 boards that have a built-in WS2812 RGB LED.
 * 
 * The WS2812 LED is connected to GPIO38 on ESP32-S3-DevKitC-1 boards and can display
 * any RGB color with smooth animations.
 * 
 * @note This example works best on ESP32-S3-DevKitC-1 boards with built-in WS2812 LEDs.
 */

#include <StatusIndicator.h>

/// @brief StatusIndicator instance - will auto-detect WS2812 on ESP32-S3-DevKitC-1
StatusIndicator indicator;

// Custom colors for demo
struct Color {
    uint8_t r, g, b;
    String name;
};

Color demoColors[] = {
    {255, 0, 0, "Red"},
    {0, 255, 0, "Green"},
    {0, 0, 255, "Blue"},
    {255, 255, 0, "Yellow"},
    {255, 0, 255, "Magenta"},
    {0, 255, 255, "Cyan"},
    {255, 128, 0, "Orange"},
    {128, 0, 255, "Purple"},
    {255, 255, 255, "White"}
};

int currentColor = 0;
unsigned long lastChange = 0;
const unsigned long DEMO_INTERVAL = 2000; // 2 seconds per color

/**
 * @brief Arduino setup function
 * 
 * Initializes serial communication and the StatusIndicator library.
 * Verifies WS2812 detection and provides guidance if not detected.
 */
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== StatusIndicator WS2812 Demo ===");
    
    // Initialize the status indicator
    indicator.begin(128);  // Medium brightness
    
    // Check if WS2812 was detected
    if (indicator.getLEDType() == LEDType::WS2812_LED) {
        Serial.println("✓ WS2812 LED detected! Starting color demo...");
        Serial.println("Colors will change every 2 seconds.");
    } else {
        Serial.println("⚠ WS2812 LED not detected.");
        Serial.println("This demo is designed for ESP32-S3-DevKitC-1 boards.");
        Serial.println("You can also manually create a WS2812 indicator:");
        Serial.println("  StatusIndicator indicator(38, LEDType::WS2812_LED);");
        Serial.println("\nFalling back to standard status demo...");
    }
    
    Serial.println();
}

/**
 * @brief Arduino main loop function
 * 
 * Demonstrates WS2812 capabilities by cycling through custom colors
 * or falls back to standard status demonstrations for other LED types.
 */
void loop() {
    // Always call update() for animations to work
    indicator.update();
    
    // Check if it's time to change demo
    if (millis() - lastChange >= DEMO_INTERVAL) {
        
        if (indicator.getLEDType() == LEDType::WS2812_LED) {
            // WS2812 custom color demo
            Color color = demoColors[currentColor];
            Serial.printf("Setting color: %s (R:%d, G:%d, B:%d)\n", 
                         color.name.c_str(), color.r, color.g, color.b);
            
            indicator.setRGBColor(color.r, color.g, color.b);
            
            currentColor = (currentColor + 1) % (sizeof(demoColors) / sizeof(demoColors[0]));
            
        } else {
            // Standard status demo for other LED types
            static int statusDemo = 0;
            
            switch (statusDemo % 6) {
                case 0:
                    Serial.println("Normal status (green)");
                    indicator.showNormal();
                    break;
                case 1:
                    Serial.println("Connecting status (cyan breathing)");
                    indicator.showConnecting();
                    break;
                case 2:
                    Serial.println("Success status (green solid)");
                    indicator.showSuccess();
                    break;
                case 3:
                    Serial.println("Warning status (orange blink)");
                    indicator.showWarning();
                    break;
                case 4:
                    Serial.println("Error status (red fast blink)");
                    indicator.showError();
                    break;
                case 5:
                    Serial.println("Off");
                    indicator.off();
                    break;
            }
            statusDemo++;
        }
        
        lastChange = millis();
        indicator.printStatus();
        Serial.println();
    }
    
    delay(10);  // Small delay for smooth operation
}
