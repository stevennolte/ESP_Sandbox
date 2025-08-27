# StatusIndicator Library

A flexible status indicator library for ESP32 devices that supports single LEDs and WS2812 addressable RGB LEDs with au#### LEDType
- `SINGLE_LED` - Simple on/off LED (single pin)
- `WS2812_LED` - Addressable RGB LED (WS2812/NeoPixel style, single data pin)tic board detection.

## Features

- **Automatic Board Detection**: Detects whether your ESP32 board has a simple LED or WS2812 addressable LED
- **Multiple Indicator Modes**: OFF, SOLID, PULSE, BLINK_SLOW, BLINK_FAST, BREATHE, RGB_CYCLE
- **Status Types**: NORMAL, WARNING, ERROR, INFO, SUCCESS, CONNECTING, RECOVERY with appropriate colors
- **WS2812 Support**: Full support for addressable RGB LEDs with custom color control using Adafruit NeoPixel library
- **Flexible Configuration**: Works with single LEDs and WS2812 addressable LEDs
- **Easy Integration**: Simple API with convenience methods
- **PWM Control**: Smooth brightness control and effects

## Supported Hardware

- **Single LED**: Standard ESP32 boards with built-in LED (typically pin 2)
- **WS2812 LED**: ESP32-S3-DevKitC-1 boards with built-in WS2812 addressable RGB LED (GPIO38)

## Installation

Copy the `StatusIndicator` folder to your `lib/` directory in your PlatformIO project.

## Usage

### Basic Usage (Auto-detection)

```cpp
#include <StatusIndicator.h>

StatusIndicator indicator;

void setup() {
    indicator.begin();
    indicator.showNormal();  // Green pulsing (or on/off for single LED)
}

void loop() {
    indicator.update();  // Call this in your main loop
    delay(50);
}
```

### Manual Configuration

```cpp
// Single LED on pin 2
StatusIndicator indicator(2);

// RGB LED on pins 25, 26, 27
StatusIndicator rgbIndicator(25, 26, 27);
```

### Status Indication

```cpp
// Convenience methods
indicator.showNormal();     // Green pulsing
indicator.showWarning();    // Orange/yellow blinking
indicator.showError();      // Red fast blinking
indicator.showInfo();       // Blue solid
indicator.showSuccess();    // Bright green solid
indicator.showConnecting(); // Cyan breathing
indicator.showRecovery();   // Red fast blinking
indicator.off();            // Turn off

// Manual control
indicator.setStatus(StatusType::WARNING);
indicator.setMode(IndicatorMode::BLINK_SLOW);

// Or combined
indicator.setStatusWithMode(StatusType::ERROR, IndicatorMode::BLINK_FAST);
```

### Brightness Control

```cpp
indicator.setBrightness(255);  // Maximum brightness
indicator.setBrightness(128);  // Medium brightness
indicator.setBrightness(32);   // Low brightness
```

### WS2812 Addressable RGB LED

For boards with WS2812 addressable RGB LEDs (like ESP32-S3-DevKitC-1):

```cpp
// Automatic detection (recommended)
StatusIndicator indicator;  // Will detect WS2812 on ESP32-S3-DevKitC-1

// Manual configuration  
StatusIndicator indicator(38, LEDType::WS2812_LED);  // GPIO38 for ESP32-S3-DevKitC-1

void setup() {
    indicator.begin();
    
    // Custom colors (WS2812 only)
    indicator.setRGBColor(255, 0, 0);    // Red
    indicator.setRGBColor(0, 255, 0);    // Green
    indicator.setRGBColor(0, 0, 255);    // Blue
    indicator.setRGBColor(255, 255, 0);  // Yellow
    indicator.setRGBColor(255, 0, 255);  // Magenta
    indicator.setRGBColor(0, 255, 255);  // Cyan
}
```

### Advanced Features

```cpp
// Enable/disable
indicator.enable();
indicator.disable();
indicator.toggle();

// Status checking
if (indicator.isEnabled()) {
    // Do something
}

// Debug information
indicator.printStatus();
String status = indicator.getStatusString();
```

## API Reference

### Enums

#### IndicatorMode
- `OFF` - LED is off
- `SOLID` - Solid color/brightness
- `PULSE` - Simple on/off pulsing
- `BLINK_SLOW` - Slow blinking (500ms intervals)
- `BLINK_FAST` - Fast blinking (150ms intervals)
- `BREATHE` - Smooth breathing effect
- `RGB_CYCLE` - Color cycling (RGB only)

#### StatusType
- `NORMAL` - Green (normal operation)
- `WARNING` - Orange/Yellow (warning condition)
- `ERROR` - Red (error condition)
- `INFO` - Blue (informational)
- `SUCCESS` - Bright Green (success/completion)
- `CONNECTING` - Cyan (connecting to network)
- `RECOVERY` - Red (recovery mode)

#### LEDType
- `SINGLE_LED` - Simple on/off LED (single pin)
- `RGB_LED` - RGB LED with separate R, G, B pins (PWM-controlled)
- `WS2812_LED` - Addressable RGB LED (WS2812/NeoPixel, single data pin)

### Methods

