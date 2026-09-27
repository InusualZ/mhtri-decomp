/* lobby/fn_801EC9F8.cpp - the lobby band's digit-entry / item-selection pane group.
 *
 * `.text` 0x801EC9F8..0x801F3294 (67 functions, 26780 B), extab 0x8001079C..0x8001094C (54 unwind-only
 * 8-byte records), extabindex 0x8002CA54..0x8002CCDC (54 x 12 B).  Registered from
 * `proposal/801EC9F8_fn_801EC9F8.cpp`; that proposal's edge is a `--max-bytes` size cap, not a
 * translation-unit boundary, and `tudiscover` finds no anchor in either direction, so the extent
 * settles as the functions match.
 *
 * Module `lobby`: both bracketing registered units are `lobby` (`lobby_scene.c` above,
 * `lobby/lb_npc.cpp` below), the range reads `lobby_w` (+0xAC menu pointer, +0x84/+0x86 counters) and
 * the lobby item database `lbl_80794880`, and its callees are the lobby UI API.
 *
 * Name.  The map has only `fn_XXXXXXXX` for this range, the runtime dump answers only `zz_` placeholders
 * (`dumpmap.py lookup 0x801EC9F8` -> `zz_01ec9f8_`), and no `__FILE__`/assert string covers the range
 * (its only data references are the numeric tables `lbl_805B85D8`/`lbl_805B8618`/`lbl_805B8638` and the
 * switch table `jumptable_805B8B98`), so the file keeps the map's stem (brief section 2, class 4).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit.py range
 * over the proposal inventory and dumpmap.py lookup on the range's addresses: all 67 names are bare
 * `.text` entries and the runtime dump has only `zz_XXXXXXXX_` placeholders for them)
 *
 * Flags.  The unit needs two pragmas the lib's `cflags_lobby` does not set, both scoped to this file:
 *   `#pragma peephole off`   retail keeps the unfused `clrlwi` + `rlwinm` + `cmpwi` triple where -O3's
 *                            peephole folds it into a record-form `rlwinm.` - fn_801EC9F8 measured
 *                            78.90 % with the pass on and 100.00 % with it off (256 -> 308 B).
 *   `#pragma dont_inline on` retail keeps `fn_801ED3E8`/`fn_801ED464` out of line; with `-inline auto`
 *                            they are inlined into fn_801ED688 (860 -> 1556 B, 7.00 %); with the pragma
 *                            that function measures 95.86 %.
 * A lib-level `flag` request is filed for both (two lobby units would have to agree before `cflags_lobby`
 * moves); until then the pragmas keep the deviation inside this unit.
 *
 * State.  31 of the 67 functions are reconstructed (all >= 83 %, six at 100 %): the digit-entry pane
 * (0x801ECD50..0x801ED200), the item-list pane (0x801ED284..0x801EDB34) and the two step machines
 * (0x801EDC4C/0x801EE1E4/0x801EEDC4/0x801EEFA0).  Unit: 33.03 % fuzzy, 632 / 26780 B matched.
 * Not reconstructed yet - the trailing 36 functions, 0x801EE988..0x801F3294 (fn_801EE988, fn_801EF3B0,
 * fn_801EF760, fn_801EFD74..fn_801F3294): their object types (a third pane with a 0x650-byte config
 * block, the icon/attribute run at 0x801F085C) still need deriving, so no body is guessed.
 *
 * Residuals in the reconstructed set (all allocator/scheduling deltas, no structural gap):
 *   fn_801ED56C 83.00, fn_801EDA34 83.27, fn_801ED184 84.35, fn_801EDC4C 85.86, fn_801EF244 88.08,
 *   fn_801EE1E4 88.20, fn_801EF73C 88.33, fn_801ED9E4 88.50, fn_801ED200 87.12, fn_801ED464 92.31,
 *   fn_801ED048 92.77, fn_801ECEF0 93.03, fn_801EDA9C 93.95, fn_801ED284 95.28, fn_801ECF74 96.42,
 *   fn_801ECD50 96.82, fn_801ECDD4 97.04, fn_801EDB34 97.43, fn_801ECB2C 98.00, fn_801ECB40 98.41.
 * At 100 %: fn_801EC9F8, fn_801ED160, fn_801ED3E8, fn_801ED53C, fn_801ED554, fn_801EF1D0.
 *
 * Types.  The two panes and the item database are this range's own views (`LbDigitPane`, `LbListPane`,
 * `LbEquipPane`, `LbItemDb`, `LbConfigWork`); `lobby_w.menu_0xAC` is also read by
 * `lobby/fn_801E7530.cpp`, whose `LbMenuWork` in `include/unsplit/lobby.h` marks +0x02/+0x06/+0x0A as
 * padding where this range reads them - filed as a shared-file request to merge the two views.
 * `include/unsplit/lobby.h` itself cannot be included: its `fn_8021213C(s32, s16)` takes two arguments
 * where this range passes one, and its `fn_802A8EC0` takes `s16`s where the call sites pass `u8`s, so
 * the callee declarations live in this file (plain prototypes, one shared-file request each).
 */
#include "types.h"

#include "Runtime.PPCEABI.H/memset.h"

#pragma peephole off
#pragma dont_inline on

/* ------------------------------------------------------------------------------------------------
 * The two work records this range drives.  Both are this range's own objects (no other registered unit
 * reads them), so their layouts are stated here rather than in a shared header.
 */

/* The digit-entry pane `fn_801ECD50` initialises and `fn_801ED048`/`fn_801ECDD4` step: three decimal
 * digits (hundreds/tens/ones), two blink timers and the pick/cursor pair.  Layout from the object's
 * own stores (`fn_801ECD50`), its field widths (`stb`/`sth`/`stw` at 0x1C/0x0A/0x04) and the accessors
 * `fn_801ED160`/`fn_801ECEF0`. */
typedef struct LbDigitPane {
    /* +0x00 */ u8 phase_0x00;          /* the step switch `fn_801EDC4C`/`fn_801EEDC4` dispatch on */
    /* +0x01 */ u8 sub_0x01;
    /* +0x02 */ u8 digit_hundreds_0x02;
    /* +0x03 */ u8 digit_hundreds_max_0x03;
    /* +0x04 */ s32 stepper_0x04;       /* the 4-byte stepper state `fn_802A8F50` walks */
    /* +0x08 */ u16 flags_0x08;         /* bit 0x100 / 0x200 arm the two blink timers */
    /* +0x0A */ s16 digit_ones_0x0A;
    /* +0x0C */ s16 digit_tens_0x0C;
    /* +0x0E */ s16 anim_0x0E;          /* the value `fn_801EC804` animates */
    /* +0x10 */ s16 timer_0x10;
    /* +0x12 */ s16 blink_a_0x12;       /* -1 when idle, ticks 0..6 */
    /* +0x14 */ s16 blink_b_0x14;
    /* +0x16 */ s16 pick_0x16;
    /* +0x18 */ s16 pick_max_0x18;
    /* +0x1A */ s16 cursor_0x1A;
    /* +0x1C */ u8 active_0x1C;         /* the "a pane is open" latch `fn_801ED048` waits on */
    /* +0x1D */ u8 done_0x1D;
} LbDigitPane; /* size: 0x1E (the extent this range reads) */

/* The 0x194-byte item list `fn_801ECF74` builds at `LbListPane::items_0x3B8`; the two words at +0x190
 * are the list's own tail flags. */
typedef struct LbItemList {
    /* +0x000 */ u8 entries_0x000[0x190];
    /* +0x190 */ s16 tail_a_0x190;
    /* +0x192 */ s16 tail_b_0x192;
} LbItemList; /* size: 0x194 */

/* The item-list pane `fn_801ED688`/`fn_801EE1E4` walk and `fn_801ECF74` fills: the cursor pair, the
 * two stepper states the pad moves, the selected item id, and the 0x194-byte entry list at +0x3B8. */
