/**
 * @file Adafruit_NeoPixel.h
 * @brief Simple stub for Adafruit NeoPixel library to test compilation
 */

#ifndef ADAFRUIT_NEOPIXEL_H
#define ADAFRUIT_NEOPIXEL_H

#include <Arduino.h>

// NeoPixel color format constants
#define NEO_GRB 0x02
#define NEO_KHZ800 0x0000

class Adafruit_NeoPixel {
public:
    Adafruit_NeoPixel(uint16_t n, int16_t pin, neoPixelType type = NEO_GRB + NEO_KHZ800);
    ~Adafruit_NeoPixel();
    
    void begin();
    void show();
    void clear();
    void setBrightness(uint8_t brightness);
    void setPixelColor(uint16_t n, uint32_t color);
    void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
    uint32_t Color(uint8_t r, uint8_t g, uint8_t b);
    
private:
    uint16_t numPixels;
    int16_t pin;
    uint32_t type;
    uint8_t brightness;
};

typedef uint32_t neoPixelType;

#endif // ADAFRUIT_NEOPIXEL_H
