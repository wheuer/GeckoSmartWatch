#ifndef __BLE__H
#define __BLE__H

#define MAX_NOTIFICATION_COUNT  5
#define MAX_LENGTH_APP_NAME     64

typedef struct Notification {
    char appName[MAX_LENGTH_APP_NAME];
    char title[64];
    char text[256];
    time_t timestamp;
} Notification;

int BLE_init(void);
void clearNotifications(void);
void clearNotification(uint8_t notificationIndex);

extern Notification activeNotifications[5];
extern uint8_t notificationCount;

#endif // __BLE__H