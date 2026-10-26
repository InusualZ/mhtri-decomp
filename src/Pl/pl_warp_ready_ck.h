/* Leaf header (docs/plan.md 6.5 rule 2): `pl_warp_ready_ck`, which `Pl/pl_act` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_PL_PL_WARP_READY_CK_H
#define MHTRI_PL_PL_WARP_READY_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {

/* 0x80274794 - whether the player work `plw` may take a warp zone. */
u32 pl_warp_ready_ck(struct _PLW* plw);

}
#endif

#endif /* MHTRI_PL_PL_WARP_READY_CK_H */
