#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include "system.h"
#include "bma400.h"
#include "bma400_defs.h"
#include "common.h"

K_THREAD_STACK_DEFINE(tapsStackArea, 1024); // 1KiB stack for now, could likely be much smaller if needed
struct k_thread tapsThreadData;
struct k_event tapsEvent;

static struct bma400_dev bma;
static struct bma400_device_conf bma_device_conf;
static struct bma400_sensor_conf bma_conf;
static struct bma400_int_enable int_en[3];
static struct gpio_callback bma400_INT1_callback_t;

struct k_timer screenTimer;

float lsb_to_ms2(int16_t accel_data, uint8_t g_range, uint8_t bit_width)
{
    float accel_ms2;
    int16_t half_scale;

    half_scale = 1 << (bit_width - 1);
    accel_ms2 = (GRAVITY_EARTH * accel_data * g_range) / half_scale;

    return accel_ms2;
}

static void bma400_INT1_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    // Due to using latching interrupts on the BMA400 and GPIO_INT_LEVEL_HIGH interrupts on the pin
    // The interrupt will continously be triggered until cleared in the BMA400
    // Unfortunately we need to read the status register to do so and that (using the APIs) cannot be done 
    //      in the interrupt handler
    // So, turn off GPIO interrupt until the taps task can clear the status register and re-enable the GPIO interrupt  
    
    // Disable GPIO interrupt to avoid deadlocking inside this ISR
    gpio_pin_interrupt_configure(gpio0_dev, BMA_INT1_PIN, GPIO_INT_DISABLE);

    // Signal taps thread to handle tap interrupt, can't do it inside interrupt context
    k_event_post(&tapsEvent, 0x01);
}

static int configBMAForTaps(void)
{
    int8_t result; 

    /* Select the type of configuration to be modified */
    bma_conf.type = BMA400_TAP_INT;

    /* Get the accelerometer configurations which are set in the sensor */
    result = bma400_get_sensor_conf(&bma_conf, 1, &bma);

    /* Modify the desired configurations as per macros
     * available in bma400_defs.h file */
    bma_conf.param.tap.int_chan = BMA400_INT_CHANNEL_1;
    bma_conf.param.tap.axes_sel = BMA400_TAP_X_AXIS_EN | BMA400_TAP_Y_AXIS_EN | BMA400_TAP_Z_AXIS_EN;
    bma_conf.param.tap.sensitivity = BMA400_TAP_SENSITIVITY_0;
    bma_conf.param.tap.tics_th = BMA400_TICS_TH_6_DATA_SAMPLES;
    bma_conf.param.tap.quiet = BMA400_QUIET_60_DATA_SAMPLES;
    bma_conf.param.tap.quiet_dt = BMA400_QUIET_DT_4_DATA_SAMPLES;
    result += bma400_set_sensor_conf(&bma_conf, 1, &bma);

    result += bma400_set_power_mode(BMA400_MODE_NORMAL, &bma);

    bma_device_conf.type = BMA400_INT_PIN_CONF;
    bma_device_conf.param.int_conf.int_chan = BMA400_INT_CHANNEL_1;
    bma_device_conf.param.int_conf.pin_conf = BMA400_INT_PUSH_PULL_ACTIVE_1;
    result += bma400_set_device_conf(&bma_device_conf, 1, &bma);

    // Enable single and double tap interrupts
    // Also enable interrupt latching, this allows us to use GPIO_INT_LEVEL_HIGH interrupts to save power
    int_en[0].type = BMA400_DOUBLE_TAP_INT_EN;
    int_en[0].conf = BMA400_ENABLE;
    int_en[1].type = BMA400_SINGLE_TAP_INT_EN;
    int_en[1].conf = BMA400_ENABLE;
    int_en[2].type = BMA400_LATCH_INT_EN;
    int_en[2].conf = BMA400_ENABLE;        
    result += bma400_enable_interrupt(int_en, 3, &bma);

    return result;
}

