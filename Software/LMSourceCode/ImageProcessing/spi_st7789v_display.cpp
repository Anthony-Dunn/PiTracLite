#include <lgpio.h>
#include <unistd.h>
#include <cstdint>
#include <vector>
#include <cstdio>

constexpr int ST7789V_SPI_CHIP = 4;      // Use 0 for RPi4, 4 for RPi5 (see your code)
constexpr int ST7789V_SPI_CHANNEL = 0;   // SPI channel (CE0)
constexpr int ST7789V_SPI_BAUD = 40000000; // 40 MHz, adjust as needed

// BCM GPIO numbers (adjust as needed)
constexpr int GPIO_DC  = 5;   // Data/Command
constexpr int GPIO_RST = 27;   // Reset
constexpr int GPIO_BL  = 6;   // Backlight

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

static const uint8_t font8x8_basic[128][8] = {
{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }, // 0x00 Null
{ 0x7E,0x81,0xA5,0x81,0xBD,0x99,0x81,0x7E }, // 0x01
{ 0x7E,0xFF,0xDB,0xFF,0xC3,0xE7,0xFF,0x7E }, // 0x02
{ 0x6C,0xFE,0xFE,0xFE,0x7C,0x38,0x10,0x00 }, // 0x03
{ 0x10,0x38,0x7C,0xFE,0x7C,0x38,0x10,0x00 }, // 0x04
{ 0x38,0x7C,0x38,0xFE,0xFE,0xD6,0x10,0x38 }, // 0x05
{ 0x10,0x38,0x7C,0xFE,0xFE,0x7C,0x10,0x38 }, // 0x06
{ 0x00,0x00,0x18,0x3C,0x3C,0x18,0x00,0x00 }, // 0x07
{ 0xFF,0xFF,0xE7,0xC3,0xC3,0xE7,0xFF,0xFF }, // 0x08
{ 0x00,0x3C,0x66,0x42,0x42,0x66,0x3C,0x00 }, // 0x09
{ 0xFF,0xC3,0x99,0xBD,0xBD,0x99,0xC3,0xFF }, // 0x0A
{ 0x0F,0x07,0x0F,0x7D,0xCC,0xCC,0xCC,0x78 }, // 0x0B
{ 0x3C,0x66,0x66,0x66,0x3C,0x18,0x7E,0x18 }, // 0x0C
{ 0x3F,0x33,0x3F,0x30,0x30,0x70,0xF0,0xE0 }, // 0x0D
{ 0x7F,0x63,0x7F,0x63,0x63,0x67,0xE6,0xC0 }, // 0x0E
{ 0x99,0x5A,0x3C,0xE7,0xE7,0x3C,0x5A,0x99 }, // 0x0F
{ 0x80,0xE0,0xF8,0xFE,0xF8,0xE0,0x80,0x00 }, // 0x10
{ 0x02,0x0E,0x3E,0xFE,0x3E,0x0E,0x02,0x00 }, // 0x11
{ 0x18,0x3C,0x7E,0x18,0x18,0x7E,0x3C,0x18 }, // 0x12
{ 0x66,0x66,0x66,0x66,0x66,0x00,0x66,0x00 }, // 0x13
{ 0x7F,0xDB,0xDB,0x7B,0x1B,0x1B,0x1B,0x00 }, // 0x14
{ 0x3E,0x61,0x3C,0x66,0x66,0x3C,0x86,0x7C }, // 0x15
{ 0x00,0x00,0x00,0x00,0x7E,0x7E,0x7E,0x00 }, // 0x16
{ 0x18,0x3C,0x7E,0x18,0x7E,0x3C,0x18,0xFF }, // 0x17
{ 0x18,0x3C,0x7E,0x18,0x18,0x18,0x18,0x18 }, // 0x18
{ 0x18,0x18,0x18,0x18,0x7E,0x3C,0x18,0x00 }, // 0x19
{ 0x00,0x18,0x0C,0xFE,0x0C,0x18,0x00,0x00 }, // 0x1A
{ 0x00,0x30,0x60,0xFE,0x60,0x30,0x00,0x00 }, // 0x1B
{ 0x00,0x00,0xC0,0xC0,0xC0,0xFE,0x00,0x00 }, // 0x1C
{ 0x00,0x24,0x66,0xFF,0x66,0x24,0x00,0x00 }, // 0x1D
{ 0x00,0x18,0x3C,0x7E,0xFF,0xFF,0x00,0x00 }, // 0x1E
{ 0x00,0xFF,0xFF,0x7E,0x3C,0x18,0x00,0x00 }, // 0x1F
{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }, // 0x20 ' '
{ 0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00 }, // 0x21 '!'
{ 0x6C,0x6C,0x24,0x00,0x00,0x00,0x00,0x00 }, // 0x22 '"'
{ 0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00 }, // 0x23 '#'
{ 0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00 }, // 0x24 '$'
{ 0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00 }, // 0x25 '%'
{ 0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00 }, // 0x26 '&'
{ 0x30,0x30,0x60,0x00,0x00,0x00,0x00,0x00 }, // 0x27 '''
{ 0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00 }, // 0x28 '('
{ 0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00 }, // 0x29 ')'
{ 0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00 }, // 0x2A '*'
{ 0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00 }, // 0x2B '+'
{ 0x00,0x00,0x00,0x00,0x18,0x18,0x30,0x00 }, // 0x2C ','
{ 0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00 }, // 0x2D '-'
{ 0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00 }, // 0x2E '.'
{ 0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00 }, // 0x2F '/'
{ 0x7C,0xC6,0xCE,0xD6,0xE6,0xC6,0x7C,0x00 }, // 0x30 '0'
{ 0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00 }, // 0x31 '1'
{ 0x7C,0xC6,0x0E,0x1C,0x70,0xC0,0xFE,0x00 }, // 0x32 '2'
{ 0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00 }, // 0x33 '3'
{ 0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x1E,0x00 }, // 0x34 '4'
{ 0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00 }, // 0x35 '5'
{ 0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00 }, // 0x36 '6'
{ 0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00 }, // 0x37 '7'
{ 0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00 }, // 0x38 '8'
{ 0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00 }, // 0x39 '9'
{ 0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00 }, // 0x3A ':'
{ 0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30 }, // 0x3B ';'
{ 0x0E,0x1C,0x38,0x70,0x38,0x1C,0x0E,0x00 }, // 0x3C '<'
{ 0x00,0x00,0x7E,0x00,0x00,0x7E,0x00,0x00 }, // 0x3D '='
{ 0x70,0x38,0x1C,0x0E,0x1C,0x38,0x70,0x00 }, // 0x3E '>'
{ 0x7C,0xC6,0x0E,0x1C,0x18,0x00,0x18,0x00 }, // 0x3F '?'
{ 0x7C,0xC6,0xDE,0xDE,0xDE,0xC0,0x78,0x00 }, // 0x40 '@'
{ 0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0xC6,0x00 }, // 0x41 'A'
{ 0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00 }, // 0x42 'B'
{ 0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00 }, // 0x43 'C'
{ 0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00 }, // 0x44 'D'
{ 0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00 }, // 0x45 'E'
{ 0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0x00 }, // 0x46 'F'
{ 0x3C,0x66,0xC0,0xC0,0xCE,0x66,0x3E,0x00 }, // 0x47 'G'
{ 0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00 }, // 0x48 'H'
{ 0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00 }, // 0x49 'I'
{ 0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00 }, // 0x4A 'J'
{ 0xE6,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00 }, // 0x4B 'K'
{ 0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00 }, // 0x4C 'L'
{ 0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0x00 }, // 0x4D 'M'
{ 0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00 }, // 0x4E 'N'
{ 0x38,0x6C,0xC6,0xC6,0xC6,0x6C,0x38,0x00 }, // 0x4F 'O'
{ 0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00 }, // 0x50 'P'
{ 0x78,0xCC,0xCC,0xCC,0xDC,0x78,0x1C,0x00 }, // 0x51 'Q'
{ 0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00 }, // 0x52 'R'
{ 0x7C,0xC6,0x60,0x38,0x0C,0xC6,0x7C,0x00 }, // 0x53 'S'
{ 0x7E,0x7E,0x5A,0x18,0x18,0x18,0x3C,0x00 }, // 0x54 'T'
{ 0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00 }, // 0x55 'U'
{ 0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x00 }, // 0x56 'V'
{ 0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00 }, // 0x57 'W'
{ 0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00 }, // 0x58 'X'
{ 0x66,0x66,0x66,0x3C,0x18,0x18,0x3C,0x00 }, // 0x59 'Y'
{ 0xFE,0xC6,0x8C,0x18,0x32,0x66,0xFE,0x00 }, // 0x5A 'Z'
{ 0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00 }, // 0x5B '['
{ 0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00 }, // 0x5C '\'
{ 0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00 }, // 0x5D ']'
{ 0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00 }, // 0x5E '^'
{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF }, // 0x5F '_'
{ 0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00 }, // 0x60 '`'
{ 0x00,0x00,0x7C,0x06,0x7E,0xC6,0x7E,0x00 }, // 0x61 'a'
{ 0xE0,0x60,0x7C,0x66,0x66,0x66,0xDC,0x00 }, // 0x62 'b'
{ 0x00,0x00,0x7C,0xC6,0xC0,0xC6,0x7C,0x00 }, // 0x63 'c'
{ 0x1C,0x0C,0x7C,0xCC,0xCC,0xCC,0x76,0x00 }, // 0x64 'd'
{ 0x00,0x00,0x7C,0xC6,0xFE,0xC0,0x7C,0x00 }, // 0x65 'e'
{ 0x3C,0x66,0x60,0xF8,0x60,0x60,0xF0,0x00 }, // 0x66 'f'
{ 0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0xF8 }, // 0x67 'g'
{ 0xE0,0x60,0x6C,0x76,0x66,0x66,0xE6,0x00 }, // 0x68 'h'
{ 0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00 }, // 0x69 'i'
{ 0x0C,0x00,0x0C,0x0C,0x0C,0xCC,0xCC,0x78 }, // 0x6A 'j'
{ 0xE0,0x60,0x66,0x6C,0x78,0x6C,0xE6,0x00 }, // 0x6B 'k'
{ 0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00 }, // 0x6C 'l'
{ 0x00,0x00,0xEC,0xFE,0xD6,0xD6,0xC6,0x00 }, // 0x6D 'm'
{ 0x00,0x00,0xDC,0x66,0x66,0x66,0x66,0x00 }, // 0x6E 'n'
{ 0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7C,0x00 }, // 0x6F 'o'
{ 0x00,0x00,0xDC,0x66,0x66,0x7C,0x60,0xF0 }, // 0x70 'p'
{ 0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0x1E }, // 0x71 'q'
{ 0x00,0x00,0xDC,0x76,0x66,0x60,0xF0,0x00 }, // 0x72 'r'
{ 0x00,0x00,0x7E,0xC0,0x7C,0x06,0xFC,0x00 }, // 0x73 's'
{ 0x30,0x30,0xFC,0x30,0x30,0x36,0x1C,0x00 }, // 0x74 't'
{ 0x00,0x00,0xCC,0xCC,0xCC,0xCC,0x76,0x00 }, // 0x75 'u'
{ 0x00,0x00,0xC6,0xC6,0xC6,0x6C,0x38,0x00 }, // 0x76 'v'
{ 0x00,0x00,0xC6,0xD6,0xFE,0xFE,0x6C,0x00 }, // 0x77 'w'
{ 0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00 }, // 0x78 'x'
{ 0x00,0x00,0xC6,0xC6,0xC6,0x7E,0x06,0xFC }, // 0x79 'y'
{ 0x00,0x00,0xFE,0x4C,0x18,0x32,0xFE,0x00 }, // 0x7A 'z'
{ 0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00 }, // 0x7B '{'
{ 0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00 }, // 0x7C '|'
{ 0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00 }, // 0x7D '}'
{ 0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00 }, // 0x7E '~'
{ 0x00,0x10,0x38,0x6C,0xC6,0xC6,0xFE,0x00 }  // 0x7F DEL
};

