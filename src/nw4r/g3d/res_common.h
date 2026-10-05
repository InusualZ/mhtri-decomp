/*
 * The nw4r g3d `ResCommon<T>`-style one-word handle (`g3d_rescommon.h`'s shape).
 *
 * A type more than one unit uses is defined once and included where needed (docs/plan.md 6.5 rule 1).
 * This header mirrors nw4r's `g3d_rescommon.h`: the one-word `ResCommon<T>` handle is shared by
 * `g3d/g3d_calcvtx.cpp` (the `g3d_resvtx_ac.h` accessor copies) and `g3d/g3d_calcworld.cpp` (the
 * `ResMdl` node-table pass), and the `IS_VALID_PTR` resource-pointer check by `g3d/g3d_resanm.c` and
 * `g3d/g3d_resanmamblight.c`, so both moved here out of the former `auto/` bulk-attribution units.
 * The resource-block types that only one of those units uses (`ResVtxNrmBlock`, `VtxEntry`,
 * `NodeCallback`, `ResAnmChrChannel`, ...) stay in the unit that owns them.
 */
#ifndef MHTRI_NW4R_G3D_RES_COMMON_H
#define MHTRI_NW4R_G3D_RES_COMMON_H

#include "types.h"

/* A `ResCommon<T>`-style one-word handle. The map names the instantiations' out-of-line copies of the
 * accessor inlines separately (`fn_800733FC`/`fn_800736F0` and friends), so each keeps its `fn_` name
 * rather than being written as a member. */
struct ResHandle {
    /* +0x0 */ void* mpData;
}; /* size: 0x4 */

/*
 * The pointer-range check nw4r's resource macros expand to: a pointer is valid when it lies in one of
 * the Wii memory regions.  Shared by `g3d/g3d_resanm.c` and `g3d/g3d_resanmamblight.c`, so it lives
 * here (rule 1).
 */
#define IS_VALID_PTR(p) ( \
    (((u32)(p) & 0xFF000000) == 0x80000000) || \
    (((u32)(p) & 0xFF800000) == 0x81000000) || \
    (((u32)(p) & 0xF8000000) == 0x90000000) || \
    (((u32)(p) & 0xFF000000) == 0xC0000000) || \
    (((u32)(p) & 0xFF800000) == 0xC1000000) || \
    (((u32)(p) & 0xF8000000) == 0xD0000000) || \
    (((u32)(p) & 0xFFFFC000) == 0xE0000000))

#endif /* MHTRI_NW4R_G3D_RES_COMMON_H */
