/*
 * `Psw` - the per-player pad record array (.bss 0x80659350, 4 x 0x350 B = 0xD40), defined by `src/mh3_pad.cpp`
 * (its `.bss` 0x806585B8-0x806694E8 is that unit's own; its static initialiser `fn_80046B94` constructs the
 * array with `__construct_array` over 0x350-byte elements, as it does the twin `Psw_prev`).  The one home of
 * the record type (rule 1; the object is declared in `Psw.h`); it merges the views the tree
 * carried before: `mh3_pad.cpp`'s own (`mode`, `field_0x..`), `Pl/fn_80273B14.h`'s control words and the
 * lobby band's button words (`include/unsplit/lobby.h`).  Only the named offsets are read by matched
 * code; the rest is padding until a unit needs it.
 */
#ifndef MHTRI_MH3_PAD_PLAYER_PAD_H
#define MHTRI_MH3_PAD_PLAYER_PAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PlayerPad {
    /* +0x000 */ u8 pad_0x000[0x30];
    /* +0x030 */ u32 field_0x30;
    /* +0x034 */ u32 mode;
    /* +0x038 */ u8 pad_0x038[0x96];
    /* +0x0CE */ u16 field_0xce;
    /* +0x0D0 */ u8 pad_0x0d0[0x2];
    /* +0x0D2 */ u16 field_0xd2;
    /* +0x0D4 */ u8 pad_0x0d4[0x2];
    /* +0x0D6 */ u16 control_0x0D6;  /* the second control word's low pair (masked with 0x88) */
    /* +0x0D8 */ u8 pad_0x0d8[0x2];
    /* +0x0DA */ u16 field_0xda;
    /* +0x0DC */ u8 pad_0x0dc[0x2];
    /* +0x0DE */ u16 control_0x0DE;
    /* +0x0E0 */ u8 pad_0x0e0[0x18];
    /* +0x0F8 */ u16 field_0xf8;
    /* +0x0FA */ u8 pad_0x0fa[0x2];
    /* +0x0FC */ u16 control_0x0FC;  /* the held-button word */
    /* +0x0FE */ u8 pad_0x0fe[0x2];
    /* +0x100 */ u16 field_0x100;
    /* +0x102 */ u8 pad_0x102[0x2];
    /* +0x104 */ u16 control_0x104;  /* the pressed-button word */
    /* +0x106 */ u8 pad_0x106[0x18];
    /* +0x11E */ u16 field_0x11e;
    /* +0x120 */ u8 pad_0x120[0x1A4];
    /* +0x2C4 */ u16 pressed_0x2C4;
    /* +0x2C6 */ u8 pad_0x2c6[0x2];
    /* +0x2C8 */ u16 press_0x2C8;
    /* +0x2CA */ u8 pad_0x2ca[0xA];
    /* +0x2D4 */ u16 held_0x2D4;
    /* +0x2D6 */ u8 pad_0x2d6[0x5F];
    /* +0x335 */ u8 field_0x335;
    /* +0x336 */ u8 pad_0x336[0x1A];
} PlayerPad; /* size: 0x350 */


#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_PLAYER_PAD_H */
