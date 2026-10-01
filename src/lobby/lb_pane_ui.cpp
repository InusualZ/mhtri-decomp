/*
 * lobby/lb_pane_ui.cpp - the lobby's pane group: the digit-entry and item-list panes, the page/panel group and the
 * character-edit (hair/inner colour) screen.
 *
 * `.text` 0x801EC9F8..0x801FBF78 (129 functions), `.data` 0x76C B, `.sdata` 0xF8 B, `.sdata2` 0x10 B, extab 0x340 B and
 * extabindex 0x4E0 B.  Phase 4 fold (docs/splits/phase4): the three registered units `lobby/fn_801EC9F8`,
 * `lobby/fn_801F3294` and `lobby/fn_801F9CD4` are one TU of the candidate (their `.sdata2`/`.data` pools run on without a
 * break across the old edges); their bodies are kept below in text order, each under its own former header.
 *
 * Scopes: the three former units were written against different views of the same lobby globals (`lobby_w`,
 * `lobby_world_block`, the `lbl_805B8xxx` tables) and call some unsplit callees with different argument lists, so the first two
 * sections keep their own declarations in a namespace (`extern "C"` names stay unmangled; the C++-linkage callees stay at global
 * scope, where their manglings are made).  Uniting the views into one declaration set is the open work of this unit.
 * Name: GUESS, from what the three bands draw (digit-entry/item-list panes, a page/panel group, the hair/inner colour
 * edit screen); no `__FILE__` string and no runtime-dump name covers the range.
 * Flags: `cflags_lobby` for all three former units; the pragmas each used are scoped to its own section below.
 * Data: no `.data`/`.sdata`/`.sdata2` is defined here yet; the claimed ranges keep the original bytes (`NonMatching`).
 */

/* ==== absorbed from lobby/fn_801EC9F8.cpp (0x801EC9F8..0x801F3188) ==== */
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
 * the lobby item database `lobby_world_block`, and its callees are the lobby UI API.
 *
 * Name.  The map has only `fn_XXXXXXXX` for this range, the runtime dump answers only `zz_` placeholders
 * (`dumpmap.py lookup 0x801EC9F8` -> `zz_01ec9f8_`), and no `__FILE__`/assert string covers the range
 * (its only data references are the numeric tables `lbl_805B85D8`/`lbl_805B8618`/`lbl_805B8638` and the
 * switch table `jumptable_805B8B98`), so the file keeps the map's stem (brief section 2, class 4).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit.py range
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
 * where this range passes one, and its `menu_cursor_step_fixed_tail` takes `s16`s where the call sites pass `u8`s, so
 * the callee declarations live in this file (plain prototypes, one shared-file request each).
 */
#include "types.h"
#include "id_value.h"
#include "menu/menu_message.h"

#include "Runtime.PPCEABI.H/memset.h"
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */
#include "fn_80047398/userdata_gunner_ck.h" /* userdata_gunner_ck (rule 2) */
#include "lobby/lobby_w.h" /* `lobby_w`, owned by lobby/lb_menu_pos_tbl.cpp (rule 2) */

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
    /* +0x04 */ s32 stepper_0x04;       /* the 4-byte stepper state `toggle_word_step` walks */
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

/* The 0x194-byte item list `fn_801ECF74` builds at `LbListPane::items_0x3B8`: 0x65 id/value pairs, the last
 * of which the rebuild loop clears before every re-insert. */
typedef struct LbItemList {
    /* +0x000 */ IdValue entries_0x000[0x65];
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
    /* +0x016 */ u8 row_sub_0x016;      /* the 8-step row pair `menu_cursor_step_fixed_tail` walks */
    /* +0x017 */ u8 row_sub_max_0x017;
    /* +0x018 */ s16 column_0x018;      /* the cursor column */
    /* +0x01A */ u8 column_sub_0x01A;   /* the 8-step column pair `menu_cursor_step` walks */
    /* +0x01B */ u8 column_sub_max_0x01B;
    /* +0x01C */ u16 move_0x01C;        /* the pad-driven 4-step state at +0x1C (menu_cursor_step_fixed_tail's base) */
    /* +0x01E */ s16 move_max_0x01E;
    /* +0x020 */ s16 entry_0x020;       /* the 4-step state at +0x20 (menu_cursor_page_move's cursor base) */
    /* +0x022 */ s16 entry_max_0x022;
    /* +0x024 */ u16 item_id_0x024;
    /* +0x026 */ u8 active_0x026;       /* the "confirm is open" latch `fn_801ED56C` waits on */
    /* +0x027 */ u8 unused_0x027[5];
    /* +0x02C */ s32 stepper_0x02C;     /* the 4-byte stepper state `toggle_word_step` walks */
    /* +0x030 */ u8 unused_0x030[0x388];
    /* +0x3B8 */ LbItemList items_0x3B8; /* the 0x65-entry list `fn_801ECF74` fills */
} LbListPane; /* size: 0x54C (the extent this range reads) */

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
    /* +0x0C */ s32 stepper_0x0C;       /* the 4-byte stepper state `toggle_word_step` walks */
    /* +0x10 */ u8 icon_0x10[6];        /* the icon run `fn_80219080` fills */
    /* +0x16 */ s16 move_0x16;          /* the 4-step state `menu_cursor_step_fixed_tail`'s base is at */
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

/* One 0xC-byte equipment record (the `fn_8004A20C`/`fn_8004BD58` copy unit and `Gunner_opt_ok_ck`'s
 * argument).  Only the kind byte is read here. */
typedef struct LbEquipRec {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u16 value_0x02;
    /* +0x04 */ u8 unused_0x04[8];
} LbEquipRec; /* size: 0xC */

/* The lobby item database `lobby_world_block` (a 4-byte pointer in `.sbss`) points at: the item slot table
 * at +0x180 (`fn_8004AFC8`/`fn_8004AF60`/`userdata_equip_item_slots_get` index it), the 0xC-byte equipment records at
 * +0xE00, and the current-equipment kind at +0x8C. */
typedef struct LbItemDb {
    /* +0x0000 */ u8 unused_0x0000[0x18];
    /* +0x0018 */ u8 data_0x18[0x74];  /* the block `score_add_clamped` is handed as a work area */
    /* +0x008C */ s16 kind_0x8C;       /* the equipped item's kind (0xB is a bowgun) */
    /* +0x008E */ s16 sub_0x8E;
    /* +0x0090 */ s16 sub_0x90;
    /* +0x0092 */ u8 unused_0x0092[0xEE];
    /* +0x0180 */ IdValue slots_0x180[0x320]; /* the slot table the item calls index */
    /* +0x0E00 */ LbEquipRec equip_0x0E00[0x100]; /* the 0xC-byte equipment records */
} LbItemDb;
namespace s_801EC9F8 {
 /* size: 0x1A00+ (only the runs this range indexes are named) */

/* ------------------------------------------------------------------------------------------------
 * The symbols this range reads.  Everything below is a plain prototype at file scope (the project's
 * convention for callees whose owner has no publishable header); each one is filed as a shared-file
 * request.  `lobby_w` and `lobby_world_block` are re-declared here with this range's own view because the
 * callee signatures `include/unsplit/lobby.h` publishes for `menu_cursor_step_fixed_tail`/`fn_8021213C` do not match
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
extern "C" { extern LbItemDb* lobby_world_block; }
extern "C" { extern s16 lbl_805B85D8[0x30]; }      /* the row -> stage table `fn_801ECB40` indexes */
extern "C" { extern u8 lbl_805B8618[0x40]; }
extern "C" { extern u8 lbl_805B8638[0x40]; }
extern "C" { extern u8 jumptable_805B8B98[0x2C]; }

extern "C" {
/* The lobby UI API.  Signatures are the caller-side ones the call sites imply (the callee is
 * `NonMatching` in every case, so the width a call site uses is what its codegen needs). */
s32 item_pair_copy(IdValue* dst, const IdValue* src);
s32 fn_8004A20C(LbEquipRec* dst, const LbEquipRec* src);
u16 fn_8004AE70(LbItemDb* db);
s16 fn_8004AE98(LbItemDb* db);
s16 fn_8004AF0C(u8 side);
s16 fn_8004AF20(LbItemDb* db);
IdValue* fn_8004AF60(LbItemDb* db, u8 side);
IdValue* userdata_equip_item_slots_get(LbItemDb* db);
u16 fn_8004AFC8(LbItemDb* db, s16 index);
s16 fn_8004AFFC(LbItemDb* db, s16 index);
s32 fn_8004B460(LbItemDb* db, u8 side);
s16 fn_8004B624(LbItemDb* db, u16 id);
s16 fn_8004B7B0(u16 id, void* slots, u16 count);
s16 item_pair_index_find(u16 id, IdValue* base, s16 count);
s32 item_take(u16 id, s16 a, void* p, s32 b, s32 c, s32 d);
void fn_8004BBEC(LbItemDb* db, u16 id, s16 a, u8 side, s32 b);
s32 fn_8004BD30(void* p);
s32 fn_8004BD58(void* dst, u16 count, u16 id, s16 v, void* tmp);
s32 fn_8004B870(IdValue* slots, u16 index, s16 delta);
s32 fn_8004BEA4(u16 id, s16 a, s16* out);
s32 fn_8004BF28(void* dst, u16 count);
u16 fn_8004C004(void* slots, u16 count);
s32 fn_8004C038(void* p, u16 a, u16 b);
void fn_8004C7BC(void* p, u16 a);
s32 score_add_clamped(s32 a, void* p);
s32 fn_8004D27C(s32 a);
s32 fn_8004D70C(s32 a);
void sysSE_stop(s32 id);
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
s32 equip_kind_table_class(u8 kind);
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
s32 hud_item_msg_push(s32 a, s32 b, s32 c);
u32 fn_803768F8(void);
}
} /* namespace s_801EC9F8 */


