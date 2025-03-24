#include "I2CLedScreen.h"

I2CLedScreen::I2CLedScreen(uint8_t i2c_address, uint8_t cols, uint8_t rows)
    : lcd(i2c_address, cols, rows) {}

void I2CLedScreen::begin() {
    lcd.init();
    lcd.backlight();
}

void I2CLedScreen::clear() {
    lcd.clear();
}

void I2CLedScreen::setCursor(uint8_t col, uint8_t row) {
    lcd.setCursor(col, row);
}

void I2CLedScreen::displayText(const String &text) {
    lcd.print(text);
}

void I2CLedScreen::scrollDisplayLeft()
{
    lcd.scrollDisplayLeft();
}

void I2CLedScreen::displayScrollingText(uint8_t row, const String &text, int &scrollPos) {
    if (text.length() <= 16) {
        setCursor(0, row);
        displayText(text);
        return; // No scrolling needed
    }

    String displayPortion = text.substring(scrollPos);

    if (displayPortion.length() > 16) {
        displayPortion = displayPortion.substring(0, 16);
    }

    setCursor(0, row);
    displayText(displayPortion);

    scrollPos++;
    if (scrollPos > text.length() - 16) {
        scrollPos = 0; // Reset scroll position
    }
}
