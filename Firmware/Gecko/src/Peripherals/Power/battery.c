#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "system.h"

#if !__DEVELOPMENT_BOARD__
    #include <zephyr/drivers/adc.h>
    static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));
    static int16_t buf;
    static struct adc_sequence sequence = {
        .buffer = &buf,
        .buffer_size = sizeof(buf), // buffer size in bytes, not number of samples 
        .calibrate = true, // Not neccesiary but probably a good idea
    };
#endif 

#include "battery.h"

LOG_MODULE_REGISTER(GeckoBattery, CONFIG_LOG_DEFAULT_LEVEL);

static int val_mv;
static int16_t buf;

int batteryMonitorInit(void)
{
    int error = 0;
    printf("Init Battery Monitoring...");
#if !__DEVELOPMENT_BOARD__   
    if (!adc_is_ready_dt(&adc_channel)) {
		printf(ANSI_COLOR_RED "ERR: ADC device is not ready" ANSI_COLOR_RESET "\n");
        return -1;
	}

	error = adc_channel_setup_dt(&adc_channel);
	if (error < 0) {
		printf(ANSI_COLOR_RED "ERR: adc_channel_setup_dt" ANSI_COLOR_RESET "\n");
        return error;
	}

	error = adc_sequence_init_dt(&adc_channel, &sequence);
	if (error < 0) {
		printf(ANSI_COLOR_RED "ERR: adc_sequence_init_dt" ANSI_COLOR_RESET "\n");
        return error;
	}

    printf(ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "\n");
#else
    // If on development board there is nothing to init
    printf(ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "\n");
#endif 
    return error;
}

int batteryReadVoltage(void)
{
#if !__DEVELOPMENT_BOARD__   
    int err = adc_read(adc_channel.dev, &sequence);
    if (err < 0) {
        LOG_ERR("Could not read ADC (%d)", err);
    }

    val_mv = (int) buf;

    err = adc_raw_to_millivolts_dt(&adc_channel, &val_mv);
    if (err < 0) {
        LOG_WRN(" (value in mV not available)\n");
    } else {
        LOG_INF(" = %d mV", val_mv);
    }
#else
    val_mv = 1230; // Dummy value for now
#endif
    return 2*val_mv;
}