/* C++-linkage callees: their map names are manglings, so the real declaration is what makes the
 * front-end emit them (docs/plan.md 6.5 rule 9). */
u8* GetItemData(u16 id);
s32 Gunner_opt_ok_ck(LbEquipRec* equip);
s32 chk_pointer(void);
namespace s_801EC9F8 {


/* ------------------------------------------------------------------------------------------------ */

/* Clamps the cursor pair to the given box after applying the D-pad bits of `buttons`; returns whether
 * the pad owned the frame. */
extern "C" s32 fn_801EC9F8(s16* x, s16* y, s16 x_min, s16 x_max, s16 y_min, s16 y_max, u16 buttons)
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
extern "C" void fn_801ECB2C(s16* x, s16* y)
{
    *x = 100;
    *y = 45;
}

/* The lobby menu panel's frame step: leaves on the cancel button, otherwise advances the two pages. */
extern "C" s32 fn_801ECB40(void)
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
                panel->row_0x06 = menu_cursor_step(panel->row_0x06, panel->row_max_0x0C, fn_802122AC(), 1, 2);
            }
            break;
        case 1:
            if (fn_8021213C(0x10) == 1U) {
                result = 1;
            } else if (fn_8021213C(0x20) == 1U) {
                panel->page_0x0A -= 1;
                sysSE_req(1);
            } else if (fn_802121F4(3) != 0) {
                panel->cursor_0x08 = menu_cursor_step(panel->cursor_0x08, panel->stage_0x0E, fn_802122AC(), 1, 2);
            }
            break;
        }
    }
    fn_801EC95C(panel);
    return result;
}

