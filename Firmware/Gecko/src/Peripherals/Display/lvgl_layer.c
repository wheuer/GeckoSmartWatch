#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>

#include "Peripherals/BMA400/taps.h" 
#include "clock.h"
#include "Peripherals/Power/battery.h"
#include "BLE/BLE.h"
#include "system.h"
#include "lvgl_layer.h"
#include "assets.h"

// Zephyr display object
const struct device* display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

// Various Screen Objects
static Home_Screen homeScreenObj;
static Notification_Summary_Screen notificationScreenObj;
static Notification_Detailed_Screen detailedNotificationScreenObj;
static Device_Status_Screen deviceScreenObj;
static Device_Brightness_Screen deviceBrightnessScreenObj;
static Charging_Screen chargingScreenObj;

static Screen_Type active_screen;
static bool screen_initialized = false;
static uint8_t active_brightness = DISPLAY_START_BRIGHTNESS; // Display API does not have a get function, this brightness is what is actually set

static char notification_roller_buffer[MAX_LENGTH_APP_NAME * (MAX_NOTIFICATION_COUNT + 1)]; // Extra notification for "Go Back" option

static void init_display_objects(void)
{
    /* 
        Home Screen
    */
    homeScreenObj.lvgl_object = lv_obj_create(NULL);

    // Create notification indicator objects (two circles to create circluar outline on display) but keep them hidden
    homeScreenObj.notification_marker_outer = lv_obj_create(homeScreenObj.lvgl_object);
    homeScreenObj.notification_marker_inner = lv_obj_create(homeScreenObj.lvgl_object);
    lv_obj_set_scrollbar_mode(homeScreenObj.notification_marker_outer, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollbar_mode(homeScreenObj.notification_marker_inner, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(homeScreenObj.notification_marker_outer, 240, 240);
    lv_obj_set_size(homeScreenObj.notification_marker_inner, 0.94 * 240, 0.94 * 240);
    lv_obj_set_style_bg_color(homeScreenObj.notification_marker_outer, lv_color_hex(0x4287f5), LV_PART_MAIN);
    lv_obj_set_style_bg_color(homeScreenObj.notification_marker_inner, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_radius(homeScreenObj.notification_marker_outer, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_radius(homeScreenObj.notification_marker_inner, LV_RADIUS_CIRCLE, 0);
    lv_obj_center(homeScreenObj.notification_marker_outer);
    lv_obj_center(homeScreenObj.notification_marker_inner);
    lv_obj_add_flag(homeScreenObj.notification_marker_outer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(homeScreenObj.notification_marker_inner, LV_OBJ_FLAG_HIDDEN);

    // Home screen saver image
    LV_IMG_DECLARE(img_sloth_desc);
    homeScreenObj.screen_saver = lv_img_create(homeScreenObj.lvgl_object);
    lv_img_set_src(homeScreenObj.screen_saver, &img_sloth_desc);
    lv_obj_align(homeScreenObj.screen_saver, LV_ALIGN_CENTER, 0, -25);

    // Current time label
    homeScreenObj.time_label = lv_label_create(homeScreenObj.lvgl_object);
    lv_obj_set_style_text_font(homeScreenObj.time_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_label_set_text(homeScreenObj.time_label, "??:?? PM");
    lv_obj_set_style_text_align(homeScreenObj.time_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(homeScreenObj.time_label, LV_ALIGN_CENTER, 0, 50);

    // Battery percentage label
    homeScreenObj.battery_percent_label = lv_label_create(homeScreenObj.lvgl_object);
    lv_label_set_text(homeScreenObj.battery_percent_label, "??%");
    lv_obj_set_style_text_align(homeScreenObj.battery_percent_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(homeScreenObj.battery_percent_label, LV_ALIGN_CENTER, 0, 85);

    /* 
        Notification Screen
    */
    notificationScreenObj.roller_is_active = false;
    notificationScreenObj.lvgl_object = lv_obj_create(NULL);

    // Create outer circle marker to show if we are "inside" the roller or not
    notificationScreenObj.roller_active_marker_outer = lv_obj_create(notificationScreenObj.lvgl_object);
    notificationScreenObj.roller_active_marker_inner = lv_obj_create(notificationScreenObj.lvgl_object);
    lv_obj_set_scrollbar_mode(notificationScreenObj.roller_active_marker_outer, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollbar_mode(notificationScreenObj.roller_active_marker_inner, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(notificationScreenObj.roller_active_marker_outer, 240, 240);
    lv_obj_set_size(notificationScreenObj.roller_active_marker_inner, 0.94 * 240, 0.94 * 240);
    lv_obj_set_style_bg_color(notificationScreenObj.roller_active_marker_outer, lv_color_hex(0x4287f5), LV_PART_MAIN);
    lv_obj_set_style_bg_color(notificationScreenObj.roller_active_marker_inner, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_radius(notificationScreenObj.roller_active_marker_outer, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_radius(notificationScreenObj.roller_active_marker_inner, LV_RADIUS_CIRCLE, 0);
    lv_obj_center(notificationScreenObj.roller_active_marker_outer);
    lv_obj_center(notificationScreenObj.roller_active_marker_inner);
    lv_obj_add_flag(notificationScreenObj.roller_active_marker_outer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(notificationScreenObj.roller_active_marker_inner, LV_OBJ_FLAG_HIDDEN);

    // Create the notification roller
    notificationScreenObj.roller = lv_roller_create(notificationScreenObj.lvgl_object);
    lv_obj_set_style_text_line_space(notificationScreenObj.lvgl_object, 23, LV_PART_MAIN);
    lv_roller_set_options(notificationScreenObj.roller,
        "???\n",
        LV_ROLLER_MODE_INFINITE);
    lv_roller_set_visible_row_count(notificationScreenObj.roller, 5);
    lv_obj_center(notificationScreenObj.roller);    

    /* 
        Detailed Notification Screen
    */
    detailedNotificationScreenObj.lvgl_object = lv_obj_create(NULL);
    detailedNotificationScreenObj.app_label = lv_label_create(detailedNotificationScreenObj.lvgl_object);
    lv_label_set_text(detailedNotificationScreenObj.app_label, "Example App Name");
    lv_obj_align(detailedNotificationScreenObj.app_label, LV_ALIGN_CENTER, 0, -75);

    detailedNotificationScreenObj.message_title_label = lv_label_create(detailedNotificationScreenObj.lvgl_object);
    lv_label_set_text(detailedNotificationScreenObj.message_title_label, "Example Msg Title | ?:?? PM");
    lv_obj_align(detailedNotificationScreenObj.message_title_label, LV_ALIGN_CENTER, 0, -45);

    detailedNotificationScreenObj.message_body_label = lv_label_create(detailedNotificationScreenObj.lvgl_object);
    lv_label_set_long_mode(detailedNotificationScreenObj.message_body_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(detailedNotificationScreenObj.message_body_label, 
        "This is an example message body text. It can be quite long with line wrapping enabled.");
    lv_obj_set_width(detailedNotificationScreenObj.message_body_label, 200); // 200 seems like an ok number, probably still room to tweak
    lv_obj_set_style_text_align(detailedNotificationScreenObj.message_body_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(detailedNotificationScreenObj.message_body_label, LV_ALIGN_CENTER, 0, 10);

    /* 
        Device Status Screen
    */
    deviceScreenObj.lvgl_object = lv_obj_create(NULL);
    deviceScreenObj.voltage_label = lv_label_create(deviceScreenObj.lvgl_object);
    lv_label_set_text(deviceScreenObj.voltage_label, "Battery Voltage: ?.?? V");
    lv_obj_align(deviceScreenObj.voltage_label, LV_ALIGN_CENTER, 0, -60);

    deviceScreenObj.battery_percent_label = lv_label_create(deviceScreenObj.lvgl_object);
    lv_label_set_text(deviceScreenObj.battery_percent_label, "Battery Percentage: ??%");
    lv_obj_align(deviceScreenObj.battery_percent_label, LV_ALIGN_CENTER, 0, -20);

    deviceScreenObj.bluetooth_label = lv_label_create(deviceScreenObj.lvgl_object);
    lv_label_set_text(deviceScreenObj.bluetooth_label, "Bluetooth Status: Unknown");
    lv_obj_align(deviceScreenObj.bluetooth_label, LV_ALIGN_CENTER, 0, 20);

    /* 
        Device Brightness Screen
    */
    deviceBrightnessScreenObj.lvgl_object = lv_obj_create(NULL);
    deviceBrightnessScreenObj.brightness_label = lv_label_create(deviceBrightnessScreenObj.lvgl_object);
    lv_label_set_text(deviceBrightnessScreenObj.brightness_label, "Brightness: ?? %");
    lv_obj_align(deviceBrightnessScreenObj.brightness_label, LV_ALIGN_CENTER, 0, 0);

    /* 
        Charging Screen
    */
    chargingScreenObj.lvgl_object = lv_obj_create(NULL);
    chargingScreenObj.charging_label = lv_label_create(chargingScreenObj.lvgl_object);
    lv_label_set_text(chargingScreenObj.charging_label, "Charging... ?? % / ?.?? V");
    lv_obj_align(chargingScreenObj.charging_label, LV_ALIGN_CENTER, 0, 0);
}

// Handle single and double tap, updating and moving screens if neccesary
void display_handle_tap(Tap_t tap)
{
    if (tap == TAP_SINGLE)
    {
        switch (active_screen)
        {
            case SCREEN_HOME:
                // Just move to notification screen
                display_switch_screen(SCREEN_NOTIFICATION_SUMMARY);
                break;
            case SCREEN_NOTIFICATION_SUMMARY:
                // Two different behaviours depending on if roller is active
                if (notificationScreenObj.roller_is_active)
                {    
                    // Increment the notification roller and update screen
                    lv_roller_set_selected(notificationScreenObj.roller, 
                                            (lv_roller_get_selected(notificationScreenObj.roller) + 1) % (notificationCount + 2), // Extra +2 for "go back" and "clear all"
                                            LV_ANIM_OFF);
                }
                else
                {
                    // Just move to device screen
                    display_switch_screen(SCREEN_DEVICE_STATUS);
                }
                break;
            case SCREEN_NOTIFICATION_DETAILED:
                // Single tap just moves back to normal notification screen without clearing the notification
                // As of now this will mean we are put outside the roller back at the first notification
                display_switch_screen(SCREEN_NOTIFICATION_SUMMARY);
                break;
            case SCREEN_DEVICE_STATUS:
                // Single tap just moves back to home screen
                display_switch_screen(SCREEN_HOME);
                break;
            case SCREEN_DEVICE_BRIGHTNESS:
                // Single tap updates the brightness
                active_brightness = (active_brightness + BRIGHTNESS_STEP) % 101;
                set_brightness(active_brightness, true);
                display_switch_screen(SCREEN_DEVICE_BRIGHTNESS);
                break;
            case SCREEN_CHARGING:
                break;
            default:
                break;
        }
    }
    else if (tap == TAP_DOUBLE)
    {
        switch (active_screen)
        {
            case SCREEN_HOME:
                // No double tap action
                break;
            case SCREEN_NOTIFICATION_SUMMARY:
                // Two different behaviours depending on if roller is active
                if (notificationScreenObj.roller_is_active)
                {    
                    // We are selecting a notificaiton, if "Go Back" was selected we move out of roller, otherwise go to detailed view
                    // "Go Back" is appended as the last notification so it's index is the number of notifications
                    if (lv_roller_get_selected(notificationScreenObj.roller) == notificationCount) 
                    {
                        // Go back was selected, set roller to inactive and remove active indicator
                        notificationScreenObj.roller_is_active = false;
                        lv_obj_add_flag(notificationScreenObj.roller_active_marker_inner, LV_OBJ_FLAG_HIDDEN);
                        lv_obj_add_flag(notificationScreenObj.roller_active_marker_outer, LV_OBJ_FLAG_HIDDEN);
                    }
                    else if (lv_roller_get_selected(notificationScreenObj.roller) == notificationCount + 1)
                    {
                        // Clear all was selected, remove all notifications and force refresh screen
                        clearNotifications();
                        display_switch_screen(SCREEN_ACTIVE);
                    }
                    else 
                    {
                        display_switch_screen(SCREEN_NOTIFICATION_DETAILED);
                    }
                }
                else
                {
                    // We want to move into roller but only if there are active notifications
                    if (notificationCount > 0)
                    {
                        // Make sure to show the outer indicator for being "inside" the roller
                        notificationScreenObj.roller_is_active = true;
                        lv_obj_clear_flag(notificationScreenObj.roller_active_marker_inner, LV_OBJ_FLAG_HIDDEN);
                        lv_obj_clear_flag(notificationScreenObj.roller_active_marker_outer, LV_OBJ_FLAG_HIDDEN);
                    }
                }
                break;
            case SCREEN_NOTIFICATION_DETAILED:
                // Now we want to clear the active notification and go back to the summary screen
                clearNotification(lv_roller_get_selected(notificationScreenObj.roller));
                display_switch_screen(SCREEN_NOTIFICATION_SUMMARY);
                break;
            case SCREEN_DEVICE_STATUS:
                // Double tap just moves to brightness screen
                display_switch_screen(SCREEN_DEVICE_BRIGHTNESS);
                break;
            case SCREEN_DEVICE_BRIGHTNESS:
                // Double tap just moves back to device screen
                display_switch_screen(SCREEN_DEVICE_STATUS);
                break;
            case SCREEN_CHARGING: 
                break;
            default:
                break;
        }
    }
    else
    {
        printf(ANSI_COLOR_RED "display_handle_tap(): Erroneous tap." ANSI_COLOR_RESET "\n");
    }
}

void display_wake(void)
{
    display_blanking_off(display_dev);
    display_switch_screen(SCREEN_HOME);
    set_brightness(active_brightness / 100.0, 0);
}

void display_sleep(void)
{
    set_brightness(0.0, 0);
    display_blanking_on(display_dev);
    notificationScreenObj.roller_is_active = false;
}

void temp_action(void)
{
    uint8_t newIndex = (lv_roller_get_selected(notificationScreenObj.roller) + 1) % (notificationCount + 1); // Extra +1 for "go back"
    lv_roller_set_selected(notificationScreenObj.roller, newIndex, LV_ANIM_OFF); // We don't have the update fps for smooth ANIM
}

Screen_Type get_active_screen(void)
{
    return active_screen;
}

// Use this function to swap to another screen or pass SCREEN_ACTIVE to update
void display_switch_screen(Screen_Type new_screen)
{
    char text_buffer[64];
    struct tm* current_time;
    uint8_t len;
    char* copy_index = notification_roller_buffer; 
    Notification* activeNotification;

    if (!screen_initialized) return;

    if (new_screen == SCREEN_ACTIVE) new_screen = active_screen;
    else active_screen = new_screen;

    switch(new_screen)
    {
        case SCREEN_HOME:
            /*
                Need to update the time, notification status, and battery percent/voltage
            */
            // Get and print the time in the standard US format and make sure to remove the leading zero if hour is 1-9
            getTime(&current_time);
            strftime(text_buffer, sizeof(text_buffer), "%I:%M %p", current_time);
            if (current_time->tm_hour % 12 >= 10 || current_time->tm_hour % 12 == 0) lv_label_set_text(homeScreenObj.time_label, text_buffer);
            else lv_label_set_text(homeScreenObj.time_label, &text_buffer[1]);

            // Notification status
            if (notificationCount)
            {
                lv_obj_clear_flag(homeScreenObj.notification_marker_outer, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(homeScreenObj.notification_marker_inner, LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(homeScreenObj.notification_marker_outer, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(homeScreenObj.notification_marker_inner, LV_OBJ_FLAG_HIDDEN);     
            }

            // Battery percent/voltage
            snprintf(text_buffer, 64, "?? %% / %.2f V", batteryReadVoltage() / 1000.0);
            lv_label_set_text(homeScreenObj.battery_percent_label, text_buffer);

            lv_scr_load(homeScreenObj.lvgl_object);
            break;
        case SCREEN_NOTIFICATION_SUMMARY:
            /*
                Need to get the active notification app names and create the lvgl roller string.
                If we have already created the string and we are just updating we need to recreate the string and set the current position
                Make sure to always include the "Go Back" and "Clear All" options 

                Note that the notification indices in the notification buffer aren't sorted by timestamp so 
                    neither will the display order be
            */
            if (notificationCount > 0)
            {
                for (int i = 0; i < notificationCount; i++)
                {
                    len = strlen(activeNotifications[i].appName);
                    strncpy(copy_index, activeNotifications[i].appName, len);
                    copy_index += len;
                    *copy_index++ = '\n';
                }
                strcpy(copy_index, "[Exit Roller]\n[Clear All]\n");

                lv_roller_set_options(notificationScreenObj.roller, notification_roller_buffer, LV_ROLLER_MODE_INFINITE);
            }
            else
            {
                lv_roller_set_options(notificationScreenObj.roller,
                    "No Current Messages\n",
                    LV_ROLLER_MODE_INFINITE);
            }
            // Always start "outside" the roller
            notificationScreenObj.roller_is_active = false;
            lv_obj_add_flag(notificationScreenObj.roller_active_marker_outer, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(notificationScreenObj.roller_active_marker_inner, LV_OBJ_FLAG_HIDDEN);
            lv_scr_load(notificationScreenObj.lvgl_object);
            break;
        case SCREEN_NOTIFICATION_DETAILED:
            if (notificationCount > 0)
            {
                // The active notification (that we are viewing) will be the current roller notification
                // Need to update the App Name, Notification Title, Timestamp, and Notification body 
                activeNotification = &activeNotifications[lv_roller_get_selected(notificationScreenObj.roller)];
                lv_label_set_text(detailedNotificationScreenObj.app_label, activeNotification->appName);
                lv_label_set_text(detailedNotificationScreenObj.message_body_label, activeNotification->text);

                len = (uint8_t) snprintf(text_buffer, sizeof(text_buffer), "%s | ", activeNotification->title);
                strftime(&text_buffer[len], sizeof(text_buffer) - len - 1, "%I:%M %p", gmtime(&(activeNotification->timestamp))); // -1 for null term as len doesn't include it
                lv_label_set_text(detailedNotificationScreenObj.message_title_label, text_buffer);
            }
            else
            {
                lv_label_set_text(detailedNotificationScreenObj.app_label, "NONE");
                lv_label_set_text(detailedNotificationScreenObj.message_body_label, "NONE");
                lv_label_set_text(detailedNotificationScreenObj.message_title_label, "NONE");
            }
            lv_scr_load(detailedNotificationScreenObj.lvgl_object);
            break;
        case SCREEN_DEVICE_STATUS:
            // Currently do not have hardware to support percentage, can implement after new hardware revision
            lv_label_set_text(deviceScreenObj.battery_percent_label, "Battery Percentage: ??");

            // Check if we are connected
            if (bluetoothConnected) lv_label_set_text(deviceScreenObj.bluetooth_label, "Bluetooth Status: Connected");
            else lv_label_set_text(deviceScreenObj.bluetooth_label, "Bluetooth Status: Disconnected");

            snprintf(text_buffer, sizeof(text_buffer), "Battery Voltage: %.2f V", batteryReadVoltage() / 1000.0);
            lv_label_set_text(deviceScreenObj.voltage_label, text_buffer);

            lv_scr_load(deviceScreenObj.lvgl_object);
            break;
        case SCREEN_DEVICE_BRIGHTNESS:
            snprintf(text_buffer, sizeof(text_buffer), "Brightness: %i %%", active_brightness);
            lv_label_set_text(deviceBrightnessScreenObj.brightness_label, text_buffer);

            lv_scr_load(deviceBrightnessScreenObj.lvgl_object);
            break;
        case SCREEN_CHARGING:
            snprintf(text_buffer, sizeof(text_buffer), "Charging... ?? %% / %.2f V", batteryReadVoltage() / 1000.0);
            lv_label_set_text(chargingScreenObj.charging_label, text_buffer);

            lv_scr_load(chargingScreenObj.lvgl_object);
            break;
        default:
            // Do nothing, should never hit this
            break;
    }
}

void set_brightness(float brightness, bool makeDefault)
{
    display_set_brightness(display_dev, brightness * 255);
    if (makeDefault) active_brightness = brightness;
}

int display_lvgl_init(void)
{
    printf("Init Display...");

    // The zephyr display driver + lvgl implementation should have already done the following:
    //  - Initialized the display, it will not have been cleared/set but it is ready to start drawing
    //  - Called lvgl_init()
    //  - Setup the lvgl ticks
    //  - Created the display object and assosiated render buffers
    // *The relevant options for memory size etc. are defined in the boards defconfig file

    display_set_brightness(display_dev, (active_brightness / 100.0) * 255);

    // Create all main objects (screens) and their children objects
    init_display_objects();

    // Start with the display on the home screen
    lv_scr_load(homeScreenObj.lvgl_object);
    active_screen = SCREEN_HOME;
    screen_initialized = true;
    display_switch_screen(SCREEN_ACTIVE);

    // Be optimistic and assume it all worked :)
    printf(ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "\n");
    return 0;
}








