#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

// I2C address for SSD1306 OLED display
#define SSD1306_I2C_ADDR 0x3C

// Display resolution
#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 32

// SSD1306 Commands
#define SSD1306_COMMAND  0x00
#define SSD1306_DATA     0x40

// Enumeration for pixel colors
typedef enum {
    SSD1306_COLOR_BLACK = 0x00,
    SSD1306_COLOR_WHITE = 0x01
} SSD1306_COLOR_t;

// Function prototypes
bool SSD1306_Init(i2c_inst_t *i2c);
void SSD1306_UpdateScreen(void);
void SSD1306_Clear(void);
void SSD1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR_t color);
void SSD1306_DrawText(uint8_t x, uint8_t y, const char *text);
void SSD1306_DisplayTest(void);

#endif
