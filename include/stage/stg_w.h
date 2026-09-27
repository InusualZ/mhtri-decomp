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

/* 0x802B0A98 - the stage table selector `enemy/em020_handlers.cpp`'s `em020_area_model_set` calls
 * for the two em020 area models: `idx` indexes the two-byte records at 0x805DEF38, whose bytes are
 * handed to `fn_802B0A3C`, and `value` is stored into the stage work block's +0x2EAC byte
 * (`0x806BB7AC`) once per call.  Added with that registration (rule 2: this range owns the address;
 * the signature is the callee's own body - `clrlwi r3,24` for the index, a `stb` for the value). */
void fn_802B0A98(u8 idx, u8 value);

/* 0x802AEC00 - the stage pack reset `light/light.cpp`'s `fn_802C2314` calls; added with that
 * registration (rule 2: this range owns the address).  The owner's body does not exist yet, so the
 * signature is the call site's view: no arguments, no result. */
void fn_802AEC00(void);

#ifdef __cplusplus
}

/* 0x802AFC84 - the current area number, a C++ free function (the map spells it `get_now_areano__Fv`),
 * so the declaration sits at C++ scope (rule 9).  Every effect setter gates its spawn on it; added
 * with `ef/eft035.cpp` (rule 2: this range owns the address). */
u8 get_now_areano(void);
#endif

#endif /* MHTRI_STAGE_STG_W_H */
