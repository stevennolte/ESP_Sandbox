/**
 * @file StatusIndicator.cpp
 * @brief Implementation of StatusIndicator class for ESP32 LED control
 * @author ESP_Sandbox Project
 * @version 1.0.0
 * @date 2025-08-26
 */

#include "StatusIndicator.h"

// Constructor - Auto-detect board type
/**
 * @brief Default constructor with automatic board detection
 * 
 * Initializes the StatusIndicator with automatic board type detection.
 * Sets default values for all internal variables and configures pins
 * based on the detected board type.
 */
StatusIndicator::StatusIndicator() {
    ledType = detectBoardLEDType();
    currentMode = IndicatorMode::OFF;
    currentStatus = StatusType::NORMAL;
    brightness = 128;
    enabled = true;
    lastUpdate = 0;
    pulseInterval = 20;
    pulseDirection = true;
    pulseValue = 0;
    hueValue = 0;
    
    // Set pins based on detected board type
    if (ledType == LEDType::RGB_LED) {
        // ESP32 DevKit-C RGB pins (common configuration)
        redPin = 25;
        greenPin = 26;
        bluePin = 27;
        redChannel = 0;
        greenChannel = 1;
        blueChannel = 2;
    } else {
        // Standard ESP32 built-in LED
        ledPin = 2;
        ledChannel = 0;
    }
}

/**
 * @brief Constructor for single LED configuration
 * @param pin GPIO pin number for the LED
 * 
 * Initializes the StatusIndicator for a single LED setup on the specified pin.
 */
StatusIndicator::StatusIndicator(int pin) {
    ledType = LEDType::SINGLE_LED;
    ledPin = pin;
    ledChannel = 0;
    currentMode = IndicatorMode::OFF;
    currentStatus = StatusType::NORMAL;
    brightness = 128;
    enabled = true;
    lastUpdate = 0;
    pulseInterval = 20;
    pulseDirection = true;
    pulseValue = 0;
    hueValue = 0;
}

/**
 * @brief Constructor for RGB LED configuration
 * @param rPin GPIO pin for red component
 * @param gPin GPIO pin for green component  
 * @param bPin GPIO pin for blue component
 * 
 * Initializes the StatusIndicator for an RGB LED setup with separate R, G, B pins.
 */
StatusIndicator::StatusIndicator(int rPin, int gPin, int bPin) {
    ledType = LEDType::RGB_LED;
    redPin = rPin;
    greenPin = gPin;
    bluePin = bPin;
    redChannel = 0;
    greenChannel = 1;
    blueChannel = 2;
    currentMode = IndicatorMode::OFF;
    currentStatus = StatusType::NORMAL;
    brightness = 128;
    enabled = true;
    lastUpdate = 0;
    pulseInterval = 20;
    pulseDirection = true;
    pulseValue = 0;
    hueValue = 0;
}

/**
 * @brief Initialize with default brightness
 * 
 * Calls begin() with the current brightness setting.
 */
void StatusIndicator::begin() {
    begin(brightness);
}

/**
 * @brief Initialize with specified brightness
 * @param initialBrightness Initial brightness level (0-255)
 * 
 * Sets up PWM channels, configures pins, and initializes the LED system.
 * This method must be called before using any other StatusIndicator methods.
 */
void StatusIndicator::begin(uint8_t initialBrightness) {
    brightness = initialBrightness;
    setupPWMChannels();
    setMode(IndicatorMode::OFF);
    
    Serial.printf("✓ StatusIndicator initialized: %s mode\n", 
                  ledType == LEDType::RGB_LED ? "RGB" : "Single LED");
    
    if (ledType == LEDType::RGB_LED) {
        Serial.printf("  RGB Pins: R=%d, G=%d, B=%d\n", redPin, greenPin, bluePin);
    } else {
        Serial.printf("  LED Pin: %d\n", ledPin);
    }
}

/**
 * @brief Setup PWM channels for LED control
 * 
 * Configures the appropriate PWM channels based on the LED type.
 * For single LEDs, configures one channel. For RGB LEDs, configures three channels.
 * Also attaches the pins to their respective PWM channels and initializes them to off.
 */
