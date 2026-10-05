/* The light module's record types, shared by `light/light.cpp` and `camera/camera_main.cpp` (the head of the module
 * sits in the camera unit's range): the light-work record `LightWork` (0x4F8 B, two of them at `lbl_806BB7E0`) and the
 * sub-records its functions touch (docs/plan.md 6.5 rule 1: one definition, included).
 */
#ifndef MHTRI_LIGHT_LIGHT_WORK_H
#define MHTRI_LIGHT_LIGHT_WORK_H

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"

/* One light channel: a world position, an enable flag, a control byte (its low five bits index the
 * intensity tables 0x805D1E7C/0x805D1E90, bit 0x80 selects the fade direction and 0x40 the half-way
 * intensity test - all read in fn_802BE7E8), a countdown timer and a table index. */
typedef struct LightChannel {
    /* +0x00 */ nw4r::math::VEC3 pos;
    /* +0x0C */ u8 enable;
    /* +0x0D */ u8 ctrl;
    /* +0x0E */ s16 timer;
    /* +0x10 */ u8 table_index;
    /* +0x11 */ u8 pad_0x11[3];
} LightChannel; /* size: 0x14 */

/* The map resource the light work points at.  Only the block fn_802B0688 is handed is traced, so the
 * extent past +0x3C is an approximation. */
typedef struct LightResource {
    /* +0x00 */ u8 unused_0x00[0x3C];
    /* +0x3C */ u8 entry;
} LightResource; /* size: 0x3D traced (the record is larger; approximation) */

/* Four vectors - the base every light record starts with. */
typedef struct LightQuad {
    /* +0x00 */ nw4r::math::VEC3 v[4];
} LightQuad; /* size: 0x30 */

/* Three vectors with the middle extent untraced (fn_802BF004's record). */
typedef struct LightTriple {
    /* +0x00 */ nw4r::math::VEC3 a;
    /* +0x0C */ nw4r::math::VEC3 b;
    /* +0x18 */ u8 unused_0x18[0x24];
    /* +0x3C */ nw4r::math::VEC3 c;
} LightTriple; /* size: 0x48 */

/* Four vectors, a gap and four more (fn_802BF044's record). */
typedef struct LightOctal {
    /* +0x00 */ nw4r::math::VEC3 v[4];
    /* +0x30 */ u8 unused_0x30[0x54];
    /* +0x84 */ nw4r::math::VEC3 w[4];
} LightOctal; /* size: 0xB4 */

/* Four vectors, that sub-record and a trailing vector (fn_802BF180's record). */
typedef struct LightQuadMid {
    /* +0x00 */ nw4r::math::VEC3 v[4];
    /* +0x30 */ u8 unused_0x30[0x1C];
    /* +0x4C */ MTX34 mid;
    /* +0x7C */ nw4r::math::VEC3 tail;
} LightQuadMid; /* size: 0x88 */

/* Four vectors and a LightTriple (fn_802BEFB4's record). */
typedef struct LightQuadTriple {
    /* +0x00 */ nw4r::math::VEC3 v[4];
    /* +0x30 */ u8 unused_0x30[0x1C];
    /* +0x4C */ LightTriple inner;
} LightQuadTriple; /* size: 0x94 */

/* The scene root's own record (fn_802BF1D8's view): fifteen vector members. */
typedef struct LightRoot {
    /* +0x000 */ nw4r::math::VEC3 v0;
    /* +0x00C */ nw4r::math::VEC3 v1;
    /* +0x018 */ nw4r::math::VEC3 v2;
    /* +0x024 */ nw4r::math::VEC3 v3;
    /* +0x030 */ u8 unused_0x030[0x1C];
    /* +0x04C */ nw4r::math::VEC3 v4;
    /* +0x058 */ nw4r::math::VEC3 v5;
    /* +0x064 */ u8 unused_0x064[0x54];
    /* +0x0B8 */ nw4r::math::VEC3 v6;
    /* +0x0C4 */ nw4r::math::VEC3 v7;
    /* +0x0D0 */ nw4r::math::VEC3 v8;
    /* +0x0DC */ nw4r::math::VEC3 v9;
    /* +0x0E8 */ nw4r::math::VEC3 v10;
    /* +0x0F4 */ nw4r::math::VEC3 v11;
    /* +0x100 */ u8 unused_0x100[0x20];
    /* +0x120 */ nw4r::math::VEC3 v12;
    /* +0x12C */ nw4r::math::VEC3 v13;
    /* +0x138 */ nw4r::math::VEC3 v14;
    /* +0x144 */ u8 unused_0x144[0x04];
} LightRoot; /* size: 0x148 (the parent record's next member starts at +0x148) */

/* The parameter record (fn_802BF0AC's view): four vectors, then a four-element vector run whose two
 * halves the constructor walks separately. */
typedef struct LightParams {
    /* +0x00 */ nw4r::math::VEC3 v0;
    /* +0x0C */ nw4r::math::VEC3 v1;
    /* +0x18 */ nw4r::math::VEC3 v2;
    /* +0x24 */ nw4r::math::VEC3 v3;
    /* +0x30 */ u8 unused_0x030[0x1C];
    /* +0x4C */ nw4r::math::VEC3 runs[4];
    /* +0x7C */ u8 unused_0x07C[0x88];
} LightParams; /* size: 0x104 (the parent record's next member starts at +0x104) */

/* A record whose vector member sits at +0x04 (fn_802C26CC's view). */
typedef struct LightVecAt4 {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ nw4r::math::VEC3 vec;
} LightVecAt4; /* size: 0x10 */

/* fn_802C2664's record: it hands its +0x04 member to fn_802C26CC. */
typedef struct LightWrap {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ LightVecAt4 inner;
} LightWrap; /* size: 0x14 */

/* fn_802C2698's record: the block at +0x08 is handed to mhchar_construct by address. */
typedef struct LightWrap2 {
    /* +0x00 */ u8 unused_0x00[0x08];
    /* +0x08 */ u32 block;
} LightWrap2; /* size: 0x0C traced (the block is larger; approximation) */

/* The light work record.  `resource` is the map resource whose +0x3C block fn_802B0688 queries; every
 * other element is a sub-record a constructor builds, an untraced gap, or a channel. */
typedef struct LightWork {
    /* +0x000 */ LightRoot root;
    /* +0x148 */ LightQuadMid anim;
    /* +0x1D0 */ u8 unused_0x1D0[0x20];
    /* +0x1F0 */ LightQuad lights;
    /* +0x220 */ u8 unused_0x220[0x24];
    /* +0x244 */ LightParams params;
    /* +0x348 */ LightOctal colors;
    /* +0x3FC */ LightQuadTriple entry;
    /* +0x490 */ u8 unused_0x490[0x04];
    /* +0x494 */ LightResource* resource;
    /* +0x498 */ u8 unused_0x498[0x10];
    /* +0x4A8 */ LightChannel channel[3];
    /* +0x4E4 */ u8 unused_0x4E4[0x14];
} LightWork; /* size: 0x4F8 */

/* The scene root whose light work sits at +0x2878.  Only that offset is traced. */
typedef struct SceneRoot {
    /* +0x0000 */ u8 unused_0x0000[0x2878];
    /* +0x2878 */ LightWork light;
} SceneRoot; /* size: 0x2D70 (approximation: only the light block's offset is traced) */

#endif /* MHTRI_LIGHT_LIGHT_WORK_H */
