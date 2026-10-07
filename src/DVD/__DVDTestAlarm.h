/* DVD/__DVDTestAlarm.h - `__DVDTestAlarm`, which `DVD/dvd.c` owns (docs/plan.md 6.5 rule 2, leaf header). */
#ifndef MHTRI_DVD___DVDTESTALARM_H
#define MHTRI_DVD___DVDTESTALARM_H

#include "types.h"
#include "OS/OSAlarm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804AB000 - nonzero when `alarm` is the DVD driver's own alarm. */
BOOL __DVDTestAlarm(OSAlarm* alarm);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DVD___DVDTESTALARM_H */
