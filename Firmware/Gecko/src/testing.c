#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>

#include "system.h"
#include "testing.h"
#include "Peripherals/Display/LCD.h"

static struct spi_buf_set spi_tx_buffer_set;
static struct spi_buf tx_spi_buf;

static struct spi_buf_set spi_rx_buffer_set;
static struct spi_buf rx_spi_buf;

/*
    These tests will only test the bare functionality of the device.
    Basically are they soldered correctly and can they be communicated with.
    All tests assume that the required initilization has been performed.
*/

int test_BMA400(void)
{
    // Test by reading the BMA400 chip ID, should read 0x90
    uint8_t versionRegister = 0x00;
    uint8_t response;
	int result = i2c_write_read(i2c_dev, BMA400_I2C_ADDRESS, &versionRegister, 1, &response, 1);
    if (result)
    {
        printf("BMA400 guage read failed. err=%d\n\n", result);
        return -1;
    }
    else if (response != 0x90)
    {
        printf("BMA400 chip ID mismatch. Expected 0x90, got 0x%02x.\n", response);
        return -1;
    }
    return 0;
}

int test_fuelGauge(void)
{
    // Test by reading the fuel guage's version address
    // From datasheet should return 0x001X, from testing should return 0x0012
    uint8_t versionRegister = 0x08;
    uint8_t response[2];
    int result = i2c_write_read(i2c_dev, FUEL_GUAGE_I2C_ADDRESS, &versionRegister, 1, response, 2);
    if (result)
    {
        printf("Fuel guage read failed. err=%d\n\n", result);
        return -1;
    }
    else if (response[0] != 0x00 || response[1] != 0x12)
    {
        printf("Fuel guage version mismatch. Expected 0x0012, got 0x%04x.\n", (response[0] << 8) | response[1]);
        return -1;
    }
    return 0;
}

int test_ExternalFlash(void)
{
    // Read the JEDEC ID, should return the manufactuor ID then the type and size of the memory
    // Datasheet says it should return 0x1F 0x89 0x01
	uint8_t command = 0x9F;
    uint8_t response[3];
    int result;

    tx_spi_buf.buf = &command;
    tx_spi_buf.len = 1;
    spi_tx_buffer_set.buffers = &tx_spi_buf;
    spi_tx_buffer_set.count = 1;

    rx_spi_buf.buf = response;
    rx_spi_buf.len = 3;
    spi_rx_buffer_set.buffers = &rx_spi_buf;
    spi_rx_buffer_set.count = 1;

    // Send CS low
    result = gpio_pin_set(gpio0_dev, EFLASH_CS_PIN, 0);

    // Send the command
    result += spi_write(spi_dev, &spi_cfg, &spi_tx_buffer_set);

    // Read back the response
    result += spi_read(spi_dev, &spi_cfg, &spi_rx_buffer_set);

    // Set CS high again
    result += gpio_pin_set(gpio0_dev, EFLASH_CS_PIN, 1);

    printf("External flash JEDEC ID 0x%02x  0x%02x  0x%02x\n", response[0], response[1], response[2]);

    if (result)
    {
        // As the error could come from multiple sources just say there was an error
        printf("External flash read failed.\n");
        return -1;
    }
    else if (response[0] != 0x1F || response[1] != 0x89 || response[2] != 0x01)
    {
        printf("External flash JEDEC ID mismatch, expected 0x1F 0x89 0x00, got 0x%02x  0x%02x  0x%02x\n", response[0], response[1], response[2]);
        return -1;
    }
    return 0;
}

int test_Buzzer(void)
{
    // Here we just have to make it buzz and the user will know if it worked or not
    // TODO: implement after PWM operation working
    return 0;
}

int test_DisplayBacklight(void)
{
    // TODO: implement after PWM operation working
    return 0;    
}

int test_Display(void)
{
    // To test display we cannot read back any info as it uses 3-wire SPI and I'm not doing that...
    // Just render something on to the display and have the user check
	lcd_clear();
    lcd_fill(0x00, 0xFF, 0x00);
    printf("Display should be green if successful. Make sure to enable backlight.\n");
    return 0;
}