#ifndef MHTRI_EF_EFT004_H
#define MHTRI_EF_EFT004_H

#include "types.h"
#include "nw4r/math.h"

/* Declarations for the symbols `src/ef/eft004.cpp` owns (docs/plan.md 6.5, rule 2).  Plain C-linkage
 * names, so C-visible.  Kept minimal.
 *
 * `fn_8010140C`'s two-argument form is the owner's (and `ef/eft007.cpp`'s); `ef/fn_80114E34.cpp`
 * passes three and keeps its own declaration, so the two arities coexist. */
#ifdef __cplusplus
extern "C" {
#endif

void fn_80101428(MTX34* out, VEC3* pos);

/* Reads the translation of an `MTX34` into a `VEC3`. */
void fn_8010140C(MTX34* mtx, VEC3* out);

/* The three per-frame handlers states 1-3 of the map/area family's dispatcher (`ef/fn_800FD864.cpp`)
 * tail-call into.  They are plain C symbols owned by this unit; the caller only ever passes the 0x48-byte
 * effect slot, so they are declared against that view here (`struct _EFT` is `ef.h`'s). */
struct _EFT;
void fn_800FF8D4(struct _EFT* self);
void fn_800FFC98(struct _EFT* self);
void fn_800FFCA8(struct _EFT* self);
void fn_80100088(void* effect, s32 flag);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT004_H */