typedef struct LbListPane {
    /* +0x000 */ u8 unused_0x000;
    /* +0x001 */ u8 phase_0x001;        /* the step switch `fn_801ED688`/`fn_801EE1E4` dispatch on */
    /* +0x002 */ u8 unused_0x002[3];
    /* +0x005 */ u8 side_0x005;         /* which of the two panes (0/1) the item comes from */
    /* +0x006 */ u8 unused_0x006[0xA];
    /* +0x010 */ void* data_0x010;      /* the equipment work block `fn_801ED284` backs up */
    /* +0x014 */ s16 row_0x014;         /* the cursor row */
    /* +0x016 */ u8 row_sub_0x016;      /* the 8-step row pair `fn_802A8EC0` walks */
    /* +0x017 */ u8 row_sub_max_0x017;
    /* +0x018 */ s16 column_0x018;      /* the cursor column */
    /* +0x01A */ u8 column_sub_0x01A;   /* the 8-step column pair `fn_802A8EFC` walks */
    /* +0x01B */ u8 column_sub_max_0x01B;
    /* +0x01C */ s16 move_0x01C;        /* the pad-driven 4-step state at +0x1C (fn_802A8EC0's base) */
    /* +0x01E */ s16 move_max_0x01E;
    /* +0x020 */ s16 entry_0x020;       /* the 4-step state at +0x20 (fn_802A91AC's cursor base) */
    /* +0x022 */ s16 entry_max_0x022;
    /* +0x024 */ u16 item_id_0x024;
    /* +0x026 */ u8 active_0x026;       /* the "confirm is open" latch `fn_801ED56C` waits on */
    /* +0x027 */ u8 unused_0x027[5];
    /* +0x02C */ s32 stepper_0x02C;     /* the 4-byte stepper state `fn_802A8F50` walks */
    /* +0x030 */ u8 unused_0x030[0x388];
    /* +0x3B8 */ LbItemList items_0x3B8; /* the 0x65-entry list `fn_801ECF74` fills */
} LbListPane; /* size: 0x54C (the extent this range reads) */

/* One 4-byte row of the item database's slot table at +0x180. */
typedef struct LbItemSlot {
    /* +0x00 */ u16 id_0x00;
    /* +0x02 */ s16 value_0x02;
} LbItemSlot; /* size: 0x4 */

/* The equipment work block `LbListPane::data_0x010` points at: the working eight-item id/kind run and
 * the backup run `fn_801ED284` copies it into. */
typedef struct LbConfigWork {
    /* +0x0000 */ u8 unused_0x0000[0x5F2];
    /* +0x05F2 */ u16 ids_0x5F2[8];
    /* +0x0602 */ u8 kinds_0x602[8];
    /* +0x060A */ u8 unused_0x060A[0x10];
    /* +0x061A */ u16 backup_ids_0x61A[8];
    /* +0x062A */ u8 backup_kinds_0x62A[8];
} LbConfigWork; /* size: 0x632+ (the extent this range reads) */

/* The equipment pane `fn_801EF1D0`/`fn_801EF244`/`fn_801EF3B0` drive: the three equipped ids, the
 * cursor pair and the two 4-step states. */
typedef struct LbEquipPane {
    /* +0x00 */ u8 phase_0x00;
    /* +0x01 */ u8 side_0x01;
    /* +0x02 */ s16 index_0x02;         /* the id slot the cursor is on */
    /* +0x04 */ u16 ids_0x04[3];        /* the three equipped item ids (0xFFFF = empty) */
    /* +0x0A */ u8 unused_0x0A[2];
    /* +0x0C */ s32 stepper_0x0C;       /* the 4-byte stepper state `fn_802A8F50` walks */
    /* +0x10 */ u8 icon_0x10[6];        /* the icon run `fn_80219080` fills */
    /* +0x16 */ s16 move_0x16;          /* the 4-step state `fn_802A8EC0`'s base is at */
    /* +0x18 */ s16 move_max_0x18;
    /* +0x1A */ s16 cursor_0x1A;
    /* +0x1C */ u8 unused_0x1C[4];
    /* +0x20 */ u8 flags_0x20;
    /* +0x21 */ u8 unused_0x21[0x6F];
    /* +0x90 */ s32 icon_index_0x90;
} LbEquipPane; /* size: 0x94 (the extent this range reads) */

/* The lobby menu work object `lobby_w.menu_0xAC` points at.  `fn_801ECB40` is the only reader in this
 * range, so only the six fields it drives are named; the full record belongs to
 * `lobby/fn_801E7530.cpp`, which owns `LbMenuWork` in `include/unsplit/lobby.h` (recorded as a
 * shared-file request: these two views of +0x02/+0x06/+0x0A should collapse into one). */
typedef struct LbMenuPanel {
    /* +0x00 */ u8 state_0x00;
    /* +0x01 */ u8 mode_0x01;
    /* +0x02 */ u8 list_kind_0x02;     /* 0 while the panel is still on its first page */
    /* +0x03 */ u8 unused_0x03;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 unused_0x05;
    /* +0x06 */ s16 row_0x06;          /* the selection row, an index into `lbl_805B85D8` */
    /* +0x08 */ s16 cursor_0x08;
    /* +0x0A */ s16 page_0x0A;         /* 0/1 page the up/down keys flip */
    /* +0x0C */ s16 row_max_0x0C;
    /* +0x0E */ s16 stage_0x0E;        /* the stage id the row selects */
} LbMenuPanel; /* size: 0x10 (the extent this range reads) */

/* The lobby work block (`lobby_w`, 0x806AAB44, 0x17C bytes in the map).  This range reads the menu
 * pointer and the two u16 counters `fn_802A91AC` takes. */
typedef struct LbLobbyRoot {
    /* +0x000 */ u8 unused_0x000[0x84];
    /* +0x084 */ u16 count_0x084;      /* the item count `fn_802A91AC` takes as its pad argument */
    /* +0x086 */ u16 count_b_0x086;
    /* +0x088 */ u8 unused_0x088[0x24];
    /* +0x0AC */ LbMenuPanel* menu_0x0AC;
    /* +0x0B0 */ u8 unused_0x0B0[0xCC];
} LbLobbyRoot; /* size: 0x17C (the map's own record) */

/* One 0xC-byte equipment record (the `fn_8004A20C`/`fn_8004BD58` copy unit and `Gunner_opt_ok_ck`'s
 * argument).  Only the kind byte is read here. */
typedef struct LbEquipRec {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u16 value_0x02;
    /* +0x04 */ u8 unused_0x04[8];
} LbEquipRec; /* size: 0xC */

/* The lobby item database `lbl_80794880` (a 4-byte pointer in `.sbss`) points at: the item slot table
 * at +0x180 (`fn_8004AFC8`/`fn_8004AF60`/`fn_8004AF78` index it), the 0xC-byte equipment records at
 * +0xE00, and the current-equipment kind at +0x8C. */
typedef struct LbItemDb {
    /* +0x0000 */ u8 unused_0x0000[0x18];
    /* +0x0018 */ u8 data_0x18[0x74];  /* the block `fn_8004D0E8` is handed as a work area */
    /* +0x008C */ s16 kind_0x8C;       /* the equipped item's kind (0xB is a bowgun) */
    /* +0x008E */ s16 sub_0x8E;
    /* +0x0090 */ s16 sub_0x90;
    /* +0x0092 */ u8 unused_0x0092[0xEE];
    /* +0x0180 */ LbItemSlot slots_0x180[0x320]; /* the slot table the item calls index */
    /* +0x0E00 */ LbEquipRec equip_0x0E00[0x100]; /* the 0xC-byte equipment records */
} LbItemDb; /* size: 0x1A00+ (only the runs this range indexes are named) */

