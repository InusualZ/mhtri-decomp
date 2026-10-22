/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/fn_80230FBC.cpp`'s `pl_se_kind_dispatch`. */
#ifndef MHTRI_PL_PL_SE_KIND_DISPATCH_H
#define MHTRI_PL_PL_SE_KIND_DISPATCH_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80230FBC: the motion-kind SE dispatcher, SE work 3's callback (registry name) */
void pl_se_kind_dispatch(struct _PLW* self, s32 event);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_SE_KIND_DISPATCH_H */
