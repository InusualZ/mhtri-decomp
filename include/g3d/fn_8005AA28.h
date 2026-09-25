/*
 * The `nw4r::g3d::ResMat`-cluster accessors owned by `g3d/fn_8005AA28.cpp`
 * (0x8005AA28-0x8005ABD8; registered from proposal/8005AA28_fn_8005AA28).
 *
 * A declaration belongs in the symbol's owner's header (docs/plan.md 6.5 rule 2), so the whole cluster
 * moves here: `g3d/g3d_anmvis.cpp` calls `fn_8005AA44` (the `g3d_resmat_ac.h` assert-then-set/clear flag
 * helper) without re-declaring it, and `g3d/g3d_calcworld.cpp` can drop its own copies of
 * `fn_8005AAEC`/`fn_8005AAE4` for the same reason.
 *
 * The functions are C-linkage (their map names are plain `fn_XXXXXXXX`), and the handle type is left as
 * `void*` here: the owner names it `ResMatHandle` in its own source, but its one-word layout is not this
 * header's business and the parameter type does not enter the symbol.
 */
#ifndef MHTRI_G3D_FN_8005AA28_H
#define MHTRI_G3D_FN_8005AA28_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The `g3d_resmat_ac.h` assert-then-set/clear helper: with `enable` non-zero it sets bit 0x100 of the
 * resource's mat flag word, otherwise it clears it. */
void fn_8005AA44(void* pSelf, u32 enable);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_FN_8005AA28_H */
