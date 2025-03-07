#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

#include <stdio.h>
#include <time.h>

#include <lvgl.h>

#include "system.h"
#include "clock.h"
#include "Peripherals/BMA400/bma400.h" 
#include "Peripherals/BMA400/common.h" 
#include "Peripherals/BMA400/taps.h" 
#include "Peripherals/Display/assets.h"
#include "Peripherals/Power/battery.h"
#include "Peripherals/ExternalFlash/externalFlash.h"
#include "Peripherals/Buzzer/buzzer.h"
#include "BLE/BLE.h"
#include "Peripherals/Display/lvgl_layer.h"

LOG_MODULE_REGISTER(GeckoMain, CONFIG_LOG_DEFAULT_LEVEL);

struct k_event userInteractionEvent;

const struct device* uart0_dev  	= DEVICE_DT_GET(UART_DEVICE_LABEL);
const struct device* pwm_dev    	= DEVICE_DT_GET(PWM_DEVICE_LABEL);
const struct device* gpio0_dev 	 	= DEVICE_DT_GET(GPIO_0_DEVICE_LABEL);
const struct device* gpio1_dev  	= DEVICE_DT_GET(GPIO_1_DEVICE_LABEL);
const struct device* spi_dev    	= DEVICE_DT_GET(SPI_DEVICE_LABEL);
static const struct device* display_dev 	= DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

#if __DEVELOPMENT_BOARD__
	const struct device* i2c_dev 	= DEVICE_DT_GET(I2C_DEVICE_LABEL);
#endif

static bool systemAwake = true;
struct tm* mTime;
char timeBuffer[64];
static struct k_timer systemSleepTimer;

static bool systemInit(void)
{
	int error = 0;
	printf("*************************\n  Initializing System...  \n*************************\n");

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

	// Display
	error += display_lvgl_init();

	if (error)
	{
		printf(ANSI_COLOR_RED "One or more init errors detected! Device will not continue." ANSI_COLOR_RESET "\n");
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
	err = device_is_ready(uart0_dev);
	if (!err)
	{
		// Can't print as uart is our console...
		return false;
	}

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

	err = device_is_ready(display_dev);
	if (!err)
	{
		printf(ANSI_COLOR_RED "Error: Display device is not ready!\n" ANSI_COLOR_RESET);
		return false;
	}

	return true;
}

static void systemSleepCallback(struct k_timer* timer_id)
{
	k_event_post(&userInteractionEvent, SYSTEM_EVENT_TIMEOUT);
	k_timer_stop(&systemSleepTimer);
}

static Screen_Type system_screen_state = SCREEN_HOME;
static struct k_timer test_timer;

static void testTimerCallback(struct k_timer* timer_id)
{
	// temp_action();
	system_screen_state = (system_screen_state + 1) % SCREEN_TYPE_COUNT;
	display_switch_screen(system_screen_state);
	printf("test timer\n");
}

int main(void)
{	
	/* Make sure zephyr initilization worked properly and devices are ready to go */
	bool systemReady = zephyrInitSanityCheck();
	if (!systemReady) return 0;

	/* Initialize system componenets */
	systemReady = systemInit();
	if (!systemReady) return 0;

	/* Wait on user interaction and handle accordingly */
	uint32_t triggeredEvent = 0;
	int batteryVoltage;
	char voltageBuffer[16];
	uint32_t timeTillNext;

	// Start the system in the awoken state and start sleep timer
	k_timer_init(&systemSleepTimer, systemSleepCallback, NULL);
	k_timer_start(&systemSleepTimer, K_SECONDS(5), K_SECONDS(5));

	for(;;)
	{
		// Wait on system events or if we are awake, keep updating display
		timeTillNext = lv_timer_handler();
		triggeredEvent = ((systemAwake) ? k_event_wait(&userInteractionEvent, SYSTEM_EVENT_MAIN_MASK, true, K_MSEC(timeTillNext)) : k_event_wait(&userInteractionEvent, SYSTEM_EVENT_MAIN_MASK, true, K_FOREVER));
		
		// If we are asleep, don't wake up on single taps, just ignore
		if (!systemAwake && (triggeredEvent & SYSTEM_EVENT_SINGLE_TAP))
		{
			continue;
		}

		if (triggeredEvent & SYSTEM_EVENT_DOUBLE_TAP || triggeredEvent & SYSTEM_EVENT_SINGLE_TAP)
		{
			// Update display based on the user interaction event, if this is not the wake up tap
			if (systemAwake) 
			{
				(triggeredEvent & SYSTEM_EVENT_SINGLE_TAP) ? display_handle_tap(TAP_SINGLE) : display_handle_tap(TAP_DOUBLE);
			}

			// User interaction event (double tap)
			// Create the sleep timer and update the system status
			if (!systemAwake && (triggeredEvent & SYSTEM_EVENT_DOUBLE_TAP))
			{
				systemAwake = true;
				display_wake();
			}

			// Always start/reset timer on user interaction
			k_timer_start(&systemSleepTimer, K_SECONDS(5), K_SECONDS(5));

		}
		
		if (triggeredEvent & SYSTEM_EVENT_TIME_UPDATE || triggeredEvent & SYSTEM_EVENT_NEW_NOTIFICATION)
		{
			// Minute update or new notification event, display should be refreshed to indicate
			if (systemAwake)
			{
				display_switch_screen(SCREEN_ACTIVE);
			}
		}
		
		if (triggeredEvent & SYSTEM_EVENT_TIMEOUT)
		{
			// Sleep timer expired, need to go into sleep
			systemAwake = false;
			display_sleep();
		}

	}

	return 0;
}
