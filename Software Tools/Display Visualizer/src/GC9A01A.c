#include <stdio.h>
#include <stdint.h>
#include "GC9A01A.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

uint8_t screenBuffer[SCREEN_BUFFER_LENGTH];

static uint16_t currentX = 0;
static uint16_t currentY = 0;
static uint16_t currentXMax = 239;
static uint16_t currentYMax = 239;
static uint16_t currentXMin = 0;
static uint16_t currentYMin = 0;

static uint8_t map(uint8_t x, uint8_t in_min, uint8_t in_max, uint8_t out_min, uint8_t out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint16_t lcd_RGB(uint8_t red, uint8_t green, uint8_t blue)
{
    // RGB -> 5-6-5 bits
    uint16_t color = 0;
    uint8_t tmp;

    // Need to map 0-255 -> 0-31 for RED and BLUE
    tmp = map(red, 0, 255, 0, 31);
    color |= tmp << 11;

    tmp = map(blue, 0, 255, 0, 31);
    color |= tmp;

    // Need to map 0-255 -> 0-63 for GREEN
    tmp = map(green, 0, 255, 0, 63);
    color |= tmp << 5;

    return color;
}

int GC9A01A_fill_screen(uint16_t rgb)
{
    for (int i = 0; i < SCREEN_BUFFER_LENGTH; i+=2)
    {
        screenBuffer[i] = rgb >> 8;
        screenBuffer[i + 1] = rgb & 0xFF;
    }
    return 0;
}

void GC9A01A_set_position(uint16_t Xstart, uint16_t Xend, uint16_t Ystart, uint16_t Yend)
{
    currentXMax = MIN(Xend, 239);
    currentYMax = MIN(Yend, 239);
    currentXMin = MIN(Xstart, 239);
    currentYMin = MIN(Ystart, 239);    

    currentX = currentXMin;
    currentY = currentYMin;
}

void GC9A01A_write(uint8_t* data, int len)
{
    //printf("Length: %d\n", len);
    uint16_t currentIndexX = 2 * currentX;
    while (len > 0)
    {
        // Need to write two bytes per pixel
        screenBuffer[currentY * SCREEN_WIDTH * 2 + currentIndexX] = *data++;
        currentIndexX++;
        screenBuffer[currentY * SCREEN_WIDTH * 2 + currentIndexX] = *data++;
        currentIndexX++;

        currentX++;
        if (currentX > currentXMax) 
        {
            //printf("Row Done, y: %d x: %d\n", currentY, currentX);
            currentX = currentXMin;
            currentIndexX = 2 * currentX;
            currentY++;
        }

        if (currentY > currentYMax)
        {
            // If we overflow the Y we are done
            //printf("Exceeded Y range: %d\n", currentY);
            currentY = currentYMin;
            return;
        }
        len-=2;
    }
}
