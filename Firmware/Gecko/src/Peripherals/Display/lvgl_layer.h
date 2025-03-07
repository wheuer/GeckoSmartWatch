#ifndef __LVGL_LAYER__
#define __LVGL_LAYER__

// *****
//  The LVGL Version that is included in this build of zephyr is v8.4
// *****

#define NOTIFICATION_SCREEN_MAX             16
#define NOTIFICATION_SCREEN_MAX_DISPLAYED   4

// There are a few different basic screens that we will have, make them types for easy updating
typedef enum {
    SCREEN_ACTIVE = 0,
    SCREEN_HOME,
    SCREEN_NOTIFICATION_SUMMARY,
    SCREEN_NOTIFICATION_DETAILED,
    SCREEN_DEVICE_STATUS,
    SCREEN_DEVICE_BRIGHTNESS,
    SCREEN_CHARGING,
    SCREEN_TYPE_COUNT // number of screens, not an actual screen
} Screen_Type;

typedef struct {
    lv_obj_t* lvgl_object;
    lv_obj_t* notification_marker_outer;
    lv_obj_t* notification_marker_inner;
    lv_obj_t* screen_saver;
    lv_obj_t* time_label;
    lv_obj_t* battery_percent_label;
} Home_Screen;

typedef struct {
    lv_obj_t* lvgl_object;
    lv_obj_t* roller;
    lv_obj_t* roller_active_marker_outer;
    lv_obj_t* roller_active_marker_inner;
    bool roller_is_active;
} Notification_Summary_Screen;

typedef struct {
    lv_obj_t* lvgl_object;
    lv_obj_t* app_label;
    lv_obj_t* message_title_label;
    lv_obj_t* message_body_label;
} Notification_Detailed_Screen;

typedef struct {
    lv_obj_t* lvgl_object;
    lv_obj_t* voltage_label;
    lv_obj_t* battery_percent_label;
    lv_obj_t* bluetooth_label;
} Device_Status_Screen;

typedef struct {
    lv_obj_t* lvgl_object;
    lv_obj_t* brightness_label;
} Device_Brightness_Screen;

typedef struct {
    lv_obj_t* lvgl_object;
    lv_obj_t* charging_label;
} Charging_Screen;

extern lv_disp_t* activeDisplay;

int display_lvgl_init(void);
void display_switch_screen(Screen_Type new_screen);
void set_brightness(float brightness, bool makeDefault);
Screen_Type get_active_screen(void);
void display_wake(void);
void display_sleep(void);
void display_handle_tap(Tap_t tap);

void temp_action(void);

#endif // __LVGL_LAYER__