/* ------------------------------------------------------------------------------------------------
 * The symbols this range reads.  Everything below is a plain prototype at file scope (the project's
 * convention for callees whose owner has no publishable header); each one is filed as a shared-file
 * request.  `lobby_w` and `lbl_80794880` are re-declared here with this range's own view because the
 * callee signatures `include/unsplit/lobby.h` publishes for `fn_802A8EC0`/`fn_8021213C` do not match
 * the ones this range calls (the header declares `fn_8021213C(s32, s16)` where this range passes one
 * argument), so including it cannot compile.
 */
extern "C" {
/* This range's own functions, called by earlier bodies. */
s32 fn_801EC9F8(s16* x, s16* y, s16 x_min, s16 x_max, s16 y_min, s16 y_max, u16 buttons);
void fn_801ECB2C(s16* x, s16* y);
s32 fn_801ECB40(void);
void fn_801ECD50(LbDigitPane* self);
void fn_801ECDD4(LbDigitPane* self);
void fn_801ECEF0(LbDigitPane* self);
void fn_801ECF74(LbListPane* self);
s32 fn_801ED048(LbDigitPane* self, LbListPane* list);
s16 fn_801ED160(LbDigitPane* self);
s16 fn_801ED184(s16 index);
s16 fn_801ED200(s16 index, u8 side);
void fn_801ED284(LbListPane* self);
void fn_801ED3E8(LbListPane* self);
void fn_801ED464(LbListPane* self);
s16 fn_801ED53C(LbListPane* self);
s16 fn_801ED554(LbListPane* self);
s32 fn_801ED56C(LbListPane* self, u8 arg);
s32 fn_801ED688(LbListPane* self, u8 arg);
s16 fn_801ED9E4(s16 index);
s16 fn_801EDA34(u16 index, u8 side);
u8 fn_801EDA9C(u16 index, u16 item_id);
u8 fn_801EDB34(LbListPane* self, u16 index, u16 item_id);
s32 fn_801EDC4C(LbDigitPane* self, LbListPane* list);
s32 fn_801EE1E4(LbListPane* self);
u32 fn_801EE900(u16 item_id);
s32 fn_801EE940(u16 index, u16 item_id);
s32 fn_801EEDC4(LbDigitPane* self, LbListPane* list);
s32 fn_801EEFA0(LbDigitPane* self, LbListPane* list);
s32 fn_801EF1D0(LbEquipPane* self);
void fn_801EF244(LbEquipPane* self, s16 a, u16 b, u16 c, u16 d, u8 e, s32 f);
s32 fn_801EF3B0(LbListPane* self, LbListPane* other, LbDigitPane* pane);
u16 fn_801EF73C(LbDigitPane* self);
void fn_801EFD74(LbListPane* self, s16 a, u8 b);
void fn_801EFFD0(LbListPane* self, s16 a, s16 b, s16 c);
s32 fn_801F1710(LbListPane* self, LbListPane* other, LbListPane* pane);
}

/* Foreign data. */
extern LbLobbyRoot lobby_w;
extern LbItemDb* lbl_80794880;
extern s16 lbl_805B85D8[0x30];      /* the row -> stage table `fn_801ECB40` indexes */
extern u8 lbl_805B8618[0x40];
extern u8 lbl_805B8638[0x40];
extern u8 jumptable_805B8B98[0x2C];

extern "C" {
/* The lobby UI API.  Signatures are the caller-side ones the call sites imply (the callee is
 * `NonMatching` in every case, so the width a call site uses is what its codegen needs). */
s32 fn_8004A1F8(LbItemSlot* dst, const LbItemSlot* src);
s32 fn_8004A20C(LbEquipRec* dst, const LbEquipRec* src);
u16 fn_8004AE70(LbItemDb* db);
s16 fn_8004AE98(LbItemDb* db);
u32 fn_8004AEC0(LbItemDb* db, void* p);
s16 fn_8004AF0C(u8 side);
s16 fn_8004AF20(LbItemDb* db);
LbItemSlot* fn_8004AF60(LbItemDb* db, u8 side);
LbItemSlot* fn_8004AF78(LbItemDb* db);
u16 fn_8004AFC8(LbItemDb* db, s16 index);
s16 fn_8004AFFC(LbItemDb* db, s16 index);
s32 fn_8004B460(LbItemDb* db, u8 side);
s16 fn_8004B624(LbItemDb* db, u16 id);
s16 fn_8004B7B0(u16 id, void* slots, u16 count);
s16 fn_8004BA00(u16 id, LbItemSlot* base, s16 count);
s32 fn_8004BA3C(u16 id, s16 a, void* p, s32 b, s32 c, s32 d);
void fn_8004BBEC(LbItemDb* db, u16 id, s16 a, u8 side, s32 b);
s32 fn_8004BD30(void* p);
s32 fn_8004BD58(void* dst, u16 count, u16 id, s16 v, void* tmp);
s32 fn_8004B870(LbItemSlot* slots, u16 index, s16 delta);
s32 fn_8004BEA4(u16 id, s16 a, s16* out);
s32 fn_8004BF28(void* dst, u16 count);
u16 fn_8004C004(void* slots, u16 count);
s32 fn_8004C038(void* p, u16 a, u16 b);
void fn_8004C7BC(void* p, u16 a);
s32 fn_8004CAD8(void);
s32 fn_8004D0E8(s32 a, void* p);
s32 fn_8004D27C(s32 a);
s32 fn_8004D70C(s32 a);
void fn_800DBC84(s32 id);
void sysSE_req(s32 id);
s32 fn_801EC7AC(LbMenuPanel* self);
void fn_801EC804(void* p);
s32 fn_801EC828(LbMenuPanel* self);
s32 fn_801EC85C(LbMenuPanel* self);
void fn_801EC95C(LbMenuPanel* self);
s32 fn_801EC9E0(void);
s32 fn_8021213C(u32 mask);
s32 fn_802121F4(u32 mask);
u16 fn_802122AC(void);
u16 fn_802122E8(void);
u16 fn_80212334(void);
s32 fn_80212250(u32 mask);
s32 fn_80212540(void* p);
s16 fn_80213374(u8 a, s16 b, void* c, u16 d, s32 e, s32 f, u8 g);
s32 fn_80214654(void* p, s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g);
s32 fn_80214798(void* p);
s32 fn_80219080(s32 a, void* p, s32 b, s32 c);
void fn_802190F4(void* p);
void fn_802190FC(void* p, void* a, void* b, void* c, void* d);
s32 fn_80219530(void* p);
void fn_80219590(void* p, void* a);
s32 fn_8021B7EC(s32 a);
s32 fn_802738E8(s32 a);
s32 fn_8027EFB4(u8 kind);
s16 fn_802A8EC0(u8 value, u8 max, u16 pad, s32 step, s32 step2, void* state);
s32 fn_802A8EEC(u8 value, u8 max, u16 pad, s32 a, s32 b, s32 c, void* state);
s16 fn_802A8EFC(s16 value, s16 max, u16 pad, s32 step, s32 step2);
u8 fn_802A8F14(s16 value, s32 max);
s32 fn_802A8F50(void* state, u16 pad, s32 a, s32 b, s32 c);
s32 fn_802A91AC(void* state, s16 index, u16 pad, u16 value, void* step);
void fn_802A98BC(void* list, u16 count);
void fn_802A9BB8(LbItemSlot* p);
s32 fn_802BBAC4(s32 a);
s32 fn_8031BFEC(void* p, s32 a, s32 b);
s32 fn_8031C028(void* p, u16 a);
s32 fn_8031C0B8(void* p);
s32 fn_8031C408(void* p, void* q, u16 a, u16 b, u16 c);
s32 fn_8031C514(void* p, s32 a);
s32 fn_8031C5FC(void* p, u16 a, u16 b);
void fn_8031C8EC(void* p);
s32 fn_8031C934(void* p, u16 a, u16 b);
s32 fn_8033AAFC(void);
s32 fn_8033AC78(u16 a, s32 b);
s32 eft052_item_value_get(u16 id);
s32 fn_8035B700(s32 a, s32 b, s32 c);
u32 fn_803768F8(void);
}