void StatusIndicator::setupPWMChannels() {
    if (ledType == LEDType::RGB_LED) {
        // Setup RGB channels
        ledcSetup(redChannel, PWM_FREQ, PWM_RESOLUTION);
        ledcSetup(greenChannel, PWM_FREQ, PWM_RESOLUTION);
        ledcSetup(blueChannel, PWM_FREQ, PWM_RESOLUTION);
        
        ledcAttachPin(redPin, redChannel);
        ledcAttachPin(greenPin, greenChannel);
        ledcAttachPin(bluePin, blueChannel);
        
        // Initialize to off
        ledcWrite(redChannel, 0);
        ledcWrite(greenChannel, 0);
        ledcWrite(blueChannel, 0);
    } else {
        // Setup single LED channel
        ledcSetup(ledChannel, PWM_FREQ, PWM_RESOLUTION);
        ledcAttachPin(ledPin, ledChannel);
        ledcWrite(ledChannel, 0);
    }
}

/**
 * @brief Set the animation mode
 * @param mode The animation mode to set
 * 
 * Changes the current animation mode and resets timing variables.
 * If mode is OFF, immediately turns off all LEDs.
 */
void StatusIndicator::setMode(IndicatorMode mode) {
    currentMode = mode;
    lastUpdate = millis();
    
    if (mode == IndicatorMode::OFF) {
        if (ledType == LEDType::RGB_LED) {
            setLEDColor(0, 0, 0);
        } else {
            setSingleLED(0);
        }
    }
}

void StatusIndicator::setStatus(StatusType status) {
    currentStatus = status;
}

void StatusIndicator::setStatusWithMode(StatusType status, IndicatorMode mode) {
    currentStatus = status;
    setMode(mode);
}

void StatusIndicator::setBrightness(uint8_t newBrightness) {
    brightness = newBrightness;
}

void StatusIndicator::enable() {
    enabled = true;
}

void StatusIndicator::disable() {
    enabled = false;
    if (ledType == LEDType::RGB_LED) {
        setLEDColor(0, 0, 0);
    } else {
        setSingleLED(0);
    }
}

void StatusIndicator::toggle() {
    if (enabled) {
        disable();
    } else {
        enable();
    }
}

/**
 * @brief Update animations - must be called regularly
 * 
 * This method handles all animation timing and LED updates. It must be called
 * regularly (typically in the main loop) for animations to work properly.
 * Different animation modes are updated at different intervals for optimal performance.
 */
void StatusIndicator::update() {
    if (!enabled || currentMode == IndicatorMode::OFF) {
        return;
    }
    
    unsigned long currentTime = millis();
    
    switch (currentMode) {
        case IndicatorMode::SOLID:
            // Set solid color based on status
            if (ledType == LEDType::RGB_LED) {
                uint8_t r, g, b;
                getStatusColor(currentStatus, r, g, b);
                setLEDColor(r, g, b);
            } else {
                setSingleLED(brightness);
            }
            break;
            
        case IndicatorMode::PULSE:
            if (currentTime - lastUpdate >= pulseInterval) {
                updatePulse();
                lastUpdate = currentTime;
            }
            break;
            
        case IndicatorMode::BLINK_SLOW:
            if (currentTime - lastUpdate >= 500) {  // 500ms intervals
                updateBlink();
                lastUpdate = currentTime;
            }
            break;
            
        case IndicatorMode::BLINK_FAST:
            if (currentTime - lastUpdate >= 150) {  // 150ms intervals
                updateBlink();
                lastUpdate = currentTime;
            }
            break;
            
        case IndicatorMode::BREATHE:
            if (currentTime - lastUpdate >= 25) {   // 25ms intervals for smooth breathing
                updateBreathe();
                lastUpdate = currentTime;
            }
            break;
            
        case IndicatorMode::RGB_CYCLE:
            if (ledType == LEDType::RGB_LED && currentTime - lastUpdate >= 50) {
                updateRGBCycle();
                lastUpdate = currentTime;
            }
            break;
            
        default:
            break;
    }
}

void StatusIndicator::updatePulse() {
    static bool pulseState = false;
    
    if (ledType == LEDType::RGB_LED) {
        uint8_t r, g, b;
        getStatusColor(currentStatus, r, g, b);
        if (pulseState) {
            setLEDColor(r, g, b);
        } else {
            setLEDColor(0, 0, 0);
        }
    } else {
        setSingleLED(pulseState ? brightness : 0);
    }
    
    pulseState = !pulseState;
}

void StatusIndicator::updateBlink() {
    static bool blinkState = false;
    
    if (ledType == LEDType::RGB_LED) {
        uint8_t r, g, b;
        getStatusColor(currentStatus, r, g, b);
        if (blinkState) {
            setLEDColor(r, g, b);
        } else {
            setLEDColor(0, 0, 0);
        }
    } else {
        setSingleLED(blinkState ? brightness : 0);
    }
    
    blinkState = !blinkState;
}

