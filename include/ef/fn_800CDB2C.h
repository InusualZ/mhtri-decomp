#ifndef MHTRI_EF_FN_800CDB2C_H
#define MHTRI_EF_FN_800CDB2C_H

#include "types.h"

/* Declarations for the symbols `src/ef/fn_800CDB2C.cpp` owns (docs/plan.md 6.5 rule 2).  `fn_800CF208`
 * is the one the owner defines (`u8 fn_800CF208(void)`); the other two are called by
 * `sound/fn_800EF7D8.cpp` before the owner's body exists, so they carry that call site's view.  The
 * caller had declared all three itself, which only became a rule-2 violation when the owner registered
 * in the same landing wave.
 */
#ifdef __cplusplus
extern "C" {
#endif

u8  fn_800CF208(void);
u32 fn_800CF280(void);
s32 fn_800CED10(char* path, u32 dma, u32 size);
/* Builds the nw4r::ef::EffectSystem the effect manager keeps at `eft_control` +0x04. */
s32 fn_800D3C0C(void);
s32 fn_800CEE2C(const char* path, void* info);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_FN_800CDB2C_H */
