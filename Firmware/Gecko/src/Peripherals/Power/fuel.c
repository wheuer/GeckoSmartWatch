#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#include "system.h"
#include "fuel.h"

static struct gpio_callback fuel_callback_t;

static void fuel_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    // Got an interrupt, need to clear it to exit interrupt as we used GPIO_INT_LEVEL_LOW instead of GPIO_INT_EDGE_FALLING

}

int batteryFuelInit(void)
{
    int error;     
    printf("Init fuel guage...");

    // Initialize the fuel alert pin, it is open drain with a external pull-up, no interrupts yet as we will need a task
    gpio_pin_configure(gpio0_dev, FUEL_GUAGE_ALERT_PIN, GPIO_INPUT);
    // gpio_pin_interrupt_configure(gpio0_dev, FUEL_GUAGE_ALERT_PIN, GPIO_INT_LEVEL_LOW); // Use GPIO_INT_LEVEL_LOW instead of GPIO_INT_EDGE_FALLING to save ~40 uA    
    // gpio_init_callback(&fuel_callback_t, fuel_callback, BIT(FUEL_GUAGE_ALERT_PIN));    
    // gpio_add_callback(gpio0_dev, &fuel_callback_t);  

    printf(ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "\n");
    return 0;
}

float getBatteryVoltage(void)
{
    uint8_t raw[2];
    uint8_t error = i2c_burst_read(i2c_dev, FUEL_GUAGE_I2C_ADDRESS, VCELL_ADDR, raw, 2);

    // The VCELL measurement that is read is a 16-bit unsigned value with an LSB of 78.125 uV
    if (error)
    {
        printf(ANSI_COLOR_RED "Fuel Guage ERR: I2C read error" ANSI_COLOR_RESET "\n");
        return -1.0;
    }
    // return (((raw[0] << 8) | raw[1]) * 78.125) / 1000;
    return (float) ((raw[0] << 8) | raw[1]);
}

float getBatteryStateOfCharge(void)
{
    uint8_t raw[2];
    uint8_t error = i2c_burst_read(i2c_dev, FUEL_GUAGE_I2C_ADDRESS, SOC_ADDR, raw, 2);

    // The state of charge measurement that is read is a 16-bit unsigned value with an LSB of 1/256%
    if (error)
    {
        printf(ANSI_COLOR_RED "Fuel Guage ERR: I2C read error" ANSI_COLOR_RESET "\n");
        return -1.0;
    }
    return ((raw[0] << 8) | raw[1]) / 256.0;
}