void StatusIndicator::updateBreathe() {
    // Smooth breathing effect
    if (pulseDirection) {
        pulseValue += 2;
        if (pulseValue >= brightness) {
            pulseValue = brightness;
            pulseDirection = false;
        }
    } else {
        pulseValue -= 2;
        if (pulseValue <= 0) {
            pulseValue = 0;
            pulseDirection = true;
        }
    }
    
    if (ledType == LEDType::RGB_LED) {
        uint8_t r, g, b;
        getStatusColor(currentStatus, r, g, b);
        // Scale colors by pulse value
        r = (r * pulseValue) / MAX_BRIGHTNESS;
        g = (g * pulseValue) / MAX_BRIGHTNESS;
        b = (b * pulseValue) / MAX_BRIGHTNESS;
        setLEDColor(r, g, b);
    } else {
        setSingleLED(pulseValue);
    }
}

void StatusIndicator::updateRGBCycle() {
    if (ledType != LEDType::RGB_LED) return;
    
    uint8_t r, g, b;
    hsvToRgb(hueValue, 255, brightness, r, g, b);
    setLEDColor(r, g, b);
    
    hueValue += 2;
    if (hueValue >= 360) {
        hueValue = 0;
    }
}

/**
 * @brief Set RGB LED color values
 * @param red Red component (0-255)
 * @param green Green component (0-255)
 * @param blue Blue component (0-255)
 * 
 * Directly sets the RGB values for RGB LED configurations.
 * Has no effect on single LED configurations.
 */
void StatusIndicator::setLEDColor(uint8_t red, uint8_t green, uint8_t blue) {
    if (ledType != LEDType::RGB_LED) return;
    
    ledcWrite(redChannel, red);
    ledcWrite(greenChannel, green);
    ledcWrite(blueChannel, blue);
}

/**
 * @brief Set single LED brightness
 * @param brightness Brightness level (0-255)
 * 
 * Sets the brightness for single LED configurations.
 * Has no effect on RGB LED configurations.
 */
void StatusIndicator::setSingleLED(uint8_t brightness) {
    if (ledType != LEDType::SINGLE_LED) return;
    
    ledcWrite(ledChannel, brightness);
}

/**
 * @brief Get RGB color values for a given status type
 * @param status The status type to get colors for
 * @param red Reference to store red component (0-255)
 * @param green Reference to store green component (0-255)
 * @param blue Reference to store blue component (0-255)
 * 
 * Maps status types to appropriate RGB color values based on common conventions:
 * - NORMAL/SUCCESS: Green
 * - WARNING: Orange/Yellow  
 * - ERROR/RECOVERY: Red
 * - INFO: Blue
 * - CONNECTING: Cyan
 */
void StatusIndicator::getStatusColor(StatusType status, uint8_t& red, uint8_t& green, uint8_t& blue) {
    // Scale brightness
    uint8_t scaledBrightness = brightness;
    
    switch (status) {
        case StatusType::NORMAL:
            red = 0;
            green = scaledBrightness;
            blue = 0;
            break;
            
        case StatusType::WARNING:
            red = scaledBrightness;
            green = scaledBrightness / 2;
            blue = 0;
            break;
            
        case StatusType::ERROR:
            red = scaledBrightness;
            green = 0;
            blue = 0;
            break;
            
        case StatusType::INFO:
            red = 0;
            green = 0;
            blue = scaledBrightness;
            break;
            
        case StatusType::SUCCESS:
            red = 0;
            green = scaledBrightness;
            blue = 0;
            break;
            
        case StatusType::CONNECTING:
            red = 0;
            green = scaledBrightness / 2;
            blue = scaledBrightness;
            break;
            
        case StatusType::RECOVERY:
            red = scaledBrightness;
            green = 0;
            blue = 0;
            break;
            
        default:
            red = scaledBrightness;
            green = scaledBrightness;
            blue = scaledBrightness;
            break;
    }
}

/**
 * @brief Convert HSV color space to RGB
 * @param hue Hue value (0-359 degrees)
 * @param sat Saturation value (0-255)
 * @param val Value/brightness (0-255)
 * @param red Reference to store red component (0-255)
 * @param green Reference to store green component (0-255)
 * @param blue Reference to store blue component (0-255)
 * 
 * Converts HSV (Hue, Saturation, Value) color representation to RGB.
 * Used primarily for RGB color cycling animations.
 */
