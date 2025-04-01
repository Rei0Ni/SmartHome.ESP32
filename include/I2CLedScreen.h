#ifndef I2C_LED_SCREEN_H
#define I2C_LED_SCREEN_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

class I2CLedScreen {
public:
    I2CLedScreen(uint8_t i2c_address, uint8_t cols, uint8_t rows);
    
    void begin();
    void clear();
    void setCursor(uint8_t col, uint8_t row);
    void displayText(const String &text);
    void scrollDisplayLeft();
    void displayScrollingText(uint8_t row, const String &text, int &scrollPos);

private:
    LiquidCrystal_I2C lcd;
};

#endif // I2C_LED_SCREEN_H
