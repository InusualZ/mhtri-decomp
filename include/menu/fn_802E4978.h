/*
 * `menu/fn_802E4978.cpp`'s records and outbound declarations.
 *
 * The unit is the in-game cockpit/HUD band: `fn_802E4978` builds one player's cockpit work in the
 * global `cockpit_work` array (2 entries, stride 0x194), `fn_802E4AD4`/`fn_802E4B8C` pick which
 * player views it, and the rest drive the item bar, the quest text and the action-button prompt.
 *
 * The 0x194-byte work record and the two records it points at are this unit's own view; where a
 * record is also read by `Pl`/`enemy` the owner header (`include/pl.h`'s `_PLW`) carries the fuller
 * layout and this file keeps only the offsets it reads (a view, folded by the next pass - rule 1).
 *
 * The outbound block at the bottom is rule 2's blocked case: the callees whose owner's header cannot
 * be included from this translation unit, with the clash that blocks each named there.
 */
#ifndef MHTRI_MENU_FN_802E4978_H
#define MHTRI_MENU_FN_802E4978_H

#include "types.h"

/* The player move-work record `get_move_work_adrs` hands back (0xB20 B): only the bytes this unit
 * reads are named. size: 0xB20 */
typedef struct CockpitMove {
    /* +0x000 */ u8 field_0x000;   /* non-zero once the slot is live */
    /* +0x001 */ u8 pad_0x001[0x008 - 0x001];
    /* +0x008 */ u8 field_0x008;
    /* +0x009 */ u8 unused_0x009;
    /* +0x00A */ u8 field_0x00A;
    /* +0x00B */ u8 unused_0x00B;
    /* +0x00C */ u8 pad_0x00C[0x0EF - 0x00C];
    /* +0x0EF */ u8 field_0x0EF;
    /* +0x0F0 */ u8 pad_0x0F0[0x304 - 0x0F0];
    /* +0x304 */ u16 field_0x304;
    /* +0x306 */ u8 pad_0x306[0x308 - 0x306];
    /* +0x308 */ u8 field_0x308;
    /* +0x309 */ u8 field_0x309;
    /* +0x30A */ u8 pad_0x30A[0x36C - 0x30A];
    /* +0x36C */ s16 field_0x36C;
    /* +0x36E */ u8 pad_0x36E[0x3A4 - 0x36E];
    /* +0x3A4 */ u8 field_0x3A4;
    /* +0x3A5 */ u8 field_0x3A5;
    /* +0x3A6 */ u8 pad_0x3A6[0x3C4 - 0x3A6];
    /* +0x3C4 */ u8 field_0x3C4;
    /* +0x3C5 */ u8 pad_0x3C5[0x3E4 - 0x3C5];
    /* +0x3E4 */ u8 field_0x3E4;
    /* +0x3E5 */ u8 pad_0x3E5[0x3E9 - 0x3E5];
    /* +0x3E9 */ u8 field_0x3E9;
    /* +0x3EA */ u8 pad_0x3EA[0x5BC - 0x3EA];
    union {
        /* +0x5BC */ u32 field_0x5BC;
        struct {
            /* +0x5BC */ u8 byte_0x5BC;
            /* +0x5BD */ u8 field_0x5BD;
            /* +0x5BE */ u8 field_0x5BE;
            /* +0x5BF */ u8 byte_0x5BF;
        };
    };
    /* +0x5C0 */ u8 pad_0x5C0[0xB20 - 0x5C0];
} CockpitMove;

/* The one-player move record `get_move_work_adrs(0)` hands back; this unit reads only its +0xEF
 * flag.  Kept separate so the (0) and (2) kinds stay distinguishable (the record sizes differ).
 * size: 0xB20 */
typedef struct CockpitMove0 {
    /* +0x000 */ u8 pad_0x000[0xEF];
    /* +0x0EF */ u8 field_0x0EF;
    /* +0x0F0 */ u8 pad_0x0F0[0xB20 - 0x0F0];
} CockpitMove0; /* size: 0xB20 */

/* The text/colour block inside one cockpit work record, at +0x2C (both `fn_802E4C5C` and the
 * frame counters index it by this base). size: 0x1A (to the record's next named member) */
