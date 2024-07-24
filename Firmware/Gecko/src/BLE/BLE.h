#ifndef __BLE__H
#define __BLE__H

typedef struct Notification {
    char appName[64];
    char title[64];
    char text[256];
    time_t timestamp;
} Notification;

int BLE_init(void);
void clearNotifications(void);

extern Notification activeNotifications[5];
extern uint8_t notificationCount;

#endif // __BLE__H