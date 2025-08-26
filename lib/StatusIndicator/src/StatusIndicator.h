/**
 * @file StatusIndicator.h
 * @brief Status indicator library for ESP32 devices with LED and RGB support
 * @author ESP_Sandbox Project
 * @version 1.0.0
 * @date 2025-08-26
 * 
 * Provides a unified interface for status indication on ESP32 devices, supporting both 
 * simple on/off LEDs and RGB LEDs with automatic board detection.
 * 
 * @defgroup StatusIndicator StatusIndicator Library
 * @brief ESP32 Status Indicator with LED and RGB support
 * 
 * This module provides comprehensive status indication capabilities for ESP32 devices.
 * It supports both simple single LEDs and RGB LEDs with automatic hardware detection,
 * multiple animation modes, and predefined status types with appropriate colors.
 * 
 * @{
 */

#ifndef STATUS_INDICATOR_H
#define STATUS_INDICATOR_H

#include <Arduino.h>

/**
 * @brief Status indicator animation modes
 * 
 * Defines the different animation patterns available for the status indicator.
 * These modes control how the LED(s) behave over time.
 * 
 * @ingroup StatusIndicator
 */
enum class IndicatorMode {
    OFF,         ///< LED is completely off
    SOLID,       ///< Solid color/brightness without animation
    PULSE,       ///< Simple on/off pulsing animation
    BLINK_SLOW,  ///< Slow blinking animation (500ms intervals)
    BLINK_FAST,  ///< Fast blinking animation (150ms intervals)
    BREATHE,     ///< Smooth breathing effect with gradual fade
    RGB_CYCLE    ///< Color cycling through RGB spectrum (RGB LEDs only)
};

/**
 * @brief Status types with associated colors and meanings
 * 
 * Defines different status types that map to specific colors and behaviors.
 * Each status type has predefined color associations for RGB LEDs.
 * 
 * @ingroup StatusIndicator
 */
enum class StatusType {
    NORMAL,      ///< Green - Normal operation
    WARNING,     ///< Orange/Yellow - Warning condition
    ERROR,       ///< Red - Error condition
    INFO,        ///< Blue - Informational status
    SUCCESS,     ///< Bright Green - Success/completion
    CONNECTING,  ///< Cyan - Connecting to network
    RECOVERY     ///< Red - Recovery mode
};

/**
 * @brief LED hardware type detection
 * 
 * Specifies whether the hardware uses a single LED or RGB LED configuration.
 * This affects how colors and animations are rendered.
 * 
 * @ingroup StatusIndicator
 */
enum class LEDType {
    SINGLE_LED,  ///< Simple on/off LED (single pin)
    RGB_LED      ///< RGB LED with separate R, G, B pins
};

/**
 * @brief Main StatusIndicator class for ESP32 LED control
 * 
 * This class provides a unified interface for controlling status indicators on ESP32 devices.
 * It automatically detects the hardware type (single LED vs RGB LED) and provides appropriate
 * control methods for various status indications with multiple animation modes.
 * 
 * @ingroup StatusIndicator
 * 
 * @par Class Relationships:
 * - Uses IndicatorMode enum for animation control
 * - Uses StatusType enum for color/status mapping  
 * - Uses LEDType enum for hardware detection
 * - Integrates with Arduino PWM functions
 * - Manages timing through millis() function
 * 
 * @par Usage Pattern:
 * 1. Create StatusIndicator instance
 * 2. Call begin() to initialize
 * 3. Set status with convenience methods or setStatusWithMode()
 * 4. Call update() regularly in main loop
 * 
 * @example BasicUsage.ino
 * 
 * Key features:
 * - Automatic board detection
 * - Support for single LED and RGB LED configurations
 * - Multiple animation modes (pulse, blink, breathe, etc.)
 * - Pre-defined status types with appropriate colors
 * - Non-blocking animations
 * - Brightness control
 * - Easy integration with existing projects
 */
