#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <nrfx.h>
#include <nrfx_spim.h>
#include "GC9A01A.h"
#include "system.h"

GC9A01A_device_t mGC9A01A_device = {
    .spi_cfg = &spi_cfg,
    .chip_select_pin = LCD_CS_PIN,
    .data_cmd_pin = LCD_DC_PIN,
    .reset_pin = LCD_RESET_PIN,
    .spi_dev = DEVICE_DT_GET(SPI_DEVICE_LABEL),
    .gpio_dev = DEVICE_DT_GET(GPIO_0_DEVICE_LABEL)
};

static struct spi_buf_set   spi_tx_buffer_set;
static struct spi_buf       tx_spi_buf;

static GC9A01A_init_cmd_t GC9A01A_init_cmds[] = {
    {0xEF, {0}, 0, 0},
    {0xEB, {0x14}, 1, 0},
    {0xFE, {0}, 0, 0},
    {0xEF, {0}, 0, 0},
    {0xEB, {0x14}, 1, 0},
    {0x84, {0x40}, 1, 0},
    {0x85, {0xFF}, 1, 0},
    {0x86, {0xFF}, 1, 0},
    {0x87, {0xFF}, 1, 0},
    {0x88, {0x0A}, 1, 0},
    {0x89, {0x21}, 1, 0},
    {0x8A, {0x00}, 1, 0},
    {0x8B, {0x80}, 1, 0},
    {0x8C, {0x01}, 1, 0},
    {0x8D, {0x01}, 1, 0},
    {0x8E, {0xFF}, 1, 0},
    {0x8F, {0xFF}, 1, 0},
    {0xB6, {0x00, 0x00}, 2, 0},
    {0x36, {0x48}, 1, 0}, // This one might need to be changed if it's being weird
    {0x3A, {0x05}, 1, 0},
    {0x90, {0x08, 0x08, 0x08, 0x08}, 4, 0},
    {0xBD, {0x06}, 1, 0},
    {0xBC, {0x00}, 1, 0},
    {0xFF, {0x60, 0x01, 0x04}, 3, 0},
    {0xC3, {0x13}, 1, 0},
    {0xC4, {0x13}, 1, 0},
    {0xC9, {0x22}, 1, 0},
    {0xBE, {0x11}, 1, 0},
    {0xE1, {0x10, 0x0E}, 2, 0},
    {0xDF, {0x21, 0x0C, 0x02}, 3, 0},
    {0xF0, {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A}, 6, 0},
    {0xF1, {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F}, 6, 0},
    {0xF2, {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A}, 6, 0},
    {0xF3, {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F}, 6, 0},
    {0xED, {0x1B, 0x0B}, 2, 0},
    {0xAE, {0x77}, 1, 0},
    {0xCD, {0x63}, 1, 0},
    {0x70, {0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03}, 9, 0},
    {0xE8, {0x34}, 1, 0},
    {0x62, {0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70}, 12, 0},
    {0x63, {0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70}, 12, 0},
    {0x64, {0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07}, 7, 0},
    {0x66, {0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00}, 10, 0},
    {0x67, {0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98}, 10, 0},
    {0x74, {0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00}, 7, 0},
    {0x98, {0x3E, 0x07}, 2, 0},
    {0x35, {0}, 0, 0},
    {0x21, {0}, 0, 0},
    {0x11, {0}, 0, 120},
    {0x29, {0}, 0, 20},
    {0x00, {0}, 0xFF, 20} // End of sequence command
};

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

void GC9A01A_reset(void)
{
    // Nothing here, all done in init
}

int GC9A01A_cmd(uint8_t cmd)
{
    if (!spiEnabled) enableSPI();

    tx_spi_buf.buf = &cmd, 
    tx_spi_buf.len = 1;
    spi_tx_buffer_set.buffers = &tx_spi_buf;
    spi_tx_buffer_set.count = 1;

    int error;
    error = gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.data_cmd_pin, 0);
    error += spi_write(mGC9A01A_device.spi_dev, mGC9A01A_device.spi_cfg, &spi_tx_buffer_set);
    return error;
}

