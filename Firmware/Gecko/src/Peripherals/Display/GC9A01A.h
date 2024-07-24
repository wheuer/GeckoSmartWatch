#ifndef __GC9A01A_H
#define __GC9A01A_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240

typedef struct GC9A01A_device_t {
    struct spi_config* spi_cfg;
    gpio_pin_t chip_select_pin;
    gpio_pin_t data_cmd_pin;
    gpio_pin_t reset_pin;
    const struct device* spi_dev;
    const struct device* gpio_dev;
} GC9A01A_device_t;

uint16_t lcd_RGB(uint8_t red, uint8_t green, uint8_t blue);

void GC9A01A_reset(void);

int GC9A01A_cmd(uint8_t cmd);

int GC9A01A_data(uint8_t* data, int len);

int GC9A01A_init(void);

int GC9A01A_fill_screen(uint16_t rgb);

void GC9A01A_set_position(uint16_t Xstart, uint16_t Xend, uint16_t Ystart, uint16_t Yend);

void GC9A01A_write(uint8_t* data, int len);

void GC9A01A_sleep(void);

void GC9A01A_wake(void);

typedef struct
{
  uint8_t cmd;
  uint8_t data[16];
  uint8_t databytes;
  uint8_t delay_ms;
} GC9A01A_init_cmd_t;

#endif