#include "system.h"
#include "clock.h"

static volatile time_t currentEpochTime = 0;
static uint8_t timeBuffer[64];
static struct tm* mTime;
struct k_timer clockTimer;

static void clockUpdate(struct k_timer* timer_id)
{
    currentEpochTime++;
	if (currentEpochTime % 60 == 0)
	{
		// Minute boundary, alert main application
		k_event_post(&userInteractionEvent, 0x02);
	}
}

time_t getEpochTime(void)
{
	return currentEpochTime;
}

void getTime(struct tm** timeObject)
{
	*timeObject = gmtime((const time_t*) &currentEpochTime);
}

void setEpochTime(uint64_t newEpochTime)
{
	currentEpochTime = newEpochTime;
}

void printSystemTime(void)
{
	mTime = gmtime((const time_t*) &currentEpochTime);
	strftime(timeBuffer, sizeof(timeBuffer), "%I:%M%p", mTime);
	printf("%s\n", timeBuffer);
}

int clockInit(void)
{
	printf("Init system clock...");
	k_timer_init(&clockTimer, clockUpdate, NULL);
	k_timer_start(&clockTimer, K_SECONDS(1), K_SECONDS(1));
	printf(ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "\n");
	return 0;
}