/* ef/fn_80114E34_types.h - the types and macros `ef/fn_80114E34.cpp` and `ef/eft022_fx.cpp` share. */
#ifndef MHTRI_EF_FN_80114E34_TYPES_H
#define MHTRI_EF_FN_80114E34_TYPES_H

#include "types.h"
#include "nw4r/math.h"
#include "Pl/plw.h"
#include "gx.h"
#include "unsplit/unknown.h"
#include "unsplit/enemy.h"
#include "enemy/fn_8012BDF4.h"
#include "sys_mem.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/fn_802693C4.h"
#include "ef/fn_8011722C.h"
#include "unsplit/g3d.h"
#include "unsplit/sound.h"
#include "unsplit/Pl.h"
#include "unsplit/ef.h"
#include "sound/fn_800D7F54.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */

namespace nw4r {

namespace ef {
struct Effect;
}
}

/* The 4-byte colour the effect material calls take by value comes from `gx.h` - one definition, in the
 * owner's header (rule 1). */
struct _EFT;

struct MHchar;

struct _PLW_PHYSICS;

/* The effect record `eft_res_slot_get` hands out.  The work block at +0x38 is typed per family, so it
 * stays `void*` here and each function casts it. size: 0x48 */
struct _EFT {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 flag_0x01;
    /* +0x02 */ u8 type_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 state_0x05;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ u8 mode_0x08;
    /* +0x09 */ u8 unused_0x09[0x0C - 0x09];
    /* +0x0C */ s32 timer_0x0C;
    /* +0x10 */ s32 field_0x10;
    /* +0x14 */ u8 unused_0x14[0x18 - 0x14];
    /* +0x18 */ nw4r::math::VEC3 pos_0x18;
    /* +0x24 */ s32 field_0x24;
    /* +0x28 */ s32 field_0x28;
    /* +0x2C */ s32 field_0x2C;
    /* +0x30 */ void* source_0x30;
    /* +0x34 */ void (*dispatch_0x34)(_EFT*);
    /* +0x38 */ void* work_0x38;
    /* +0x3C */ u8 unused_0x3C[0x40 - 0x3C];
    /* +0x40 */ void (*release_0x40)(_EFT*);
    /* +0x44 */ u8 area_0x44;
    /* +0x45 */ u8 unused_0x45[0x48 - 0x45];
};

/* `_PLW`, the player work record, comes from `Pl/plw.h` - one definition, in the owner's header (rule 1). */

/* The effect character `eft_res_model_get` hands out (the pointer it returns is slot+4).  Only the fields
 * these functions touch are named. */
struct MHchar {
    /* +0x000 */ u8 unused_0x000[0x04];
    /* +0x004 */ nw4r::math::VEC3 pos_0x04;
    /* +0x010 */ u8 unused_0x010[0x1C - 0x10];
    /* +0x01C */ nw4r::math::VEC3 scale_0x1C;
    /* +0x028 */ s32 field_0x28;
    /* +0x02C */ s32 field_0x2C;
    /* +0x030 */ s32 field_0x30;
    /* +0x034 */ u8 unused_0x034;
    /* +0x035 */ u8 ready_0x35;
    /* +0x036 */ u8 unused_0x036[0x40 - 0x36];
    /* +0x040 */ s32 field_0x40;
    /* +0x044 */ u8 unused_0x044[0x114 - 0x44];
    /* +0x114 */ s32 field_0x114;
    /* +0x118 */ s32 field_0x118;
    /* +0x11C */ u8 unused_0x11C[0x13C - 0x11C];
    /* +0x13C */ u8* field_0x13C;
};

/* size: 0x140 (lower bound) */

/* The player's body sub-object `_PLW.physics_0x13C` points at: its `MHchar` sits at +4. */
struct _PLW_PHYSICS {
    /* +0x00 */ u8 pad_0x00[4];
    /* +0x04 */ MHchar chr_0x04;
};

/* The enemy actor an effect was spawned for (opaque here). */
struct _ENEMY_WORK {
    /* +0x000 */ u8 unused_0x000[0xB14];
    /* +0xB14 */ s32 field_0xB14;
};
#endif /* MHTRI_EF_FN_80114E34_TYPES_H */
