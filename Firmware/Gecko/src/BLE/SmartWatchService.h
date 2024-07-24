#include <zephyr/types.h>

#ifndef __SMARTWATCHSERVICE__
#define __SMARTWATCHSERVICE__
/*
    Create UUIDs for the SmartWatchService
*/
// Overall SmartWatchService(SWS) UUID
#define BT_UUID_SWS_VAL     BT_UUID_128_ENCODE(0x1f96e240, 0x7e6e, 0x452c, 0xab50, 0x3e0feb504976)

// SmartWatchService(SWS) Notification Characteristic(NC) UUID 
#define BT_UUID_SWS_NC_VAL  BT_UUID_128_ENCODE(0x1f96e241, 0x7e6e, 0x452c, 0xab50, 0x3e0feb504976)

// SmartWatchService(SWS) Battery Level Characteristic(BLC) UUID
#define BT_UUID_SWS_BLC_VAL BT_UUID_128_ENCODE(0x1f96e242, 0x7e6e, 0x452c, 0xab50, 0x3e0feb504976)

// SmartWatchService(SWS) Time Characteristic(TC) UUID
#define BT_UUID_SWS_TC_VAL  BT_UUID_128_ENCODE(0x1f96e243, 0x7e6e, 0x452c, 0xab50, 0x3e0feb504976)

// Declare the UUIDS from the more readable format above
#define BT_UUID_SWS         BT_UUID_DECLARE_128(BT_UUID_SWS_VAL) 
#define BT_UUID_SWS_NC      BT_UUID_DECLARE_128(BT_UUID_SWS_NC_VAL)
#define BT_UUID_SWS_BLC     BT_UUID_DECLARE_128(BT_UUID_SWS_BLC_VAL)
#define BT_UUID_SWS_TC      BT_UUID_DECLARE_128(BT_UUID_SWS_TC_VAL)

/*
    Create callbacks for the SmartWatchService operations
*/
// Callback type for when the battery level is requested
typedef float (*battery_level_cb_t)(void);

// Callback type for when a new notification write is issued
typedef void (*notification_cb_t)(char* notification, int len);

// Callback type for when a new time object is issued
typedef void (*time_update_cb_t)(char* time, int len);

struct SmartWatchService_cb {
    battery_level_cb_t      battery_level_cb;
    notification_cb_t       notification_cb;
    time_update_cb_t        time_update_cb;
};

// Register application callback functions with the SmartWatchService
int SmartWatchService_init(struct SmartWatchService_cb* callbacks);

#endif