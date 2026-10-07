/* OS/OSAlarm.h - the OS alarm record and the entry points `OS/OSAlarm.c` owns beyond those in `OS/OSSetAlarm.h`. */
#ifndef MHTRI_OS_OSALARM_H
#define MHTRI_OS_OSALARM_H

#include "types.h"
#include "OS/OSContext.h"

typedef struct OSAlarm OSAlarm;

/* The callback an alarm runs when it fires. */
typedef void (*OSAlarmHandler)(OSAlarm* alarm, OSContext* context);

/* size: 0x30 */
struct OSAlarm {
    /* +0x00 */ OSAlarmHandler handler; /* null when the alarm is not queued */
    /* +0x04 */ u32 tag;                /* 0xFFFFFFFF marks an alarm the OS itself owns */
    /* +0x08 */ s64 fire;               /* system time the alarm fires at */
    /* +0x10 */ OSAlarm* prev;
    /* +0x14 */ OSAlarm* next;
    /* +0x18 */ s64 period;             /* zero for a one-shot alarm */
    /* +0x20 */ s64 start;              /* the periodic alarm's first fire time */
    /* +0x28 */ void* userData; /* untyped: caller-owned payload */
    /* +0x2C */ u8 pad_0x2C[0x4];
};

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CB440 - hooks the decrementer exception to the alarm queue and registers the shutdown hook. */
void __OSInitAlarm(void);

/* 0x804CBC30 / 0x804CBC40 - stores / reads the alarm's user data. */
/* untyped: caller-owned payload */
void OSSetAlarmUserData(OSAlarm* alarm, void* userData);
/* untyped: caller-owned payload */
void* OSGetAlarmUserData(OSAlarm* alarm);

/* 0x804CBC50 - stores `userData` and marks the alarm as OS-owned. */
/* untyped: caller-owned payload */
void cPhs_Set(OSAlarm* alarm, void* userData);

/* 0x804CBC60 - cancels every OS-owned alarm whose user data is `userData`. */
void __OSCancelInternalAlarms(u32 userData);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_OS_OSALARM_H */
