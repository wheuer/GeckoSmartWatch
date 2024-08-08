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
static struct bma400_device_conf bma_device_conf[2];
static struct bma400_sensor_conf bma_conf[2];
static struct bma400_int_enable int_en[4];
static struct gpio_callback bma400_INT1_callback_t;
static struct gpio_callback bma400_INT2_callback_t;

struct k_timer screenTimer;

float lsb_to_ms2(int16_t accel_data, uint8_t g_range, uint8_t bit_width)
{
    float accel_ms2;
    int16_t half_scale;

    half_scale = 1 << (bit_width - 1);
    accel_ms2 = (GRAVITY_EARTH * accel_data * g_range) / half_scale;

    return accel_ms2;
}

static volatile bool int1Triggered = false;
static volatile bool int2Triggered = false;

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

    int1Triggered = true;
}

static int configBMAForTaps(void)
{
    int8_t result; 

    /* Select the type of configuration to be modified */
    bma_conf[0].type = BMA400_TAP_INT;
    bma_conf[1].type = BMA400_ORIENT_CHANGE_INT;

    /* Get the accelerometer configurations which are set in the sensor */
    result = bma400_get_sensor_conf(bma_conf, 2, &bma);

    /* Modify the desired configurations as per macros
     * available in bma400_defs.h file */
    // Taps
    bma_conf[0].param.tap.int_chan = BMA400_INT_CHANNEL_1;
    bma_conf[0].param.tap.axes_sel = BMA400_TAP_X_AXIS_EN | BMA400_TAP_Y_AXIS_EN | BMA400_TAP_Z_AXIS_EN;
    bma_conf[0].param.tap.sensitivity = BMA400_TAP_SENSITIVITY_0;
    bma_conf[0].param.tap.tics_th = BMA400_TICS_TH_6_DATA_SAMPLES;
    bma_conf[0].param.tap.quiet = BMA400_QUIET_60_DATA_SAMPLES;
    bma_conf[0].param.tap.quiet_dt = BMA400_QUIET_DT_4_DATA_SAMPLES;

    // Orientation Change (acc_filt2 is fixed to 100 Hz so no need to init)
    bma_conf[1].param.orient.int_chan = BMA400_INT_CHANNEL_1;
    bma_conf[1].param.orient.axes_sel = BMA400_AXIS_Z_EN;
    bma_conf[1].param.orient.data_src = BMA400_DATA_SRC_ACC_FILT2;
    bma_conf[1].param.orient.ref_update = BMA400_ORIENT_REFU_ACC_FILT_2;
    bma_conf[1].param.orient.orient_thres = 125;
    bma_conf[1].param.orient.stability_thres = 10;
    bma_conf[1].param.orient.orient_int_dur = 10;

    result += bma400_set_sensor_conf(bma_conf, 2, &bma);

    result += bma400_set_power_mode(BMA400_MODE_NORMAL, &bma);

    // Configure Interrupt Types
    bma_device_conf[0].type = BMA400_INT_PIN_CONF;
    bma_device_conf[0].param.int_conf.int_chan = BMA400_INT_CHANNEL_1;
    bma_device_conf[0].param.int_conf.pin_conf = BMA400_INT_PUSH_PULL_ACTIVE_1;
    result += bma400_set_device_conf(bma_device_conf, 1, &bma);

    // Enable single, double tap, and orientation change interrupts
    // Also enable interrupt latching, this allows us to use GPIO_INT_LEVEL_HIGH interrupts to save power
    int_en[0].type = BMA400_DOUBLE_TAP_INT_EN;
    int_en[0].conf = BMA400_ENABLE;
    int_en[1].type = BMA400_SINGLE_TAP_INT_EN;
    int_en[1].conf = BMA400_ENABLE;
    int_en[2].type = BMA400_LATCH_INT_EN;
    int_en[2].conf = BMA400_ENABLE; 
    int_en[3].type = BMA400_ORIENT_CHANGE_INT_EN;
    int_en[3].conf = BMA400_ENABLE;        
    result += bma400_enable_interrupt(int_en, 4, &bma);

    return result;
}

// static int configBMAForTaps(void)
// {
//     // int8_t rslt = 0;

//     // struct bma400_orient_int_conf test_orient_conf = { 0 };

