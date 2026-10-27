/* Pl/pl_guard_ck.h - leaf header: `pl_guard_ck` (0x80264C3C), owned by `Pl/pl_act_step.cpp`. C linkage. */
#ifndef MHTRI_PL_PL_GUARD_CK_H
#define MHTRI_PL_PL_GUARD_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif
/* Whether the player guards a hit coming from `angle` (relative to its facing). */
s32 pl_guard_ck(struct _PLW* self, s32 angle);
#ifdef __cplusplus
}
#endif

#endif
