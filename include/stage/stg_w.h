/* The stage loader unit `stage/stg_w.cpp`.
 *
 * Declarations owned by that unit that other translation units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_STAGE_STG_W_H
#define MHTRI_STAGE_STG_W_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B0598 - the stage's area/kind classifier the player act band (`Pl/fn_80273B14.cpp`), the
 * player act helpers (`Pl/pl_act.cpp`) and the stage units read; the owner defines it `extern "C"`
 * at `stage/stg_w.cpp:489`.  The consumer narrows the result to a byte, so the declaration here is
 * `u8` where the definition's own return is `u32`. */
u8 fn_802B0598(u8 id);

/* 0x802B0688 - the stage resource query the light unit's `fn_802BEE3C` hands a block to; added with
 * the `light/light.cpp` registration (rule 2: this range owns the address).  The owner defines it
 * `extern "C" u32 fn_802B0688(void* self)` at `stage/stg_w.cpp:275`. */
u32 fn_802B0688(void* self);

/* 0x802AEC00 - the stage pack reset `light/light.cpp`'s `fn_802C2314` calls; added with that
 * registration (rule 2: this range owns the address).  The owner's body does not exist yet, so the
 * signature is the call site's view: no arguments, no result. */
void fn_802AEC00(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STG_W_H */
