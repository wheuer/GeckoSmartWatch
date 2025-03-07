#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#include "system.h"

static struct spi_buf_set spi_tx_buffer_set;
static struct spi_buf tx_spi_buf;

static struct spi_buf_set spi_rx_buffer_set;
static struct spi_buf rx_spi_buf;

void readRegisters(void)
{
    uint8_t command;
    uint8_t response;

    printf("-----EXTERNAL FLASH STATUS REGISTERS-----\r\n");
    tx_spi_buf.buf = &command;
    tx_spi_buf.len = 1;
    spi_tx_buffer_set.buffers = &tx_spi_buf;
    spi_tx_buffer_set.count = 1;

    rx_spi_buf.buf = &response;
    rx_spi_buf.len = 1;
    spi_rx_buffer_set.buffers = &rx_spi_buf;
    spi_rx_buffer_set.count = 1;

    command = 0x05;

    // Send CS low
    gpio_pin_set(gpio0_dev, EFLASH_CS_PIN, 0);

    // Send the command
    spi_write(spi_dev, &spi_cfg, &spi_tx_buffer_set);

    // Read back the response
    spi_read(spi_dev, &spi_cfg, &spi_rx_buffer_set);

    // Set CS high again
    gpio_pin_set(gpio0_dev, EFLASH_CS_PIN, 1);

    printf("S0:  %u\r\n", response & 1);
    printf("S1:  %u\r\n", (response >> 1) & 1);
    printf("S2:  %u\r\n", (response >> 2) & 1);
    printf("S3:  %u\r\n", (response >> 3) & 1);
    printf("S4:  %u\r\n", (response >> 4) & 1);
    printf("S5:  %u\r\n", (response >> 5) & 1);
    printf("S6:  %u\r\n", (response >> 6) & 1);
    printf("S7:  %u\r\n", (response >> 7) & 1);

    k_msleep(1);

    command = 0x35;

    // Send CS low
    gpio_pin_set(gpio0_dev, EFLASH_CS_PIN, 0);

    // Send the command
    spi_write(spi_dev, &spi_cfg, &spi_tx_buffer_set);

    // Read back the response
    spi_read(spi_dev, &spi_cfg, &spi_rx_buffer_set);

    // Set CS high again
    gpio_pin_set(gpio0_dev, EFLASH_CS_PIN, 1);

    printf("S8:  %u\r\n", response & 1);
    printf("S9:  %u\r\n", (response >> 1) & 1);
    printf("S10: %u\r\n", (response >> 2) & 1);
    printf("S11: %u\r\n", (response >> 3) & 1);
    printf("S12: %u\r\n", (response >> 4) & 1);
    printf("S13: %u\r\n", (response >> 5) & 1);
    printf("S14: %u\r\n", (response >> 6) & 1);
    printf("S15: %u\r\n", (response >> 7) & 1);

    k_msleep(1);

    command = 0x15;

    // Send CS low
    gpio_pin_set(gpio0_dev, EFLASH_CS_PIN, 0);

    // Send the command
    spi_write(spi_dev, &spi_cfg, &spi_tx_buffer_set);

    // Read back the response
    spi_read(spi_dev, &spi_cfg, &spi_rx_buffer_set);

    // Set CS high again
    gpio_pin_set(gpio0_dev, EFLASH_CS_PIN, 1);

    printf("S16: %u\r\n", response & 1);
    printf("S17: %u\r\n", (response >> 1) & 1);
    printf("S18: %u\r\n", (response >> 2) & 1);
    printf("S19: %u\r\n", (response >> 3) & 1);
    printf("S20: %u\r\n", (response >> 4) & 1);
    printf("S21: %u\r\n", (response >> 5) & 1);
    printf("S22: %u\r\n", (response >> 6) & 1);
    printf("S23: %u\r\n", (response >> 7) & 1);
    printf("-----------------------------------------\r\n\n");
}

int externalFlashInit(void)
{
    int error;
    printf("Init External Flash...");

    error = gpio_pin_configure(gpio0_dev, EFLASH_CS_PIN, GPIO_OUTPUT);
    error += gpio_pin_set(gpio0_dev, EFLASH_CS_PIN, 1);
    if (error)
    {
        printf(ANSI_COLOR_RED "ERR" ANSI_COLOR_RESET "\n");
    }
    else 
    {
        printf(ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "\n");
    }
    return error;
}