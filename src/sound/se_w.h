/* The SE work object and its 32 slot records - the one definition of `_se_w` under `include/`
 * (docs/plan.md 6.5 rule 1).  Moved out of `sound/se.h` by `Pl/fn_80224AC4.cpp`, whose tail functions
 * are the only consumers that need the record itself: `sound/se.h` also defines its own `struct _PLW`
 * view, and a unit that has to name `_PLW` fields (this one does) cannot include both.  The layout and
 * the size are unchanged from `sound/se.h`; the four bytes at +0xA3D..+0xA40 were named here first.
 */
#ifndef MHTRI_SOUND_SE_W_H
#define MHTRI_SOUND_SE_W_H

#include "types.h"
#include "nw4r/math.h"

/* One of the 32 sound-slot records the SE work object carries at +0x3C.
 * size: 0x50 */
struct SeSlot {
    /* +0x00 */ u8 in_use;
    /* +0x01 */ u8 state;
    /* +0x02 */ u8 kind;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ nw4r::math::VEC3 pos;
    /* +0x10 */ nw4r::math::VEC3 field_0x10;
    /* +0x1C */ f32 field_0x1C;
    /* +0x20 */ f32 field_0x20;
    /* +0x24 */ u8 field_0x24;
    /* +0x25 */ u8 field_0x25;
    /* +0x26 */ u8 pad_0x26[2];
    /* +0x28 */ u32 owner;
    /* +0x2C */ u32 id;
    /* +0x30 */ s32 field_0x30;
    /* +0x34 */ s32 field_0x34;
    /* +0x38 */ u32 param;
    /* +0x3C */ u32 field_0x3C;
    /* +0x40 */ u32 field_0x40;
    /* +0x44 */ u32 field_0x44;
    /* +0x48 */ s16 field_0x48;
    /* +0x4A */ u8 field_0x4A;
    /* +0x4B */ u8 field_0x4B;
    /* +0x4C */ s16 field_0x4C;
    /* +0x4E */ u8 pad_0x4E;
    /* +0x4F */ u8 field_0x4F;
};

/* The SE work object: one per sound source.  +0x08 and +0x0C are read by `se_req_pos_ps`.
 * size: 0x295F4 (at least; `fn_800D7F54` clears 0x2966C bytes) */
struct _se_w {
    /* +0x000 */ u8 pad_0x000[8];
    /* +0x008 */ s32 field_0x08;
    /* +0x00C */ s32 field_0x0C;
    /* +0x010 */ u8 pad_0x010[0x2C];
    /* +0x03C */ SeSlot slots[32];
    /* +0x0A3C */ u8 field_0x0A3C;
    /* +0x0A3D */ u8 field_0x0A3D;  /* the actor's first SE frame code (`Pl/fn_80229E10`) */
    /* +0x0A3E */ u8 field_0x0A3E;  /* the second */
    /* +0x0A3F */ u8 field_0x0A3F;  /* the third */
    /* +0x0A40 */ u8 field_0x0A40;  /* the fourth (`Pl/fn_80229EA8`) */
    /* +0x0A41 */ u8 pad_0x0A41[0x287FB];
    /* +0x29238 */ f32 ramp[16];
    /* +0x29278 */ f32 field_0x29278;
    /* +0x2927C */ u8 pad_0x2927C[0x368];
    /* +0x295E4 */ s32 voices[2];
    /* +0x295EC */ u8 pad_0x295EC[2];
    /* +0x295EE */ u8 voice_slot;
    /* +0x295EF */ u8 field_0x295EF;
    /* +0x295F0 */ s32 field_0x295F0;
};


#endif /* MHTRI_SOUND_SE_W_H */