void tapsThread(void* p1, void* p2, void* p3)
{
    // Wait on tap interrupts, then handle them
    uint32_t triggeredEvent;
    for (;;)
    {
        triggeredEvent = k_event_wait(&tapsEvent, 0x01, true, K_FOREVER);
        if (triggeredEvent & 0x01)
        {
            // See what triggered the interrupt, either a single tap or a double tap
            // A double tap will always follow a single tap so need to timeout before asserting the tap event
            printf("Tap interrupt: ");
            uint16_t mInterruptStatus;
            bma400_get_interrupt_status(&mInterruptStatus, &bma);
            gpio_pin_interrupt_configure(gpio0_dev, BMA_INT1_PIN, GPIO_INT_LEVEL_HIGH);
            if (mInterruptStatus & BMA400_ASSERTED_S_TAP_INT)
            {
                // Got a single tap, need to wait to see if a double is coming
                // The timeout is the minimum delay until a single tap is registered
                k_event_clear(&tapsEvent, 0x01);
                triggeredEvent = k_event_wait(&tapsEvent, 0x01, true, K_MSEC(500));
                if (triggeredEvent)
                {
                    // Got another event make sure it's a double
                    bma400_get_interrupt_status(&mInterruptStatus, &bma);
                    gpio_pin_interrupt_configure(gpio0_dev, BMA_INT1_PIN, GPIO_INT_LEVEL_HIGH);
                    if (mInterruptStatus & BMA400_ASSERTED_D_TAP_INT)
                    {
                        printf("Double Tap\n");
                        k_event_post(&userInteractionEvent, SYSTEM_EVENT_DOUBLE_TAP);
                    }
                    else
                    {
                        printf("Single Tap\n");
                        k_event_post(&userInteractionEvent, SYSTEM_EVENT_SINGLE_TAP);
                    }
                }
                else
                {
                    printf("Single Tap\n");
                    k_event_post(&userInteractionEvent, SYSTEM_EVENT_SINGLE_TAP);
                }
            }
        }
    }
}

int bma400Init(void)
{
    int error;     
    printf("Init BMA400...");

#if __DEVELOPMENT_BOARD__ 
    error = bma400_interface_init(&bma, BMA400_I2C_INTF);
#else
    error = bma400_interface_init(&bma, BMA400_SPI_INTF);
#endif
    if (error)
    {
        printf(ANSI_COLOR_RED "ERR: bma400_interface_init" ANSI_COLOR_RESET "\n");
        return error;
    }

    error = bma400_soft_reset(&bma);
    if (error)
    {
        printf(ANSI_COLOR_RED "ERR: bma400_soft_reset" ANSI_COLOR_RESET "\n");
        return error;
    }

    error = bma400_init(&bma);
    if (error)
    {
        printf(ANSI_COLOR_RED "ERR: bma400_init" ANSI_COLOR_RESET "\n");
        return error;
    } 

    error = configBMAForTaps();
    if (error)
    {
        printf(ANSI_COLOR_RED "ERR: configBMAForTaps" ANSI_COLOR_RESET "\n");
        return error;
    }    

    // Set up GPIO interrupt(s) to handle incoming BMA400 interrupts
    gpio_pin_configure(gpio0_dev, BMA_INT1_PIN, GPIO_INPUT);
    gpio_pin_interrupt_configure(gpio0_dev, BMA_INT1_PIN, GPIO_INT_LEVEL_HIGH); // Use GPIO_INT_LEVEL_HIGH instead of GPIO_INT_EDGE_RISING to save ~40 uA    
    gpio_init_callback(&bma400_INT1_callback_t, bma400_INT1_callback, BIT(BMA_INT1_PIN));    
    gpio_add_callback(gpio0_dev, &bma400_INT1_callback_t);   

    // Initialize thread event(s) and launch
    k_event_init(&tapsEvent);
    k_thread_create(&tapsThreadData, tapsStackArea, K_THREAD_STACK_SIZEOF(tapsStackArea), 
                        tapsThread, NULL, NULL, NULL, 
                        TAPS_THREAD_PRIORITY, 0, K_NO_WAIT);

    printf(ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "\n");
    return error;
}
