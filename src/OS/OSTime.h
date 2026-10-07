/*
 * OS/OSTime.h - declarations of the symbols owned by `OS/OSTime.c` that other units call or read.
 */
#ifndef OS_OSTIME_H
#define OS_OSTIME_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804D4D70 / 0x804D4D50 - the low 32 bits of the time base / the whole 64-bit time base (the SDK's
 * signed `OSTime`; one tick is a quarter of the bus clock). */
u32 OSGetTick(void);

/* The broken-down time `OSTicksToCalendarTime` fills. */
typedef struct OSCalendarTime {
    /* +0x00 */ s32 sec;
    /* +0x04 */ s32 min;
    /* +0x08 */ s32 hour;
    /* +0x0C */ s32 mday;
    /* +0x10 */ s32 mon;
    /* +0x14 */ s32 year;
    /* +0x18 */ s32 wday;
    /* +0x1C */ s32 yday;
    /* +0x20 */ s32 msec;
    /* +0x24 */ s32 usec;
} OSCalendarTime; /* size: 0x28 */
/* 0x804D4E50 - converts a time-base value to calendar time. */
void OSTicksToCalendarTime(s64 ticks, OSCalendarTime* td);
/* 0x804D5180 - converts calendar time back to a time-base value. */
s64 OSCalendarTimeToTicks(const OSCalendarTime* td);

s64 OSGetTime(void);

#ifdef __cplusplus
}
#endif

#endif