class StatusIndicator {
private:
    // Hardware configuration
    LEDType ledType;         ///< Detected or configured LED type
    int ledPin;              ///< Pin number for single LED configuration
    int redPin;              ///< Red pin for RGB LED configuration
    int greenPin;            ///< Green pin for RGB LED configuration
    int bluePin;             ///< Blue pin for RGB LED configuration
    int ledChannel;          ///< PWM channel for single LED
    int redChannel;          ///< PWM channel for RGB red component
    int greenChannel;        ///< PWM channel for RGB green component
    int blueChannel;         ///< PWM channel for RGB blue component
    
    // Current state
    IndicatorMode currentMode;   ///< Current animation mode
    StatusType currentStatus;    ///< Current status type
    uint8_t brightness;          ///< Current brightness level (0-255)
    bool enabled;                ///< Whether the indicator is enabled
    
    // Timing variables
    unsigned long lastUpdate;    ///< Last update timestamp for animations
    unsigned long pulseInterval; ///< Interval for pulse animations
    bool pulseDirection;         ///< Direction flag for pulse animations
    uint8_t pulseValue;          ///< Current pulse brightness value
    uint16_t hueValue;           ///< Current hue value for RGB cycling
    
    // Hardware constants
    static const int PWM_FREQ = 5000;        ///< PWM frequency in Hz
    static const int PWM_RESOLUTION = 8;     ///< PWM resolution in bits
    static const int MAX_BRIGHTNESS = 255;   ///< Maximum brightness value
    
    // Private methods
    
    /**
     * @brief Setup PWM channels for LED control
     * 
     * Configures the appropriate PWM channels based on the LED type.
     * For single LEDs, configures one channel. For RGB LEDs, configures three channels.
     */
    void setupPWMChannels();
    
    /**
     * @brief Set RGB LED color values
     * @param red Red component (0-255)
     * @param green Green component (0-255)
     * @param blue Blue component (0-255)
     */
    void setLEDColor(uint8_t red, uint8_t green, uint8_t blue);
    
    /**
     * @brief Set single LED brightness
     * @param brightness Brightness level (0-255)
     */
    void setSingleLED(uint8_t brightness);
    
    /**
     * @brief Get RGB color values for a given status type
     * @param status The status type to get colors for
     * @param red Reference to store red component
     * @param green Reference to store green component
     * @param blue Reference to store blue component
     */
    void getStatusColor(StatusType status, uint8_t& red, uint8_t& green, uint8_t& blue);
    
    /**
     * @brief Convert HSV color space to RGB
     * @param hue Hue value (0-359)
     * @param sat Saturation value (0-255)
     * @param val Value/brightness (0-255)
     * @param red Reference to store red component
     * @param green Reference to store green component
     * @param blue Reference to store blue component
     */
    void hsvToRgb(uint16_t hue, uint8_t sat, uint8_t val, uint8_t& red, uint8_t& green, uint8_t& blue);
    
    /**
     * @brief Update pulse animation
     */
    void updatePulse();
    
    /**
     * @brief Update blink animation
     */
    void updateBlink();
    
    /**
     * @brief Update breathing animation
     */
    void updateBreathe();
    
    /**
     * @brief Update RGB color cycling animation
     */
    void updateRGBCycle();
    
public:
    // Constructors
    
    /**
     * @brief Default constructor with automatic board detection
     * 
     * Automatically detects the board type and configures appropriate pins.
     * Uses built-in LED (pin 2) for single LED boards or RGB pins (25,26,27) for DevKit boards.
     */
    StatusIndicator();
    
    /**
     * @brief Constructor for single LED configuration
     * @param pin GPIO pin number for the LED
     */
    StatusIndicator(int pin);
    
    /**
     * @brief Constructor for RGB LED configuration
     * @param redPin GPIO pin for red component
     * @param greenPin GPIO pin for green component
     * @param bluePin GPIO pin for blue component
     */
    StatusIndicator(int redPin, int greenPin, int bluePin);
    
    // Initialization
    
    /**
     * @brief Initialize the status indicator with default brightness
     * 
     * Sets up PWM channels and initializes the LED to OFF state.
     * Must be called before using other methods.
     */
    void begin();
    