/* C++-linkage callees: their map names are manglings, so the real declaration is what makes the
 * front-end emit them (docs/plan.md 6.5 rule 9). */
u8* GetItemData(u16 id);
s32 Gunner_opt_ok_ck(LbEquipRec* equip);
s32 chk_pointer(void);

/* ------------------------------------------------------------------------------------------------ */

/* Clamps the cursor pair to the given box after applying the D-pad bits of `buttons`; returns whether
 * the pad owned the frame. */
s32 fn_801EC9F8(s16* x, s16* y, s16 x_min, s16 x_max, s16 y_min, s16 y_max, u16 buttons)
{
    s16 vx;
    s16 vy;

    if (fn_803768F8() == 1U) {
        return 0;
    }
    if ((buttons & 0x3C00) != 0) {
        if ((buttons & 0x800) != 0) {
            *x -= 1;
        }
        if ((buttons & 0x400) != 0) {
            *x += 1;
        }
        if ((buttons & 0x2000) != 0) {
            *y -= 1;
        }
        if ((buttons & 0x1000) != 0) {
            *y += 1;
        }
        vx = *x;
        if (vx < x_min) {
            *x = x_min;
        } else if (vx > x_max) {
            *x = x_max;
        }
        vy = *y;
        if (vy < y_min) {
            *y = y_min;
        } else if (vy > y_max) {
            *y = y_max;
        }
        return 1;
    }
    return 0;
}

/* The cursor's default position (100, 45). */
void fn_801ECB2C(s16* x, s16* y)
{
    *x = 100;
    *y = 45;
}

/* The lobby menu panel's frame step: leaves on the cancel button, otherwise advances the two pages. */
s32 fn_801ECB40(void)
{
    LbMenuPanel* panel;
    s32 result;
    s16 row;

    panel = lobby_w.menu_0x0AC;
    result = 0;
    if (fn_8021213C(0x80) == 1U && panel->list_kind_0x02 == 0) {
        fn_801EC828(panel);
    } else {
        switch (panel->page_0x0A) {
        case 0:
            if (fn_8021213C(0x10) == 1U) {
                sysSE_req(0);
                panel->page_0x0A += 1;
                row = panel->row_0x06;
                if (row == 4) {
                    panel->stage_0x0E = lbl_805B85D8[row];
                    if (fn_8004D70C(0x7D4) == 0 && fn_8004D70C(0x36B1) == 0) {
                        panel->stage_0x0E -= 2;
                    } else if (fn_8004D27C(0x10) == 0) {
                        panel->stage_0x0E -= 1;
                    }
                } else {
                    panel->stage_0x0E = lbl_805B85D8[row];
                }
                panel->cursor_0x08 = 0;
            } else if (fn_8021213C(0x20) == 1U) {
                result = 2;
                sysSE_req(1);
            } else if (fn_802121F4(3) != 0) {
                panel->row_0x06 = fn_802A8EFC(panel->row_0x06, panel->row_max_0x0C, fn_802122AC(), 1, 2);
            }
            break;
        case 1:
            if (fn_8021213C(0x10) == 1U) {
                result = 1;
            } else if (fn_8021213C(0x20) == 1U) {
                panel->page_0x0A -= 1;
                sysSE_req(1);
            } else if (fn_802121F4(3) != 0) {
                panel->cursor_0x08 = fn_802A8EFC(panel->cursor_0x08, panel->stage_0x0E, fn_802122AC(), 1, 2);
            }
            break;
        }
    }
    fn_801EC95C(panel);
    return result;
}

/* Clears the digit-entry pane and seeds its digit limit from the item count. */
void fn_801ECD50(LbDigitPane* self)
{
    self->phase_0x00 = 0;
    self->sub_0x01 = 0;
    self->digit_hundreds_0x02 = 0;
    self->digit_hundreds_max_0x03 = fn_802A8F14(fn_8004AE70(lbl_80794880), 100);
    self->stepper_0x04 = 0;
    self->anim_0x0E = 0;
    self->timer_0x10 = 0;
    self->blink_a_0x12 = -1;
    self->blink_b_0x14 = -1;
    self->digit_ones_0x0A = 0;
    self->digit_tens_0x0C = 0;
    self->active_0x1C = 0;
    self->done_0x1D = 0;
}

/* The digit-entry pad's frame step: the right stick walks the ones/tens digits, the D-pad the hundreds. */
void fn_801ECDD4(LbDigitPane* self)
{
    if (fn_802121F4(0xF) != 0) {
        if (fn_802121F4(0xC) != 0) {
            self->digit_ones_0x0A = fn_802A8EFC(self->digit_ones_0x0A, 10, fn_802122AC(), 4, 8);
        }
        if (fn_802121F4(3) != 0) {
            self->digit_tens_0x0C = fn_802A8EFC(self->digit_tens_0x0C, 10, fn_802122AC(), 1, 2);
        }
    } else if (fn_8021213C(0x300) != 0) {
        self->digit_hundreds_0x02 = fn_802A8EEC(self->digit_hundreds_0x02, self->digit_hundreds_max_0x03,
                                                fn_802122E8(), 0x100, 0x200, 6, &self->flags_0x08);
        if ((self->flags_0x08 & 0x100) != 0) {
            self->blink_a_0x12 = 0;
        }
        if ((self->flags_0x08 & 0x200) != 0) {
            self->blink_b_0x14 = 0;
        }
    }
}

/* Ticks the two blink timers and the pane's frame counter. */
void fn_801ECEF0(LbDigitPane* self)
{
    s16 v;

    v = self->blink_a_0x12;
    if (v >= 0) {
        v += 1;
        self->blink_a_0x12 = v;
        if (v > 6) {
            self->blink_a_0x12 = -1;
            self->timer_0x10 = 0;
        }
    }
    v = self->blink_b_0x14;
    if (v >= 0) {
        v += 1;
        self->blink_b_0x14 = v;
        if (v > 6) {
            self->blink_b_0x14 = -1;
            self->timer_0x10 = 0;
        }
    }
    v = self->timer_0x10 + 1;
    self->timer_0x10 = v;
    if (v > 0x1E) {
        self->timer_0x10 = 0;
    }
}

/* Rebuilds the pane's 101-entry item list from the database's slot table. */
void fn_801ECF74(LbListPane* self)
{
    LbItemSlot* slot;
    LbItemList* list;
    LbEquipRec tmp;
    u16 count;
    u16 i;

    slot = &lbl_80794880->slots_0x180[0];
    list = &self->items_0x3B8;
    memset(list, 0, 404);
    i = 0;
    do {
        fn_8004BD58(list, 101, slot->id_0x00, slot->value_0x02, &tmp);
        i++;
        slot++;
    } while (i < 101U);
    fn_802A98BC(list, 101);
    count = fn_8004AE70(lbl_80794880);
    for (; i < count; i++, slot++) {
        list->tail_a_0x190 = 0;
        list->tail_b_0x192 = 0;
        fn_8004BD58(list, 101, slot->id_0x00, slot->value_0x02, &tmp);
        fn_802A98BC(list, 101);
    }
}