typedef struct CockpitText {
    /* +0x000 */ u8 unused_0x000[0x00C];
    /* +0x00C */ u8 field_0x00C;   /* frame counter, wraps at 190 */
    /* +0x00D */ u8 field_0x00D;   /* frame counter, wraps at 90 */
    /* +0x00E */ u8 field_0x00E;   /* frame counter, wraps at 50 */
    /* +0x00F */ u8 field_0x00F;   /* the scaled float result the bar width is drawn with */
    /* +0x010 */ u32 colour_0x010; /* the packed colour the name/bar is drawn in */
    /* +0x014 */ u8 field_0x014;   /* the previous item id (re-detect when it changes) */
    /* +0x015 */ u8 field_0x015;   /* the current item id */
    /* +0x016 */ u8 field_0x016;   /* set when the entry cannot be used */
    /* +0x017 */ u8 field_0x017;   /* set when the sub-menu is open */
    /* +0x018 */ u8 field_0x018;
    /* +0x019 */ u8 field_0x019;   /* the item's sub index the icon is drawn with */
} CockpitText; /* size: 0x1C (0x1A named, rounded to the u32 member's alignment) */

/* The menu frame work `fn_802D27E0` returns; only the one byte this unit reads is named.
 * size: 0x3C8 (approximate: max touched offset + 1, 4-aligned) */
typedef struct MenuWorkView {
    /* +0x000 */ u8 unused_0x000[0x3C4];
    /* +0x3C4 */ u8 field_0x3C4;
    /* +0x3C5 */ u8 unused_0x3C5[0x3C8 - 0x3C5];
} MenuWorkView;

/* One player's cockpit work (`cockpit_work` holds two, stride 0x194). Only the bytes this unit
 * touches are named. size: 0x194 */
typedef struct CockpitWork {
    /* +0x000 */ CockpitMove* move;      /* the player move record this work is drawn for */
    /* +0x004 */ u8 unused_0x004[0x008 - 0x004];
    /* +0x008 */ u32 field_0x008;
    /* +0x00C */ u8 unused_0x00C[0x019 - 0x00C];
    /* +0x019 */ u8 field_0x019;
    /* +0x01A */ u8 unused_0x01A;
    /* +0x01B */ u8 field_0x01B;
    /* +0x01C */ u8 unused_0x01C;
    /* +0x01D */ u8 field_0x01D;
    /* +0x01E */ u8 unused_0x01E;
    /* +0x01F */ u8 field_0x01F;
    /* +0x020 */ u8 unused_0x020;
    /* +0x021 */ u8 field_0x021;
    /* +0x022 */ u8 unused_0x022;
    /* +0x023 */ u8 field_0x023;
    /* +0x024 */ u8 unused_0x024;
    /* +0x025 */ u8 field_0x025;
    /* +0x026 */ u8 unused_0x026;
    /* +0x027 */ u8 field_0x027;
    /* +0x028 */ u8 unused_0x028;
    /* +0x029 */ u8 field_0x029;
    /* +0x02A */ u8 unused_0x02A;
    /* +0x02B */ u8 field_0x02B;
    /* +0x02C */ CockpitText text;
    /* +0x048 */ MenuWorkView* field_0x048; /* the menu work `fn_802D27E0` hands back */
    /* +0x04C */ u32 field_0x04C;
    /* +0x050 */ u32 field_0x050;
    /* +0x054 */ u8 unused_0x054[0x0BE - 0x054];
    /* +0x0BE */ s16 field_0x0BE;
    /* +0x0C0 */ s16 field_0x0C0;
    /* +0x0C2 */ s16 field_0x0C2;
    /* +0x0C4 */ s16 field_0x0C4;
    /* +0x0C6 */ s16 field_0x0C6;
    /* +0x0C8 */ u8 unused_0x0C8[0x0CD - 0x0C8];
    /* +0x0CD */ u8 field_0x0CD;
    /* +0x0CE */ u8 field_0x0CE;
    /* +0x0CF */ u8 field_0x0CF;
    /* +0x0D0 */ u8 unused_0x0D0;
    /* +0x0D1 */ u8 field_0x0D1;
    /* +0x0D2 */ u8 unused_0x0D2[0x134 - 0x0D2];
    /* +0x134 */ s16 field_0x134;
    /* +0x136 */ u8 field_0x136;
    /* +0x137 */ u8 field_0x137;
    /* +0x138 */ u8 field_0x138;
    /* +0x139 */ u8 field_0x139;
    /* +0x13A */ u8 field_0x13A;
    /* +0x13B */ u8 unused_0x13B;
    /* +0x13C */ s16 field_0x13C;
    /* +0x13E */ u8 unused_0x13E[0x164 - 0x13E];
    /* +0x164 */ u8 field_0x164;
    /* +0x165 */ u8 unused_0x165;
    /* +0x166 */ u8 field_0x166;
    /* +0x167 */ u8 field_0x167;
    /* +0x168 */ s16 field_0x168;
    /* +0x16A */ s8 field_0x16A;
    /* +0x16B */ u8 field_0x16B;
    /* +0x16C */ u8 unused_0x16C[0x16F - 0x16C];
    /* +0x16F */ u8 field_0x16F;
    /* +0x170 */ u8 unused_0x170[0x174 - 0x170];
    /* +0x174 */ s16 field_0x174;
    /* +0x176 */ u8 field_0x176;
    /* +0x177 */ u8 field_0x177;
    /* +0x178 */ u8 field_0x178;
    /* +0x179 */ u8 field_0x179;
    /* +0x17A */ u8 field_0x17A;
    /* +0x17B */ u8 field_0x17B;
    /* +0x17C */ u8 field_0x17C;
    /* +0x17D */ u8 field_0x17D;
    /* +0x17E */ u8 field_0x17E;
    /* +0x17F */ u8 field_0x17F;
    union {
        /* +0x180 */ u8 byte_0x180;
        /* +0x180 */ u16 half_0x180;
    };
    /* +0x181 */ u8 unused_0x181[0x184 - 0x181];
    /* +0x184 */ u32 field_0x184;
    /* +0x188 */ u32 field_0x188;
    /* +0x18C */ s16 field_0x18C;
    /* +0x18E */ s16 field_0x18E;
    /* +0x190 */ u32 field_0x190;
} CockpitWork;