int GC9A01A_data(uint8_t* data, int len)
{
    if (!spiEnabled) enableSPI();
    
    tx_spi_buf.buf = data, 
    tx_spi_buf.len = len;
    spi_tx_buffer_set.buffers = &tx_spi_buf;
    spi_tx_buffer_set.count = 1;  

    int error;
    error = gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.data_cmd_pin, 1);
    error += spi_write(mGC9A01A_device.spi_dev, mGC9A01A_device.spi_cfg, &spi_tx_buffer_set);
    return error;
}

int GC9A01A_init(void)
{
    int error;
    error = gpio_pin_configure(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, GPIO_OUTPUT);    
    error += gpio_pin_configure(mGC9A01A_device.gpio_dev, mGC9A01A_device.reset_pin, GPIO_OUTPUT);
    error += gpio_pin_configure(mGC9A01A_device.gpio_dev, mGC9A01A_device.data_cmd_pin, GPIO_OUTPUT);
    if (error)
    {
        printf(ANSI_COLOR_RED "ERR: GPIO Configure" ANSI_COLOR_RESET "\n");
        return error;
    }

    error = gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, 1);
    k_msleep(5);
    error += gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.reset_pin, 0);
    k_msleep(10);
    error += gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.reset_pin, 1);
    k_msleep(120);
    if (error)
    {
        printf(ANSI_COLOR_RED "ERR: GC9A01A Reset Sequence" ANSI_COLOR_RESET "\n");
        return error;
    }

    int cmd = 0;
    while (GC9A01A_init_cmds[cmd].databytes != 0xff)
    {
        error += gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, 0);
        error += GC9A01A_cmd(GC9A01A_init_cmds[cmd].cmd);
        if (GC9A01A_init_cmds[cmd].databytes > 0)
        {
            error += GC9A01A_data(GC9A01A_init_cmds[cmd].data, GC9A01A_init_cmds[cmd].databytes);
        }
        error += gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, 1);
        k_msleep(GC9A01A_init_cmds[cmd].delay_ms);
        cmd++;
    }
    if (error)
    {
        printf(ANSI_COLOR_RED "ERR: GC9A01A Init Sequence" ANSI_COLOR_RESET "\n");
        return error;
    }

    return error;
}

int GC9A01A_fill_screen(uint16_t rgb)
{
    unsigned int i,j;
    uint8_t temp[2];
    GC9A01A_set_position(0, 239, 0, 239);
    gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, 0);
    for (i=0; i < 240; i++)
    {
        for (j=0; j<240; j++) 
        {
            temp[0] = rgb >> 8;
            temp[1] = rgb & 0xFF;
            GC9A01A_data(temp, 2);
        }
    }
    gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, 1);
    return 0;
}

void GC9A01A_set_position(uint16_t Xstart, uint16_t Xend, uint16_t Ystart, uint16_t Yend)
{
    uint8_t temp[4];
    gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, 0);
	GC9A01A_cmd(0x2a);   
    temp[0] = Xstart >> 8;
    temp[1] = Xstart;
    temp[2] = Xend >> 8;
    temp[3] = Xend;
	GC9A01A_data(temp, 4);
    gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, 1);

    gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, 0);
	GC9A01A_cmd(0x2b);   
    temp[0] = Ystart >> 8;
    temp[1] = Ystart;
    temp[2] = Yend >> 8;
    temp[3] = Yend;
	GC9A01A_data(temp, 4);
    gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, 1);

    // Memory write command, this will reset the write pointer to the row and column just set
    gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, 0);
    GC9A01A_cmd(0x2C);
    gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, 1);
}

void GC9A01A_write(uint8_t* data, int len)
{
    gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, 0);
    GC9A01A_cmd(0x2C);
    GC9A01A_data(data, len);
    gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, 1);
}

void GC9A01A_sleep(void)
{
    // Sleep command is 0x10
    gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, 0);
    GC9A01A_cmd(0x10);
    gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, 1);

    // Must wait at least 5 ms before next command, 120 ms before sending sleep out
    // Fix this in the future, we need a mutex and timer here, for now just wait it out
    k_msleep(150);
}

void GC9A01A_wake(void)
{
    // Sleep out command is 0x11
    gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, 0);
    GC9A01A_cmd(0x11);
    gpio_pin_set(mGC9A01A_device.gpio_dev, mGC9A01A_device.chip_select_pin, 1);

    // Must wait at least 5 ms before next command, 120 ms before sending sleep in
    // Fix this in the future, we need a mutex and timer here, for now just wait it out
    k_msleep(150);
}