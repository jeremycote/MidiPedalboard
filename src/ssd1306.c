#include "ssd1306.h"
#include "hardware/i2c.h"
#include <string.h>
#include <pico/printf.h>

// I2C instance
static i2c_inst_t *ssd1306_i2c;
static uint8_t buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

// Function to send a command to the SSD1306 display
static bool SSD1306_WriteCommand(uint8_t cmd) {
    uint8_t data[2] = { SSD1306_COMMAND, cmd };
    int ret = i2c_write_blocking(ssd1306_i2c, SSD1306_I2C_ADDR, data, 2, false);
    
    if (ret < 0) {
        printf("Error sending command: 0x%02X (I2C Error: %d)\n", cmd, ret);
        return false;
    }
    return true;
}

// Initialize the SSD1306 OLED display
bool SSD1306_Init(i2c_inst_t *i2c) {
    printf("Initializing SSD1306...\n");
    ssd1306_i2c = i2c;

    // Give the display time to power on
    sleep_ms(100);

    // Full SSD1306 initialization sequence
    SSD1306_WriteCommand(0xAE); // Display OFF
    SSD1306_WriteCommand(0xD5); // Set Display Clock Divide
    SSD1306_WriteCommand(0x80); // Suggested ratio
    SSD1306_WriteCommand(0xA8); 
    SSD1306_WriteCommand(SSD1306_HEIGHT - 1); // Multiplex ratio for 128x32
    SSD1306_WriteCommand(0xD3); // Display Offset
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(0x40); // Start Line 0
    SSD1306_WriteCommand(0x8D); // Charge Pump
    SSD1306_WriteCommand(0x14);
    SSD1306_WriteCommand(0xA1); // Segment Re-map 127 to 0
    SSD1306_WriteCommand(0xC8); // COM Output Scan Direction
    SSD1306_WriteCommand(0xDA); // COM Pins
    SSD1306_WriteCommand(0x02);
    SSD1306_WriteCommand(0x81); // Contrast
    SSD1306_WriteCommand(0x7F);
    SSD1306_WriteCommand(0xD9); // Pre-charge period
    SSD1306_WriteCommand(0xF1);
    SSD1306_WriteCommand(0xDB); // VCOMH Deselect level
    SSD1306_WriteCommand(0x40);
    SSD1306_WriteCommand(0xA4); // Resume to RAM content
    SSD1306_WriteCommand(0xA6); // Normal Display Mode
    SSD1306_WriteCommand(0xAF); // Display ON

    SSD1306_Clear();
    SSD1306_UpdateScreen();
    printf("SSD1306 Initialized Successfully\n");

    return true;
}



// Clear the display buffer
void SSD1306_Clear(void) {
    memset(buffer, 0, sizeof(buffer));
    SSD1306_UpdateScreen();
}

// Update the display with the buffer contents
void SSD1306_UpdateScreen(void) {
    uint8_t data[SSD1306_WIDTH + 1];
    data[0] = SSD1306_DATA;

    for (uint8_t page = 0; page < (SSD1306_HEIGHT / 8); page++) {
        SSD1306_WriteCommand(0xB0 + page);
        SSD1306_WriteCommand(0x00);
        SSD1306_WriteCommand(0x10);

        memcpy(&data[1], &buffer[page * SSD1306_WIDTH], SSD1306_WIDTH);
        i2c_write_blocking(ssd1306_i2c, SSD1306_I2C_ADDR, data, sizeof(data), false);
    }
}

// Draw a pixel on the display
void SSD1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR_t color) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;

    if (color == SSD1306_COLOR_WHITE)
        buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y % 8));
    else
        buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
}

// Draw simple text on the screen
void SSD1306_DrawText(uint8_t x, uint8_t y, const char *text) {
    while (*text) {
        for (uint8_t i = 0; i < 6; i++) {  // 6-pixel-wide font
            buffer[x + (y / 8) * SSD1306_WIDTH] = (*text);
            x += 6; // Move to next character position
        }
        text++;  // Move to next character in string
    }
    SSD1306_UpdateScreen();
}


// Display test message
void SSD1306_DisplayTest(void) {
    SSD1306_Clear();
    SSD1306_DrawText(10, 10, "Hello MIDI!");
    SSD1306_UpdateScreen();
}
