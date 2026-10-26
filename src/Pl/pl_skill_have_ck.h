/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `Pl/pl_act.cpp` defines, for a consumer that does not include the
 * owner's header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_PL_PL_SKILL_HAVE_CK_H
#define MHTRI_PL_PL_SKILL_HAVE_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802714F0 - 1 when `skill` is one of the eight skill ids the player's work holds at +0x5F2. */
u32 pl_skill_have_ck(struct _PLW* plw, u16 skill);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_SKILL_HAVE_CK_H */