    /**
     * @brief Initialize the status indicator with specified brightness
     * @param brightness Initial brightness level (0-255)
     */
    void begin(uint8_t brightness);
    
    // Main control methods
    
    /**
     * @brief Set the animation mode
     * @param mode The animation mode to set
     */
    void setMode(IndicatorMode mode);
    
    /**
     * @brief Set the status type (affects color)
     * @param status The status type to set
     */
    void setStatus(StatusType status);
    
    /**
     * @brief Set both status type and animation mode
     * @param status The status type to set
     * @param mode The animation mode to set
     */
    void setStatusWithMode(StatusType status, IndicatorMode mode);
    
    /**
     * @brief Set the brightness level
     * @param brightness Brightness level (0-255)
     */
    void setBrightness(uint8_t brightness);
    
    /**
     * @brief Enable the status indicator
     */
    void enable();
    
    /**
     * @brief Disable the status indicator (turns off LED)
     */
    void disable();
    
    /**
     * @brief Toggle the enabled state
     */
    void toggle();
    
    // Utility methods
    
    /**
     * @brief Update animations - must be called regularly in main loop
     * 
     * This method handles all animation timing and updates. It must be called
     * regularly (typically in the main loop) for animations to work properly.
     */
    void update();
    
    /**
     * @brief Check if the indicator is enabled
     * @return true if enabled, false otherwise
     */
    bool isEnabled() const { return enabled; }
    
    /**
     * @brief Get the current animation mode
     * @return Current IndicatorMode
     */
    IndicatorMode getMode() const { return currentMode; }
    
    /**
     * @brief Get the current status type
     * @return Current StatusType
     */
    StatusType getStatus() const { return currentStatus; }
    
    /**
     * @brief Get the current brightness level
     * @return Current brightness (0-255)
     */
    uint8_t getBrightness() const { return brightness; }
    
    /**
     * @brief Get the LED hardware type
     * @return LEDType (SINGLE_LED or RGB_LED)
     */
    LEDType getLEDType() const { return ledType; }
    
    // Convenience methods
    
    /**
     * @brief Show normal status (green pulsing)
     */
    void showNormal() { setStatusWithMode(StatusType::NORMAL, IndicatorMode::PULSE); }
    
    /**
     * @brief Show warning status (orange/yellow slow blinking)
     */
    void showWarning() { setStatusWithMode(StatusType::WARNING, IndicatorMode::BLINK_SLOW); }
    
    /**
     * @brief Show error status (red fast blinking)
     */
    void showError() { setStatusWithMode(StatusType::ERROR, IndicatorMode::BLINK_FAST); }
    
    /**
     * @brief Show info status (blue solid)
     */
    void showInfo() { setStatusWithMode(StatusType::INFO, IndicatorMode::SOLID); }
    
    /**
     * @brief Show success status (bright green solid)
     */
    void showSuccess() { setStatusWithMode(StatusType::SUCCESS, IndicatorMode::SOLID); }
    
    /**
     * @brief Show connecting status (cyan breathing)
     */
    void showConnecting() { setStatusWithMode(StatusType::CONNECTING, IndicatorMode::BREATHE); }
    
    /**
     * @brief Show recovery status (red fast blinking)
     */
    void showRecovery() { setStatusWithMode(StatusType::RECOVERY, IndicatorMode::BLINK_FAST); }
    
    /**
     * @brief Turn off the indicator
     */
    void off() { setMode(IndicatorMode::OFF); }
    
    // Board detection
    
    /**
     * @brief Detect the LED type based on board configuration
     * @return Detected LEDType
     */
    static LEDType detectBoardLEDType();
    
    /**
     * @brief Get the board type name
     * @return String containing board type information
     */
    static String getBoardTypeName();
    
    // Debug methods
    
    /**
     * @brief Print current status to Serial
     */
    void printStatus();
    
    /**
     * @brief Get status information as a formatted string
     * @return String containing current status information
     */
    String getStatusString();
};

/** @} */ // end of StatusIndicator group

#endif // STATUS_INDICATOR_H
