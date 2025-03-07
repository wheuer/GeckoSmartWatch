#ifndef __MAIN__H
#define __MAIN__H

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <nrfx.h>
#include <time.h>
#include <stdio.h>
#include "console.h"

#define __DEVELOPMENT_BOARD__ 1

/* 
    Thread priorites:
    Lower value is higher priority
    Negative priorities will never be preempted 
    Main thread has a default priority of 0
*/
#define TAPS_THREAD_PRIORITY 5

/* System Events */
#define SYSTEM_EVENT_DOUBLE_TAP         0x01
#define SYSTEM_EVENT_SINGLE_TAP         0x02
#define SYSTEM_EVENT_TIMEOUT            0x04
#define SYSTEM_EVENT_TIME_UPDATE        0x08
#define SYSTEM_EVENT_NEW_NOTIFICATION   0x10
#define SYSTEM_EVENT_MAIN_MASK          0x1F // 0b'11111

/* Generic Device Labels */
#define PWM_DEVICE_LABEL        DT_NODELABEL(pwm0)
#define GPIO_0_DEVICE_LABEL     DT_NODELABEL(gpio0)
#define GPIO_1_DEVICE_LABEL     DT_NODELABEL(gpio1)
#define SPI_DEVICE_LABEL        DT_NODELABEL(spi3)
#define UART_DEVICE_LABEL       DT_NODELABEL(uart0)
#define I2C_DEVICE_LABEL        DT_NODELABEL(i2c0)
#if !__DEVELOPMENT_BOARD__
    #define ADC_DEVICE_LABEL        DT_NODELABEL(adc)
#endif

/* SPI Pins */
#if __DEVELOPMENT_BOARD__
    #define SCK_PIN 21
    #define MOSI_PIN 20
    #define MISO_PIN 13
#else
    #define SCK_PIN 19
    #define MOSI_PIN 14
    #define MISO_PIN 13
#endif

/* I2C Device Address' */
#define FUEL_GUAGE_I2C_ADDRESS  0x36
#define BMA400_I2C_ADDRESS      0x14

/* BLE Defines */
#define BT_DEVICE_NAME      CONFIG_BT_DEVICE_NAME
#define BT_DEVICE_NAME_LEN  (sizeof(BT_DEVICE_NAME) - 1)

/* LCD Pins and Port */
#define DISPLAY_START_BRIGHTNESS    60 // %
#define BRIGHTNESS_STEP             20 // %

#if __DEVELOPMENT_BOARD__
    #define LCD_RESET_PIN           19 // P0.19
    #define LCD_CS_PIN              22 // P0.22
    #define LCD_DC_PIN              23 // P0.23
    #define LCD_PWM_CHANNEL         0  // PWM_OUT0 -> Pin is defined in the devicetree
    #define LCD_PWM_PERIOD          PWM_USEC(50) // 10 kHz    
#else
    #define LCD_RESET_PIN           17 // P0.17
    #define LCD_CS_PIN              20 // P0.20
    #define LCD_DC_PIN              21 // P0.21
    #define LCD_PWM_CHANNEL         0  // P0.22, PWM_OUT0
    #define LCD_PWM_PERIOD          PWM_USEC(50) // 10 kHz    
#endif

/* Accelerometer (BMA400) Pins*/
#if __DEVELOPMENT_BOARD__
    #define BMA_INT1_PIN    31 // P0.31
    #define BMA_INT2_PIN    30 // P0.30
#else
    #define BMA_CS_PIN      8  // P1.08
    #define BMA_INT1_PIN    11 // P0.11
    #define BMA_INT2_PIN    9  // P1.09
#endif

/* External FLASH Pins */
#if __DEVELOPMENT_BOARD__
    #define EFLASH_CS_PIN   12  // P0.12
    #define EFLASH_WP_PIN   14  // P0.14
    #define EFLASH_HOLD_PIN 17  // P0.17
#else
    #define EFLASH_CS_PIN   8  // P0.08
    #define EFLASH_WP_PIN   7  // P0.07
    #define EFLASH_HOLD_PIN 12 // P0.12
#endif

/* Buzzer Pins */
// The buzzer pin is defined in the device tree
#define BUZZER_PWM_PERIOD       PWM_USEC(50) // 10 kHz
#define BUZZER_PWM_CHANNEL      1            // PWM_OUT1 -> Pin is defined in the devicetree


/* Power Management (nPM1100) and ADC Pins */
#define PWR_ADC_RESOLUTION          12
#define PWR_ADC_CHANNEL             0
#define PWR_ADC_PORT                SAADC_CH_PSELP_PSELP_AnalogInput6 // P0.30, AIN6
#define PWR_ADC_GAIN                ADC_GAIN_1_6 // 1/6 gain, this is the reset value
#define PWR_ADC_REFERENCE           ADC_REF_INTERNAL // 0.6 V, which is default
#define PWR_ADC_ACQUISITION_TIME    ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40) // 40 is the highest

#if __DEVELOPMENT_BOARD__
    #define PWR_CHARGE_ERR_PIN  2  // P0.02
    #define PWR_CHARGING_PIN    28 // P0.28
#else
    #define PWR_CHARGE_ERR_PIN  4 // P0.04
    #define PWR_CHARGING_PIN    5 // P0.05
#endif

extern struct spi_config spi_cfg;

extern time_t currentSystemTime;
extern bool screenState;
extern volatile bool bluetoothConnected;

extern const struct device* uart0_dev;
extern const struct device* pwm_dev;
extern const struct device* gpio0_dev;
extern const struct device* gpio1_dev;
extern const struct device* spi_dev;
extern const struct device* adc_dev;
extern const struct device* i2c_dev;

extern struct k_event userInteractionEvent;

extern volatile bool spiEnabled;

void disableSPI(void);
void enableSPI(void);

#endif // __MAIN_H