/* The one-player cockpit state at `cockpit_state` (the sub-screen selector). size: 0x88 */
typedef struct CockpitState {
    /* +0x000 */ u8 field_0x000;   /* 1 for the item view, 2 for the equip view */
    /* +0x001 */ u8 unused_0x001;
    /* +0x002 */ s16 field_0x002;
    /* +0x004 */ s16 field_0x004;
    /* +0x006 */ u8 unused_0x006[0x058 - 0x006];
    /* +0x058 */ u8 field_0x058;
    /* +0x059 */ u8 field_0x059;
    /* +0x05A */ u8 field_0x05A;
    /* +0x05B */ u8 unused_0x05B;
    /* +0x05C */ s16 field_0x05C;
    /* +0x05E */ u8 unused_0x05E[0x070 - 0x05E];
    /* +0x070 */ u32 field_0x070;
    /* +0x074 */ u8 field_0x074;
    /* +0x075 */ u8 unused_0x075[0x078 - 0x075];
    /* +0x078 */ u32 field_0x078;
} CockpitState;

/* The player move work `get_move_work_adrs` hands back. */
void* get_move_work_adrs(u8 kind);
u32 get_move_work_max(u8 kind);

/* Outbound callees a registered unit owns whose header cannot be included from here (rule 2's blocked
 * case - the owner's header is unusable in this translation unit, and the declaration cannot sit in
 * `include/unsplit/menu.h` at all):
 *
 *   * `include/hud/fn_802EBED8.h` (0x802EC4F0-0x802EF730), `include/hud/cockpit_quest.h`
 *     (0x802E796C-0x802EA33C) and `include/hud/layout.h` (0x802E0B54/0x802E270C): all three clash with
 *     `include/unsplit/lobby.h`, which this unit needs for `drawshape_*`/`draw_sprite_*`, on
 *     `_mh_ivec2_`, `spr_data_copy`, `fn_802E0DA8`, `draw_sprite_anim_ary`, `get_move_work_adrs` and
 *     `get_move_work_max`; `hud/cockpit_quest.h` additionally redefines `CockpitWork` and declares
 *     `cockpit_work` with its own record type.
 *   * `include/ai/fn_802D0F34.h` (0x802D27E0): redefines `_HIT_W` against `include/menu/menu_item.h`
 *     and declares `get_move_work_adrs` as `u8*` against this header's `void*`.
 *   * `include/mh3_pad.h` (0x80046F0C): clashes with `include/pl.h` on its own pre-existing
 *     `setVec3` declaration.
 *
 * Each is a `shared-file` request in the unit's outbox, `fn_80050BC0`'s included.  The signatures are
 * this unit's call sites, measured against the target object (a change here moves a row, so they must
 * not be "tidied"). */
