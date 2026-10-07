/* OS/OSSetPeriodicAlarm.h - the declaration of `OSSetPeriodicAlarm`, which `OS/OSAlarm.c` owns (docs/plan.md 6.5 rule 2,
 *   leaf header). */
#ifndef MHTRI_OS_OSSETPERIODICALARM_H
#define MHTRI_OS_OSSETPERIODICALARM_H

#include "OS/OSAlarm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CB770 - arms `alarm` to run `handler` every `period` ticks from `start`. */
void OSSetPeriodicAlarm(OSAlarm* alarm, s64 start, s64 period, OSAlarmHandler handler);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_OS_OSSETPERIODICALARM_H */
