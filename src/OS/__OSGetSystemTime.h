/* OS/__OSGetSystemTime.h - the declaration of `__OSGetSystemTime`, which `OS/OSTime.c` owns (docs/plan.md 6.5 rule 2): the system time in bus ticks / 4. */
#ifndef MHTRI_OS___OSGETSYSTEMTIME_H
#define MHTRI_OS___OSGETSYSTEMTIME_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
s64 __OSGetSystemTime(void);
#ifdef __cplusplus
}
#endif

#endif