#ifdef __cplusplus
extern "C" {
#endif

u32 fn_80046F0C(void*);                /* 0x80046F0C mh3_pad.cpp */
s32 fn_802D27E0(void);                  /* 0x802D27E0 ai/fn_802D0F34.cpp */
s32 fn_802E0B54(u16);                   /* 0x802E0B54 hud/layout.cpp */
u32 color_lerp(u32, u32, u8, f32, f32);/* 0x802E270C hud/layout.cpp */
/* 0x80050BC0 - `include/fn_8004CAD8.h` settles ONE float argument (from the callee's own body), and
 * that is the honest declaration; it cannot be used here yet: this unit's call site reproduces the
 * target's register allocation only with the 3-argument view, measured on `fn_802E5D68` at 95.675674
 * against 95.47298 for the one-argument form (`fn_80050BC0((f32)(sq + dy * dy))`, and the same with
 * `(f32)(dx * dx + dy * dy)`).  Both measurements are in the outbox. */
f32 fn_80050BC0(s32, s32, f32);
void fn_80053960(u32, s32, s32, u32);   /* 0x80053960 fn_8004CAD8.cpp */
void drawshape_set_offset_ivec2(s16*);                  /* 0x80054178 fn_8004CAD8.cpp */
void quest_gauge_update(void*, s32);           /* 0x802E796C hud/cockpit_quest.cpp */
void quest_gauge_draw(void);                 /* 0x802E7FA0 hud/cockpit_quest.cpp */
void quest_targets_update_b(void*);                /* 0x802E8C8C hud/cockpit_quest.cpp */
void quest_marker_arm(void);                 /* 0x802E8E64 hud/cockpit_quest.cpp */
void quest_marks_flush(void*);                /* 0x802EA33C hud/cockpit_quest.cpp */
void fn_802EC4F0(void*);                /* 0x802EC4F0 hud/fn_802EBED8.cpp */
s32 fn_802EC6C4(void*);                 /* 0x802EC6C4 hud/fn_802EBED8.cpp */
void fn_802EC700(void);                 /* 0x802EC700 hud/fn_802EBED8.cpp */
void fn_802ED480(void);                 /* 0x802ED480 hud/fn_802EBED8.cpp */
void fn_802ED88C(void);                 /* 0x802ED88C hud/fn_802EBED8.cpp */
void fn_802EDAF0(void*, void*, void*);  /* 0x802EDAF0 hud/fn_802EBED8.cpp */
void fn_802EDB0C(void*, void*, void*);  /* 0x802EDB0C hud/fn_802EBED8.cpp */
void fn_802EDCE4(void*, void*);         /* 0x802EDCE4 hud/fn_802EBED8.cpp */
void fn_802EE65C(void*, void*);         /* 0x802EE65C hud/fn_802EBED8.cpp */
void fn_802EE82C(void*);                /* 0x802EE82C hud/fn_802EBED8.cpp */
void fn_802EF0FC(void*);                /* 0x802EF0FC hud/fn_802EBED8.cpp */
void fn_802EF230(void);                 /* 0x802EF230 hud/fn_802EBED8.cpp */
void fn_802EF424(void);                 /* 0x802EF424 hud/fn_802EBED8.cpp */
void fn_802EF6B0(void);                 /* 0x802EF6B0 hud/fn_802EBED8.cpp */
void fn_802EF730(void);                 /* 0x802EF730 hud/fn_802EBED8.cpp */
/* 0x8033A850 `lobby/lb_companion_ui.cpp` (the companion/status UI band, formerly the band entry
 * `fn_8033A850` in `include/unsplit/menu.h` - the band may not carry a symbol a registered unit
 * owns).  The owner's header cannot be included here: it redefines `_mh_ivec2_` against
 * `include/unsplit/lobby.h`, which this unit needs (measured - `(10296) class '_mh_ivec2_'
 * redefined`).  The zero-argument signature is this unit's call site. */
s32 lb_quest_work_active_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_FN_802E4978_H */