ST7789VDisplay::ST7789VDisplay()
    : spi_handle_(-1), gpio_chip_handle_(-1), gpio_claimed_(false) {}

ST7789VDisplay::~ST7789VDisplay() {
    if (spi_handle_ >= 0) lgSpiClose(spi_handle_);
    if (gpio_claimed_) {
        lgGpioFree(gpio_chip_handle_, GPIO_DC);
        lgGpioFree(gpio_chip_handle_, GPIO_RST);
        lgGpioFree(gpio_chip_handle_, GPIO_BL);
    }
    if (gpio_chip_handle_ >= 0) lgGpiochipClose(gpio_chip_handle_);
}

bool ST7789VDisplay::init() {
    // Open GPIO chip
    gpio_chip_handle_ = lgGpiochipOpen(ST7789V_SPI_CHIP);
    if (gpio_chip_handle_ < 0) {
        printf("Failed to open GPIO chip\n");
        return false;
    }
    // Claim GPIOs
    if (lgGpioClaimOutput(gpio_chip_handle_, 0, GPIO_DC, 0) != LG_OKAY ||
        lgGpioClaimOutput(gpio_chip_handle_, 0, GPIO_RST, 1) != LG_OKAY ||
        lgGpioClaimOutput(gpio_chip_handle_, 0, GPIO_BL, 1) != LG_OKAY) {
        printf("Failed to claim GPIOs\n");
        return false;
    }
    gpio_claimed_ = true;

    // Open SPI
    spi_handle_ = lgSpiOpen(ST7789V_SPI_CHIP, ST7789V_SPI_CHANNEL, ST7789V_SPI_BAUD, 0);
    if (spi_handle_ < 0) {
        printf("Failed to open SPI\n");
        return false;
    }

    reset();

    // ST7789V initialization (minimal)
    writeCommand(0x36); uint8_t madctl = 0x00; writeData(&madctl, 1);
    writeCommand(0x3A); uint8_t colmod = 0x05; writeData(&colmod, 1); // 16-bit color
    writeCommand(0x11); usleep(120000); // Sleep out
    writeCommand(0x29); // Display on

    gpioSet(GPIO_BL, 1); // Backlight on

    return true;
}

