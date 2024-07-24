#include <zephyr/kernel.h>
// #include <zephyr/drivers/gpio.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/poweroff.h>

#define BMA_CS_PIN      8  // P1.08
#define FLASH_CS_PIN    8  // P0.08
#define LCD_CS_PIN      20 // P0.20
#define LCD_PWM_PIN     22 // P0.22

const struct device* gpio0_dev  = DEVICE_DT_GET(DT_NODELABEL(gpio0));
const struct device* gpio1_dev  = DEVICE_DT_GET(DT_NODELABEL(gpio1));

int main(void)
{
        // All peripherals disabled, only initalize and set the CS lines for the SPI devices
        // gpio_pin_configure(gpio1_dev, BMA_CS_PIN, GPIO_OUTPUT);
	// gpio_pin_set(gpio1_dev, BMA_CS_PIN, 1);

        // gpio_pin_configure(gpio0_dev, FLASH_CS_PIN, GPIO_OUTPUT);
	// gpio_pin_set(gpio0_dev, FLASH_CS_PIN, 1);

        // gpio_pin_configure(gpio0_dev, LCD_CS_PIN, GPIO_OUTPUT);
	// gpio_pin_set(gpio0_dev, LCD_CS_PIN, 1);

        // gpio_pin_configure(gpio0_dev, LCD_PWM_PIN, GPIO_OUTPUT);
	// gpio_pin_set(gpio0_dev, LCD_PWM_PIN, 0);

        // sys_poweroff();

        while (1)
        {
                // Do work
                volatile uint32_t bruh = 0;
                for (int i = 0; i < 100000000; i++)
                {
                        bruh++;
                }
                bruh = 0;
                k_msleep(1000);
        }

        return 0;
}

