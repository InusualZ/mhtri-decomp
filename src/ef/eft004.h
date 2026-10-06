#ifndef MHTRI_EF_EFT004_H
#define MHTRI_EF_EFT004_H

#include "types.h"
#include "nw4r/math.h"

/* ef/eft004.h - the eft004 family's declarations its consumers call (docs/plan.md 6.5 rule 2): the matrix helpers
 * and `fn_80101594` are `ef/eft004_fx.cpp`'s, the per-frame handlers `ef/fn_800FD864_fx.cpp`'s.  Plain C linkage.
 * `mtx34_trans_get`'s two-argument form is the owner's; `ef/fn_80114E34.cpp` passes three and keeps its own
 * declaration, so the two arities coexist. */
#ifdef __cplusplus
extern "C" {
#endif

void mtx34_trans_add(MTX34* out, VEC3* pos);

/* Reads the translation of an `MTX34` into a `VEC3`. */
void mtx34_trans_get(MTX34* mtx, VEC3* out);

/* The per-frame handlers states 1-3 of the family's dispatcher (`ef/fn_800FD864_fx.cpp`'s `fn_800FE93C`) tail-call
 * into, declared against the 0x48-byte effect slot (`struct _EFT` is `ef.h`'s). */
struct _EFT;
void fn_800FF8D4(struct _EFT* self);
void fn_800FFC98(struct _EFT* self);
void fn_800FFCA8(struct _EFT* self);
void fn_80100088(void* effect, s32 flag);
/* 0x80101594 - the player actor's effect-anchor refresh `Pl/fn_802489D4.cpp` calls after it arms a motion; it takes
 * the caller's `_PLW*` as an opaque pointer. */
void fn_80101594(void* self);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT004_H */
