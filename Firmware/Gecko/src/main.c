#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>

#include <stdio.h>
#include <time.h>

#include "system.h"
#include "clock.h"
#include "Peripherals/Display/LCD.h"
#include "Peripherals/BMA400/bma400.h" 
#include "Peripherals/BMA400/common.h" 
#include "Peripherals/BMA400/taps.h" 
#include "Peripherals/Display/assets.h"
#include "Peripherals/Power/battery.h"
#include "Peripherals/ExternalFlash/externalFlash.h"
#include "Peripherals/Buzzer/buzzer.h"
#include "BLE/BLE.h"

LOG_MODULE_REGISTER(GeckoMain, CONFIG_LOG_DEFAULT_LEVEL);

struct k_event userInteractionEvent;

const struct device* uart0_dev  = DEVICE_DT_GET(UART_DEVICE_LABEL);
const struct device* pwm_dev    = DEVICE_DT_GET(PWM_DEVICE_LABEL);
const struct device* gpio0_dev  = DEVICE_DT_GET(GPIO_0_DEVICE_LABEL);
const struct device* gpio1_dev  = DEVICE_DT_GET(GPIO_1_DEVICE_LABEL);
const struct device* spi_dev    = DEVICE_DT_GET(SPI_DEVICE_LABEL);

#if __DEVELOPMENT_BOARD__
	const struct device* i2c_dev 	= DEVICE_DT_GET(I2C_DEVICE_LABEL);
#endif

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

static int8_t screenIndex = -1;
static bool systemAwake = 0;
struct tm* mTime;
char timeBuffer[64];
static struct k_timer systemSleepTimer;
volatile bool spiEnabled = true;

// Need to disable SPIM3 inbetween using it otherwise it'll continue to draw +700 uA
// See here: https://infocenter.nordicsemi.com/index.jsp?topic=%2Ferrata_nRF52840_Rev3%2FERR%2FnRF52840%2FRev3%2Flatest%2Ferr_840.html
void disableSPI(void)
{
	spiEnabled = false;
	NRF_SPIM3->ENABLE = 0;
	*(volatile uint32_t *)0x4002F004 = 1;
}

void enableSPI(void)
{
	// Seven is the enable value, see datasheet
	NRF_SPIM3->ENABLE = 7;
	spiEnabled = true;
}

static bool systemInit(void)
{
	int error;
	printf("*************************\n  Initilizing System...  \n*************************\n");

	// BLE
	error = BLE_init();

	// Taps/BMA400
	k_event_init(&userInteractionEvent);
	error += bma400Init();

	// System Clock
	error += clockInit();

	// Power and battery management
	error += batteryMonitorInit();

	// External Flash
	error += externalFlashInit();

	// Buzzer
	error += buzzerInit();

	// LCD
	error += lcd_init();
	lcd_set_brightness(0.0);
	lcd_sleep();

	disableSPI();

	if (error)
	{
		printf(ANSI_COLOR_RED "One or more errors detected! Device will not continue." ANSI_COLOR_RESET "\n");
		printf("*************************\n*************************\n");
		return false;
	}
	else
	{
		printf("*************************\n  Initilization Done...  \n*************************\n");
		return true;
	}	
}

static bool zephyrInitSanityCheck(void)
{
	bool err;
	// err = device_is_ready(uart0_dev);
	// if (!err)
	// {
	// 	// Can't print as uart is our console...
	// 	return false;
	// }

	err = device_is_ready(pwm_dev);
	if (!err)
	{
		printf(ANSI_COLOR_RED "Error: PWM device is not ready!\n" ANSI_COLOR_RESET);
		return false;
	}

	err = device_is_ready(spi_dev);
	if (!err) {
		printf(ANSI_COLOR_RED "Error: SPI device is not ready!\n" ANSI_COLOR_RESET);
		return false;
	}

#if __DEVELOPMENT_BOARD__
	err = device_is_ready(i2c_dev);
	if (!err) {
		printf(ANSI_COLOR_RED "Error: I2C device is not ready!\n" ANSI_COLOR_RESET);
		return false;
	}
#endif

	err = device_is_ready(gpio0_dev);
	if (!err) {
		printf(ANSI_COLOR_RED "Error: GPIO_0 device is not ready!\n" ANSI_COLOR_RESET);
		return false;
	}
	
	err = device_is_ready(gpio1_dev);
	if (!err) {
		printf(ANSI_COLOR_RED "Error: GPIO_1 device is not ready!\n" ANSI_COLOR_RESET);
		return false;
	}

	return true;
}

static void systemSleepCallback(struct k_timer* timer_id)
{
	k_event_post(&userInteractionEvent, SYSTEM_EVENT_TIMEOUT);
	k_timer_stop(&systemSleepTimer);
}

