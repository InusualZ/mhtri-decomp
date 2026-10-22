/* ef/res_eft_create_ex.h - leaf header (docs/plan.md 6.5 rule 2): `ef/eft_res.cpp`'s `res_eft_create_ex`
 *   (0x800F91C4), for `lobby/fn_802FA9A0.cpp`.  C linkage.  GUESS name (registry). */
#ifndef MHTRI_EF_RES_EFT_CREATE_EX_H
#define MHTRI_EF_RES_EFT_CREATE_EX_H

#include "types.h"

namespace nw4r { namespace ef { class Effect; } }

extern "C" {

/* 0x800F91C4: creates pooled effect `id` of resource group `group` (`res_eft_create` forwards here with `param` 0). */
nw4r::ef::Effect* res_eft_create_ex(u16 group, u8 id, u32 flags, u32 param);

}

#endif /* MHTRI_EF_RES_EFT_CREATE_EX_H */
