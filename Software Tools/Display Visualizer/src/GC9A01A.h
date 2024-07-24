#ifndef __GC9A01A_H
#define __GC9A01A_H

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240
#define SCREEN_BUFFER_LENGTH (SCREEN_HEIGHT * SCREEN_WIDTH * 2) // 2 bytes per pixel, 5-6-5 color scheme

extern uint8_t screenBuffer[SCREEN_BUFFER_LENGTH];

uint16_t lcd_RGB(uint8_t red, uint8_t green, uint8_t blue);

int GC9A01A_fill_screen(uint16_t rgb);

void GC9A01A_set_position(uint16_t Xstart, uint16_t Xend, uint16_t Ystart, uint16_t Yend);

void GC9A01A_write(uint8_t* data, int len);

#endif