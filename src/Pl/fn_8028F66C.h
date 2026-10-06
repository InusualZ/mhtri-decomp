/*
 * `Pl/pl_coll.cpp`'s collision records and outbound declarations.  `fn_8012A624`/`fn_8012A8F8` belong to
 * `enemy/em_common.cpp`, whose header does not declare them, so the shapes here are this unit's call sites.
 */
#ifndef MHTRI_PL_FN_8028F66C_H
#define MHTRI_PL_FN_8028F66C_H

#include "types.h"
#include "nw4r/math.h"
#include "Pl/fn_80288CEC.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One 0x14-byte land/hit record: the six scalar fields the hit queries carry plus the hit position
 * at +0x08.  Evidence: `fn_802977E4` - the record's own initialiser at 0x802977E4 - writes exactly
 * this shape (`sth` +0x00, `stb` +0x02, `stb` +0x03, `sth` +0x04, `stb` +0x06, `stb` +0x07, then
 * `setVector3(+0x08, ...)`), and `fn_8012A624` constructs only the +0x08 vector.  The scalar fields
 * keep offsets until a reader names them (rule 5's accepted fallback, as in `pl.h`).
 * size: 0x14 */
struct LandData {
    /* +0x00 */ u16 field_0x00;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u16 field_0x04;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ nw4r::math::VEC3 vec_0x08;
};

/* 0x8029403C - sweeps the segment `from`..`to` (with a radius) through the area's collision grid and
 * answers 1 on the first hit.  `flags` bit 0 tests the floor cells, bit 1 the wall cells; `area` 0xFF
 * means the current area.  The float sits third, ahead of the integers, because the target's callers
 * load it first.  `enemy/em024_ai.cpp` calls it. */
s32 pl_coll_sweep_ck(nw4r::math::VEC3* from, nw4r::math::VEC3* to, f32 radius, u32 flags, u32 arg, u8 area, u16 mask);

/* 0x8012A624 - the `LandData` constructor: constructs the +0x08 vector. */
void fn_8012A624(LandData* land);

/* 0x8012A8F8 - the `PlBox` constructor: zeroes (constructs) all three of its vectors. */
void fn_8012A8F8(PlBox* box);

/* 0x802977E4 - resets a `LandData` to its default record (all six scalars zero, +0x08 a constant vector). */
void fn_802977E4(LandData* land);

/* 0x80291B08 - the fixed-layer ground query: it takes the player work at r3 and forwards the caller's
 * `LandData`/out/kind.  The AI band reads the ground under its own work record, so its call site casts it. */
s32 fn_80291B08(struct _PLW* self, nw4r::math::VEC3* pos, LandData* land, f32* out, u32 kind);

#ifdef __cplusplus
}
#endif


/* 0x802907BC - called by `enemy/em_action.cpp`. */
u32 fn_802907BC(void* a, void* b);

#endif /* MHTRI_PL_FN_8028F66C_H */
