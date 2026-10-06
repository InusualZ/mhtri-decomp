/* OS/OSAlarm.h - the RVL SDK alarm record.  The functions that take it (`OSCreateAlarm`, `OSSetAlarm`,
 *   `OSCancelAlarm`) are `NAND/nand.c`'s and are declared with their owner. */
#ifndef MHTRI_OS_OSALARM_H
#define MHTRI_OS_OSALARM_H

#include "types.h"

typedef struct OSAlarm OSAlarm;
typedef struct OSContext OSContext;

/* The callback an alarm runs when it fires. */
typedef void (*OSAlarmHandler)(OSAlarm* alarm, OSContext* context);

/* size: 0x30 */
struct OSAlarm {
    /* +0x00 */ OSAlarmHandler handler;
    /* +0x04 */ u32 tag;
    /* +0x08 */ s64 fire;
    /* +0x10 */ OSAlarm* prev;
    /* +0x14 */ OSAlarm* next;
    /* +0x18 */ s64 period;
    /* +0x20 */ s64 start;
    /* +0x28 */ void* userData; /* untyped: caller-owned payload */
    /* +0x2C */ u8 pad_0x2C[0x4];
};

#endif /* MHTRI_OS_OSALARM_H */
