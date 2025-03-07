#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <time.h>

#include "bma400.h"
#include "common.h"
#include "system.h"

static uint8_t BMA_addr;
int8_t interfaceResult;

#if __DEVELOPMENT_BOARD__
int8_t bma400_read_i2c(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void* intf_ptr){
    i2c_burst_read(i2c_dev, *((uint8_t*) intf_ptr), reg_addr, reg_data, length);
    return BMA400_OK;
}

int8_t bma400_write_i2c(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void* intf_ptr){
    i2c_burst_write(i2c_dev, *((uint8_t*) intf_ptr), reg_addr, reg_data, length);
    return BMA400_OK;
}
#else

struct spi_config spi_cfg = {
    .frequency = 33554432,
    .operation = SPI_WORD_SET(8),
    .slave = 0,
	// Expand out but still set to null to avoid compiler warning
    .cs = {
		.gpio = {
			.dt_flags = 0,
			.pin = 0,
			.port = 0
		},
		.delay = 0
	}
};

static struct spi_buf_set spi_tx_buffer_set;
static struct spi_buf tx_spi_buf[2];

static struct spi_buf_set spi_rx_buffer_set;
static struct spi_buf rx_spi_buf;
int8_t bma400_read_spi(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    if (!spiEnabled) enableSPI();

    // Cannot combine the spi_write and spi_read into a spi_transceive becuase
    //  the different data transfers mess with each other. 
    gpio_pin_set(gpio1_dev, BMA_CS_PIN, 0);
    
    // One transfer for the register address
    tx_spi_buf[0].buf = &reg_addr;
    tx_spi_buf[0].len = 1;
    spi_tx_buffer_set.buffers = tx_spi_buf;
    spi_tx_buffer_set.count = 1;

    int err = spi_write(spi_dev, &spi_cfg, &spi_tx_buffer_set);
    if (err) printf("[BMA400,read SPI WRITE] Error: %u\r\n", err);

    // One receive transfer for the actual data
    rx_spi_buf.buf = reg_data;
    rx_spi_buf.len = len;
    spi_rx_buffer_set.buffers = &rx_spi_buf;
    spi_rx_buffer_set.count = 1;

    err = spi_read(spi_dev, &spi_cfg, &spi_rx_buffer_set);
    if (err) printk("[BMA400,read SPI READ] Error: %u\r\n", err);

    gpio_pin_set(gpio1_dev, BMA_CS_PIN, 1);

    return BMA400_OK;
}

int8_t bma400_write_spi(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    if (!spiEnabled) enableSPI();

    gpio_pin_set(gpio1_dev, BMA_CS_PIN, 0);
    
    // One transfer for the register address
    tx_spi_buf[0].buf = &reg_addr;
    tx_spi_buf[0].len = 1;
    
    // Another transfer for the actual data
    tx_spi_buf[1].buf = reg_data; 
    tx_spi_buf[1].len = len;
    spi_tx_buffer_set.buffers = tx_spi_buf;
    spi_tx_buffer_set.count = 2;

    int err = spi_write(spi_dev, &spi_cfg, &spi_tx_buffer_set);
    if (err) printk("[BMA400,write SPI WRITE] Error: %u\r\n", err);

    gpio_pin_set(gpio1_dev, BMA_CS_PIN, 1);

    return BMA400_OK;
}
#endif

int8_t bma400_interface_init(struct bma400_dev *bme, uint8_t intf){
    #if __DEVELOPMENT_BOARD__
        bme->read = bma400_read_i2c;
        bme->write = bma400_write_i2c;
        bme->intf = BMA400_I2C_INTF;
        BMA_addr = BMA400_I2C_ADDRESS;
    #else
        gpio_pin_configure(gpio1_dev, BMA_CS_PIN, GPIO_OUTPUT);
	    gpio_pin_set(gpio1_dev, BMA_CS_PIN, 1);
        bme->read = bma400_read_spi;
        bme->write = bma400_write_spi;
        bme->intf = BMA400_SPI_INTF;
    #endif

    bme->intf_ptr = &BMA_addr;
    bme->chip_id = 0x00;
    bme->delay_us = bma400_delay_us;
    bme->read_write_len = 46;
    bme->resolution = 12;
    return BMA400_OK; 
}

void bma400_delay_us(uint32_t period_us, void* intf_ptr){
    k_sleep(K_USEC(period_us));
}

void bma400_check_rslt(const char api_name[], int8_t rslt){
    switch (rslt)
    {
        case BMA400_OK:
            /* Do nothing */
            break;
        case BMA400_E_NULL_PTR:
            printf("%s :: Error [%d] : Null pointer\r\n", api_name, rslt);
            break;
        case BMA400_E_COM_FAIL:
            printf("%s :: Error [%d] : Communication failure\r\n", api_name, rslt);
            break;
        case BMA400_E_INVALID_CONFIG:
            printf("%s :: Error [%d] : Invalid configuration\r\n", api_name, rslt);
            break;
        case BMA400_E_DEV_NOT_FOUND:
            printf("%s :: Error [%d] : Device not found\r\n", api_name, rslt);
            break;
        default:
            printf("%s :: Error [%d] : Unknown error code\r\n", api_name, rslt);
            break;
    }
}

void bma400_coines_deinit(void){
    return; // Should never have to deinit
}
