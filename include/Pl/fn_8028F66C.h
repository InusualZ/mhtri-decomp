/*
 * `Pl/fn_8028F66C.cpp`'s outbound declarations (the ground/hit collision TU).
 *
 * Two of these are callees whose owner's header cannot supply a usable declaration:
 *
 * * `fn_80041E40` is owned by `mh3_pad.cpp`, whose `include/mh3_pad.h` spells it `(void*, const void*)`
 *   *and* spells `fn_80043EA8` as `(void*)`.  `fn_80043EA8` is declared `(VEC3*)` by `include/ef.h`,
 *   which this unit reaches through `pl.h` and through `ef/fn_800AEE48.h` (for `fn_800B0B90`), so
 *   including `mh3_pad.h` here is `(10197) illegal function overloading` on `fn_80043EA8` - measured,
 *   not guessed.  The shape below is the call sites' (a 0xC-byte record copy), matching the owner's
 *   body, and it is C linkage.
 * * `fn_8012A8F8` is owned by `enemy/fn_801251D0.cpp`; its header does not declare the symbol yet
 *   (`(u16*)writer`-style stub), so the shape here is this unit's call site: it zeroes a `PlBox`.
 *
 * Both follow the practice `include/enemy/fn_80165FC8.h` documents for the same situation ("their
 * owners' headers do not declare these, or declare a different signature; the shapes here are the
 * call sites'").  When those owners write their bodies the declarations move to their headers.
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

/* 0x80041E40 - copies one 0xC-byte float record. */
void fn_80041E40(nw4r::math::VEC3* dst, const nw4r::math::VEC3* src);

/* 0x8012A624 - the `LandData` constructor: constructs the +0x08 vector. */
void fn_8012A624(LandData* land);

/* 0x8012A8F8 - the `PlBox` constructor: zeroes (constructs) all three of its vectors. */
void fn_8012A8F8(PlBox* box);

/* 0x802977E4 - resets a `LandData` to its default record (all six scalars zero, +0x08 a constant
 * vector).  Its owner is in the unclaimed band after this unit, so the shape here is that body's. */
void fn_802977E4(LandData* land);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_FN_8028F66C_H */
