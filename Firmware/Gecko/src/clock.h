#ifndef __CLOCK_H__
#define __CLOCK_H__

#include "system.h"

time_t getEpochTime(void);
void getTime(struct tm** timeObject);
void setEpochTime(uint64_t newEpochTime);
int clockInit(void);

#endif // __CLOCK_H__