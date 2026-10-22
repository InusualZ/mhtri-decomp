/* Leaf header (docs/plan.md 6.5 rule 2): `eft_res_release`, which `ef/eft_res` owns (the name is a registry proposal of lane
 * enemy-control; the map row is renamed when it lands).  C linkage. */
#ifndef MHTRI_EF_EFT_RES_RELEASE_H
#define MHTRI_EF_EFT_RES_RELEASE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F7778 - releases one registered effect resource id. */
void eft_res_release(u8 id);

#ifdef __cplusplus
}
#endif

#endif