void StatusIndicator::hsvToRgb(uint16_t hue, uint8_t sat, uint8_t val, uint8_t& red, uint8_t& green, uint8_t& blue) {
    uint16_t h = hue % 360;
    uint8_t s = sat;
    uint8_t v = val;
    
    uint8_t c = (v * s) / 255;
    uint16_t x = (c * (60 - abs((h % 120) - 60))) / 60;
    uint8_t m = v - c;
    
    if (h < 60) {
        red = c + m;
        green = x + m;
        blue = m;
    } else if (h < 120) {
        red = x + m;
        green = c + m;
        blue = m;
    } else if (h < 180) {
        red = m;
        green = c + m;
        blue = x + m;
    } else if (h < 240) {
        red = m;
        green = x + m;
        blue = c + m;
    } else if (h < 300) {
        red = x + m;
        green = m;
        blue = c + m;
    } else {
        red = c + m;
        green = m;
        blue = x + m;
    }
}

/**
 * @brief Detect the LED type based on board configuration
 * @return Detected LEDType (SINGLE_LED or RGB_LED)
 * 
 * Attempts to detect the board type and determine whether it has RGB capabilities.
 * Currently uses board name detection, but can be enhanced with hardware probing.
 */
LEDType StatusIndicator::detectBoardLEDType() {
    // For now, default to single LED
    // This can be enhanced with actual board detection logic
    String boardType = getBoardTypeName();
    
    if (boardType.indexOf("DevKit") >= 0 || boardType.indexOf("DEVKIT") >= 0) {
        // DevKit boards often have RGB capabilities
        return LEDType::RGB_LED;
    } else {
        // Standard ESP32 boards typically have a single LED
        return LEDType::SINGLE_LED;
    }
}

/**
 * @brief Get the board type name
 * @return String containing board type information
 * 
 * Returns a string representation of the board type for debugging and logging.
 */
String StatusIndicator::getBoardTypeName() {
    // This can be enhanced with more specific board detection
    return String(ARDUINO_BOARD);
}

/**
 * @brief Print current status to Serial
 * 
 * Outputs detailed status information to the Serial console for debugging purposes.
 * Includes LED type, mode, status, brightness, and enabled state.
 */
void StatusIndicator::printStatus() {
    Serial.printf("StatusIndicator: %s | Mode: %d | Status: %d | Brightness: %d | Enabled: %s\n",
                  ledType == LEDType::RGB_LED ? "RGB" : "Single",
                  (int)currentMode,
                  (int)currentStatus,
                  brightness,
                  enabled ? "Yes" : "No");
}

/**
 * @brief Get status information as a formatted string
 * @return String containing current status information
 * 
 * Returns a formatted string with current status information including LED type,
 * animation mode, status type, brightness, and enabled state. Useful for web
 * interfaces and logging.
 */
String StatusIndicator::getStatusString() {
    String modeStr;
    switch (currentMode) {
        case IndicatorMode::OFF: modeStr = "OFF"; break;
        case IndicatorMode::SOLID: modeStr = "SOLID"; break;
        case IndicatorMode::PULSE: modeStr = "PULSE"; break;
        case IndicatorMode::BLINK_SLOW: modeStr = "BLINK_SLOW"; break;
        case IndicatorMode::BLINK_FAST: modeStr = "BLINK_FAST"; break;
        case IndicatorMode::BREATHE: modeStr = "BREATHE"; break;
        case IndicatorMode::RGB_CYCLE: modeStr = "RGB_CYCLE"; break;
        default: modeStr = "UNKNOWN"; break;
    }
    
    String statusStr;
    switch (currentStatus) {
        case StatusType::NORMAL: statusStr = "NORMAL"; break;
        case StatusType::WARNING: statusStr = "WARNING"; break;
        case StatusType::ERROR: statusStr = "ERROR"; break;
        case StatusType::INFO: statusStr = "INFO"; break;
        case StatusType::SUCCESS: statusStr = "SUCCESS"; break;
        case StatusType::CONNECTING: statusStr = "CONNECTING"; break;
        case StatusType::RECOVERY: statusStr = "RECOVERY"; break;
        default: statusStr = "UNKNOWN"; break;
    }
    
    return String(ledType == LEDType::RGB_LED ? "RGB" : "Single") + 
           " | " + modeStr + " | " + statusStr + 
           " | Brightness: " + String(brightness) + 
           " | " + (enabled ? "Enabled" : "Disabled");
}