#### Initialization
- `StatusIndicator()` - Auto-detect board type
- `StatusIndicator(int pin)` - Single LED constructor
- `StatusIndicator(int redPin, int greenPin, int bluePin)` - RGB LED constructor
- `void begin()` - Initialize with default brightness
- `void begin(uint8_t brightness)` - Initialize with specific brightness

#### Control
- `void setMode(IndicatorMode mode)` - Set indicator mode
- `void setStatus(StatusType status)` - Set status type
- `void setStatusWithMode(StatusType status, IndicatorMode mode)` - Set both
- `void setBrightness(uint8_t brightness)` - Set brightness (0-255)
- `void update()` - Update indicator (call in main loop)

#### State Management
- `void enable()` - Enable indicator
- `void disable()` - Disable indicator
- `void toggle()` - Toggle enabled state

#### Convenience Methods
- `void showNormal()` - Show normal status
- `void showWarning()` - Show warning status
- `void showError()` - Show error status
- `void showInfo()` - Show info status
- `void showSuccess()` - Show success status
- `void showConnecting()` - Show connecting status
- `void showRecovery()` - Show recovery status
- `void off()` - Turn off indicator

#### Status Query
- `bool isEnabled()` - Check if enabled
- `IndicatorMode getMode()` - Get current mode
- `StatusType getStatus()` - Get current status
- `uint8_t getBrightness()` - Get current brightness
- `LEDType getLEDType()` - Get LED type (SINGLE_LED or WS2812_LED)

#### Debug
- `void printStatus()` - Print status to Serial
- `String getStatusString()` - Get status as string

## Hardware Configuration

### Single LED (Default)
- Pin 2 (built-in LED on most ESP32 boards)
- PWM channel 0

### RGB LED (DevKit-C or custom)
- Red: Pin 25 (PWM channel 0)
- Green: Pin 26 (PWM channel 1)
- Blue: Pin 27 (PWM channel 2)

### Custom Pin Configuration

You can specify custom pins when creating the StatusIndicator:

```cpp
// Custom single LED on pin 4
StatusIndicator indicator(4);

// Custom RGB pins
StatusIndicator rgbIndicator(12, 13, 14);  // R, G, B pins
```

## Integration Example

```cpp
#include <StatusIndicator.h>

StatusIndicator statusLED;

void setup() {
    Serial.begin(115200);
    
    // Initialize status indicator
    statusLED.begin(128);  // Medium brightness
    statusLED.showConnecting();
    
    // Your WiFi connection code here
    // ...
    
    if (WiFi.status() == WL_CONNECTED) {
        statusLED.showSuccess();
        delay(1000);
        statusLED.showNormal();
    } else {
        statusLED.showError();
    }
}

void loop() {
    statusLED.update();  // Essential for animations
    
    // Your main code here
    // ...
    
    delay(50);
}
```

## Notes

- Always call `update()` in your main loop for animations to work
- The library automatically detects board type, but you can override with manual constructors
- PWM frequency is set to 5kHz with 8-bit resolution
- Colors are optimized for common cathode RGB LEDs
- For common anode RGB LEDs, you may need to invert the values in the library

## Documentation Generation

This library includes comprehensive Doxygen documentation with interactive diagrams. To generate the documentation:

1. **Install Doxygen** (if not already installed):
   ```bash
   # Ubuntu/Debian
   sudo apt-get install doxygen
   
   # macOS
   brew install doxygen
   
   # Windows
   # Download from https://www.doxygen.nl/download.html
   ```

2. **Install Graphviz** (for diagrams - optional but recommended):
   ```bash
   # Ubuntu/Debian  
   sudo apt-get install graphviz
   
   # macOS
   brew install graphviz
   
   # Windows
   # Download from https://graphviz.org/download/
   ```

3. **Generate Documentation**:
   ```bash
   cd lib/StatusIndicator
   ./generate_docs.sh        # Linux/macOS
   # or
   generate_docs.bat         # Windows
   ```

4. **View Documentation**:
   Open `docs/html/index.html` in your web browser to view the generated documentation.

### 📊 Generated Diagrams

With Graphviz installed, the documentation includes various code linkage diagrams:

- **Class Hierarchy Diagrams**: Shows inheritance relationships
- **Collaboration Diagrams**: Shows which classes work together
- **Call Graphs**: Shows function call relationships
- **Caller Graphs**: Shows which functions call a specific function
- **Include Dependency Graphs**: Shows header file dependencies
- **Directory Structure Diagrams**: Shows file organization
- **UML-style Class Diagrams**: Shows class members and relationships

The diagrams are generated as interactive SVG files, allowing you to:
- Zoom in/out for detail
- Click on elements to navigate to their documentation
- View relationships between different parts of the code
- Understand the overall code structure visually

### 📋 Documentation Features

The documentation includes:
- Complete API reference with parameter descriptions
- Interactive code linkage diagrams
- Class hierarchy and relationships
- Source code cross-references
- Call graphs showing function dependencies
- Include graphs showing file dependencies
- Automatically generated from source code comments

## Contributing

When contributing to this library, please:
- Follow the existing Doxygen comment style
- Document all public methods and parameters
- Include usage examples for complex features
- Update the version number in library.properties when making changes
