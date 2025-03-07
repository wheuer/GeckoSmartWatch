#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <time.h>

#include "system.h"
#include "SmartWatchService.h"
#include "clock.h"
#include "BLE.h"

static char stringBuffer[512]; // To hold notification transfer, the max characteristic size is 512
volatile bool bluetoothConnected = false;

// We should be using a linked list for this but brute force an array for now
// This buffer will be managed as a ring buffer
Notification activeNotifications[MAX_NOTIFICATION_COUNT];
uint8_t notificationCount = 0;
static uint8_t nextNotificationIndex = 0;

void clearNotifications(void)
{
    // We're not worried about securely deleting the notification data, just leave it and mark it unused
    notificationCount = 0;
    nextNotificationIndex = 0;
}

void clearNotification(uint8_t notificationIndex)
{
    // Make sure the index is valid for the notification buffer size
    // Also make sure that there is actually a notification to clear 
    if (notificationIndex < MAX_NOTIFICATION_COUNT && notificationCount > notificationIndex)
    {
        // Clear the notification info
        memset(&activeNotifications[notificationIndex], 0, sizeof(Notification));

        // Shift upper notification data to their new indices
        for (int i = notificationIndex; i < notificationCount - 1; i++)
        {
            memcpy(&activeNotifications[i], &activeNotifications[i+1], sizeof(Notification));
        }

        // We removed a notification and now there is a empty spot at the end
        notificationCount--;
        nextNotificationIndex = notificationCount;
    }
}

/* 
--------------- START OF ADVERTISING SETUP --------------- 
*/
// Create BLE advertisement parameter structure
static struct bt_le_adv_param* my_adv_param = BT_LE_ADV_PARAM(
    (BT_LE_ADV_OPT_CONNECTABLE|BT_LE_ADV_OPT_USE_IDENTITY),
    800, // Minimum advertising interval of 500ms (800 * 0.625ms)
    801, // Maximum advertising interval of 500.625ms (801 * 0.625ms)
    NULL // No undirected advertising
);

// Define the advertising data that will be passed to bt_le_adv_start()
static const struct bt_data my_advertising_data[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, BT_DEVICE_NAME, BT_DEVICE_NAME_LEN)
};

// Define the advertising scan response data
static const struct bt_data my_scan_response_data[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_SWS_VAL)
};
/* 
--------------- END OF ADVERTISING SETUP --------------- 
*/

/* 
--------------- START OF SMARTWATCHSERVICE CALLBACK SETUP --------------- 
*/
static float app_battery_level_cb(void) {
    printf("Battery level read.\r\n");
    return 2.22;
}

static void app_notification_cb(char* notification, int len) {
    char* splitIndex;

    // There is no null terminator but need it to parse so copy string into buffer and add it
    for (int i = 0; i < len; i++){
        stringBuffer[i] = notification[i];
    }
    stringBuffer[len] = '\0';

    printf("%s\n", stringBuffer);

    // Read in app name
    splitIndex = strtok(stringBuffer, ":");
    strncpy(activeNotifications[notificationCount].appName, splitIndex, 64);

    // Read in title
    splitIndex = strtok(NULL, ":");
    strncpy(activeNotifications[notificationCount].title, splitIndex, 64);

    // Read in text
    splitIndex = strtok(NULL, ":");
    strncpy(activeNotifications[notificationCount].text, splitIndex, 256);

    // Read in timestamp
    splitIndex = strtok(NULL, ":");
    activeNotifications[notificationCount].timestamp = (time_t) strtoull(splitIndex, NULL, 10);

    printf("Received and read in notification: %s, %s, %s, %lld\r\n", activeNotifications[notificationCount].appName,
                                                                      activeNotifications[notificationCount].title,
                                                                      activeNotifications[notificationCount].text,
                                                                      activeNotifications[notificationCount].timestamp);

    nextNotificationIndex = (nextNotificationIndex + 1) % 5;
    notificationCount++;

    // Alert the user inteface that a new notification has appeared
	k_event_post(&userInteractionEvent, SYSTEM_EVENT_NEW_NOTIFICATION);
}

static void app_update_time_cb(char* time, int len) {
    // Looks to have worked as the len matched the string length that I sent on the phone
    // There is no null terminator
    uint64_t epochTime = 0;
    for (int i = 0; i < len; i++){
        epochTime = (epochTime << 8) | (uint8_t) time[i];
    }
    
    // Any time zone or daylight savings time will be implemented on host
    setEpochTime(epochTime);

    printf("Synced time to: %llu\r\n", epochTime);
}

static struct SmartWatchService_cb my_SmartWatchService_cbs = {
    .battery_level_cb = app_battery_level_cb,
    .notification_cb  = app_notification_cb,
    .time_update_cb   = app_update_time_cb
};
/* 
--------------- END OF SMARTWATCHSERVICE CALLBACK SETUP --------------- 
*/

/* 
--------------- START OF BLE CONNECTION CALLBACK SETUP --------------- 
*/
static void on_connected(struct bt_conn* connection, uint8_t error){
    if (error) {
        printf("Connection attempt failed.\r\n");
        return;
    }
    bluetoothConnected = true;
    printf("Bluetooth Connected.\r\n");
};

static void on_disconnected(struct bt_conn* connection, uint8_t reason){
    bluetoothConnected = false;
    printf("Bluetooth Disconnected.\r\n");
};

struct bt_conn_cb my_connection_callbacks = {
    .connected      = on_connected,
    .disconnected   = on_disconnected
};

/* 
--------------- END OF BLE CONNECTION CALLBACK SETUP --------------- 
*/

int BLE_init(void)
{
    printf("Init Bluetooth...");

	int error;

	// Enable bluetooth before doing anything with the BLE stack
	error = bt_enable(NULL);
	if (error) {
		printf(ANSI_COLOR_RED "ERR: bt_enable" ANSI_COLOR_RESET "\n");
		return error;
	}

	// Register bluetooth connection callbacks
	bt_conn_cb_register(&my_connection_callbacks);

	// Pass the SmartWatchService the callbacks defined above
	error = SmartWatchService_init(&my_SmartWatchService_cbs);
	if (error) {
		printf(ANSI_COLOR_RED "ERR: SmartWatchService_init" ANSI_COLOR_RESET "\n");
		return error;
	}

	// Start advertising
	error = bt_le_adv_start(
		my_adv_param,
		my_advertising_data,
		ARRAY_SIZE(my_advertising_data),
		my_scan_response_data,
		ARRAY_SIZE(my_scan_response_data)
	);
	if (error) {
		printf(ANSI_COLOR_RED "ERR: bt_le_adv_start" ANSI_COLOR_RESET "\n");
		return error;
	}

    printf(ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "\n");

    return error;
}