//     // test_orient_conf.axes_sel = BMA400_AXIS_XYZ_EN;
//     // test_orient_conf.data_src = BMA400_DATA_SRC_ACC_FILT1;
//     // test_orient_conf.int_chan = BMA400_INT_CHANNEL_2;
//     // test_orient_conf.orient_int_dur = 7; /* 10ms/LSB */
//     // test_orient_conf.orient_thres = 125; /* 1 LSB = 8mg */
//     // test_orient_conf.ref_update = BMA400_ORIENT_REFU_ACC_FILT_2;
//     // test_orient_conf.stability_thres = 10; /* 1 LSB = 8mg */

//     // struct bma400_sensor_conf accel_settin[2];
//     // struct bma400_sensor_data accel;
//     // struct bma400_int_enable int_en[2];

//     // uint16_t int_status = 0;
//     // uint8_t orient_cnter = 0;

//     // accel_settin[0].type = BMA400_ORIENT_CHANGE_INT;
//     // accel_settin[1].type = BMA400_ACCEL;

//     // rslt = bma400_get_sensor_conf(accel_settin, 2, &bma);

//     // accel_settin[0].param.orient.axes_sel = test_orient_conf.axes_sel;
//     // accel_settin[0].param.orient.data_src = test_orient_conf.data_src;
//     // accel_settin[0].param.orient.int_chan = test_orient_conf.int_chan;
//     // accel_settin[0].param.orient.orient_int_dur = test_orient_conf.orient_int_dur;
//     // accel_settin[0].param.orient.orient_thres = test_orient_conf.orient_thres;
//     // accel_settin[0].param.orient.ref_update = test_orient_conf.ref_update;
//     // accel_settin[0].param.orient.stability_thres = test_orient_conf.stability_thres;
//     // accel_settin[0].param.orient.orient_ref_x = test_orient_conf.orient_ref_x;
//     // accel_settin[0].param.orient.orient_ref_y = test_orient_conf.orient_ref_y;
//     // accel_settin[0].param.orient.orient_ref_z = test_orient_conf.orient_ref_z;

//     // accel_settin[1].param.accel.odr = BMA400_ODR_100HZ;
//     // accel_settin[1].param.accel.range = BMA400_RANGE_2G;
//     // accel_settin[1].param.accel.data_src = BMA400_DATA_SRC_ACCEL_FILT_1;

//     // rslt = bma400_set_sensor_conf(accel_settin, 2, &bma);

//     // rslt = bma400_set_power_mode(BMA400_MODE_NORMAL, &bma);

//     // int_en[0].type = BMA400_ORIENT_CHANGE_INT_EN;
//     // int_en[0].conf = BMA400_ENABLE;

//     // int_en[1].type = BMA400_LATCH_INT_EN;
//     // int_en[1].conf = BMA400_ENABLE;

//     // rslt = bma400_enable_interrupt(int_en, 2, &bma);

//     int8_t result; 
//     uint16_t int_status = 0;

//     /* Select the type of configuration to be modified */
//     bma_conf[0].type = BMA400_TAP_INT;
//     bma_conf[1].type = BMA400_ORIENT_CHANGE_INT;

//     /* Get the accelerometer configurations which are set in the sensor */
//     result = bma400_get_sensor_conf(bma_conf, 2, &bma);

//     /* Modify the desired configurations as per macros
//      * available in bma400_defs.h file */
//     // Taps
//     bma_conf[0].param.tap.int_chan = BMA400_INT_CHANNEL_1;
//     bma_conf[0].param.tap.axes_sel = BMA400_TAP_X_AXIS_EN | BMA400_TAP_Y_AXIS_EN | BMA400_TAP_Z_AXIS_EN;
//     bma_conf[0].param.tap.sensitivity = BMA400_TAP_SENSITIVITY_0;
//     bma_conf[0].param.tap.tics_th = BMA400_TICS_TH_6_DATA_SAMPLES;
//     bma_conf[0].param.tap.quiet = BMA400_QUIET_60_DATA_SAMPLES;
//     bma_conf[0].param.tap.quiet_dt = BMA400_QUIET_DT_4_DATA_SAMPLES;

//     // Orientation Change (acc_filt2 is fixed to 100 Hz so no need to init)
//     bma_conf[1].param.orient.int_chan = BMA400_INT_CHANNEL_1;
//     bma_conf[1].param.orient.axes_sel = BMA400_AXIS_Z_EN;
//     bma_conf[1].param.orient.data_src = BMA400_DATA_SRC_ACC_FILT2;
//     bma_conf[1].param.orient.ref_update = BMA400_ORIENT_REFU_ACC_FILT_2;
//     bma_conf[1].param.orient.orient_thres = 125;
//     bma_conf[1].param.orient.stability_thres = 10;
//     bma_conf[1].param.orient.orient_int_dur = 10;

