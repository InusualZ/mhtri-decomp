/* Pl/pl_class7_meter_add.h - leaf header: `pl_class7_meter_add` (0x8027CA48), owned by `Pl/pl_act.cpp`. C linkage. */
#ifndef MHTRI_PL_PL_CLASS7_METER_ADD_H
#define MHTRI_PL_PL_CLASS7_METER_ADD_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif
/* For an actor of class 7, adds `delta` to its +0x386 meter (capped at 100) and re-arms the meter's timer. */
void pl_class7_meter_add(struct _PLW* self, s16 delta);
#ifdef __cplusplus
}
#endif

#endif