int main(void)
{	
	k_msleep(100); // Stop nRF connect sdk print out from messing with console

	/* Make sure zephyr initilization worked properly and devices are ready to go */
	bool systemReady = zephyrInitSanityCheck();
	if (!systemReady) return 0;

	/* Initialize system componenets */
	systemReady = systemInit();
	if (!systemReady) return 0;

	/* Wait on user interaction and handle accordingly */
	uint32_t triggeredEvent;
	int batteryVoltage;
	char voltageBuffer[16];
	k_timer_init(&systemSleepTimer, systemSleepCallback, NULL);

	for(;;)
	{
		// Wait on system events, have k_event_wait will clear relevant events 
		triggeredEvent = k_event_wait(&userInteractionEvent, 0x07, true, K_FOREVER);
		
		// Don't wake up on single taps
		if (!systemAwake && (triggeredEvent & SYSTEM_EVENT_SINGLE_TAP))
		{
			disableSPI();
			continue;
		}

		if (triggeredEvent & SYSTEM_EVENT_DOUBLE_TAP || triggeredEvent & SYSTEM_EVENT_SINGLE_TAP)
		{
			// User interaction event (double tap)
			// Create the sleep timer and update the system status
			if (!systemAwake && (triggeredEvent & SYSTEM_EVENT_DOUBLE_TAP))
			{
				systemAwake = true;
				lcd_wake();
			}

			// Always start/reset timer on user interaction
			k_timer_start(&systemSleepTimer, K_SECONDS(5), K_SECONDS(5));

			// Double taps always incremet before displaying, otherwise screenIndex will be wrong (reset value is -1)
			if (triggeredEvent & SYSTEM_EVENT_DOUBLE_TAP) screenIndex = (screenIndex + 1) % 3;

			switch (screenIndex)
			{
				case 0:
					// Start screen, show the current time, no difference between single/double tap
					printf("Start screen\n");					
					lcd_draw_bitmap((uint8_t*) watchBack, 240, 240, 0, 0);
					getTime(&mTime);
					strftime(timeBuffer, sizeof(timeBuffer), "%I:%M%p", mTime);
					printf("%s\n", timeBuffer);
					lcd_write_str(timeBuffer, 72, 110, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);
					break;
				case 1:
					// Notification screen,
					//	 Double tap - Just show the name of any active notifications
					//   Single tap - Clear all current notifications and display
					printf("Notifications screen\n");
					if (triggeredEvent & SYSTEM_EVENT_SINGLE_TAP) clearNotifications();
					lcd_clear();
					lcd_write_str_grid("Notifications", 2, 11, LCD_CHAR_EXTRA_SMALL, 0xFF, 0xFF, 0xFF);
					if (notificationCount >= 1) lcd_write_str_grid(activeNotifications[0].appName, 3, 2, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);
					if (notificationCount >= 2) lcd_write_str_grid(activeNotifications[1].appName, 5, 1, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);
					if (notificationCount >= 3) lcd_write_str_grid(activeNotifications[2].appName, 7, 1, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);
					if (notificationCount >= 4) lcd_write_str_grid(activeNotifications[3].appName, 9, 1, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);
					if (notificationCount >= 5) lcd_write_str_grid(activeNotifications[4].appName, 11, 2, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);
					break;
				case 2:
					// System status screen, show useful information, no difference between single/double tap
					printf("Status screen\n");
					lcd_clear();
					batteryVoltage = batteryReadVoltage();
					sprintf(voltageBuffer, "Batt: %.2fV", batteryVoltage / 1000.0);
					lcd_write_str_grid(voltageBuffer, 2, 3, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);
					bluetoothConnected ? lcd_write_str_grid("BLE: Connected", 4, 2, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF) : lcd_write_str_grid("BLE: Disconn.", 4, 2, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);
					break;
				default:
					break;
			}

			lcd_set_brightness(1.0);
		
		}
		else if (triggeredEvent & SYSTEM_EVENT_TIME_UPDATE)
		{
			// Minute update event
			if (screenIndex == 0 && systemAwake)
			{
				// Update the time
				getTime(&mTime);
				strftime(timeBuffer, sizeof(timeBuffer), "%I:%M%p", mTime);
				lcd_write_str(timeBuffer, 72, 110, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);
				printf("New minute update: %s\n", timeBuffer);
			}
		}
		else if (triggeredEvent & SYSTEM_EVENT_TIMEOUT)
		{
			// Sleep timer expired
			lcd_set_brightness(0.0);
			systemAwake = false;
			screenIndex = -1;
			lcd_sleep();
			disableSPI();
		}
	}

	return 0;
}