//     result += bma400_set_sensor_conf(bma_conf, 2, &bma);

//     result += bma400_set_power_mode(BMA400_MODE_NORMAL, &bma);

//     // Configure Interrupt Types
//     bma_device_conf[0].type = BMA400_INT_PIN_CONF;
//     bma_device_conf[0].param.int_conf.int_chan = BMA400_INT_CHANNEL_1;
//     bma_device_conf[0].param.int_conf.pin_conf = BMA400_INT_PUSH_PULL_ACTIVE_1;

//     bma_device_conf[1].type = BMA400_INT_PIN_CONF;
//     bma_device_conf[1].param.int_conf.int_chan = BMA400_INT_CHANNEL_2;
//     bma_device_conf[1].param.int_conf.pin_conf = BMA400_INT_PUSH_PULL_ACTIVE_1;

//     result += bma400_set_device_conf(bma_device_conf, 2, &bma);

//     // Enable single, double tap, and orientation change interrupts
//     // Also enable interrupt latching, this allows us to use GPIO_INT_LEVEL_HIGH interrupts to save power
//     int_en[0].type = BMA400_DOUBLE_TAP_INT_EN;
//     int_en[0].conf = BMA400_ENABLE;
//     int_en[1].type = BMA400_SINGLE_TAP_INT_EN;
//     int_en[1].conf = BMA400_ENABLE;
//     // int_en[2].type = BMA400_LATCH_INT_EN;
//     // int_en[2].conf = BMA400_ENABLE; 
//     int_en[2].type = BMA400_ORIENT_CHANGE_INT_EN;
//     int_en[2].conf = BMA400_ENABLE;        
//     result += bma400_enable_interrupt(int_en, 3, &bma);

//     // while (1)
//     // {
//     //     result = bma400_get_interrupt_status(&int_status, &bma);

//     //     if (int_status & BMA400_ASSERTED_ORIENT_CH)
//     //     {
//     //         printf("Orientation interrupt detected\n");
//     //     }
//     //     else if (int_status & BMA400_ASSERTED_S_TAP_INT)
//     //     {
//     //         printf("Single tap interrupt detected\n");
//     //     }
//     //     else if (int_status & BMA400_ASSERTED_D_TAP_INT)
//     //     {
//     //         printf("Double tap interrupt detected\n");
//     //     }

//     //     if (int2Triggered)
//     //     {
//     //         printf("Int 2\n");
//     //         int2Triggered = false;
//     //     }
//     //     if (int1Triggered)
//     //     {
//     //         printf("Int 1\n");
//     //         int1Triggered = false;
//     //     }
//     // }

//     return result;
// }

void tapsThread(void* p1, void* p2, void* p3)
{
    // Wait on tap interrupts, then handle them
    uint32_t triggeredEvent;
    uint16_t mInterruptStatus;
    for (;;)
    {
        triggeredEvent = k_event_wait(&tapsEvent, 0x01, true, K_FOREVER);
        if (triggeredEvent & 0x01)
        {
            // See what triggered the interrupt, either a single tap or a double tap
            // A double tap will always follow a single tap so need to timeout before asserting the tap event
            printf("BMA400 Interrupt: ");
            bma400_get_interrupt_status(&mInterruptStatus, &bma);
            
            // Handle triggered interrupts
            if (mInterruptStatus & BMA400_ASSERTED_S_TAP_INT)
            {
                printf("Single Tap\n");
            } 
            
            if (mInterruptStatus & BMA400_ASSERTED_D_TAP_INT)
            {
                printf("Double tap interrupt\n");
            }
            
            if (mInterruptStatus & BMA400_ASSERTED_ORIENT_CH)
            {
                printf("Orientation change\n");
            }

            // Re-enable BMA400 interrupt by re-initializing gpio interrupt  
            gpio_pin_interrupt_configure(gpio0_dev, BMA_INT1_PIN, GPIO_INT_LEVEL_HIGH);
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


/* 
TODO:
    Route all interrupts to single INT channel, so one callback
    Handle different events by just reading the interrupt status register
    Handle double tap by starting a timer when first single tap occurs, don't block other interrupts in the meantime
        -> Will need to handle in the callback, either acknowledge the single or confirm the double (reset for next tap)
*/