void ST7789VDisplay::reset() {
    gpioSet(GPIO_RST, 0);
    usleep(10000);
    gpioSet(GPIO_RST, 1);
    usleep(10000);
}

void ST7789VDisplay::writeCommand(uint8_t cmd) {
    gpioSet(GPIO_DC, 0);
    lgSpiWrite(spi_handle_, (char*)&cmd, 1);
}

void ST7789VDisplay::writeData(const uint8_t* data, size_t len) {
    gpioSet(GPIO_DC, 1);
    lgSpiWrite(spi_handle_, (char*)data, len);
}

void ST7789VDisplay::gpioSet(int gpio, int value) {
    lgGpioWrite(gpio_chip_handle_, gpio, value);
}

void ST7789VDisplay::fillScreen(uint16_t color) {
    // Set address window to full screen
    writeCommand(0x2A);
    uint8_t data2A[] = {0, 0, 1, 0x3F}; // 0-319
    writeData(data2A, 4);
    writeCommand(0x2B);
    uint8_t data2B[] = {0, 0, 0, 0xEF}; // 0-239
    writeData(data2B, 4);
    writeCommand(0x2C);

    std::vector<uint8_t> buf(320 * 240 * 2);
    for (size_t i = 0; i < buf.size(); i += 2) {
        buf[i] = color >> 8;
        buf[i + 1] = color & 0xFF;
    }
    // Send in chunks to avoid large transfers
    size_t chunk = 4096;
    for (size_t i = 0; i < buf.size(); i += chunk) {
        size_t len = (i + chunk > buf.size()) ? (buf.size() - i) : chunk;
        writeData(&buf[i], len);
    }
}