/* Clears the digit-entry pane and seeds its digit limit from the item count. */
extern "C" void fn_801ECD50(LbDigitPane* self)
{
    self->phase_0x00 = 0;
    self->sub_0x01 = 0;
    self->digit_hundreds_0x02 = 0;
    self->digit_hundreds_max_0x03 = menu_page_count(fn_8004AE70(lobby_world_block), 100);
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
extern "C" void fn_801ECDD4(LbDigitPane* self)
{
    if (fn_802121F4(0xF) != 0) {
        if (fn_802121F4(0xC) != 0) {
            self->digit_ones_0x0A = menu_cursor_step(self->digit_ones_0x0A, 10, fn_802122AC(), 4, 8);
        }
        if (fn_802121F4(3) != 0) {
            self->digit_tens_0x0C = menu_cursor_step(self->digit_tens_0x0C, 10, fn_802122AC(), 1, 2);
        }
    } else if (fn_8021213C(0x300) != 0) {
        self->digit_hundreds_0x02 = menu_cursor_step_forward(self->digit_hundreds_0x02, self->digit_hundreds_max_0x03,
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
extern "C" void fn_801ECEF0(LbDigitPane* self)
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
extern "C" void fn_801ECF74(LbListPane* self)
{
    IdValue* slot;
    LbItemList* list;
    LbEquipRec tmp;
    u16 count;
    u16 i;

    slot = &lobby_world_block->slots_0x180[0];
    list = &self->items_0x3B8;
    memset(list, 0, 404);
    i = 0;
    do {
        fn_8004BD58(list, 101, slot->id, slot->value, &tmp);
        i++;
        slot++;
    } while (i < 101U);
    item_pairs_compact_sort(list->entries_0x000, 101);
    count = fn_8004AE70(lobby_world_block);
    for (; i < count; i++, slot++) {
        list->entries_0x000[0x64].id = 0;
        list->entries_0x000[0x64].value = 0;
        fn_8004BD58(list, 101, slot->id, slot->value, &tmp);
        item_pairs_compact_sort(list->entries_0x000, 101);
    }
}

/* Confirms the digit entry: the button opens the pane's item list, the pad steps it. */
extern "C" s32 fn_801ED048(LbDigitPane* self, LbListPane* list)
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
    result = toggle_word_step(&self->stepper_0x04, fn_802122E8(), 4, 8, 0xFFFF);
    switch (result) {
    case 1:
        count = fn_8004AE70(lobby_world_block);
        fn_8004BF28(&lobby_world_block->slots_0x180[0], count);
        item_pairs_compact_sort(&lobby_world_block->slots_0x180[0], count);
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
extern "C" s16 fn_801ED160(LbDigitPane* self)
{
    return (s16)((self->digit_ones_0x0A + self->digit_hundreds_0x02 * 100) + self->digit_tens_0x0C * 10);
}

/* How many of the slot's item the player can afford. */
extern "C" s16 fn_801ED184(s16 index)
{
    LbItemDb* db;
    u16 count;

    if (index >= fn_8004AF20(lobby_world_block)) {
        return 0;
    }
    db = lobby_world_block;
    count = fn_8004AE70(db);
    return fn_8004B7B0(fn_8004AFC8(db, index), &db->slots_0x180[0], count);
}

/* Whether the slot holds an item at all (and which side it came from). */
extern "C" s16 fn_801ED200(s16 index, u8 side)
{
    if (index >= fn_8004AF20(lobby_world_block)) {
        return 0;
    }
    if (fn_8004AFC8(lobby_world_block, index) == 0) {
        return 0;
    }
    return fn_8004B460(lobby_world_block, side == 0);
}

/* Copies the working equipment set into the backup run and re-arms the pane's two counters. */
extern "C" void fn_801ED284(LbListPane* self)
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
    if (userdata_gunner_ck(lobby_world_block) == 1U) {
        self->side_0x005 = 1;
        self->row_sub_max_0x017 = menu_page_count(fn_8004AF0C(1U), 8);
        self->column_sub_max_0x01B = menu_page_count(fn_8004AF0C(0U), 8);
    } else {
        self->side_0x005 = 0;
        self->row_sub_max_0x017 = menu_page_count(fn_8004AF0C(0U), 8);
        self->column_sub_max_0x01B = menu_page_count(fn_8004AF0C(1U), 8);
    }
    self->move_0x01C = 0;
    self->active_0x026 = 0;
}

/* Slides the pane's item out of the slot by one column. */
extern "C" void fn_801ED3E8(LbListPane* self)
{
    s16 out;

    fn_8004BEA4(fn_8004AFC8(lobby_world_block, self->entry_max_0x022), self->entry_0x020, &out);
    fn_8004BBEC(lobby_world_block, fn_8004AFC8(lobby_world_block, self->entry_max_0x022), -self->entry_0x020,
                self->side_0x005, 1);
    self->entry_max_0x022 = out;
}

/* Moves the pane's whole item stack to the other side. */
extern "C" void fn_801ED464(LbListPane* self)
{
    u8 side;
    u16 id;
    s16 limit;
    s16 v;

    side = self->side_0x005 == 0;
    id = fn_8004AFC8(lobby_world_block, self->entry_max_0x022);
    fn_8004BBEC(lobby_world_block, id, self->entry_0x020, side, 1);
    fn_8004BBEC(lobby_world_block, id, -self->entry_0x020, self->side_0x005, 1);
    limit = fn_8004AF0C(side);
    v = item_pair_index_find(id, fn_8004AF60(lobby_world_block, side), limit);
    self->entry_max_0x022 = v;
    self->column_sub_0x01A = v / 8;
}

/* The pane's cursor row as an absolute row. */
extern "C" s16 fn_801ED53C(LbListPane* self)
{
    return (s16)(self->row_0x014 + self->row_sub_0x016 * 8);
}

/* The pane's cursor column as an absolute column. */
extern "C" s16 fn_801ED554(LbListPane* self)
{
    return (s16)(self->column_0x018 + self->column_sub_0x01A * 8);
}


/* Confirms the pane's selection: the button opens the confirm step, the pad steps it. */
extern "C" s32 fn_801ED56C(LbListPane* self, u8 arg)
{
    s32 result;
    u8 side;
    IdValue* pool;
    IdValue* off;

    if (self->active_0x026 == 0) {
        if (fn_8021213C(0x40) == 1U) {
            self->active_0x026 = 1;
            self->stepper_0x02C = 0;
            sysSE_req(0xD);
            return 1;
        }
        return 0;
    }
    result = toggle_word_step(&self->stepper_0x02C, fn_802122E8(), 4, 8, 0xFFFF);
    switch (result) {
    case 1:
        if (arg == 0) {
            side = self->side_0x005;
        } else {
            side = self->side_0x005 == 0;
        }
        pool = fn_8004AF60(lobby_world_block, side);
        if (side == 1) {
            off = &pool[0x18];
        } else {
            off = 0;
        }
        item_pages_sort(pool, off);
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
extern "C" s32 fn_801ED688(LbListPane* self, u8 arg)
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
                if (fn_8004AFFC(lobby_world_block, self->entry_max_0x022) < self->move_max_0x01E) {
                    self->move_max_0x01E = fn_8004AFFC(lobby_world_block, self->entry_max_0x022);
                }
                self->entry_0x020 = self->move_max_0x01E;
                if (arg == 0) {
                    fn_801ED3E8(self);
                } else {
                    fn_801ED464(self);
                }
                sysSE_stop(5);
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
                if (fn_8004AFFC(lobby_world_block, self->entry_max_0x022) < self->move_max_0x01E) {
                    self->move_max_0x01E = fn_8004AFFC(lobby_world_block, self->entry_max_0x022);
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
            self->row_0x014 = menu_cursor_step(self->row_0x014, 8, fn_802122AC(), 1, 2);
        } else if (fn_802121F4(0xC) != 0) {
            self->row_sub_0x016 = menu_cursor_step_fixed_tail(self->row_sub_0x016, self->row_sub_max_0x017,
                                                              fn_802122AC(), 4, 8, &self->move_0x01C);
        }
        break;
    case 1:
        r = menu_cursor_page_move(&self->entry_0x020, self->move_max_0x01E, lobby_w.count_0x084,
                                  fn_802122AC(), &self->move_0x01C);
        switch (r) {
        case 1:
            self->phase_0x001 += 1;
            if (arg == 0) {
                fn_801ED3E8(self);
            } else {
                fn_801ED464(self);
            }
            sysSE_stop(5);
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
extern "C" s16 fn_801ED9E4(s16 index)
{
    IdValue* slot;
    s16 limit;
    s16 value;

    slot = &lobby_world_block->slots_0x180[index];
    limit = fn_8004B624(lobby_world_block, slot->id);
    value = slot->value;
    if (limit < value) {
        value = limit;
    }
    return value;
}

/* The affordable count of the `index`-th entry of the side's own slot table. */
extern "C" s16 fn_801EDA34(u16 index, u8 side)
{
    IdValue* slot;
    s16 limit;
    s16 value;

    slot = &fn_8004AF60(lobby_world_block, side == 0)[index];
    limit = fn_8004B624(lobby_world_block, slot->id);
    value = slot->value;
    if (limit < value) {
        return limit;
    }
    return value;
}

/* Whether the entry can be picked: 1 unknown item, 2 an occupied slot, 0 free. */
extern "C" u8 fn_801EDA9C(u16 index, u16 item_id)
{
    if ((u8)*GetItemData(item_id) != 1 && index >= 0x18U) {
        return 1;
    }
    if (fn_8004C004(&lobby_world_block->slots_0x180[0], fn_8004AE70(lobby_world_block)) == 0
        && fn_8004AFC8(lobby_world_block, index) != 0) {
        return 2;
    }
    return 0;
}

/* Whether the pane's entry can be moved: 1 unknown, 2 too big, 3 no room, 0 ok. */
extern "C" u8 fn_801EDB34(LbListPane* self, u16 index, u16 item_id)
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
    slot_id = fn_8004AFC8(lobby_world_block, index);
    if (slot_id != 0) {
        data = GetItemData(slot_id);
        limit = fn_8004AF0C(side);
        if (item_pair_index_find(item_id, fn_8004AF60(lobby_world_block, side), limit) >= 0x18 && *data != 1) {
            return 2;
        }
        limit = fn_8004AF0C(side);
        if (item_pair_index_find(slot_id, fn_8004AF60(lobby_world_block, side), limit) >= 0) {
            return 3;
        }
    }
    return 0;
}

/* The item-selection step machine driven by the digit entry (pick / drag / confirm). */
extern "C" s32 fn_801EDC4C(LbDigitPane* self, LbListPane* list)
{
    IdValue tmp;
    IdValue* slots;
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
                limit = fn_8004AF20(lobby_world_block);
                move = item_pair_index_find(lobby_world_block->slots_0x180[self->cursor_0x1A].id,
                                   userdata_equip_item_slots_get(lobby_world_block), limit);
                list->entry_max_0x022 = move;
                list->item_id_0x024 = lobby_world_block->slots_0x180[self->cursor_0x1A].id;
                if (move < 0) {
                    self->phase_0x00 += 1;
                    limit = fn_8004AF20(lobby_world_block);
                    move = item_pair_index_find(0, userdata_equip_item_slots_get(lobby_world_block), limit);
                    if (move >= 0) {
                        if (list->side_0x005 == 1 && *GetItemData(list->item_id_0x024) == 1) {
                            s16 extra = item_pair_index_find(0, &userdata_equip_item_slots_get(lobby_world_block)[0x18], 8);
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
            list->row_0x014 = menu_cursor_step(list->row_0x014, 8, fn_802122AC(), 1, 2);
        } else if (fn_802121F4(0xC) != 0) {
            list->row_sub_0x016 = menu_cursor_step_fixed_tail(list->row_sub_0x016, list->row_sub_max_0x017,
                                                              fn_802122AC(), 4, 8, &list->move_0x01C);
        }
        break;
    case 2:
        index = menu_cursor_page_move(&self->pick_0x16, self->pick_max_0x18, lobby_w.count_0x084,
                                      fn_802122AC(), &self->flags_0x08);
        switch (index) {
        case 1:
            slots = &lobby_world_block->slots_0x180[0];
            id = slots[self->cursor_0x1A].id;
            if (userdata_equip_item_slots_get(lobby_world_block)[list->entry_max_0x022].id != 0
                && id != userdata_equip_item_slots_get(lobby_world_block)[list->entry_max_0x022].id) {
                item_pair_copy(&tmp, &userdata_equip_item_slots_get(lobby_world_block)[list->entry_max_0x022]);
                fn_8004BD30(&fn_8004AF60(lobby_world_block, list->side_0x005)[list->entry_max_0x022]);
                item_take(id, self->pick_max_0x18,
                            &fn_8004AF60(lobby_world_block, list->side_0x005)[list->entry_max_0x022], 1, 1, 0);
                fn_8004B870(&lobby_world_block->slots_0x180[0], self->cursor_0x1A, -self->pick_max_0x18);
                fn_8004BEA4(tmp.id, tmp.value, &value);
            } else {
                item_take(id, self->pick_max_0x18,
                            &fn_8004AF60(lobby_world_block, list->side_0x005)[list->entry_max_0x022], 1, 1, 0);
                fn_8004B870(&lobby_world_block->slots_0x180[0], self->cursor_0x1A, -self->pick_max_0x18);
            }
            limit = fn_8004AF20(lobby_world_block);
            list->entry_max_0x022 = item_pair_index_find(id, userdata_equip_item_slots_get(lobby_world_block), limit);
            list->row_sub_0x016 = list->entry_max_0x022 / 8;
            self->phase_0x00 += 1;
            sysSE_stop(5);
            break;
        case 2:
            limit = fn_8004AF20(lobby_world_block);
            if (item_pair_index_find(lobby_world_block->slots_0x180[self->cursor_0x1A].id,
                            userdata_equip_item_slots_get(lobby_world_block), limit) < 0) {
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
extern "C" s32 fn_801EE1E4(LbListPane* self)
{
    IdValue tmp2;
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
                limit = fn_8004AF20(lobby_world_block);
                move = item_pair_index_find(fn_8004AF60(lobby_world_block, side)[id].id,
                                   userdata_equip_item_slots_get(lobby_world_block), limit);
                self->entry_max_0x022 = move;
                self->item_id_0x024 = fn_8004AF60(lobby_world_block, side)[id].id;
                if (move < 0) {
                    self->phase_0x001 += 1;
                    limit = fn_8004AF20(lobby_world_block);
                    move = item_pair_index_find(0, userdata_equip_item_slots_get(lobby_world_block), limit);
                    if (move >= 0) {
                        if (self->side_0x005 == 1 && *GetItemData(self->item_id_0x024) == 1) {
                            s16 extra = item_pair_index_find(0, &userdata_equip_item_slots_get(lobby_world_block)[0x18], 8);
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
            self->column_0x018 = menu_cursor_step(self->column_0x018, 8, fn_802122AC(), 1, 2);
        } else if (fn_802121F4(0xC) != 0) {
            self->column_sub_0x01A = menu_cursor_step_fixed_tail(self->column_sub_0x01A, self->column_sub_max_0x01B,
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
                if (fn_8004AFC8(lobby_world_block, id) != 0
                    && fn_8004B460(lobby_world_block, side) <= 0) {
                    self->phase_0x001 = 3;
                    item_pair_copy(&tmp2, &userdata_equip_item_slots_get(lobby_world_block)[id]);
                    fn_8004BD30(&fn_8004AF60(lobby_world_block, self->side_0x005)[id]);
                    item_take(self->item_id_0x024, self->move_max_0x01E,
                                &fn_8004AF60(lobby_world_block, self->side_0x005)[id], 1, 1, 0);
                    fn_8004BBEC(lobby_world_block, self->item_id_0x024, -self->move_max_0x01E, side, 1);
                    fn_8004BBEC(lobby_world_block, tmp2.id, tmp2.value, side, 1);
                    limit = fn_8004AF20(lobby_world_block);
                    self->entry_max_0x022 = item_pair_index_find(self->item_id_0x024,
                                                        userdata_equip_item_slots_get(lobby_world_block), limit);
                    self->row_sub_0x016 = self->entry_max_0x022 / 8;
                    sysSE_stop(5);
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
            self->row_0x014 = menu_cursor_step(self->row_0x014, 8, fn_802122AC(), 1, 2);
        } else if (fn_802121F4(0xC) != 0) {
            self->row_sub_0x016 = menu_cursor_step_fixed_tail(self->row_sub_0x016, self->row_sub_max_0x017,
                                                              fn_802122AC(), 4, 8, &self->move_0x01C);
        }
        break;
    case 2:
        r = menu_cursor_page_move(&self->entry_0x020, self->move_max_0x01E, lobby_w.count_0x084,
                                  fn_802122AC(), &self->move_0x01C);
        switch (r) {
        case 1:
            side = self->side_0x005 == 0;
            if (userdata_equip_item_slots_get(lobby_world_block)[self->entry_max_0x022].id != 0
                && self->item_id_0x024 != userdata_equip_item_slots_get(lobby_world_block)[self->entry_max_0x022].id) {
                item_pair_copy(&tmp2, &userdata_equip_item_slots_get(lobby_world_block)[self->entry_max_0x022]);
                fn_8004BD30(&fn_8004AF60(lobby_world_block, self->side_0x005)[self->entry_max_0x022]);
                item_take(self->item_id_0x024, self->entry_0x020,
                            &fn_8004AF60(lobby_world_block, self->side_0x005)[self->entry_max_0x022], 1, 1, 0);
                fn_8004BBEC(lobby_world_block, self->item_id_0x024, -self->entry_0x020, side, 1);
                fn_8004BBEC(lobby_world_block, tmp2.id, tmp2.value, side, 1);
            } else {
                item_take(self->item_id_0x024, self->entry_0x020,
                            &fn_8004AF60(lobby_world_block, self->side_0x005)[self->entry_max_0x022], 1, 1, 0);
                fn_8004BBEC(lobby_world_block, self->item_id_0x024, -self->entry_0x020, side, 1);
            }
            limit = fn_8004AF20(lobby_world_block);
            self->entry_max_0x022 = item_pair_index_find(self->item_id_0x024, userdata_equip_item_slots_get(lobby_world_block), limit);
            self->row_sub_0x016 = self->entry_max_0x022 / 8;
            self->phase_0x001 += 1;
            sysSE_stop(5);
            break;
        case 2:
            limit = fn_8004AF20(lobby_world_block);
            if (item_pair_index_find(self->item_id_0x024, userdata_equip_item_slots_get(lobby_world_block), limit) < 0) {
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
extern "C" u32 fn_801EE900(u16 item_id)
{
    if (item_id != 0 && fn_8033AAFC() == 1U) {
        return 1;
    }
    return 0;
}

/* Whether the entry's item is one the shop can accept. */
extern "C" s32 fn_801EE940(u16 index, u16 item_id)
{
    if (item_id != 0 && fn_8033AC78(index, 0) != 0) {
        return 1;
    }
    return 0;
}


/* The item-selection step machine driven by the digit entry's confirm (the swap variant). */
extern "C" s32 fn_801EEDC4(LbDigitPane* self, LbListPane* list)
{
    IdValue tmp;
    IdValue* slots;
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
                slots = &lobby_world_block->slots_0x180[0];
                if ((slots[value].id == 0 && slots[self->cursor_0x1A].id == 0)
                    || value == self->cursor_0x1A) {
                    sysSE_req(2);
                } else {
                    self->phase_0x00 -= 1;
                    item_pair_copy(&tmp, &slots[value]);
                    item_pair_copy(&slots[value], &slots[self->cursor_0x1A]);
                    item_pair_copy(&slots[self->cursor_0x1A], &tmp);
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
extern "C" s32 fn_801EEFA0(LbDigitPane* self, LbListPane* list)
{
    LbItemDb* db;
    s32 result;
    s16 value;
    s16 index;
    s32 r;

    db = lobby_world_block;
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
            index = db->slots_0x180[value].value;
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
        r = menu_cursor_page_move(&self->pick_max_0x18, self->pick_0x16, lobby_w.count_0x084,
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
        r = toggle_word_step(&self->stepper_0x04, fn_802122E8(), 4, 8, 0xFFFF);
        switch (r) {
        case 1:
            self->phase_0x00 = 0;
            score_add_clamped(self->pick_max_0x18 * eft052_item_value_get(db->slots_0x180[self->cursor_0x1A].id),
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
extern "C" s32 fn_801EF1D0(LbEquipPane* self)
{
    u16 id;

    id = self->ids_0x04[0];
    if (id != 0xFFFFU && self->ids_0x04[1] != 0xFFFFU && self->ids_0x04[2] != 0xFFFFU) {
        return 1;
    }
    if (id != 0xFFFFU) {
        return Gunner_opt_ok_ck(&lobby_world_block->equip_0x0E00[id]) == 0;
    }
    return 0;
}

/* Seeds the equipment pane with the three ids and the icon run. */
extern "C" void fn_801EF244(LbEquipPane* self, s16 index, u16 a, u16 b, u16 c, u8 side, s32 icon_index)
{
    u16 id_a;
    u16 id_b;
    u16 id_c;

    id_a = a;
    id_b = b;
    id_c = c;
    self->phase_0x00 = 0;
    self->index_0x02 = index;
    if (lobby_world_block->equip_0x0E00[lobby_world_block->kind_0x8C].kind_0x00 == 0xB) {
        if (a == 0xFFFFU) {
            if (Gunner_opt_ok_ck(&lobby_world_block->equip_0x0E00[lobby_world_block->kind_0x8C]) == 0) {
                id_a = 0xFFFF;
            } else {
                id_a = lobby_world_block->kind_0x8C;
            }
        }
        if (id_a != 0xFFFFU) {
            if (Gunner_opt_ok_ck(&lobby_world_block->equip_0x0E00[id_a]) == 0) {
                id_b = 0xFFFF;
                id_c = 0xFFFF;
            }
        } else {
            id_b = 0xFFFF;
            id_c = 0xFFFF;
        }
        if (id_b == 0xFFFFU) {
            id_b = lobby_world_block->sub_0x8E;
        }
        if (id_c == 0xFFFFU) {
            id_c = lobby_world_block->sub_0x90;
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
extern "C" u16 fn_801EF73C(LbDigitPane* self)
{
    return (u16)(s16)((self->digit_ones_0x0A + self->digit_hundreds_0x02 * 100)
                      + self->digit_tens_0x0C * 10);
}

} /* namespace s_801EC9F8 */
#pragma dont_inline reset
#pragma peephole reset

/* ==== absorbed from lobby/fn_801F3294.cpp (0x801F3294..0x801F9AF8) ==== */
#include "unsplit/lobby.h"
#include "hud/spr_data.h"
/* C++-linkage callees of this section that `unsplit/lobby.h` does not declare: they stay at global scope (their manglings are made there). */
void PutPageArrow(u16* table, s16 a, s16 b, u16 c, const _mh_ivec2_* pos, u8 flags);
void sysSE_req(s32 id);
void set_blendmode(u8 a, u8 b, u8 c);
/* lobby/fn_801F3294.cpp - a lobby page/panel group.
 *
 * `.text` 0x801F3294..0x801F9CD4 (48 functions, 27200 B), extab 0x8001094C..0x80010A7C (38 unwind-only
 * 8-byte records), extabindex 0x8002CCDC..0x8002CEA4 (38 x 12 B).  Registered once, at its final home
 * (docs/plan.md 12), from proposal/801F3294_fn_801F3294.cpp.
 *
 * Module `lobby`.  The range's callees are the lobby UI/equipment API (`LbStr`, `get_lsp_data`,
 * `chk_pointer`, `PutPageArrow`, `LbPutAnaPageArrow`, `draw_sprite*`, `sysSE_req`) and its neighbours in
 * splits.txt are `lobby/fn_801E7530.cpp` below and `lobby/lb_npc.cpp` above.  Language C++: the callee
 * set is full of compiler manglings (`get_lsp_data__FUsP10_mh_ivec2_`, `LbStr__FUcUs`,
 * `PutPageArrow__FPUsssUsPC10_mh_ivec2_Uc`) and the target objects carry extab.
 *
 * Name.  No `__FILE__` string sits in the range's data pool and `dumpmap.py lookup 0x801F3294` answers
 * only `zz_01f3294_`, so the file keeps the map's stem (brief section 2, class 3+4).
 *
 * Seam.  The right edge 0x801F9CD4 is the discovery byte cap, not a proven TU end - the next proposal
 * (0x801F9CD4) continues the same band - and the left edge is a weak cut (`tudiscover at 0x801F3294`
 * reports only closure-edge signals on both sides).  The extent settles as the rows match.
 *
 * State: 20 of the 48 rows are written, in address order fn_801F3294..fn_801F60D4 and fn_801F6A9C,
 * fn_801F6AC8, fn_801F865C, fn_801F86FC, fn_801F8ABC.  Seventeen are byte-identical and three are above
 * the 80 % bar.  `ninja build/RMHE08/report.json`: 10.302206 % fuzzy, 20 of 48 functions.
 * The 28 unwritten rows are absent, not stubbed, so the next session continues in address order at
 * 0x801F35B0 (284 B), 0x801F36CC (252 B), 0x801F3828 (368 B), 0x801F39E8 (1028 B), 0x801F3FDC (576 B),
 * 0x801F421C/0x801F4330 (276 B each), 0x801F44CC (336 B), 0x801F461C (256 B), 0x801F471C (232 B),
 * 0x801F4804 (1040 B), 0x801F4C14 (1892 B) ... and the two largest, fn_801F6168 (2356 B) and
 * fn_801F6DBC (5008 B), which alone are 27 % of the range.
 *
 * Shapes that earn their score:
 *   - `#pragma peephole off` over the whole unit: the target keeps the unfused forms the pass folds - a
 *     `clrlwi r0,r3,24` before `cmpwi`/`cmplwi` (fn_801F3998, fn_801F54C4, fn_801F3F78, fn_801F3DEC,
 *     fn_801F5534) and a `clrlwi` + `slwi` where we emit one `rlwinm` (fn_801F3F14) - playbook 39.
 *   - `__declspec(noinline)` on fn_801F353C, fn_801F3588 and fn_801F6034: `-inline auto` (cflags_lobby)
 *     folds each of the three into its caller, where the target keeps the out-of-line call.
 *   - `while (*table != -1)`, not `for (;;) { if (*table == -1) break; }`: the target rotates the loop so
 *     the test sits *below* the body and the preheader jumps to it.
 *   - a `switch` (not an if/else chain) wherever the target tests a value with `beq` to a body placed
 *     after the chain: fn_801F37C8, fn_801F3998 and fn_801F4444 (labels in the source order -1, -3, -2),
 *     fn_801F3EDC, and fn_801F3DEC (which needs the `default`-first block order).
 *   - the parameter widths the call sites show: a `u32`/`u16` parameter with the narrowing cast *at the
 *     use* keeps the target's `clrlwi` (fn_801F8ABC, fn_801F60D4), while a `u8` parameter keeps the
 *     target's raw `stb` and its own `clrlwi` for the compare (fn_801F3DEC).
 *   - two signatures the target's register use pins down: `fn_801F4444` takes three parameters, the
 *     middle one unused, and `fn_801F60D4` keeps a 12-byte `_SPR_DATA_` local (`+0x1C` is the colour
 *     word it overwrites), so `_SPR_DATA_` is completed here (a config_request asks for it to become the
 *     one shared definition - `include/unsplit/lobby.h` only forward-declares it).
 *
 * Residuals (measured with `ninja build/RMHE08/report.json`, per symbol):
 *   - fn_801F3294 97.03 %: the target's case-0 "done" step *shares* the case-2 body (a `b` into it) where
 *     our build duplicates the five-instruction block and pays a trailing `b` - everything else,
 *     including the dispatch, the mode switch and the unrolled `fn_802738E8` chain, matches.  Tried and
 *     rejected: the duplicated form (88.50), and the label order 0/2/1 that makes the share a real
 *     fallthrough (86.68 - it moves the case-2 body in front of case 1's and reorders the chain).
 *   - fn_801F86FC 97.00 %: register allocation only - the target keeps the `flags` pointer in r30 and
 *     uses r31 as the copy's scratch; ours takes r31 for the pointer.  The `world + 0xE00` base is
 *     hoisted to the top for both.
 *   - fn_801F5534 96.72 %: `(u16)count <= 1` needs the target's bare `cmplwi r5,1`; our `(u16)` cast
 *     emits `clrlwi r0,r5,16` first.  A `u16` local instead drops the mask but turns the index load into
 *     `lhz` where the target has `lha`.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap.py lookup
 * over the range's inventory: all 48 names are bare `.text` entries in config/RMHE08/symbols.txt and the
 * runtime dump answers only `zz_XXXXXXXX_` placeholders for them)
 */
#include "types.h"

#include "Runtime.PPCEABI.H/memset.h"
namespace s_801F3294 {
#define LOBBY_VIEW_IN_NAMESPACE 1
#include "lobby/lb_pane_ui.h"




extern "C" {

#pragma peephole off
/* 0x801F3294 - tick one lobby page request: a three-state machine that reads the pad mask, advances the
 * page cursor and pushes the new selection through `fn_802738E8`. */
u32 fn_801F3294(LbPageOwner* self, LbPage* req)
{
    u32 ret = 0;
    LbWorldBlock* world = lobby_world_block;
    LbPage* sub = self->page_0x010;

    switch (req->state_0x000) {
    case 0:
        if (req->done_0x001 == 1) {
            req->state_0x000 = 2;
            if (fn_801EC9E0(sub) == 1) {
                ret = 2;
            }
            break;
        }
        if (fn_8021213C(0x80) == 1) {
            fn_801EC828(self);
            break;
        }
        if (fn_8021213C(0x10) == 1) {
            if (fn_801EC9E0(sub) != 1) {
                break;
            }
            switch (req->mode_0x002) {
            case 0:
                if (world->count_0x009E != req->value_0x004) {
                    LbStepArg arg;

                    req->state_0x000++;
                    sysSE_stop(3);
                    fn_8004D0D8(world, (u8)req->value_0x004);
                    sub->flag_0x01C = world->count_0x009E;
                    memset(&arg, 0, sizeof(arg));
                    arg.kind_0x00 = 1;
                    fn_802738E8((struct _PLW*)sub, (u8*)&arg);
                    arg.kind_0x00 = 2;
                    fn_802738E8((struct _PLW*)sub, (u8*)&arg);
                    arg.kind_0x00 = 3;
                    fn_802738E8((struct _PLW*)sub, (u8*)&arg);
                    arg.kind_0x00 = 4;
                    fn_802738E8((struct _PLW*)sub, (u8*)&arg);
                    arg.kind_0x00 = 5;
                    fn_802738E8((struct _PLW*)sub, (u8*)&arg);
                    fn_801EC7AC(self);
                } else {
                    sysSE_req(2);
                }
                break;
            case 1:
                if (world->count_0x0002 != req->value_0x004) {
                    req->state_0x000++;
                    sysSE_stop(0x28);
                    fn_8004D0E0(world, (u8)req->value_0x004);
                    fn_80273998((struct _PLW*)sub, 5, world->count_0x0002);
                    fn_801EC7AC(self);
                } else {
                    sysSE_req(2);
                }
                break;
            }
        } else {
            if (fn_8021213C(0x20) == 1) {
                fn_8004C038(sub, world);
                req->done_0x001 = 1;
                sysSE_req(1);
            } else if (fn_802121F4(3) != 0) {
                req->value_0x004 = menu_cursor_step(req->value_0x004, req->count_0x006, fn_802122AC(), 1, 2);
            }
        }
        break;
    case 1:
        if (fn_801EC9E0(sub) == 1 && fn_8021213C(0x30) == 1) {
            req->state_0x000 = 0;
            sysSE_req(0);
        }
        break;
    case 2:
        if (fn_801EC9E0(sub) == 1) {
            ret = 2;
        }
        break;
    }
    return ret;
}

/* 0x801F353C - walk a `-1`-terminated `s16` table and report its largest value and its length.  The
 * target keeps the out-of-line call from `fn_801F3DEC`, so `-inline auto` may not fold it. */
__declspec(noinline) void fn_801F353C(const s16* table, s16* out_max, s16* out_count)
{
    s16 count = 0;
    s16 max = 0;

    while (*table != -1) {
        if (*table > max) {
            max = *table;
        }
        count++;
        table++;
    }
    if (out_max != NULL) {
        *out_max = max;
    }
    if (out_count != NULL) {
        *out_count = count;
    }
}

/* 0x801F3588 - sum a `-1`-terminated `s16` table (each entry as an unsigned 16-bit value).  The target
 * keeps the out-of-line call, so `-inline auto` must not fold it into its callers. */
__declspec(noinline) u32 fn_801F3588(const s16* table)
{
    u32 total = 0;

    while (*table != -1) {
        total += (u16)*table;
        table++;
    }
    return total;
}

/* 0x801F37C8 - store the selection the given row id names: the packed table bytes for an ordinary row,
 * or the page's own id for the "current" row marker. */
void fn_801F37C8(LbPage* self, s16 sel)
{
    u32 value;

    switch (sel) {
    case -3:
        value = self->id_0x014;
        break;
    case -2:
        return;
    default: {
        const LbSelRec* rec = &lbl_805B8674[sel];

        value = ((u32)rec->bits_0x03 << 24) | ((u32)rec->bits_0x04 << 16) | ((u32)rec->bits_0x05 << 8) | 0xFF;
        break;
    }
    }
    self->slot_0x018 = value;
}

/* 0x801F3998 - write the three colour bytes of one of the page's two palettes from the top three bytes
 * of a packed value. */
void fn_801F3998(LbPage* self, u32 rgb, u8 which)
{
    s32 kind = which;

    switch (kind) {
    case 1:
        self->rgb_a_0x258[0] = (u8)(rgb >> 24);
        self->rgb_a_0x258[1] = (u8)((rgb >> 16) & 0xFF);
        self->rgb_a_0x258[2] = (u8)((rgb >> 8) & 0xFF);
        break;
    case 0:
        self->rgb_b_0x25C[0] = (u8)(rgb >> 24);
        self->rgb_b_0x25C[1] = (u8)((rgb >> 16) & 0xFF);
        self->rgb_b_0x25C[2] = (u8)((rgb >> 8) & 0xFF);
        break;
    }
}

/* 0x801F3DEC - initialise one page record: clear its state, copy its kind from the world block and seed
 * the entry bounds from the kind's table. */
void fn_801F3DEC(LbPage* self, LbPage* next, u8 kind)
{
    self->state_0x000 = 0;
    self->done_0x001 = 0;
    self->kind_0x003 = kind;
    if (kind == 1 && fn_8004D70C(19001) == 1) {
        self->kind_0x003 = 2;
    }
    self->mode_0x002 = lobby_world_block->field_0x39BA;
    self->value_0x004 = 0;
    self->count_0x006 = 0;
    self->field_0x008 = 0;
    self->field_0x00E = 0;
    self->field_0x010 = 1;
    self->field_0x012 = 1;
    self->sel_id_0x014 = 0;
    self->field_0x023 = 0;
    switch (self->kind_0x003) {
    default:
        self->max_0x00A = 1;
        self->num_0x00C = 5;
        break;
    case 1:
        fn_801F353C(lbl_805B8768, &self->max_0x00A, (s16*)&self->num_0x00C);
        break;
    case 2:
        fn_801F353C(lbl_805B8798, &self->max_0x00A, (s16*)&self->num_0x00C);
        break;
    }
    self->next_0x028 = next;
}

/* 0x801F3EDC - the page's entry count: either table's total for its two scrollable kinds, else the
 * count the page itself carries. */
u32 fn_801F3EDC(LbPage* self)
{
    s32 kind = self->kind_0x003;

    if (kind != 1) {
        if (kind == 2) {
            return fn_801F3588(lbl_805B8798);
        }
        return self->num_0x00C;
    }
    return fn_801F3588(lbl_805B8768);
}

/* 0x801F3F14 - whether the given index table entry is the one the page is currently showing. */
u32 fn_801F3F14(LbPage* self, u8 index)
{
    s16 entry = lbl_805B875C[index];
    u8 current;

    if (entry == -2) {
        return 0;
    }
    current = self->sub_0x262;
    if (current == 0) {
        return entry == -1;
    }
    return lbl_805B8674[entry].match_0x02 == current;
}

/* 0x801F3F78 - forward a byte to the child panel and refresh its seven slots from the index table. */
void fn_801F3F78(LbPage* self, u8 value)
{
    u32 i;

    self->next_0x028->sub_0x262 = value;
    for (i = 0; i < 7; i++) {
        u8 slot = fn_802738B8((u8)i);

        if (slot != 0xFF) {
            fn_80223258((::_PLW*)self->next_0x028, slot);
        }
    }
}

/* 0x801F4444 - decode the selected table row into the page's selection word; the two negative markers
 * only raise/lower the "open" flag. */
void fn_801F4444(LbPage* self, u16 value_0x004, s16 sel)
{
    u16 value;

    switch (sel) {
    case -1:
        self->sel_flag_0x022 = 1;
        return;
    case -3:
        value = self->sel_id_0x014;
        break;
    case -2:
        return;
    default: {
        const LbSelRec* rec = &lbl_805B8674[sel];

        value = (u16)(((s32)(rec->bits_0x03 & 0xF8) << 8) | ((s32)(rec->bits_0x04 & 0xFC) << 3) |
                      ((s32)(rec->bits_0x05 & 0xF8) >> 3));
        break;
    }
    }
    self->sel_flag_0x022 = 0;
    self->sel_0x020 = value;
}

/* 0x801F54C4 - show the page-turn arrow for the current cursor, at the option's own row offset. */
void fn_801F54C4(void)
{
    _mh_ivec2_ pos;

    if ((u32)chk_pointer() == 1) {
        return;
    }
    get_lsp_data(6833, &pos);
    pos.y += 30;
    if (get_option_cfg(7) == 0) {
        fn_802DF7CC(5, &pos);
    } else {
        fn_802DF7CC(15, &pos);
    }
}

/* 0x801F5534 - the page's current row height, with the "extra" rows the two special pages add. */
s16 fn_801F5534(LbPage* self)
{
    s16 value = lbl_805B85F8[self->count_0x006];

    if (self->max_0x00A == 1) {
        s16 extra = self->field_0x008 + 1;

        value += extra;
        if ((u16)self->count_0x006 <= 1 && self->field_0x008 == 1 && userdata_gunner_ck(lobby_world_block) == 0) {
            value = (s16)(value + 1);
        }
    }
    return value;
}

/* 0x801F5FB0 - draw a page arrow at the given screen position. */
void fn_801F5FB0(s16 a, s16 b, u16 c, u8 kind)
{
    _mh_ivec2_ pos;

    if (kind == 1) {
        get_lsp_data(6877, &pos);
    } else {
        get_lsp_data(6862, &pos);
    }
    fn_802DB140(lbl_805B88D4, a, b, c, &pos);
}

/* 0x801F6034 - draw one entry's icon with the add-blend the item icons need, and the cursor underneath
 * when the caller asks for it.  `fn_801F60D4` must keep the out-of-line call, so `-inline auto` may not
 * fold the body into it. */
__declspec(noinline) void fn_801F6034(const _mh_ivec2_* pos, u32 id, u8 flag)
{
    if ((s16)id != -1 && (u32)chk_pointer() == 0) {
        set_blendmode(4, 1, 1);
        draw_sprite_anim_idx(6941, (u16)id, pos);
        set_blendmode(4, 5, 1);
    }
    if (flag == 1) {
        draw_sprite_idx(6942, pos);
    }
}

/* 0x801F60D4 - draw one item icon through the shared sprite block. */
void fn_801F60D4(const _mh_ivec2_* pos, u32 id, s16 sel, u32 flag, u32 extra)
{
    _SPR_DATA_ spr;

    fn_801F6034(pos, (s16)sel, (u8)flag);
    if ((u16)id != 0) {
        sprite_frame_apply(&spr, 6943, (u8)extra, 0);
        if ((u8)extra != 0) {
            spr.color_0x1C = 0x505050FF;
        }
        draw_itemicon_item_id(spr, (u16)id, pos);
    }
}

/* 0x801F6A9C - run the page's entry walk with the row and kind mask the caller names. */
void fn_801F6A9C(LbPageOwner* self, u32 row, u32 flags)
{
    fn_801F6168(self->slots_0x054, self->rows_0x030, row, (u8)flags, -1, self->tail_0x3B8, self->page_0x010);
}

/* 0x801F6AC8 - run the page's entry walk from its head, in the "no page" state. */
void fn_801F6AC8(u32 id, s16 row)
{
    fn_801F6168(NULL, NULL, id, 5, row, NULL, NULL);
}

/* 0x801F865C - run the state's own draw call: the entry list for states 0 and 2, the icon strip for
 * state 1. */
void fn_801F865C(LbPage* self, u32 id_a, void* page_b, u32 id_c, u8* ptr_f)
{
    _mh_ivec2_ pos;

    switch (self->state_0x000) {
    case 0:
    case 2:
        get_lsp_data((u16)id_a, &pos);
        fn_801F8318(self, pos.x, pos.y);
        break;
    case 1:
        get_lsp_data((u16)id_c, &pos);
        fn_801F6DBC(page_b, pos.x, pos.y, ptr_f);
        break;
    }
}

/* 0x801F86FC - set one icon record: copy the world's row for the id, or clear and label it when the id
 * is the "none" marker, then OR the record's bit into the caller's flag word. */
void fn_801F86FC(LbIconRec* recs, u32 id, u32 sel, u32 kind, u32 mode, u16* flags)
{
    LbIconRec* src = lobby_world_block->entries_0x0E00;
    LbIconRec* rec;

    if ((u16)id != 0xFFFF) {
        rec = &recs[(u8)kind];
        fn_8004A20C(rec, &src[(u16)id]);
    } else {
        rec = &recs[(u8)kind];
        memset(rec, 0, sizeof(LbIconRec));
        rec->kind_0x00 = (u8)sel;
    }
    if ((u8)mode == 1) {
        *flags |= (u16)(1 << (u8)kind);
    }
}

/* 0x801F8ABC - draw the page-turn arrow for one entry, with the "disabled" bit set when the caller
 * passes no room. */
void fn_801F8ABC(u32 id, u8 a, u8 b, u32 c, s32 room)
{
    _mh_ivec2_ pos;
    u8 flags = 1;

    get_lsp_data((u16)id, &pos);
    if (room == 0) {
        flags |= 0x80;
    }
    PutPageArrow(lbl_805B8810, (s16)a, (s16)b, (u16)c, &pos, flags);
}

#pragma peephole on

} /* extern "C" */

} /* namespace s_801F3294 */

/* ==== absorbed from lobby/fn_801F9CD4.cpp (0x801F9CD4..0x801FBB64) ==== */
/* lobby/fn_801F9CD4.cpp - the lobby character-edit (hair/inner colour) screen group.
 *
 * `.text` 0x801F9CD4..0x801FBF78 (14 functions, 8868 B), extab 0x80010A7C..0x80010ADC (12 unwind-only
 * 8-byte records), extabindex 0x8002CEA4..0x8002CF34 (12 x 12 B).  Registered from
 * `proposal/801F9CD4_fn_801F9CD4.cpp`; the extent is a maximal unclaimed run whose seam is unproven.
 *
 * Module `lobby`: the range reads the lobby state blocks (`lobby_w`, `lb_param_w`, `Screen_w`,
 * `system_w`), its callees are the lobby UI API (`get_lsp_data`, `draw_sprite_ary`,
 * `draw_sprite_anim_ary`, `GetMenuFontColor`, `LbStr`) and both bracketing registered units are
 * `lobby/*`.  Language C++: the two named symbols in the range are mangled and most of its callees
 * are (`GetMenuFontColor__Fbbbb`, `create_move_work__Fl`, `file_loading_ck__FPcPl`, ...).
 *
 * Flags: `cflags_lobby`, the group its two neighbours use.  Two file-scope pragmas are load-bearing and
 * measured:
 *   - `#pragma peephole off`: retail keeps the unfused `clrlwi`+`slwi` index scale and the separate
 *     `slwi`/`or` steps of the ARGB pack where the default peephole fuses them into `rlwinm`/`rlwimi`
 *     (`get_change_hair_color` 84.23 -> 100.00, `get_change_inner_color` the same).
 *   - `-Cpp_exceptions on` (`cflags_lobby`, flags-audit 2026-09-28): the target object carries
 *     extab/extabindex and the old `-Cpp_exceptions off` default emitted none.  With the pragma the `.text` is unchanged (all nine written
 *     functions keep their scores) and the unwind sections appear - one 8-byte record and one 12-byte
 *     index entry per written function, 0x38/0x54 against the target's 0x60/0x90 for the twelve the
 *     range will have.
 *
 * Shared headers this unit needed (each filed as a shared-file request in the handoff):
 *   - `unsplit/lobby.h`: the `.data`/`.sdata` tables of the range, `LbChangeColorRec`, the typed
 *     `lb_param_w` block, the `Psw` pad records, and `LbLobbyWork`'s +0x01/+0x02/+0x06/+0x14 fields.
 *   - `unsplit/unknown.h`: `system_w` +0x2D/+0x7CE/+0x8B1 split out of padding, and `unk2149` renamed
 *     to `field_0x865` (main.cpp's one use renamed with it).
 *   - two spellings in those headers disagree with the DOL and are worked around here rather than
 *     edited under another unit: `lobby_world_block` is a `.sbss` **pointer** (`lwz` in every reader, and
 *     `get_userdata` writes it), and `fn_80215C98` takes **five** arguments (all four DOL call sites
 *     pass `r7`).  Both are declared in the `lb_chg` scope below with the real shape.
 *
 * Residuals (measured with `recompile.py ... --measure <symbol>`):
 *   - `fn_801FB364` 99.89: the instructions are equal and only the frame differs - ours 0x30, the
 *     target's 0x20, with the same local offsets (0x8/0xC/0x10), so 16 bytes of the frame are an
 *     allocation difference, not a source one.
 *   - `fn_801FA0DC` 93.13: the range's shape is reproduced (both sentinel arms, the mode tail), but
 *     our allocator keeps the `lbl_805B875C` switch value in a callee-saved register across the two
 *     `get_lsp_data` calls where retail reloads it, which costs a save/restore and one register.
 *   - `fn_801FB524` 50.16: the state machine is reconstructed but 992 B against the target's 744 B -
 *     MWCC did not merge this compile's three identical `system_w.field_0x865 = 0` / `state = 3`
 *     blocks into the one copy retail has, and the dispatch came out a linear compare chain where
 *     retail has a binary search.
 *   - not reconstructed: `fn_801F9CD4` (0x408), `fn_801FA2C8` (0x690), `fn_801FA958` (0x880),
 *     `fn_801FB80C` (0x2D8), `fn_801FBB64` (0x414).  They are left unwritten rather than guessed;
 *     `fn_801FA0DC` is the sibling of `fn_801F9CD4` and already shows the shape the pair shares.
 *   - the unit's `.data` tables are declared, not claimed: the split range is `.text` + extab +
 *     extabindex only, and claiming data moves relocation handling (playbook row 23).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap.py lookup
 * on all 14 names in config/RMHE08/symbols.txt: 12 are bare `fn_` entries and the dump answers only
 * `zz_XXXXXXXX_` placeholders for them)
 */
#include "types.h"

#include "lobby/lb_npc.h" /* LbResId / LbResource / LbResRec - the lobby resource types (rule 2) */
#include "unsplit/lobby.h"
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */
#include "fn_8004CAD8/get_qResult_work.h" /* get_qResult_work (rule 2) */
#include "quest/quest_result_work.h"        /* Q_ResultWork (rule 1) */

#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * The two shared spellings that disagree with the DOL (see the header): `lobby_world_block` is a pointer,
 * and `fn_80215C98` takes five arguments.  A scoped `extern "C"` declaration cannot be beaten by a
 * global re-declaration - MWCC reports `illegal function overloading` (10197) - so the correct shape
 * lives in this scope, which names the same symbols.
 * ------------------------------------------------------------------------------------------------- */
namespace lb_chg {
extern "C" {
extern u8* lobby_world_block;
s32 fn_80215C98(s32 text, u8 flag, s16* pos, s32 color, u8 mode);
}
}  // namespace lb_chg

/* ---------------------------------------------------------------------------------------------------
 * Types this unit reconstructs.
 * ------------------------------------------------------------------------------------------------- */
/* The character-edit screen work object the two `self`-taking functions read. */
typedef struct LbChgColorWork {
    /* +0x00 */ u8 mode_0x00; /* 0 = body, 1 = hair - the tail's switch value */
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u8 table_0x02; /* 0 selects the body colour table, non-zero the hair one */
    /* +0x03 */ u8 unused_0x03;
    /* +0x04 */ s16 value_0x04;
    /* +0x06 */ s16 value_0x06;
    /* +0x08 */ s16 selected_0x08;
    /* +0x0A */ u8 unused_0x0A[2];
    /* +0x0C */ s16 value_0x0C;
    /* +0x0E */ s16 offset_0x0E;
    /* +0x10 */ s16 offset_0x10;
    /* +0x12 */ u8 unused_0x12[2];
    /* +0x14 */ u32 color_0x14;
    /* +0x18 */ u32 value_0x18;
    /* +0x1C */ u8 unused_0x1C[4];
    /* +0x20 */ void* cursor_0x20;
    /* +0x24 */ void* cursor_0x24;
    /* +0x28 */ void* table_0x28;
} LbChgColorWork; /* size: 0x2C */

/* The sequence record `fn_801FB524`'s caller owns: a step counter and the state the driver switches
 * on. */
typedef struct LbChgSeqWork {
    /* +0x00 */ u8 state_0x00;
    /* +0x01 */ u8 pad_0x01[9];
    /* +0x0A */ u8 step_0x0A;
    /* +0x0B */ u8 state_0x0B;
} LbChgSeqWork; /* size: 0xC */

/* One 8-byte row of the archive table `fn_801FB478` streams: the byte count and the source name. */
typedef struct LbChgFileReq {
    /* +0x0 */ u32 size_0x00;
    /* +0x4 */ const char* name_0x04;
} LbChgFileReq; /* size: 0x8 */

/* ---------------------------------------------------------------------------------------------------
 * Declarations.  `fn_*`/`ckResourceName` are plain C symbols; the lobby UI helpers come from
 * `unsplit/lobby.h`; the mangled C++ callees are declared with the signatures that reproduce their map
 * names (rule 9).  Declarations whose owner is another registered unit (`fn_801FCA80`, `fn_801FCADC`,
 * `fn_803C3F60`, ... - the `lobby/lb_npc.cpp` range) are filed as shared-file requests.
 * ------------------------------------------------------------------------------------------------- */
extern "C" {
s32 score_add_clamped(s32 delta, s32* value);
void fn_80040DE8(u8 mode);
s32 res_file_assign(void* dst, void* src);
void* res_file_ctor(void* out, u32 value);
void fn_800D5CAC(void* rec);
void fn_800E3358(u32 kind, u8 arg, void* str);
void fn_800F6520(void);
void fn_800F65B4(void);
void fn_800F6710(void);
void fn_801E92B0(void);
void fn_801F3588(const void* table);
s32 fn_801F36CC(const void* table, s16 a, s16 b);
s32 fn_801F3EDC(LbChgColorWork* self);
s32 fn_801F3F14(void* table, u8 index);
void fn_801F54C4(void);
void fn_801F9AF8(s16* pos, s32 color, s32 selected, s16 value, s32 mode);
void fn_801FB2A8(u8 a, u8 b, u8 c);
void fn_801FB364(u8 index);
void fn_801FB438(void);
void fn_801FB478(void);
void fn_801FBAE4(void);
void fn_801FC318(void);
void fn_801FC6A0(u32 unused, s32* value);
void fn_801FC810(void);
void fn_801FC874(void);
void fn_801FC8C8(void);
void fn_801FC8F0(void);
void fn_801FCA80(u32 index);
void fn_801FCADC(u8* self, u32 index);
void fn_8021D9F8(void);
void fn_802A0568(void);
void fn_802AD9CC(void);
s32 fn_802AE5C0(u32 a, u8 b, u8 c);
s32 fn_802AEEF8(u8 a, u8 b);
void fn_802AFA40(void);
s32 fn_802AFB48(u8 a, u8 b);
void fn_802B2318(u8 mode);
void fn_802B56E0(void);
void fn_802BB0EC(void);
void fn_802BECB8(char* buf, u8 index);
void sprite_frame_apply(void* dst, u16 id, u8 flag, const _mh_ivec2_* src);
void fn_8032422C(void);
void fn_8035A9E4(void);
void fn_803A75D8(void);
void fn_803C3A70(void);
void fn_803C3F60(void);
s32 fn_80449860(void);
s32 fn_804498C8(void);
s32 fn_804498CC(void);
s32 game_save_wait(void);
s32 chg_nand_err2msgcode(void);
}

/* The C++-linkage callees: their map names are manglings, and these signatures reproduce them. */
void* ckResourceName(char* name);
s32 create_move_work(long kind);
s32 file_loading_ck(char* name, s32* out);
void init_player_work(void);
void light_init(void);
void load_file(char* name, u32 dst, long size);
void player_control_move(void);
void player_init_data_load(void);
void setSoftresetFlag(bool flag);
void subTransSet(u32 a, long b, u32* value);
void village_tex_load(void);
void* work_mem_alloc(u32 size);

/**
 * Fills the caller's colour word and shape outputs from the hair-colour table entry the selection
 * index names.
 */
void get_change_hair_color(u8 index, u32* color, u16* out_id, s16* out_value_0x06, s16* out_value_0x08)
{
    const LbChangeColorRec* rec = &lbl_805B8674[lbl_805B87D8[index]];

    *out_id = rec->id_0x00;
    *color = ((u32)rec->r_0x03 << 24) | ((u32)rec->g_0x04 << 16) | ((u32)rec->b_0x05 << 8) | 0xFF;
    *out_value_0x06 = rec->value_0x06;
    *out_value_0x08 = rec->value_0x08;
}

/**
 * Fills the caller's colour word and shape outputs from the inner-colour table entry the selection
 * index names.
 */
void get_change_inner_color(u8 index, u32* color, u16* out_id, s16* out_value_0x06, s16* out_value_0x08)
{
    const LbChangeColorRec* rec = &lbl_805B8674[lbl_805B87F4[index]];

    *out_id = rec->id_0x00;
    *color = ((u32)rec->r_0x03 << 24) | ((u32)rec->g_0x04 << 16) | ((u32)rec->b_0x05 << 8) | 0xFF;
    *out_value_0x06 = rec->value_0x06;
    *out_value_0x08 = rec->value_0x08;
}

/* The range's own `fn_XXXXXXXX` definitions keep C linkage so objdiff pairs them by the map's
 * spelling (`extern "C"` - the mangled stem would pair nothing; playbook row 42). */
extern "C" {

/* 0x801FA0DC - draw the colour-slot list: one row per selectable colour with the cursor on the
 * current one, then the mode-dependent tail. */
void fn_801FA0DC(LbChgColorWork* self)
{
    _mh_ivec2_ origin;
    _mh_ivec2_ pos;
    u32 i;
    u16 count;
    s16 value;
    s32 color;
    s32 selected;
    s32 active;

    get_lsp_data(0x1C61U, &origin);
    draw_sprite_ary((const u16*)lbl_805B89D0, &origin);
    get_lsp_data(0x1C5BU, &origin);
    count = (u16)fn_801F3EDC(self);
    i = 0;
    for (; (u8)i < count; i++) {
        if (self->selected_0x08 == (s32)(u8)i) {
            selected = 1;
            active = 1;
        } else {
            selected = 0;
            active = 0;
        }
        color = GetMenuFontColor(1, active, 1, fn_801F3F14(self->table_0x28, (u8)i) == 1);
        get_lsp_data(lbl_805B8A04[(u8)i], &pos);
        pos.x += origin.x;
        pos.y += origin.y;
        value = lbl_805B875C[(u8)i];
        switch (value) {
        case -1:
            lb_chg::fn_80215C98((s32)LbStr(0, 0x231), (u8)active, (s16*)&pos, color, 8);
            break;
        case -2:
            fn_8021565C((u8)(selected | 0x80), (s16*)&pos);
            break;
        default:
            get_lsp_data(lbl_805B8A04[(u8)i], &pos);
            pos.x += origin.x;
            pos.y += origin.y;
            fn_801F9AF8((s16*)&pos, color, selected, lbl_805B875C[(u8)i], 0);
            break;
        }
    }
    {
        s32 mode = -1;

        switch (self->mode_0x00) {
        case 0:
            fn_801F54C4();
            mode = 0x120;
            break;
        case 1:
            mode = 0x124;
            fn_80215170(0x1879, (s32)self->cursor_0x24);
            break;
        }
        if (mode != -1) {
            fn_80214EF0(0x1877, (s16)mode);
        }
    }
}

/* 0x801FB2A8 - bring the editor screen up: the player/word init chain, the two lobby resource loads,
 * the colour-table setup and the mode handover. */
void fn_801FB2A8(u8 a, u8 b, u8 c)
{
    init_player_work();
    fn_803A75D8();
    fn_802AFA40();
    player_init_data_load();
    fn_800D5CAC((void*)lbl_80582988);
    fn_800D5CAC((void*)lbl_8058AA98);
    fn_801FCA80(a);
    fn_800F6520();
    fn_800F6710();
    if (isCityMode() == 0 || system_w.field_0x8b1 == 1) {
        village_tex_load();
    }
    fn_80040DE8(b);
    fn_802B2318(b);
    fn_802AE5C0(0, b, c);
}

/* 0x801FB364 - queue the model resources the editor needs: the `index + 1`-th row of the lobby
 * resource id table, the shared `mot_aicom.brres`, then the caller's own table entry. */
void fn_801FB364(u8 index)
{
    LbResRec dst;

    res_file_ctor(&dst, 0);
    {
        LbResId* ids = (LbResId*)lbl_80582B30;
        u32 name = ids[index + 1].b_0x04; /* the row's name pointer, a `u32` in the shared header */
        LbResource* res = (LbResource*)ckResourceName((char*)name);

        if (res != 0) {
            u32 tmp;

            res_file_assign(&dst, res_file_ctor(&tmp, (u32)res->payload_0x44));
            fn_800E3358(0, 0, &dst);
        }
    }
    {
        LbResource* res = (LbResource*)ckResourceName((char*)lbl_805B8CE0);

        if (res != 0) {
            u32 tmp;

            res_file_assign(&dst, res_file_ctor(&tmp, (u32)res->payload_0x44));
            fn_800E3358(3, 0, &dst);
        }
    }
    fn_801FCADC(NULL, index);
}

/* 0x801FB438 - run the nine subsystem initialisers the editor's first frame needs. */
void fn_801FB438(void)
{
    light_init();
    fn_800F65B4();
    fn_802AD9CC();
    fn_802B56E0();
    fn_8021D9F8();
    fn_802A0568();
    fn_8032422C();
    fn_8035A9E4();
    fn_803C3A70();
}

/* 0x801FB478 - load the ten `07/dcm%03d.bin` archives the editor streams, one work allocation each. */
void fn_801FB478(void)
{
    char name[0x20];
    LbChgFileReq* req;
    void** dst;
    s32 i;

    i = 3;
    req = (LbChgFileReq*)lbl_8058AFE8 + i;
    dst = (void**)lbl_806BC1D0 + i;
    for (; i <= 0xD; i++) {
        if (req != NULL && req->size_0x00 != 0 && *dst == NULL) {
            fn_802BECB8(name, (u8)i);
            *dst = work_mem_alloc(req->size_0x00);
            load_file(name, (u32)*dst, (s32)req->size_0x00);
        }
        req++;
        dst++;
    }
}

/* 0x801FBAE4 - hand the pending colour choice to the network side, then clear the option parameter
 * block back to its empty state. */
void fn_801FBAE4(void)
{
    Q_ResultWork* q = get_qResult_work();

    if (q->phase_0x1E2 == 3 && q->credit_0x3F0 > 0) {
        score_add_clamped(q->credit_0x3F0, (s32*)(lb_chg::lobby_world_block + 0x18));
    }
    lb_param_w.field_0x00 = 0;
    lb_param_w.field_0x04 = 0;
    lb_param_w.flag_0x0C[0] = 0;
    lb_param_w.value_0x10[0] = 0;
    lb_param_w.flag_0x0C[1] = 0;
    lb_param_w.value_0x10[1] = 0;
    lb_param_w.flag_0x0C[2] = 0;
    lb_param_w.value_0x10[2] = 0;
    lb_param_w.value_0x16 = 0;
    lb_param_w.value_0x18 = 0;
    lb_param_w.value_0x1A = 0;
    lb_param_w.value_0x1C = 0;
}

/* 0x801FB524 - the editor's sequence driver: one state step per frame, returning whether the sequence
 * has finished. */
s32 fn_801FB524(LbChgSeqWork* self)
{
    u16 status = Psw[0].button_0x2C0.pressed_0x04;
    s32 done = 0;
    Q_ResultWork* q;
    s32 v;

    switch (self->state_0x0B) {
    case 0:
        fn_801FBAE4();
        fn_801FC810();
        lobby_w.field_0x006 = 1;
        lobby_w.field_0x001 = 0x16;
        lobby_w.field_0x002 = 0;
        create_move_work(2);
        if (system_w.field_0x7ce == 2) {
            lobby_w.field_0x002 = 2;
        }
        fn_801FC318();
        fn_801FC874();
        fn_801FC8C8();
        fn_801E92B0();
        q = get_qResult_work();
        if (q != NULL && system_w.field_0x2d == 0 &&
            (q->kind_0x1E3 == 1 || q->kind_0x1E3 == 3)) {
            if (fn_80449860() != 0) {
                self->state_0x0B = 1;
                system_w.field_0x865 = 1;
                break;
            }
            if (fn_804498C8() != 0) {
                self->state_0x0B = 2;
                break;
            }
        }
        system_w.field_0x865 = 0;
        self->state_0x0B = 3;
        break;
    case 1:
        v = game_save_wait();
        if ((u8)(v + 2) <= 1U) {
            self->state_0x0B = 0x64;
            break;
        }
        if (v != 1) {
            break;
        }
        if (fn_804498C8() != 0) {
            self->state_0x0B = 2;
            break;
        }
        system_w.field_0x865 = 0;
        self->state_0x0B = 3;
        break;
    case 2:
        v = fn_804498CC();
        if ((u8)(v + 2) <= 1U) {
            self->state_0x0B = 0x64;
            break;
        }
        if (v != 1) {
            break;
        }
        if (fn_804498C8() != 0) {
            self->state_0x0B = 2;
            break;
        }
        system_w.field_0x865 = 0;
        self->state_0x0B = 3;
        break;
    case 3:
        fn_801FB2A8(0, lobby_w.field_0x001, lobby_w.field_0x002);
        self->state_0x0B++;
        setSoftresetFlag(0);
        break;
    case 4:
        if (file_loading_ck(NULL, NULL) != 1) {
            fn_801FB364(0);
            lobby_w.slots_0x00C[lobby_w.field_0x014] = (u32)(s8)fn_802AFB48(lobby_w.field_0x001, lobby_w.field_0x002);
            self->state_0x0B++;
        }
        break;
    case 6:
        fn_802AEEF8(lobby_w.field_0x001, lobby_w.field_0x002);
        fn_803C3F60();
        fn_801FC8F0();
        fn_802BB0EC();
        self->step_0x0A++;
        self->state_0x0B = 0;
        setSoftresetFlag(1);
        return 0;
    case 0x64:
        if ((status & 0x10) != 0) {
            system_w.field_0x865 = 0;
            self->state_0x0B = 3;
        } else {
            s32 code = chg_nand_err2msgcode();

            done = 1;
            subTransSet((u32)fn_801FC6A0, 1, (u32*)&code);
        }
        break;
    }
    if (self->state_0x0B >= 3) {
        player_control_move();
    }
    return done;
}

} /* extern "C" */
#pragma peephole reset
