/* Pl/pl_sharpness_add.h - leaf header: `pl_sharpness_add` (0x80278674), owned by `Pl/pl_act.cpp`. C linkage. */
#ifndef MHTRI_PL_PL_SHARPNESS_ADD_H
#define MHTRI_PL_PL_SHARPNESS_ADD_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif
/* Adds `delta` to a master player's weapon wear (a loss is halved by the wear skills unless `halved` says it already
 * was). */
void pl_sharpness_add(struct _PLW* self, s16 delta, u8 halved);
#ifdef __cplusplus
}
#endif

#endif
