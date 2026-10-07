/* OS/OSSetAlarm.h - the OS alarm entry points `OS/OSAlarm.c` owns (docs/plan.md 6.5 rule 2, leaf header). */
#ifndef MHTRI_OS_OSSETALARM_H
#define MHTRI_OS_OSSETALARM_H

#include "OS/OSAlarm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CB4A0 - prepares an alarm record. */
void OSCreateAlarm(OSAlarm* alarm);

/* 0x804CB700 - arms `alarm` to run `handler` after `tick` time-base ticks. */
void OSSetAlarm(OSAlarm* alarm, s64 tick, OSAlarmHandler handler);

/* 0x804CB800 - disarms `alarm`. */
void OSCancelAlarm(OSAlarm* alarm);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_OS_OSSETALARM_H */
