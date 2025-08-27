/**
 * @file Adafruit_NeoPixel.cpp
 * @brief Simple stub implementation for Adafruit NeoPixel library to test compilation
 */

#include "Adafruit_NeoPixel.h"

Adafruit_NeoPixel::Adafruit_NeoPixel(uint16_t n, int16_t pin, neoPixelType type) {
    numPixels = n;
    this->pin = pin;
    this->type = type;
    brightness = 255;
}

Adafruit_NeoPixel::~Adafruit_NeoPixel() {
    // Stub destructor
}

void Adafruit_NeoPixel::begin() {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void Adafruit_NeoPixel::show() {
    // Stub - would normally send data to WS2812
}

void Adafruit_NeoPixel::clear() {
    // Stub - would normally clear all pixels
}

void Adafruit_NeoPixel::setBrightness(uint8_t brightness) {
    this->brightness = brightness;
}

void Adafruit_NeoPixel::setPixelColor(uint16_t n, uint32_t color) {
    // Stub - would normally set pixel color
}

void Adafruit_NeoPixel::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
    // Stub - would normally set pixel color
}

uint32_t Adafruit_NeoPixel::Color(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}
