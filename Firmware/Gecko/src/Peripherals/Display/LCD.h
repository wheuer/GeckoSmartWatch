#ifndef __LCD__
#define __LCD__

#include <zephyr/types.h>
#include <zephyr/drivers/pwm.h>
#include "GC9A01A.h"

#define SCREEN_BUFFER_SIZE 115200 // 240 * 240 * 2

typedef enum {
    LCD_CHAR_EXTRA_SMALL = 1,
    LCD_CHAR_SMALL,
    LCD_CHAR_MEDIUM,
    LCD_CHAR_LARGE,
    LCD_CHAR_EXTRA_LARGE,
    LCD_CHAR_TITLE
} LCD_CHAR_SIZE;

int lcd_init(void);

// Write character of specifed size to specified row/column
// Return negative reponse if the row/column is out of bounds
void lcd_write_char(char character, uint8_t x, uint8_t y, LCD_CHAR_SIZE size, uint8_t red, uint8_t green, uint8_t blue);

// Write string of specified size to starting at specified row/column
// No return as this function should wrap any out of bounds characters
void lcd_write_str(char* string, uint8_t x, uint8_t y, LCD_CHAR_SIZE size, uint8_t red, uint8_t green, uint8_t blue);

void lcd_write_char_grid(char character, uint8_t row, uint8_t column, LCD_CHAR_SIZE size, uint8_t red, uint8_t green, uint8_t blue);

void lcd_write_str_grid(char* string, uint8_t row, uint8_t column, LCD_CHAR_SIZE size, uint8_t red, uint8_t green, uint8_t blue);

// Draw specified bitmap starting at the given coordinates
// It will be assumed the bitmap is already in the 16-bit color format
void lcd_draw_bitmap(uint8_t* bitmap, uint8_t width, uint8_t height, uint8_t x, uint8_t y);

// Fill section of screen with solid color
void lcd_fill(uint8_t red, uint8_t green, uint8_t blue);

// Clear the screen (fill with black, avoid calling as it'll be slow)
void lcd_clear(void);

// Convert standard 24-bit RGB value to 16-bit LCD version
uint16_t lcd_RGB(uint8_t red, uint8_t green, uint8_t blue);

void lcd_sleep(void);

void lcd_wake(void);

// Duty cycle is 0.0-1.0
void lcd_set_brightness(float duty_cycle);

/*
    Possible other functions: Draw circle, rectangle, line, etc 
*/

#endif

