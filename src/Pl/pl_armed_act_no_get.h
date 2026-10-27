/* Leaf header (docs/plan.md 6.5 rule 2): `pl_armed_act_no_get` (0x802745DC), defined by `src/Pl/pl_act.cpp`. */
#ifndef MHTRI_PL_PL_ARMED_ACT_NO_GET_H
#define MHTRI_PL_PL_ARMED_ACT_NO_GET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

u32 pl_armed_act_no_get(struct _PLW* plw, s16 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ARMED_ACT_NO_GET_H */
