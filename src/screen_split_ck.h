/* Leaf header (docs/plan.md 6.5 rule 2): `screen_split_ck`, which `main` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_SCREEN_SPLIT_CK_H
#define MHTRI_SCREEN_SPLIT_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800403F8 - whether the screen is split for two views (`Screen_w` +0x1A as 0/1). */
u32 screen_split_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SCREEN_SPLIT_CK_H */
