#pragma once
#include <cstdint>

class ST7789VDisplay {
public:
    ST7789VDisplay();
    ~ST7789VDisplay();
    bool init();
    void fillScreen(uint16_t color);
    void drawPixel(uint16_t x, uint16_t y, uint16_t color);
    void drawText(uint16_t x, uint16_t y, const char* text, uint16_t color);

private:
    int spi_handle_;
    int gpio_chip_handle_;
    bool gpio_claimed_;
    void writeCommand(uint8_t cmd);
    void writeData(const uint8_t* data, size_t len);
    void gpioSet(int gpio, int value);
    void reset();
};