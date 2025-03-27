#ifndef __FUEL__H
#define __FUEL__H

#define VCELL_ADDR  0x02
#define SOC_ADDR    0x04

int batteryFuelInit(void);
float getBatteryVoltage(void);
float getBatteryStateOfCharge(void);

#endif // __FUEL__H