void ST7789VDisplay::drawPixel(uint16_t x, uint16_t y, uint16_t color) {
    writeCommand(0x2A);
    uint8_t data2A[] = {uint8_t(x >> 8), uint8_t(x & 0xFF), uint8_t(x >> 8), uint8_t(x & 0xFF)};
    writeData(data2A, 4);
    writeCommand(0x2B);
    uint8_t data2B[] = {uint8_t(y >> 8), uint8_t(y & 0xFF), uint8_t(y >> 8), uint8_t(y & 0xFF)};
    writeData(data2B, 4);
    writeCommand(0x2C);
    uint8_t colorBuf[] = {uint8_t(color >> 8), uint8_t(color & 0xFF)};
    writeData(colorBuf, 2);
}

void ST7789VDisplay::drawText(uint16_t x, uint16_t y, const char* text, uint16_t color) {
    uint16_t orig_x = x;
    for (size_t i = 0; text[i] != '\0'; ++i) {
        char c = text[i];
        if (c == '\n') {
            y += 10; // Move to next line
            x = orig_x;
            continue;
        }
        if (c < 0 || c > 127) c = '?'; // fallback for non-ASCII
        for (int row = 0; row < 8; ++row) {
            uint8_t bits = font8x8_basic[(int)c][row];
            for (int col = 0; col < 8; ++col) {
                if (bits & (1 << col)) {
                    drawPixel(x + col, y + row, color);
                }
            }
        }
        x += 8; // Move to next character position
        if (x + 8 > 320) { // Wrap if needed
            x = orig_x;
            y += 10;
        }
    }
}


