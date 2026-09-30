/*
 * `lobby_w` - the lobby work block (.bss 0x806AAB44, 0x17C B), defined by `src/lobby/fn_8021E1EC.cpp`
 * (its `.bss` 0x806AA8C8-0x806AACC0; the static constructor `fn_8021FF5C` calls `fn_8021FFFC(&lobby_w)`).
 * The one home of the record type (rule 1; the object is declared in `lobby_w.h`).  It merges the views the
 * lobby band carried: `include/unsplit/lobby.h`'s (the menu pointer, the NPC band's +0x76/+0x77 state
 * bytes, the act-layer hold at +0x12C), and `lobby/fn_8021E1EC.cpp`'s (+0x027, +0x052, +0x0B1).  Only the
 * named offsets are read by matched code.
 */
#ifndef MHTRI_LOBBY_LOBBY_WORK_H
#define MHTRI_LOBBY_LOBBY_WORK_H

#include "types.h"

struct LbMenuWork;

typedef struct LbLobbyWork {
    /* +0x000 */ u8 state_0x000;
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ union {
        u8 field_0x002;   /* the menu-layer state `src/lobby/fn_801F9CD4.cpp` sets (0/2) and passes to
                           * `fn_801FB2A8`/`fn_802AEEF8` */
        u8 area_0x002;    /* the scene's area id, compared against `_PLW::area_0x16`
                           * (`src/lobby/fn_802076D4.cpp`) - the same byte, two consumers */
    };
    /* +0x003 */ u8 unused_0x003[3];
    /* +0x006 */ u8 field_0x006;
    /* +0x007 */ u8 unused_0x007[0x05];
    /* +0x00C */ u32 slots_0x00C[2];
    /* +0x014 */ u8 field_0x014;
    /* +0x015 */ u8 unused_0x015[0x12];
    /* +0x027 */ u8 flag_0x027;
    /* +0x028 */ u8 unused_0x028[0x2A];
    /* +0x052 */ u8 page_0x052;
    /* +0x053 */ u8 unused_0x053[0x23];
    /* +0x076 */ u8 field_0x076;
    /* +0x077 */ u8 field_0x077;
    /* +0x078 */ u8 unused_0x078[0x5];
    /* +0x07D */ u8 slots_0x07D[0x2F];
    /* +0x0AC */ struct LbMenuWork* menu_0xAC;
    /* +0x0B0 */ u8 field_0x0B0;
    /* +0x0B1 */ u8 sub_0x0B1;
    /* +0x0B2 */ u8 unused_0x0B2[0x7A];
    /* +0x12C */ u8 field_0x12C;   /* 1 puts the lobby act layer on hold */
    /* +0x12D */ u8 param_0x12D;
    /* +0x12E */ u8 unused_0x12E[0x1];
    /* +0x12F */ u8 param_0x12F;
    /* +0x130 */ u8 unused_0x130[0x2C];
    /* +0x15C */ u8 field_0x15C;
    /* +0x15D */ u8 field_0x15D;
    /* +0x15E */ u8 field_0x15E;
    /* +0x15F */ u8 field_0x15F;
    /* +0x160 */ u8 field_0x160;
    /* +0x161 */ u8 field_0x161;
    /* +0x162 */ u8 unused_0x162[0x14];
    /* +0x176 */ u8 field_0x176;
    /* +0x177 */ u8 unused_0x177[0x5];
} LbLobbyWork; /* size: 0x17C */

#endif /* MHTRI_LOBBY_LOBBY_WORK_H */
