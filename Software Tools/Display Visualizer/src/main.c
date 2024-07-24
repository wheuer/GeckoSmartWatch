#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>

#include "GC9A01A.h"
#include "LCD.h"
#include "assets.h"

typedef struct Notification {
    char appName[64];
    char title[64];
    char text[256];
    time_t timestamp;
} Notification;
uint8_t notificationCount = 0;
static char stringBuffer[512]; // To hold notification transfer

Notification activeNotifications[5] = 
{
    {
        "Textra",
        "Beany Boy",
        "I like beans....",
        1721259814
    },
    {
        "Gmail",
        "email@gmail.com",
        "This is an email",
        1721259814
    },
    {
        "Textra",
        "Albertian Penguin",
        "The polar bear at the penguin.",
        1721259814
    },        
    {
        "Microsoft Auth",
        "Push me broksy",
        "Approve this maybe?",
        1721259814
    },
    {
        "Alarmy",
        "Fuck you",
        "Wake up bitch",
        1721259814
    }
};

uint8_t notificationIndex = 0;

void writeScreenBufferToFile(void)
{
    FILE* file = fopen("./image.txt", "w");
    for (int i = 0; i < SCREEN_BUFFER_LENGTH - 1; i++)
    {
        fprintf(file, "%x,", screenBuffer[i]);
    }
    fprintf(file, "%x", screenBuffer[SCREEN_BUFFER_LENGTH - 1]);
    fclose(file);
}

static void app_notification_cb(char* notification, int len) {
    char* splitIndex;

    // There is no null terminator but need it to parse so copy string into buffer and add it
    for (int i = 0; i < len; i++){
        stringBuffer[i] = notification[i];
    }
    stringBuffer[len] = '\0';

    // Read in app name
    splitIndex = strtok(stringBuffer, "::");
    strncpy(activeNotifications[notificationCount].appName, splitIndex, 64);

    // Read in title
    splitIndex = strtok(NULL, "::");
    strncpy(activeNotifications[notificationCount].title, splitIndex, 64);

    // Read in text
    splitIndex = strtok(NULL, "::");
    strncpy(activeNotifications[notificationCount].text, splitIndex, 256);

    // Read in timestamp
    splitIndex = strtok(NULL, ":");
    activeNotifications[notificationCount].timestamp = (time_t) atoi(splitIndex);

    printf("Received and read in notification: %s, %s, %s, %lld\r\n", activeNotifications[notificationCount].appName,
                                                                    activeNotifications[notificationCount].title,
                                                                    activeNotifications[notificationCount].text,
                                                                    activeNotifications[notificationCount].timestamp);
}

int main()
{
    // lcd_write_str(activeNotifications[0].appName, 50, 50, LCD_CHAR_MEDIUM, 0xFF, 0x00, 0x00);
    // for (int i = 0; i < 15; i++) lcd_write_str_grid(activeNotifications[0].appName, i, 2, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);

    lcd_write_str_grid("Notifications", 2, 11, LCD_CHAR_EXTRA_SMALL, 0xFF, 0xFF, 0xFF);
    lcd_write_str_grid(activeNotifications[0].appName, 3, 2, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);
    lcd_write_str_grid(activeNotifications[1].appName, 5, 1, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);
    lcd_write_str_grid(activeNotifications[2].appName, 7, 1, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);
    lcd_write_str_grid(activeNotifications[3].appName, 9, 1, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);
    lcd_write_str_grid(activeNotifications[4].appName, 11, 2, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);

    // lcd_write_str_grid("Batt: 3.44V", 2, 3, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);
    // lcd_write_str_grid("BLE: Disconn.", 4, 2, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);

    // lcd_draw_bitmap(watchBack, 240, 240, 0, 0);

    // time_t currentTime = 1721259814;
    // struct tm* mTime;
    // mTime = gmtime(&currentTime);
    // char timeBuffer[64];
    // strftime(timeBuffer, sizeof(timeBuffer), "%I:%M%p", mTime);
    // lcd_write_str(timeBuffer, 72, 110, LCD_CHAR_SMALL, 0xFF, 0xFF, 0xFF);
    // printf("%s\n", timeBuffer);


    // printf("Yo\n");
    // char testString[] = "Textra::Beans::On Toast::1234";
    
    // app_notification_cb(testString, 30);


    // char* location = strtok(testString, "::");
    // printf("%s\n", location);

    // location = strtok(NULL, "::");
    // printf("%s\n", location);

    // location = strtok(NULL, "::");
    // printf("%s\n", location);

    // location = strtok(NULL, "::");
    // printf("%s\n", location);

    // location = strtok(NULL, ":");
    // printf("%s\n", location);    

    // printf("What?\n");
    
    writeScreenBufferToFile();
    return 0;
}