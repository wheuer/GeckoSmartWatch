#include "system.h"
#include "zephyr/drivers/pwm.h"

int buzzerInit(void)
{
    int error;
    printf("Init Buzzer...");
    error = pwm_set(pwm_dev, BUZZER_PWM_CHANNEL, BUZZER_PWM_PERIOD, 0, PWM_POLARITY_NORMAL);
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