/* Confirms the digit entry: the button opens the pane's item list, the pad steps it. */
s32 fn_801ED048(LbDigitPane* self, LbListPane* list)
{
    s32 result;
    u16 count;

    if (self->active_0x1C == 0) {
        if (fn_8021213C(0x40) == 1U) {
            self->active_0x1C = 1;
            self->done_0x1D = 0;
            self->stepper_0x04 = 0;
            fn_801ECF74(list);
            sysSE_req(0xD);
            return 1;
        }
        return 0;
    }
    result = fn_802A8F50(&self->stepper_0x04, fn_802122E8(), 4, 8, 0xFFFF);
    switch (result) {
    case 1:
        count = fn_8004AE70(lbl_80794880);
        fn_8004BF28(&lbl_80794880->slots_0x180[0], count);
        fn_802A98BC(&lbl_80794880->slots_0x180[0], count);
        self->active_0x1C = 0;
        self->digit_ones_0x0A = 0;
        self->digit_tens_0x0C = 0;
        self->digit_hundreds_0x02 = 0;
        sysSE_req(0);
        break;
    case 2:
        self->active_0x1C = 0;
        break;
    }
    return 1;
}

/* The three decimal digits as a number. */
s16 fn_801ED160(LbDigitPane* self)
{
    return (s16)((self->digit_ones_0x0A + self->digit_hundreds_0x02 * 100) + self->digit_tens_0x0C * 10);
}

/* How many of the slot's item the player can afford. */
s16 fn_801ED184(s16 index)
{
    LbItemDb* db;
    u16 count;

    if (index >= fn_8004AF20(lbl_80794880)) {
        return 0;
    }
    db = lbl_80794880;
    count = fn_8004AE70(db);
    return fn_8004B7B0(fn_8004AFC8(db, index), &db->slots_0x180[0], count);
}

/* Whether the slot holds an item at all (and which side it came from). */
s16 fn_801ED200(s16 index, u8 side)
{
    if (index >= fn_8004AF20(lbl_80794880)) {
        return 0;
    }
    if (fn_8004AFC8(lbl_80794880, index) == 0) {
        return 0;
    }
    return fn_8004B460(lbl_80794880, side == 0);
}

/* Copies the working equipment set into the backup run and re-arms the pane's two counters. */
void fn_801ED284(LbListPane* self)
{
    LbConfigWork* cfg;
    s32 i;

    cfg = (LbConfigWork*)self->data_0x010;
    self->row_0x014 = 0;
    self->column_0x018 = 0;
    self->row_sub_0x016 = 0;
    self->column_sub_0x01A = 0;
    for (i = 0; i < 8; i++) {
        cfg->backup_ids_0x61A[i] = cfg->ids_0x5F2[i];
        cfg->backup_kinds_0x62A[i] = cfg->kinds_0x602[i];
    }
    if (fn_8004AEC0(lbl_80794880, cfg) == 1U) {
        self->side_0x005 = 1;
        self->row_sub_max_0x017 = fn_802A8F14(fn_8004AF0C(1U), 8);
        self->column_sub_max_0x01B = fn_802A8F14(fn_8004AF0C(0U), 8);
    } else {
        self->side_0x005 = 0;
        self->row_sub_max_0x017 = fn_802A8F14(fn_8004AF0C(0U), 8);
        self->column_sub_max_0x01B = fn_802A8F14(fn_8004AF0C(1U), 8);
    }
    self->move_0x01C = 0;
    self->active_0x026 = 0;
}

/* Slides the pane's item out of the slot by one column. */
void fn_801ED3E8(LbListPane* self)
{
    s16 out;

    fn_8004BEA4(fn_8004AFC8(lbl_80794880, self->entry_max_0x022), self->entry_0x020, &out);
    fn_8004BBEC(lbl_80794880, fn_8004AFC8(lbl_80794880, self->entry_max_0x022), -self->entry_0x020,
                self->side_0x005, 1);
    self->entry_max_0x022 = out;
}

/* Moves the pane's whole item stack to the other side. */
void fn_801ED464(LbListPane* self)
{
    u8 side;
    u16 id;
    s16 limit;
    s16 v;

    side = self->side_0x005 == 0;
    id = fn_8004AFC8(lbl_80794880, self->entry_max_0x022);
    fn_8004BBEC(lbl_80794880, id, self->entry_0x020, side, 1);
    fn_8004BBEC(lbl_80794880, id, -self->entry_0x020, self->side_0x005, 1);
    limit = fn_8004AF0C(side);
    v = fn_8004BA00(id, fn_8004AF60(lbl_80794880, side), limit);
    self->entry_max_0x022 = v;
    self->column_sub_0x01A = v / 8;
}

/* The pane's cursor row as an absolute row. */
s16 fn_801ED53C(LbListPane* self)
{
    return (s16)(self->row_0x014 + self->row_sub_0x016 * 8);
}

/* The pane's cursor column as an absolute column. */
s16 fn_801ED554(LbListPane* self)
{
    return (s16)(self->column_0x018 + self->column_sub_0x01A * 8);
}


/* Confirms the pane's selection: the button opens the confirm step, the pad steps it. */
s32 fn_801ED56C(LbListPane* self, u8 arg)
{
    s32 result;
    u8 side;
    LbItemSlot* off;

    if (self->active_0x026 == 0) {
        if (fn_8021213C(0x40) == 1U) {
            self->active_0x026 = 1;
            self->stepper_0x02C = 0;
            sysSE_req(0xD);
            return 1;
        }
        return 0;
    }
    result = fn_802A8F50(&self->stepper_0x02C, fn_802122E8(), 4, 8, 0xFFFF);
    switch (result) {
    case 1:
        if (arg == 0) {
            side = self->side_0x005;
        } else {
            side = self->side_0x005 == 0;
        }
        if (side == 1) {
            off = &fn_8004AF60(lbl_80794880, side)[0x18];
        } else {
            off = 0;
        }
        fn_802A9BB8(off);
        self->active_0x026 = 0;
        sysSE_req(0);
        break;
    case 2:
        self->active_0x026 = 0;
        break;
    }
    return 1;
}

