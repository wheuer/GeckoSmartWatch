#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "SmartWatchService.h"

static struct SmartWatchService_cb  app_SmartWatchService_cbs;
static float battery_level = 0.0;

static ssize_t battery_level_read_callback(
    struct bt_conn* conn,
    const struct bt_gatt_attr* attr,
    void* buf,
    uint16_t len,
    uint16_t offset
){
    // Get the value that was 
    const float* value = attr->user_data;

    if (app_SmartWatchService_cbs.battery_level_cb) {
        // Call the apps battery level callback to get the apps battery level
        battery_level = app_SmartWatchService_cbs.battery_level_cb();

        // Send back the battery level to the central device
        return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
    }

    return 0;
}

static ssize_t notification_write_callback(
    struct bt_conn* conn,
    const struct bt_gatt_attr* attr,
    const void* buf,
    uint16_t len,
    uint16_t offset,
    uint8_t flags
){
    // The buf input should contain what the write was
    char* val = (char*) buf;

    if (app_SmartWatchService_cbs.notification_cb){
        app_SmartWatchService_cbs.notification_cb(val, len);
    }

    return len;
}

static ssize_t update_time_callback(
    struct bt_conn* conn,
    const struct bt_gatt_attr* attr,
    const void* buf,
    uint16_t len,
    uint16_t offset,
    uint8_t flags
){
    // The buf input should contain the time string
    char* val = (char*) buf;

    if (app_SmartWatchService_cbs.time_update_cb){
        app_SmartWatchService_cbs.time_update_cb(val, len);
    }

    return len;
}

// Declare that we are using the SmartWatchSerice
BT_GATT_SERVICE_DEFINE(
    my_SmartWatchSerice,                  // Name of GATT service instance
    BT_GATT_PRIMARY_SERVICE(BT_UUID_SWS), // UUID of the SmartWatchService
    BT_GATT_CHARACTERISTIC(
        BT_UUID_SWS_BLC,            // Battery level characterisitc UUID
        BT_GATT_CHRC_READ,          // Characteristic attribute properties, just read for battery level
        BT_GATT_PERM_READ,          // Chatacteristic permissions, just read for battery level
        battery_level_read_callback,// Callback for receiving a read request
        NULL,                       // No callback for write requests
        &battery_level              // User_data attr; Actual battery characteristic data attribute
    ),
    BT_GATT_CHARACTERISTIC(
        BT_UUID_SWS_NC,             // Notification Characteristic UUID
        BT_GATT_CHRC_WRITE,         // Characteristic attribute properties, just write for notifications
        BT_GATT_PERM_WRITE,         // Characteristic permissions, just write for notifications
        NULL,                       // No read callbacks
        notification_write_callback,// Callback for receiving a write request
        NULL                        // No user_data for notification
    ),
    BT_GATT_CHARACTERISTIC(
        BT_UUID_SWS_TC,         // Time update Characteristic UUID
        BT_GATT_CHRC_WRITE,     // Characteristic attribute properties, just write for updating system time
        BT_GATT_PERM_WRITE,     // Characteristic permissions, just write for notifications
        NULL,                   // No read callbacks
        update_time_callback,   // Callback for receiving a write request
        NULL                    // No user_data for notification
    )
);

// Implement the SmartWatchService initilization function
int SmartWatchService_init(struct SmartWatchService_cb* callbacks){
    if (callbacks) {
        app_SmartWatchService_cbs.battery_level_cb = callbacks->battery_level_cb;
        app_SmartWatchService_cbs.notification_cb  = callbacks->notification_cb;
        app_SmartWatchService_cbs.time_update_cb   = callbacks->time_update_cb;
        return 0;
    } 
    else
    {
        return -1;
    }
}