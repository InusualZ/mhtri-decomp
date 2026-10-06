/* NAND/__OSGetSystemTime.h - the declaration of `__OSGetSystemTime`, which `NAND/nand.c` owns (docs/plan.md 6.5 rule 2): the system time in bus ticks / 4. */
#ifndef MHTRI_NAND___OSGETSYSTEMTIME_H
#define MHTRI_NAND___OSGETSYSTEMTIME_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
s64 __OSGetSystemTime(void);
#ifdef __cplusplus
}
#endif

#endif
