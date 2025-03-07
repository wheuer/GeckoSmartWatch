#ifndef __TAPS_H__
#define __TAPS_H__

typedef enum {
    TAP_SINGLE,
    TAP_DOUBLE
} Tap_t;

int bma400Init(void);

#endif // __TAPS_H__