/* The pane's item-selection step machine (pick / drag / confirm). */
s32 fn_801ED688(LbListPane* self, u8 arg)
{
    s16 idx;
    s32 result;
    s32 r;

    result = 0;
    self->move_0x01C = 0;
    switch (self->phase_0x001) {
    case 0:
        if (fn_801ED56C(self, 0) == 1U) {
            return 0;
        }
        if (fn_8021213C(0x10) == 1U) {
            idx = fn_801ED53C(self);
            self->entry_max_0x022 = idx;
            if (arg == 0) {
                self->move_max_0x01E = fn_801ED184(idx);
            } else {
                self->move_max_0x01E = fn_801ED200(idx, self->side_0x005);
            }
            if (self->move_max_0x01E > 0) {
                self->phase_0x001 = 2;
                if (fn_8004AFFC(lbl_80794880, self->entry_max_0x022) < self->move_max_0x01E) {
                    self->move_max_0x01E = fn_8004AFFC(lbl_80794880, self->entry_max_0x022);
                }
                self->entry_0x020 = self->move_max_0x01E;
                if (arg == 0) {
                    fn_801ED3E8(self);
                } else {
                    fn_801ED464(self);
                }
                fn_800DBC84(5);
            } else {
                sysSE_req(2);
            }
        } else if (fn_8021213C(0x8000) == 1U) {
            idx = fn_801ED53C(self);
            self->entry_max_0x022 = idx;
            if (arg == 0) {
                self->move_max_0x01E = fn_801ED184(idx);
            } else {
                self->move_max_0x01E = fn_801ED200(idx, self->side_0x005);
            }
            if (self->move_max_0x01E > 0) {
                self->phase_0x001 += 1;
                if (fn_8004AFFC(lbl_80794880, self->entry_max_0x022) < self->move_max_0x01E) {
                    self->move_max_0x01E = fn_8004AFFC(lbl_80794880, self->entry_max_0x022);
                }
                self->entry_0x020 = 1;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (fn_8021213C(0x20) == 1U) {
            sysSE_req(1);
            result = 2;
        } else if (fn_802121F4(3) != 0) {
            self->row_0x014 = fn_802A8EFC(self->row_0x014, 8, fn_802122AC(), 1, 2);
        } else if (fn_802121F4(0xC) != 0) {
            self->row_sub_0x016 = fn_802A8EC0(self->row_sub_0x016, self->row_sub_max_0x017,
                                             fn_802122AC(), 4, 8, &self->move_0x01C);
        }
        break;
    case 1:
        r = fn_802A91AC(&self->entry_0x020, self->move_max_0x01E, lobby_w.count_0x084,
                        fn_802122AC(), &self->move_0x01C);
        switch (r) {
        case 1:
            self->phase_0x001 += 1;
            if (arg == 0) {
                fn_801ED3E8(self);
            } else {
                fn_801ED464(self);
            }
            fn_800DBC84(5);
            break;
        case 2:
            self->phase_0x001 -= 1;
            sysSE_req(1);
            break;
        }
        break;
    case 2:
        if (fn_8021213C(0x30) == 1U) {
            self->phase_0x001 = 0;
            sysSE_req(0);
        }
        break;
    }
    return result;
}

/* The affordable count of the slot's item. */
s16 fn_801ED9E4(s16 index)
{
    LbItemSlot* slot;
    s16 limit;
    s16 value;

    slot = &lbl_80794880->slots_0x180[index];
    limit = fn_8004B624(lbl_80794880, slot->id_0x00);
    value = slot->value_0x02;
    if (limit < value) {
        value = limit;
    }
    return value;
}

/* The affordable count of the `index`-th entry of the side's own slot table. */
s16 fn_801EDA34(u16 index, u8 side)
{
    LbItemSlot* slot;
    s16 limit;
    s16 value;

    slot = &fn_8004AF60(lbl_80794880, side == 0)[index];
    limit = fn_8004B624(lbl_80794880, slot->id_0x00);
    value = slot->value_0x02;
    if (limit < value) {
        return limit;
    }
    return value;
}

/* Whether the entry can be picked: 1 unknown item, 2 an occupied slot, 0 free. */
u8 fn_801EDA9C(u16 index, u16 item_id)
{
    if ((u8)*GetItemData(item_id) != 1 && index >= 0x18U) {
        return 1;
    }
    if (fn_8004C004(&lbl_80794880->slots_0x180[0], fn_8004AE70(lbl_80794880)) == 0
        && fn_8004AFC8(lbl_80794880, index) != 0) {
        return 2;
    }
    return 0;
}

/* Whether the pane's entry can be moved: 1 unknown, 2 too big, 3 no room, 0 ok. */
u8 fn_801EDB34(LbListPane* self, u16 index, u16 item_id)
{
    u8 side;
    u8* data;
    u16 slot_id;
    s16 limit;

    side = self->side_0x005 == 0;
    data = GetItemData(item_id);
    if (index >= 0x18U && *data != 1) {
        return 1;
    }
    slot_id = fn_8004AFC8(lbl_80794880, index);
    if (slot_id != 0) {
        data = GetItemData(slot_id);
        limit = fn_8004AF0C(side);
        if (fn_8004BA00(item_id, fn_8004AF60(lbl_80794880, side), limit) >= 0x18 && *data != 1) {
            return 2;
        }
        limit = fn_8004AF0C(side);
        if (fn_8004BA00(slot_id, fn_8004AF60(lbl_80794880, side), limit) >= 0) {
            return 3;
        }
    }
    return 0;
}

/* The item-selection step machine driven by the digit entry (pick / drag / confirm). */
s32 fn_801EDC4C(LbDigitPane* self, LbListPane* list)
{
    LbItemSlot tmp;
    LbItemSlot* slots;
    s32 result;
    s16 value;
    s16 index;
    s16 limit;
    s16 move;
    u16 id;

    result = 0;
    self->flags_0x08 = 0;
    fn_801EC804(&self->anim_0x0E);
    fn_801ECEF0(self);
    switch (self->phase_0x00) {
    case 0:
        if (fn_801ED048(self, list) == 1U) {
            return 0;
        }
        if (fn_8021213C(0x10) == 1U) {
            value = fn_801ED160(self);
            self->cursor_0x1A = value;
            index = fn_801ED9E4(value);
            self->pick_0x16 = index;
            if (index > 0) {
                limit = fn_8004AF20(lbl_80794880);
                move = fn_8004BA00(lbl_80794880->slots_0x180[self->cursor_0x1A].id_0x00,
                                   fn_8004AF78(lbl_80794880), limit);
                list->entry_max_0x022 = move;
                list->item_id_0x024 = lbl_80794880->slots_0x180[self->cursor_0x1A].id_0x00;
                if (move < 0) {
                    self->phase_0x00 += 1;
                    limit = fn_8004AF20(lbl_80794880);
                    move = fn_8004BA00(0, fn_8004AF78(lbl_80794880), limit);
                    if (move >= 0) {
                        if (list->side_0x005 == 1 && *GetItemData(list->item_id_0x024) == 1) {
                            s16 extra = fn_8004BA00(0, &fn_8004AF78(lbl_80794880)[0x18], 8);
                            if (extra >= 0) {
                                move = extra + 0x18;
                            }
                        }
                        list->row_sub_0x016 = move / 8;
                        list->row_0x014 = move % 8;
                    } else {
                        list->row_sub_0x016 = 0;
                        list->row_0x014 = 0;
                    }
                } else {
                    self->phase_0x00 = 2;
                    list->row_sub_0x016 = move / 8;
                    list->row_0x014 = move % 8;
                }
                self->pick_max_0x18 = 1;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (fn_8021213C(0x20) == 1U) {
            sysSE_req(1);
            result = 2;
        } else {
            fn_801ECDD4(self);
        }
        break;
    case 1:
        if (fn_801ED56C(list, 0) == 1U) {
            return 0;
        }
        if (fn_8021213C(0x10) == 1U) {
            id = fn_801ED53C(list);
            value = id;
            if (fn_801EDA9C(id, list->item_id_0x024) == 0) {
                self->phase_0x00 += 1;
                list->entry_max_0x022 = value;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (fn_8021213C(0x20) == 1U) {
            self->phase_0x00 -= 1;
            sysSE_req(1);
        } else if (fn_802121F4(3) != 0) {
            list->row_0x014 = fn_802A8EFC(list->row_0x014, 8, fn_802122AC(), 1, 2);
        } else if (fn_802121F4(0xC) != 0) {
            list->row_sub_0x016 = fn_802A8EC0(list->row_sub_0x016, list->row_sub_max_0x017,
                                              fn_802122AC(), 4, 8, &list->move_0x01C);
        }
        break;
    case 2:
        index = fn_802A91AC(&self->pick_0x16, self->pick_max_0x18, lobby_w.count_0x084,
                            fn_802122AC(), &self->flags_0x08);
        switch (index) {
        case 1:
            slots = &lbl_80794880->slots_0x180[0];
            id = slots[self->cursor_0x1A].id_0x00;
            if (fn_8004AF78(lbl_80794880)[list->entry_max_0x022].id_0x00 != 0
                && id != fn_8004AF78(lbl_80794880)[list->entry_max_0x022].id_0x00) {
                fn_8004A1F8(&tmp, &fn_8004AF78(lbl_80794880)[list->entry_max_0x022]);
                fn_8004BD30(&fn_8004AF60(lbl_80794880, list->side_0x005)[list->entry_max_0x022]);
                fn_8004BA3C(id, self->pick_max_0x18,
                            &fn_8004AF60(lbl_80794880, list->side_0x005)[list->entry_max_0x022], 1, 1, 0);
                fn_8004B870(&lbl_80794880->slots_0x180[0], self->cursor_0x1A, -self->pick_max_0x18);
                fn_8004BEA4(tmp.id_0x00, tmp.value_0x02, &value);
            } else {
                fn_8004BA3C(id, self->pick_max_0x18,
                            &fn_8004AF60(lbl_80794880, list->side_0x005)[list->entry_max_0x022], 1, 1, 0);
                fn_8004B870(&lbl_80794880->slots_0x180[0], self->cursor_0x1A, -self->pick_max_0x18);
            }
            limit = fn_8004AF20(lbl_80794880);
            list->entry_max_0x022 = fn_8004BA00(id, fn_8004AF78(lbl_80794880), limit);
            list->row_sub_0x016 = list->entry_max_0x022 / 8;
            self->phase_0x00 += 1;
            fn_800DBC84(5);
            break;
        case 2:
            limit = fn_8004AF20(lbl_80794880);
            if (fn_8004BA00(lbl_80794880->slots_0x180[self->cursor_0x1A].id_0x00,
                            fn_8004AF78(lbl_80794880), limit) < 0) {
                self->phase_0x00 -= 1;
            } else {
                self->phase_0x00 = 0;
            }
            sysSE_req(1);
            break;
        }
        break;
    case 3:
        if (fn_8021213C(0x30) == 1U) {
            self->phase_0x00 = 0;
            sysSE_req(0);
        }
        break;
    }
    return result;
}

/* The pane's item-selection step machine (the second pane's variant). */
s32 fn_801EE1E4(LbListPane* self)
{
    LbItemSlot tmp2;
    s16 value;
    s16 index;
    s16 limit;
    s16 move;
    u16 id;
    s32 result;
    s32 r;
    u8 side;

    result = 0;
    self->move_0x01C = 0;
    switch (self->phase_0x001) {
    case 0:
        if (fn_801ED56C(self, 1) == 1U) {
            return 0;
        }
        if (fn_8021213C(0x10) == 1U) {
            id = fn_801ED554(self);
            index = fn_801EDA34(id, self->side_0x005);
            self->move_max_0x01E = index;
            if (index > 0) {
                side = self->side_0x005 == 0;
                limit = fn_8004AF20(lbl_80794880);
                move = fn_8004BA00(fn_8004AF60(lbl_80794880, side)[id].id_0x00,
                                   fn_8004AF78(lbl_80794880), limit);
                self->entry_max_0x022 = move;
                self->item_id_0x024 = fn_8004AF60(lbl_80794880, side)[id].id_0x00;
                if (move < 0) {
                    self->phase_0x001 += 1;
                    limit = fn_8004AF20(lbl_80794880);
                    move = fn_8004BA00(0, fn_8004AF78(lbl_80794880), limit);
                    if (move >= 0) {
                        if (self->side_0x005 == 1 && *GetItemData(self->item_id_0x024) == 1) {
                            s16 extra = fn_8004BA00(0, &fn_8004AF78(lbl_80794880)[0x18], 8);
                            if (extra >= 0) {
                                move = extra + 0x18;
                            }
                        }
                        self->row_sub_0x016 = move / 8;
                        self->row_0x014 = move % 8;
                    } else {
                        self->row_sub_0x016 = 0;
                        self->row_0x014 = 0;
                    }
                } else {
                    self->phase_0x001 = 2;
                    self->row_sub_0x016 = move / 8;
                    self->row_0x014 = move % 8;
                }
                self->entry_0x020 = 1;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (fn_8021213C(0x20) == 1U) {
            sysSE_req(1);
            result = 2;
        } else if (fn_802121F4(3) != 0) {
            self->column_0x018 = fn_802A8EFC(self->column_0x018, 8, fn_802122AC(), 1, 2);
        } else if (fn_802121F4(0xC) != 0) {
            self->column_sub_0x01A = fn_802A8EC0(self->column_sub_0x01A, self->column_sub_max_0x01B,
                                                 fn_802122AC(), 4, 8, &self->move_0x01C);
        }
        break;
    case 1:
        if (fn_801ED56C(self, 0) == 1U) {
            return 0;
        }
        if (fn_8021213C(0x10) == 1U) {
            id = fn_801ED53C(self);
            if (fn_801EDB34(self, id, self->item_id_0x024) == 0) {
                side = self->side_0x005 == 0;
                if (fn_8004AFC8(lbl_80794880, id) != 0
                    && fn_8004B460(lbl_80794880, side) <= 0) {
                    self->phase_0x001 = 3;
                    fn_8004A1F8(&tmp2, &fn_8004AF78(lbl_80794880)[id]);
                    fn_8004BD30(&fn_8004AF60(lbl_80794880, self->side_0x005)[id]);
                    fn_8004BA3C(self->item_id_0x024, self->move_max_0x01E,
                                &fn_8004AF60(lbl_80794880, self->side_0x005)[id], 1, 1, 0);
                    fn_8004BBEC(lbl_80794880, self->item_id_0x024, -self->move_max_0x01E, side, 1);
                    fn_8004BBEC(lbl_80794880, tmp2.id_0x00, tmp2.value_0x02, side, 1);
                    limit = fn_8004AF20(lbl_80794880);
                    self->entry_max_0x022 = fn_8004BA00(self->item_id_0x024,
                                                        fn_8004AF78(lbl_80794880), limit);
                    self->row_sub_0x016 = self->entry_max_0x022 / 8;
                    fn_800DBC84(5);
                } else {
                    self->phase_0x001 += 1;
                    self->entry_max_0x022 = id;
                    sysSE_req(0);
                }
            } else {
                sysSE_req(2);
            }
        } else if (fn_8021213C(0x20) == 1U) {
            self->phase_0x001 -= 1;
            sysSE_req(1);
        } else if (fn_802121F4(3) != 0) {
            self->row_0x014 = fn_802A8EFC(self->row_0x014, 8, fn_802122AC(), 1, 2);
        } else if (fn_802121F4(0xC) != 0) {
            self->row_sub_0x016 = fn_802A8EC0(self->row_sub_0x016, self->row_sub_max_0x017,
                                              fn_802122AC(), 4, 8, &self->move_0x01C);
        }
        break;
    case 2:
        r = fn_802A91AC(&self->entry_0x020, self->move_max_0x01E, lobby_w.count_0x084,
                        fn_802122AC(), &self->move_0x01C);
        switch (r) {
        case 1:
            side = self->side_0x005 == 0;
            if (fn_8004AF78(lbl_80794880)[self->entry_max_0x022].id_0x00 != 0
                && self->item_id_0x024 != fn_8004AF78(lbl_80794880)[self->entry_max_0x022].id_0x00) {
                fn_8004A1F8(&tmp2, &fn_8004AF78(lbl_80794880)[self->entry_max_0x022]);
                fn_8004BD30(&fn_8004AF60(lbl_80794880, self->side_0x005)[self->entry_max_0x022]);
                fn_8004BA3C(self->item_id_0x024, self->entry_0x020,
                            &fn_8004AF60(lbl_80794880, self->side_0x005)[self->entry_max_0x022], 1, 1, 0);
                fn_8004BBEC(lbl_80794880, self->item_id_0x024, -self->entry_0x020, side, 1);
                fn_8004BBEC(lbl_80794880, tmp2.id_0x00, tmp2.value_0x02, side, 1);
            } else {
                fn_8004BA3C(self->item_id_0x024, self->entry_0x020,
                            &fn_8004AF60(lbl_80794880, self->side_0x005)[self->entry_max_0x022], 1, 1, 0);
                fn_8004BBEC(lbl_80794880, self->item_id_0x024, -self->entry_0x020, side, 1);
            }
            limit = fn_8004AF20(lbl_80794880);
            self->entry_max_0x022 = fn_8004BA00(self->item_id_0x024, fn_8004AF78(lbl_80794880), limit);
            self->row_sub_0x016 = self->entry_max_0x022 / 8;
            self->phase_0x001 += 1;
            fn_800DBC84(5);
            break;
        case 2:
            limit = fn_8004AF20(lbl_80794880);
            if (fn_8004BA00(self->item_id_0x024, fn_8004AF78(lbl_80794880), limit) < 0) {
                self->phase_0x001 -= 1;
            } else {
                self->phase_0x001 = 0;
            }
            sysSE_req(1);
            break;
        }
        break;
    case 3:
        if (fn_8021213C(0x30) == 1U) {
            self->phase_0x001 = 0;
            sysSE_req(0);
        }
        break;
    }
    return result;
}

/* Whether the entry's item is one the shop can hand over. */
u32 fn_801EE900(u16 item_id)
{
    if (item_id != 0 && fn_8033AAFC() == 1U) {
        return 1;
    }
    return 0;
}

/* Whether the entry's item is one the shop can accept. */
s32 fn_801EE940(u16 index, u16 item_id)
{
    if (item_id != 0 && fn_8033AC78(index, 0) != 0) {
        return 1;
    }
    return 0;
}


/* The item-selection step machine driven by the digit entry's confirm (the swap variant). */
s32 fn_801EEDC4(LbDigitPane* self, LbListPane* list)
{
    LbItemSlot tmp;
    LbItemSlot* slots;
    s32 result;
    s16 value;

    result = 0;
    self->flags_0x08 = 0;
    fn_801EC804(&self->anim_0x0E);
    fn_801ECEF0(self);
    switch (self->phase_0x00) {
    case 0:
        if (fn_801ED048(self, list) == 1U) {
            return 0;
        }
        /* falls through into the step handling, like the target's shared tail */
    case 1:
        if (fn_8021213C(0x10) == 1U) {
            value = fn_801ED160(self);
            switch (self->phase_0x00) {
            case 0:
                self->phase_0x00 += 1;
                self->cursor_0x1A = value;
                sysSE_req(0xE);
                break;
            case 1:
                slots = &lbl_80794880->slots_0x180[0];
                if ((slots[value].id_0x00 == 0 && slots[self->cursor_0x1A].id_0x00 == 0)
                    || value == self->cursor_0x1A) {
                    sysSE_req(2);
                } else {
                    self->phase_0x00 -= 1;
                    fn_8004A1F8(&tmp, &slots[value]);
                    fn_8004A1F8(&slots[value], &slots[self->cursor_0x1A]);
                    fn_8004A1F8(&slots[self->cursor_0x1A], &tmp);
                    sysSE_req(0xF);
                }
                break;
            }
        } else if (fn_8021213C(0x20) == 1U) {
            switch (self->phase_0x00) {
            case 0:
                sysSE_req(1);
                result = 2;
                break;
            case 1:
                self->phase_0x00 -= 1;
                sysSE_req(1);
                break;
            }
        } else {
            fn_801ECDD4(self);
        }
        break;
    default:
        break;
    }
    return result;
}

/* The item-selection step machine driven by the digit entry (the stock variant). */
s32 fn_801EEFA0(LbDigitPane* self, LbListPane* list)
{
    LbItemDb* db;
    s32 result;
    s16 value;
    s16 index;
    s32 r;

    db = lbl_80794880;
    result = 0;
    self->flags_0x08 = 0;
    fn_801EC804(&self->anim_0x0E);
    fn_801ECEF0(self);
    switch (self->phase_0x00) {
    case 0:
        if (fn_801ED048(self, list) == 1U) {
            return 0;
        }
        if (fn_8021213C(0x10) == 1U) {
            value = fn_801ED160(self);
            self->cursor_0x1A = value;
            index = db->slots_0x180[value].value_0x02;
            self->pick_0x16 = index;
            if (index > 0) {
                self->phase_0x00 += 1;
                self->pick_max_0x18 = 1;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (fn_8021213C(0x20) == 1U) {
            sysSE_req(1);
            result = 2;
        } else {
            fn_801ECDD4(self);
        }
        break;
    case 1:
        r = fn_802A91AC(&self->pick_max_0x18, self->pick_0x16, lobby_w.count_0x084,
                        fn_802122AC(), &self->flags_0x08);
        switch (r) {
        case 1:
            self->phase_0x00 += 1;
            self->stepper_0x04 = 0;
            sysSE_req(0);
            break;
        case 2:
            self->phase_0x00 -= 1;
            sysSE_req(1);
            break;
        }
        break;
    case 2:
        r = fn_802A8F50(&self->stepper_0x04, fn_802122E8(), 4, 8, 0xFFFF);
        switch (r) {
        case 1:
            self->phase_0x00 = 0;
            fn_8004D0E8(self->pick_max_0x18 * eft052_item_value_get(db->slots_0x180[self->cursor_0x1A].id_0x00),
                        &db->data_0x18[0]);
            sysSE_req(9);
            fn_8004B870(&db->slots_0x180[0], self->cursor_0x1A, -self->pick_max_0x18);
            break;
        case 2:
            self->phase_0x00 -= 1;
            break;
        }
        break;
    }
    return result;
}

/* Whether the equipment pane holds a complete set (or whether its bowgun can be fired). */
s32 fn_801EF1D0(LbEquipPane* self)
{
    u16 id;

    id = self->ids_0x04[0];
    if (id != 0xFFFFU && self->ids_0x04[1] != 0xFFFFU && self->ids_0x04[2] != 0xFFFFU) {
        return 1;
    }
    if (id != 0xFFFFU) {
        return Gunner_opt_ok_ck(&lbl_80794880->equip_0x0E00[id]) == 0;
    }
    return 0;
}

/* Seeds the equipment pane with the three ids and the icon run. */
void fn_801EF244(LbEquipPane* self, s16 index, u16 a, u16 b, u16 c, u8 side, s32 icon_index)
{
    u16 id_a;
    u16 id_b;
    u16 id_c;

    id_a = a;
    id_b = b;
    id_c = c;
    self->phase_0x00 = 0;
    self->index_0x02 = index;
    if (lbl_80794880->equip_0x0E00[lbl_80794880->kind_0x8C].kind_0x00 == 0xB) {
        if (a == 0xFFFFU) {
            if (Gunner_opt_ok_ck(&lbl_80794880->equip_0x0E00[lbl_80794880->kind_0x8C]) == 0) {
                id_a = 0xFFFF;
            } else {
                id_a = lbl_80794880->kind_0x8C;
            }
        }
        if (id_a != 0xFFFFU) {
            if (Gunner_opt_ok_ck(&lbl_80794880->equip_0x0E00[id_a]) == 0) {
                id_b = 0xFFFF;
                id_c = 0xFFFF;
            }
        } else {
            id_b = 0xFFFF;
            id_c = 0xFFFF;
        }
        if (id_b == 0xFFFFU) {
            id_b = lbl_80794880->sub_0x8E;
        }
        if (id_c == 0xFFFFU) {
            id_c = lbl_80794880->sub_0x90;
        }
    }
    self->ids_0x04[0] = id_a;
    self->ids_0x04[1] = id_b;
    self->ids_0x04[2] = id_c;
    self->side_0x01 = side;
    self->icon_index_0x90 = icon_index;
    if (side == 0) {
        fn_80219080(icon_index, &self->icon_0x10[0], 5, 7);
    } else {
        fn_80219080(icon_index, &self->icon_0x10[0], 5, 6);
    }
    self->move_max_0x18 = 4;
    self->flags_0x20 |= 1;
}

/* The digit entry's number, as the id lookup takes it. */
u16 fn_801EF73C(LbDigitPane* self)
{
    return (u16)(s16)((self->digit_ones_0x0A + self->digit_hundreds_0x02 * 100)
                      + self->digit_tens_0x0C * 10);
}
