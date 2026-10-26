/* Pl/pl_act_no_eft_ck.h - leaf header for `pl_act_no_eft_ck`, which `src/Pl/pl_act.cpp` defines with C linkage
 * (docs/plan.md 6.5 rule 2).  A leaf because `src/unsplit/Pl.h` still declares it with another parameter type and a
 * unit that includes both `Pl/pl_act.h` and the band would see two spellings. */
#ifndef MHTRI_PL_PL_ACT_NO_EFT_CK_H
#define MHTRI_PL_PL_ACT_NO_EFT_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027D7EC - 1 when the player's act hides its effects (act 10..14/102..103 for mode <= 1, 74..78/104..105 for
 * mode 0 or 2, with +0x0A clear), else 0.  GUESS name. */
u32 pl_act_no_eft_ck(struct _PLW* plw, s32 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_NO_EFT_CK_H */
