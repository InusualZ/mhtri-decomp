/*
 * `Pl/fn_8028F66C.cpp`'s outbound declarations (the ground/hit collision TU).
 *
 * `copyVec3` (0x80041E40) is `src/mh3_pad.cpp`'s and comes from `include/mh3_pad.h`, which this unit
 * includes - the `(10197)` clash this header used to record (`include/ef.h` spelling `VEC3_ctor` as
 * `(VEC3*)` against `mh3_pad.h`'s `(void*)`, measured 2026-09-27) is closed: both headers spell the
 * three helpers identically now.
 *
 * `fn_8012A8F8` is owned by `enemy/fn_801251D0.cpp`; its header does not declare the symbol yet
 * (`(u16*)writer`-style stub), so the shape here is this unit's call site: it zeroes a `PlBox`.
 * It follows the practice `include/enemy/fn_80165FC8.h` documents for the same situation ("their
 * owners' headers do not declare these, or declare a different signature; the shapes here are the
 * call sites'").  When that owner writes its body the declaration moves to its header.
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
 * keep offsets until a reader names them (rule 5's accepted fallback, as in `include/pl.h`).
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

/* 0x80041E40 - copies one 0xC-byte float record and returns `dst` (the owner `src/mh3_pad.cpp`'s
 * body); normalised with the declaration fold-in of 2026-09-27. */

/* 0x8012A624 - the `LandData` constructor: constructs the +0x08 vector. */
void fn_8012A624(LandData* land);

/* 0x8012A8F8 - the `PlBox` constructor: zeroes (constructs) all three of its vectors. */
void fn_8012A8F8(PlBox* box);

/* 0x802977E4 - resets a `LandData` to its default record (all six scalars zero, +0x08 a constant
 * vector).  Its owner is in the unclaimed band after this unit, so the shape here is that body's. */
void fn_802977E4(LandData* land);

/* 0x80291B08 - the fixed-layer ground query: the owner's own definition (`fn_8028F66C.cpp:262`) takes
 * the player work at r3 and forwards the caller's `LandData`/out/kind.  The AI band reads the ground
 * under its own work record rather than a player's, so its call site casts the record it holds.  Added
 * with `ai/fn_802C474C.cpp`, the first consumer outside the owner. */
s32 fn_80291B08(struct _PLW* self, nw4r::math::VEC3* pos, LandData* land, f32* out, u32 kind);

#ifdef __cplusplus
}
#endif


/* Added by `enemy/em_action.cpp` (rule 2: the declaration belongs with the owner TU, which
 * had not declared it yet). */
u32 fn_802907BC(void* a, void* b);

#endif /* MHTRI_PL_FN_8028F66C_H */
