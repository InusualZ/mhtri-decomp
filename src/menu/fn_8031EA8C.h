/*
 * menu/fn_8031EA8C.h - this band's view of the two records its functions drive.
 *
 * The band (`src/menu/fn_8031EA8C.cpp`, `.text` 0x8031EA8C..0x80324F7C) is the continuation of the
 * `menu` selection-screen band below it (`menu/fn_8031A6C0.cpp`): its head is the per-state updater of
 * the selected item's 3D effect instance, and its tail is the surrounding screen's button/keyboard
 * handlers.  Everything here is traced from the range's own disassembly; every offset is one the
 * target instructions address and every width is the load/store the target uses there.
 *
 * `MenuFxWork` is the effect instance's +0x38 work record, cast from `_EFT::work_0x38` (`ef.h`
 * is the owner of `_EFT` itself).  `MenuQuestWork` is this band's view of the 0x2000-byte screen work
 * block the tail functions reset and edit; only the fields the band touches are named.
 */
#ifndef MHTRI_MENU_FN_8031EA8C_H
#define MHTRI_MENU_FN_8031EA8C_H

#include "types.h"
#include "ef.h"
#include "pl.h"

/* The effect instance's `+0x38` work record, in the family view these updaters use: a model count, the
 * `MHchar` model pointers the count covers, the per-model s16 the state machine seeds, and a counter.
 * size: 0x18 lower bound (the highest offset the band touches is +0x16). */
struct MenuFxWork {
    /* +0x00 */ s32 count;
    /* +0x04 */ MHchar* models[3];
    /* +0x10 */ s16 value_0x10[3];
    /* +0x16 */ s16 counter_0x16;
};
/* size: 0x18 (approximate) */

/* The second effect family's `+0x38` view: a single model and the s16 byte `fn_8031F304` seats beside
 * it (the same bytes `MenuFxWork` reads as `models[0]`/`models[1]`; the family spawns one model). */
struct MenuFxWorkB {
    /* +0x00 */ s32 count;
    /* +0x04 */ MHchar* model_0x04;
    /* +0x08 */ s16 value_0x08;
    /* +0x0A */ u8  pad_0x0A[0x06];
};
/* size: 0x10 (approximate) */

/* This band's view of the screen work block at `*(lobby_w + 0xAC)` (0x2000 bytes; the tail functions
 * memset it whole).  The two editable names sit at +0x43 (5 bytes) and +0x48 (0x91 bytes); the edit
 * copies the keyboard writes back to sit at +0x19C and +0x1A6.  Only the offsets the band reads are
 * named - the rest is `pad_0xNN`.
 * size: 0x237 lower bound (the highest offset the band touches is +0x1A6 + 0x91). */
struct MenuQuestWork {
    /* +0x000 */ u8  pad_0x000;
    /* +0x001 */ u8  state_0x001;      /* the keyboard state machine `fn_80321A14` switches on */
    /* +0x002 */ u8  submode_0x002;    /* the sub-state that machine's states switch on */
    /* +0x003 */ u8  pad_0x003[0x007];
    /* +0x00A */ u16 field_0x00A;
    /* +0x00C */ u8  pad_0x00C[0x016];
    /* +0x022 */ s16 value_0x022;
    /* +0x024 */ u8  pad_0x024[0x012];
    /* +0x036 */ u16 field_0x036;      /* the label id the draw pass prints */
    /* +0x038 */ u8  pad_0x038[0x004];
    /* +0x03C */ s16 field_0x03C;
    /* +0x03E */ u8  pad_0x03E[0x002];
    /* +0x040 */ u16 field_0x040;      /* the label id `fn_80321990` mirrors from +0x36 */
    /* +0x042 */ s8  row_0x042;        /* the row cursor, 1..4 `fn_80321A14` clamps */
    /* +0x043 */ char name_0x043[5];   /* the 4-character name + NUL the keyboard edits */
    /* +0x048 */ char text_0x048[0x91];/* the 0x90-character message + NUL the keyboard edits */
    /* +0x0D9 */ u8  pad_0x0D9[0x08B];
    /* +0x164 */ s16 field_0x164;
    /* +0x166 */ s16 field_0x166;      /* 4 or 2, mirrored into +0x42 */
    /* +0x168 */ u32 field_0x168;
    /* +0x16C */ u32 field_0x16C;
    /* +0x170 */ u32 field_0x170;
    /* +0x174 */ u8  pad_0x174[0x026];
    /* +0x19A */ u16 field_0x19A;
    /* +0x19C */ char edit_name_0x19C[5];
    /* +0x1A1 */ u8  pad_0x1A1[0x005];
    /* +0x1A6 */ char edit_text_0x1A6[0x91];
    /* +0x237 */ u8  pad_0x237[0x111];
    /* +0x348 */ u32 field_0x348;
};
/* size: 0x237 (approximate: the highest offset this band touches) */

/* The label table `lbl_806BE120` (0x110 B, .bss): the screen writes its seventeen label words, which
 * start at +0xEE. */
struct MenuLabelTable {
    /* +0x000 */ u8  pad_0x000[0xEE];
    /* +0x0EE */ u16 labels_0x0EE[17];
};
/* size: 0x110 */

/* `lobby_w`'s `+0x48` word: the lobby work block is declared by `unsplit/lobby.h`, whose view stops
 * short of +0x48, so this band reads it through its own view. */
struct MenuLobbyView {
    /* +0x00 */ u8  pad_0x00[0x48];
    /* +0x48 */ u16 field_0x48;
};

#endif /* MHTRI_MENU_FN_8031EA8C_H */
