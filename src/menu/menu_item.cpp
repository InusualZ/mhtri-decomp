/*
 * menu/menu_item.cpp - the item menu: the hit/land query tail of the Pl band (the hit registry, the id lists, the tile
 *   lookups), the item-name and item-data helpers, and the action and option lists' per-frame state machines and draw
 *   walk.  C++; every `fn_` stem is `extern "C"` so objdiff pairs it by the map's name.
 * RANGE. .text 0x80297E34-0x802A6624 (170 functions); extab, extabindex, .data 0x805CDC88-0x805CE00C, .bss
 *   0x806AC8A8-0x806AD698, .sdata 0x80792278-0x807922A0, .sdata2 0x8079A380-0x8079A3F8.  The left half
 *   (0x80297E34-0x8029F3C8) is the tail of the old Pl hit/land band, whose head is `Pl/pl_coll.cpp`.
 * FLAGS. `cflags_menu` (configure.py: the lib's `-opt nopeephole`, which makes the old `#pragma peephole off`
 *   redundant).  The Pl tail's functions compile instruction-identical under Wii/1.0 and Wii/1.3; the menu half needs 1.3.
 *   The range keeps `bl`s to its tiny same-file helpers (`GetItemData` from `fn_8029F704`/`item_category_ck`,
 *   `hit_flag_set` from `fn_8029F4C4`): `cflags_main`'s `-inline noauto`.
 * NAMES. Module and file from the `__FILE__` string `lbl_805CDFC8` = "menu_item.cpp", passed by every
 *   `nw4r::db::Panic` of the range (the dump's `_802a22a4s_menu_item.cpp_805cdfc8`, emitted by 0x802A22A4).  The runtime
 *   dump's real names: `body_set`, `hit_flag_set`, `hit_result_check`, `get_item_data_ptr`, `ItemName`, `ItemExp`,
 *   `GetItemData`, `get_menu_tbl_ptr`, `get_menu_lsp_tbl`, `put_menu_cursor`; the map's own `get_hit_id__Fv`.  The other
 *   named rows are GUESSes from their bodies (the dump answers `zz_` for them).
 * RESIDUALS. 96 rows unwritten (objdiff scores them zero): 0x80297E34-0x80299ED8, 0x80299EF8-0x8029A140,
 *   0x8029A19C-0x8029B8F4, 0x8029B918-0x8029D6FC, 0x8029D744-0x8029EFDC, 0x8029F204-0x8029F3C8 (`hit_data_apply`),
 *   0x8029F834-0x8029FA74, 0x8029FCFC-0x8029FFB8, 0x802A0568-0x802A16F8, 0x802A1714-0x802A2550,
 *   0x802A2564-0x802A2C98 (`put_menu_cursor` and five more), 0x802A2CB8-0x802A3190, 0x802A31D4-0x802A441C,
 *   0x802A4430-0x802A47F4, 0x802A4810-0x802A5444.  Blocked so far: `hit_data_apply`'s owner kinds 1 and 2 land on
 *   offsets `pl.h` does not name (+0x1E1 inside an `_EQUIP`, +0x08); the `fn_802996B4`..`fn_802997E4` family stores a
 *   float triple at the owner's +0x470, an `s16` in `pl.h`; every jump table of the Pl tail is zero-filled in the DOL,
 *   so a sparse switch's case mapping (`fn_80299EF8`, `fn_8029A358`, `fn_8029B918`, `fn_8029E35C`) is not recoverable.
 *   The 11 partial rows:
 *  - `fn_8029A140`: the branch sense of one `fcmpo`, and its tail block laid out in the other order;
 *  - `fn_8029F084`: retail keeps an unused accumulator (`li r5,0`, `+9` per outer iteration) MWCC deletes;
 *  - `item_category_ck`: the `and` operands in the other order (both spellings tried);
 *  - `fn_8029FA74`, `fn_8029FB00`: retail loads the byte straight into the argument register (`lbz r4` + `extsb r4,r4`),
 *    ours through r0;
 *  - `menu_item_frame_update`: retail masks `GameMode_ck`'s result (`clrlwi r3,r30,24`), ours moves it (`mr r3,r30`);
 *  - `fn_802A5444`, `fn_802A579C`, `fn_802A64B0`: one `mullw` with its operands in the other order; `fn_802A598C`
 *    the same plus two r0-vs-r4 argument loads;
 *  - `fn_802A5E64`: the same instructions, a different permutation of r24-r31.
 *  - source order is not address order (the 0x802A5444-0x802A6624 block precedes the 0x8029F3C8 bodies, `fn_8029F084`
 *    precedes the 0x8029A140 rows), so the emitted order differs from retail's (`relocdiff.py menu/menu_item --by-owner`:
 *    the extabindex symbols mismatch); a flip blocker whose fix is a source reorder, a decompilation task.
 *   flipcheck: `.data` (0x384), `.bss` (0xDF0) and `.sdata` (0x28) claimed but not emitted; short `.text` 0x2258 of
 *   0xE7F0, extab 0xB0 of 0x380, extabindex 0x108 of 0x540, `.sdata2` 0x10 of 0x78; the bytes of all four differ.
 * SHAPES. Two views of the menu slot at `.bss` 0x806AC8C8 meet here: `MenuSlot` (`menu/menu_item.h`) and `MENU_ITEM_W`
 *   (below, the option/action lists' view); where one symbol has one declaration the `MenuSlot` one wins and the list
 *   call site carries the difference:
 *  - `fn_8029FFFC(MenuSlot*, s32)` is called with `(s8)self->grid_row`: retail sign-extends there (`extsb r4,r4`);
 *  - `fn_802A4D98(MenuSlot*)` takes a pointer cast; `fn_8029F7C4`/`fn_8029F7E0`/`get_menu_lsp_tbl` return `u32`/`u32*` and
 *    the call sites cast to the pointer they hand on;
 *  - `ItemDataRecord`'s +0x000 is a union of both views (`kind_0x00`/`level_0x01` beside one `u16`).
 */

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "ef/fn_800CDB2C.h"   /* PlayMode_ck (rule 2: the owner is `ef/system_core.cpp`) */
#include "menu/hit_attack_list_push.h"
#include "Pl/pl_coll.h"   /* the owner of the `.bss` arrays `pl_land_data` / `pl_hit_id_list` (rule 2) */

/* Whether `id` is *absent* from the two ten-entry halves of a caller's 20-entry id table. */
extern "C" u32 fn_8029F084(u16* ids, u16 id)
{
    s32 checked;   /* the original's running count of tested entries (its increments are +9) */
    s32 row;

    checked = 0;
    for (row = 0; row < 2; row++, ids += 10) {
        s32 j;

        for (j = 0; j < 10; j++) {
            if (id == ids[j]) {
                return 0;
            }
        }
        checked += 9;
    }
    return 1;
}

/* Empties a 20-entry id table and clears the count byte that indexes it. */
extern "C" void hit_id_list_clear(u16* ids, u8* count)
{
    s32 i;

    *count = 0;
    for (i = 0; i < 20; i++) {
        ids[i] = 0xFFFF;
    }
}

/* Appends `id` to the ring, wrapping the count around at 20. */
extern "C" void fn_8029F1D4(u16* ids, u8* count, u16 id)
{
    ids[*count] = id;
    (*count)++;
    if ((u8)*count >= 20) {
        *count = 0;
    }
}

/* ------------------------------------------------------------------------------------------------ *
 * The hit registry (`.bss` `lbl_806AC8A8`).
 * ------------------------------------------------------------------------------------------------ */

/* Clears the registry: both list heads, both counts, the id counter and the trailing word. */
extern "C" void fn_80299ED8(void)
{
    lbl_806AC8A8.head_0x00 = 0;
    lbl_806AC8A8.head_0x04 = 0;
    lbl_806AC8A8.count_0x08 = 0;
    lbl_806AC8A8.count_0x0A = 0;
}

/* Pushes an active hit onto the registry's first list. */
extern "C" void hit_attack_list_push(_HIT_W* hit)
{
    HitRegistry* reg = &lbl_806AC8A8;
    _HIT_W* prev;

    if (hit->active_0x05 == 0) {
        return;
    }
    prev = reg->head_0x00;
    reg->head_0x00 = hit;
    hit->next_0x00 = prev;
    reg->count_0x08++;
}

/* The same push onto the registry's second list. */
extern "C" void hit_body_list_push(_HIT_W* hit)
{
    HitRegistry* reg = &lbl_806AC8A8;
    _HIT_W* prev;

    if (hit->active_0x05 == 0) {
        return;
    }
    prev = reg->head_0x04;
    reg->head_0x04 = hit;
    hit->next_0x00 = prev;
    reg->count_0x0A++;
}

/* Records one hit's owner and its kind. */
extern "C" void hit_source_set(_HIT_W* self, u8 kind, void* owner)
{
    self->owner_0x10 = owner;
    self->owner_kind_0x06 = kind;
}

/* Records the second owner slot of a hit. */
extern "C" void hit_owner_set(_HIT_W* self, u8 kind, void* owner)
{
    self->owner_0x14 = owner;
    self->kind_0x07 = kind;
}

/* Hands out the next hit id, wrapping around at 60000. */
u16 get_hit_id(void)
{
    HitRegistry* reg = &lbl_806AC8A8;

    reg->next_id_0x0C++;
    if (reg->next_id_0x0C >= 0xEA60) {
        reg->next_id_0x0C = 0;
    }
    return reg->next_id_0x0C;
}

/* ------------------------------------------------------------------------------------------------ *
 * The two id/tile lookups at the tail of the band.
 * ------------------------------------------------------------------------------------------------ */

/* The byte `lbl_805CDC88` holds for a tile id, or 0 for an id past the table. */
extern "C" u32 fn_8029B8F4(u32 id)
{
    if ((u8)id > 0x24) {
        return 0;
    }
    return lbl_805CDC88[(u8)id];
}

/* Whether two menu/quest ids agree, or the scene is in play mode 3. */
extern "C" u32 fn_8029D6FC(u8 a, u8 b)
{
    if (a == b || PlayMode_ck() != 3) {
        return 1;
    }
    return 0;
}

/* The four-way kind `hit_data_apply`'s two floats select: 0 when neither value is usable, 1/2 by which
 * slot carries the positive value and 3 when the second slot is the unusable default. */
extern "C" u32 fn_8029A140(_HIT_W* self)
{
    if (self->value_0x50 <= -100.0f) {
        return 0;
    }
    if (self->value_0x50 > 0.0f) {
        return 1;
    }
    if (self->value_0x54 > 0.0f) {
        return 2;
    }
    if (self->value_0x54 <= -100.0f) {
        return 3;
    }
    return 2;
}

/* ---- the item menu: the item-record helpers, the option and action lists and their draw walk (0x8029F3C8-0x802A6624) ---- */

#include "types.h"
#include "id_value.h"
#include "menu/menu_item.h"
#include "Pl/pl_act.h"
#include "unsplit/unknown.h"
#include "unsplit/lobby.h"
#include "unsplit/ef.h"
#include "Runtime.PPCEABI.H/memset.h"

#include "unsplit/lobby.h"
#include "unsplit/ef.h"
#include "Pl/fn_80273B14.h"               /* `Pl_item_id_usable_ck`, the id-usable predicate (rule 2) */
#include "Pl/fn_8027D684.h"
#include "g3d/g3d_anmchr.h"
#include "sound/fn_800D7F54.h"
/* nw4r's debug panic: the map name `Panic__Q24nw4r2dbFPCciPCce` is the front-end's spelling of this
 * declaration, so it sits at C++ scope (rule 9). */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r

/* ------------------------------------------------------------------------------------------------
 * the unit's own pooled literals and tables (declared, never defined - playbook 29)
 * ------------------------------------------------------------------------------------------------ */

/* The three literals every `Panic` in this range passes: the source file name and the assert
 * message.  `.data` 0x805CDFC8 (0xE B), 0x805CDFD8 (0x18 B). */
extern "C" const char lbl_805CDFC8[];
extern "C" const char lbl_805CDFD8[];

/* The cursor-arrow sprite rows `fn_802A6414` hands `fn_802DB140` (`.data` 0x805CDFF0). */
extern "C" u16 lbl_805CDFF0[];

/* One 0x130-byte record of the option table `lbl_806BE340` (`.bss`): +0x00 says the row is filled,
 * +0x03 is its label and +0x0D its value text.  Only those three fields are named.
 * size: 0x130 */
typedef struct MENU_OPTION_REC {
    /* +0x000 */ u8 valid_0x00;
    /* +0x001 */ u8 unused_0x01[0x02];
    /* +0x003 */ s8 label_0x03[0x0A];
    /* +0x00D */ s8 value_0x0D[0x123];
} MENU_OPTION_REC;

/* One 0xB20-byte record of the move-work array `get_move_work_adrs(2)` returns: the action list reads
 * the item name at +0x5CA and its value text at +0x5DB. size: 0xB20 */
typedef struct MENU_MOVE_WORK {
    /* +0x000 */ u8 unused_0x000[0x5CA];
    /* +0x5CA */ s8 name_0x5CA[0x11];
    /* +0x5DB */ s8 value_0x5DB[0x545];
} MENU_MOVE_WORK;

/* The option-list records `menu_list_fill`/`menu_list_names_set` and the draw walk: 0x130-byte records whose
 * +0x00 byte says the row is filled, +0x03 the label and +0x0D its value text.  `.bss` 0x806BE340. */
extern u8 lbl_806BE340[];

/* The block `fn_802AA0DC` draws for the panel (`self` +0x24).  Only ever passed by pointer, so the
 * run is opaque here. size: 0x48 */
typedef struct MENU_ITEM_PANEL {
    /* +0x00 */ u8 unused_0x00[0x48];
} MENU_ITEM_PANEL;

/* One 0x18-byte cell of the item grid at +0x6C that `menu_frame_draw_page` lays out.  The one byte this range
 * reads is +0x02: nonzero when the cell's entry may be picked. size: 0x18 */
typedef struct MENU_ITEM_SLOT {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ s8 ready_0x02;
    /* +0x03 */ u8 unused_0x03[0x15];
} MENU_ITEM_SLOT;

/* The sprite-data block the `draw_font`/`draw_sprite` family takes by reference: only the colour word
 * `_SPR_DATA_` +0x1C is named here (it is what the highlighted row overwrites).  `unsplit/lobby.h`
 * forward-declares the tag; this is its definition. size: 0x20 (the frame
 * `lobby/lb_pane_ui.cpp` reserves for it) */
typedef struct _SPR_DATA_ {
    /* +0x00 */ u8 unused_0x00[0x1C];
    /* +0x1C */ u32 color_0x1C;
} _SPR_DATA_;

/* The menu item list: `menu_item.cpp`'s window object.  Every offset below is read or written by this
 * range; the run from +0x3A to +0x18C is the layout the sprite/font helpers fill and is subsumed by
 * the two blocks they are handed. size: 0x23C (approximate: the highest offset the range touches;
 * the object is only ever reached as a pointer, never allocated here) */
typedef struct MENU_ITEM_W {
    /* +0x000 */ u8 pad_0x000[0x01];
    /* +0x001 */ u8 state;          /* 0 = idle, 1 = the list is being picked, 2 = the list is open */
    /* +0x002 */ u8 phase;          /* the phase inside `state` */
    /* +0x003 */ u8 pad_0x003[0x01];
    /* +0x004 */ u16 field_0x004;   /* the pressed-button word, OR'd with `field_0x008`'s low nibble */
    /* +0x006 */ u8 pad_0x006[0x02];
    /* +0x008 */ u16 field_0x008;   /* the held/repeat button word (only its low nibble is used) */
    /* +0x00A */ u8 pad_0x00A[0x05];
    /* +0x00F */ u8 menu_kind;      /* 1 = the action (motion) list, 2 = the option list */
    /* +0x010 */ u8 two_page;       /* when set, an index >= 24 selects `page_0x198` */
    /* +0x011 */ u8 slot_id;        /* the player slot `fn_802A04EC` is asked about */
    /* +0x012 */ u8 pad_0x012[0x04];
    /* +0x016 */ s8 field_0x016;    /* -1 = every entry, else the entry kind the list keeps */
    /* +0x017 */ u8 pad_0x017[0x03];
    /* +0x01A */ s8 action_no;      /* the selected action inside the player's move work */
    /* +0x01B */ u8 pad_0x01B[0x09];
    /* +0x024 */ MENU_ITEM_PANEL panel_0x024;
    /* +0x06C */ MENU_ITEM_SLOT slots_0x06C[12]; /* the run to +0x18C (12 x 0x18) */
    /* +0x18C */ u32 field_0x18C;  /* the pick-state word `toggle_word_step_dpad` advances (its low byte is the state `fn_802A9F48` draws) */
    /* +0x190 */ _PLW* plw;         /* the player work record the item list acts on */
    /* +0x194 */ IdValue* page_0x194; /* the first page's entries */
    /* +0x198 */ IdValue* page_0x198; /* the second page's entries (see `two_page`) */
    /* +0x19C */ u16 field_0x19C;   /* the key word `menu_cursor_column_step` is handed */
    /* +0x19E */ u16 field_0x19E;   /* its output word */
    /* +0x1A0 */ u8 grid_w;         /* the grid's column count */
    /* +0x1A1 */ u8 grid_h;         /* its row count */
    /* +0x1A2 */ u8 grid_row;
    /* +0x1A3 */ u8 grid_col;
    /* +0x1A4 */ u8 pad_0x1A4[0x0C];
    /* +0x1B0 */ u8 flag_0x1B0;     /* the grid owns the cursor this frame */
    /* +0x1B1 */ u8 flag_0x1B1;
    /* +0x1B2 */ u16 field_0x1B2;
    /* +0x1B4 */ s16 field_0x1B4;
    /* +0x1B6 */ s16 cursor;        /* the cursor `menu_cursor_page_move` moves */
    /* +0x1B8 */ s16 cursor_max;    /* its bound */
    /* +0x1BA */ s8 item_cursor;    /* the entry cursor (`items_0x1BE`) */
    /* +0x1BB */ u8 item_count;     /* how many entries `items_0x1BE` holds */
    /* +0x1BC */ u8 flag_0x1BC;     /* the pick was accepted */
    /* +0x1BD */ u8 pad_0x1BD[0x01];
    /* +0x1BE */ u8 items_0x1BE[0x0A]; /* the filled cells' entry indexes (the option list fills at most 10) */
    /* +0x1C8 */ char text_0x1C8[0x0A];/* the copied label */
    /* +0x1D2 */ char text_0x1D2[0x14];/* the copied value text */
    /* +0x1E6 */ s8 field_0x1E6;    /* the entry index `fn_802A6438` re-tests */
    /* +0x1E7 */ u8 field_0x1E7;    /* whether that entry may be picked (its high bit colours the row) */
    /* +0x1E8 */ u8 pad_0x1E8[0x50];
    /* +0x238 */ u16 move_result_0x238;/* which button last moved the cursor (`menu_cursor_page_move` writes it) */
    /* +0x23A */ u8 timer_0x23A;    /* the draw's cooldown, counted up to 3 */
    /* +0x23B */ u8 pad_0x23B[0x01];
} MENU_ITEM_W;

/* The folded half's callees (map stems, so C linkage - the front-end emits the map's
 * plain names) plus the two it hands a *pointer* view of the record to.  This unit's own
 * symbols among them - `fn_802A04EC`, `fn_8029FFFC`, `fn_802A4D98`, `fn_8029F7C4`,
 * `fn_8029F7E0`, `get_menu_lsp_tbl`, `GetItemData` - are declared once, in
 * `menu/menu_item.h`, and the call sites below follow those declarations (see
 * VIEWS in the file header for the two that cross the two views of the record). */
extern "C" {

void fn_802A4EF8(MENU_ITEM_W* self, s8 index);
s32 fn_802A4FEC(MENU_ITEM_W* self);
s32 fn_802A5320(MENU_ITEM_W* self);
void menu_list_count_update(MENU_ITEM_W* self);
u8 menu_list_mode_get(MENU_ITEM_W* self);
u8 menu_list_fill(MENU_ITEM_W* self, s8 filter);
s32 menu_list_names_set(MENU_ITEM_W* self, u8 index);
void menu_cursor_column_step(void* key);
void menu_frame_entries_build(void* page_a, void* page_b, s8 rows, s32 id, s32 flag);
void menu_frame_draw_page(void* page, void* slots, s8 cols, s8 rows, u16 timer, u32 flags);
void fn_802A9BCC(_PLW* plw, void* page_a, void* page_b);
u8 menu_slot_index_is_page_b(MENU_ITEM_W* self, s16 index);
u32 menu_item_slot_accepts(MENU_ITEM_W* self, u16 item_id, s16 index);
IdValue* menu_slot_get(MENU_ITEM_W* self, s16 index);
void fn_802A9F48(u8 value, u16 kind, s8* text, const _mh_ivec2_* pos, s32 flag);
void fn_802AA0DC(void* panel, s32 index, const _mh_ivec2_* pos);
void fn_802E2358(_SPR_DATA_* spr, s8* text, u32 length, const _mh_ivec2_* pos);
void fn_802E4828(s16 x, s16 y, s32 flag);
void fn_802DB140(u16* rows, s16 a, s16 b, u16 c, const _mh_ivec2_* pos);
s16 fn_802D72EC(u16 item_id);
u32 quest_move_state_valid_ck(void);
char* strcpy(char* dst, const char* src);
s32 fn_802A6434(_PLW* plw);
u32 fn_802A6438(MENU_ITEM_W* self, s16 index);
s32 fn_802A5444(MENU_ITEM_W* self);
s32 fn_802A579C(MENU_ITEM_W* self);
s32 fn_802A598C(MENU_ITEM_W* self);
void fn_802A5E64(MENU_ITEM_W* self);
s32 fn_802A64B0(MENU_ITEM_W* self);
void fn_802A6414(MENU_ITEM_W* self, const _mh_ivec2_* pos);

}  // extern "C"

/* The option list's frame handler: runs the menu's own state machine on the key word, then the item
 * list's two waits. */
extern "C" s32 fn_802A5444(MENU_ITEM_W* self) {
    s32 result = 0;
    u16 keys = (u16)(self->field_0x004 | (self->field_0x008 & 0xF));
    u8 mode;
    u32 ok;
    _PLW* plw;

    if (fn_802A6434(self->plw) == 0 && self->phase != 3) {
        return 3;
    }

    mode = menu_list_mode_get(self);
    switch (mode) {
    case 0:
    case 3:
        nw4r::db::Panic(lbl_805CDFC8, 396, lbl_805CDFD8);
        return 4;
    default:
        break;
    }

    switch (self->phase) {
    case 0:
        switch (menu_cursor_page_move(&self->cursor, self->cursor_max, self->field_0x004, self->field_0x008,
                                      &self->move_result_0x238)) {
        case 1:
            if (mode == 1) {
                sysSE_req(0);
                self->phase = 1;
            } else if (mode == 2) {
                sysSE_req(0);
                self->phase = 2;
            }
            break;
        case 2:
            result = 2;
            break;
        default:
            break;
        }
        break;

    case 1:
        if (keys & 0x10) {
            ok = 0;
            if (menu_list_fill(self, -1) != 0) {
                ok = 1;
            }
            if (ok == 1) {
                self->phase = 2;
            } else {
                sysSE_req(2);
            }
        } else if (keys & 0x20) {
            if (self->cursor_max != 1) {
                self->phase = 0;
                sysSE_req(1);
            } else {
                result = 2;
            }
        } else {
            if (menu_list_fill(self, -1) != 0) {
                if (keys & 3) {
                    self->item_cursor = (s8)menu_cursor_step(self->item_cursor, self->item_count, keys, 1, 2);
                }
            } else {
                self->item_cursor = 0;
                result = 2;
            }
        }
        break;

    case 2:
        plw = self->plw;
        if (mode == 1) {
            plw->field_0x655 = self->items_0x1BE[self->item_cursor];
        } else if (mode == 2) {
            plw->field_0x655 = 0x80;
        }
        if (menu_list_names_set(self, plw->field_0x655) == 0) {
            result = 2;
            break;
        }
        plw->field_0x650 = menu_slot_get(self, (s16)((s8)self->grid_row + (s8)self->grid_col * (s8)self->grid_w))->id;
        plw->field_0x652 = self->cursor;
        plw->pad_0x654[0] = 1;
        self->flag_0x1BC = 0;
        if (mode == 1) {
            if (get_move_work_adrs(2) != 0) {
                self->field_0x1B2 = plw->field_0x650;
                self->field_0x1B4 = plw->field_0x652;
            }
        } else {
            self->field_0x1B2 = 0;
            self->field_0x1B4 = 0;
        }
        self->phase = 3;
        break;

    case 3:
        if (fn_8027D738(self->plw) != 0) {
            if (mode == 1) {
                self->flag_0x1BC = 1;
            }
            result = 4;
            sysSE_req(0);
        } else {
            result = 1;
            sysSE_req(2);
        }
        break;

    default:
        break;
    }

    return result;
}

/* The pick list's frame handler: advances the grid cursor, then opens the entry the cursor sits on. */
extern "C" s32 fn_802A579C(MENU_ITEM_W* self) {
    s32 result = 0;
    u8 mode;
    _PLW* plw;
    MENU_MOVE_WORK* move_work;

    if (fn_802A6434(self->plw) == 0 && self->phase < 2) {
        return 3;
    }

    mode = menu_list_mode_get(self);
    if (mode != 3) {
        return 4;
    }

    switch (self->phase) {
    case 0:
        switch (menu_cursor_page_move(&self->cursor, self->cursor_max, self->field_0x004, self->field_0x008,
                                      &self->move_result_0x238)) {
        case 1:
            self->phase = 1;
            sysSE_req(0);
            break;
        case 2:
            result = 2;
            break;
        default:
            break;
        }
        break;

    case 1: {
        s16 cell = (s16)((s8)self->grid_row + (s8)self->grid_col * (s8)self->grid_w);
        plw = self->plw;
        plw->field_0x655 = self->items_0x1BE[0];
        plw->field_0x650 = menu_slot_get(self, cell)->id;
        plw->field_0x652 = self->cursor;
        plw->pad_0x654[0] = 1;
        self->flag_0x1BC = 0;
        move_work = (MENU_MOVE_WORK*)get_move_work_adrs(2);
        if (move_work != 0) {
            strcpy(self->text_0x1D2, (char*)move_work[plw->field_0x655].name_0x5CA);
            self->field_0x1B2 = plw->field_0x650;
            self->field_0x1B4 = plw->field_0x652;
        } else {
            nw4r::db::Panic(lbl_805CDFC8, 579, lbl_805CDFD8);
            result = 1;
        }
        self->phase = 2;
        break;
    }

    case 2:
        if (fn_8027D738(self->plw) != 0) {
            if (mode == 3) {
                self->flag_0x1BC = 1;
            }
            result = 4;
            sysSE_req(0);
        } else {
            result = 1;
            sysSE_req(1);
        }
        break;

    default:
        break;
    }

    return result;
}

/* The item list's own frame handler: the per-state cursor/key machine, then the item walk that
 * commits the selection. */
extern "C" s32 fn_802A598C(MENU_ITEM_W* self) {
    MENU_ITEM_W* unused_self = self;
    s32 result = 0;
    u16 keys = (u16)(self->field_0x004 | (self->field_0x008 & 0xF));
    IdValue* entry;
    u8 mode;
    s32 pick;
    s8 index;

    self->field_0x19C = keys;
    self->field_0x19E = 0;

    switch (self->state) {
    case 0:
        if (self->timer_0x23A != 0) {
            self->timer_0x23A--;
        }
        if (self->flag_0x1B0 != 0) {
            switch (toggle_word_step_dpad(&self->field_0x18C, keys, 1, 2)) {
            case 1:
                if (self->menu_kind == 2) {
                    item_pages_sort(self->page_0x194, self->page_0x198);
                } else {
                    fn_802A9BCC(self->plw, self->page_0x194, self->page_0x198);
                }
                self->flag_0x1B0 = 0;
                sysSE_req(0);
                break;
            case 2:
                self->flag_0x1B0 = 0;
                break;
            default:
                break;
            }
            if (self->flag_0x1B0 != 0) {
                break;
            }
            keys = 0;
        } else if (keys & 0x40) {
            self->flag_0x1B0 = 1;
            self->field_0x18C = 0;
            sysSE_req(13);
            break;
        }

        if (keys & 0x10) {
            if (self->slots_0x06C[(s8)self->grid_row].ready_0x02 != 0) {
                menu_list_count_update(self);
                fn_802A4EF8(self, self->action_no);
                self->state = 1;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (keys & 0x20) {
            result = 2;
            sysSE_req(1);
        } else {
            menu_cursor_column_step(&self->field_0x19C);
            if (keys & 0x8000) {
                if (self->flag_0x1B1 == 0) {
                    sysSE_req(1);
                } else {
                    sysSE_req(5);
                }
                self->flag_0x1B1 = self->flag_0x1B1 ^ 1;
            }
        }
        fn_8029FFFC((MenuSlot*)self, (s8)self->grid_row);
        break;

    case 1:
        entry = menu_slot_get(self, (s16)((s8)self->grid_row + (s8)self->grid_col * (s8)self->grid_w));
        if (entry->id == 0) {
            self->state = 0;
            break;
        }
        if (self->timer_0x23A < 3) {
            self->timer_0x23A++;
        }
        if (keys & 0x10) {
            if (fn_802A6434(self->plw) == 0) {
                sysSE_req(2);
                break;
            }
            switch (self->action_no) {
            case 1:
                self->state = 2;
                index = (s8)(self->grid_row + self->grid_col * self->grid_w);
                self->field_0x1E6 = index;
                fn_802A6438(self, (s8)(u8)index);
                sysSE_req(14);
                break;
            case 0:
                self->field_0x18C = 1;
                self->state = 2;
                sysSE_req(20);
                break;
            case 2:
                if (fn_802A64B0(self) == 1) {
                    mode = menu_list_mode_get(self);
                    self->item_cursor = 0;
                    self->cursor = 1;
                    self->cursor_max = entry->value;
                    if (mode == 2) {
                        s16 value = fn_802D72EC(entry->id);
                        if (value < self->cursor_max) {
                            self->cursor_max = value;
                        }
                    }
                    self->state = 2;
                    if (self->cursor_max != 1) {
                        self->phase = 0;
                    } else if (mode == 1) {
                        self->phase = 1;
                    } else {
                        self->phase = 0;
                    }
                    sysSE_req(0);
                } else {
                    sysSE_req(2);
                }
                break;
            default:
                break;
            }
        } else if (keys & 0x20) {
            self->state = 0;
            sysSE_req(1);
        } else if (keys & 3) {
            self->action_no = (s8)menu_cursor_step(self->action_no, self->field_0x016, keys, 1, 2);
        }
        fn_802A4EF8(self, self->action_no);
        break;

    case 2:
        pick = 2;
        switch (self->action_no) {
        case 1:
            pick = fn_802A4FEC(self);
            break;
        case 0:
            pick = fn_802A5320(self);
            break;
        case 2:
            pick = fn_802A5444(self);
            break;
        default:
            break;
        }
        switch (pick) {
        case 1:
            self->state = 0;
            break;
        case 2:
            sysSE_req(1);
            /* falls through */
        case 3:
            self->state = 1;
            fn_802A4EF8(self, self->action_no);
            break;
        case 4:
            result = 4;
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }

    fn_802A4D98((MenuSlot*)unused_self);
    return result;
}

/* Draws the whole item list: the two pages' grid, the panel lines the current phase selects, and the
 * per-entry label/icon row with the cursor. */
extern "C" void fn_802A5E64(MENU_ITEM_W* self) {
    u32 flags = 0;
    s32 draw_panel = 0;
    s32 draw_list = 0;
    s32 draw_arrow = 0;
    s32 draw_ok = 0;
    s32 line_kind = 0;
    u32 span = 0xFF;
    u32 two_page;
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    IdValue* entry;
    MENU_MOVE_WORK* move_work;
    u8 item;
    s32 i;
    u32 length;

    if (self->field_0x19E & 4) {
        flags |= 8;
    }
    if (self->field_0x19E & 8) {
        flags |= 16;
    }
    two_page = menu_slot_index_is_page_b(self, (s16)((s8)self->grid_w * (s8)self->grid_col));

    switch (self->state) {
    case 0:
        if (two_page != 0) {
            flags |= 1;
        }
        flags |= 0x80;
        if (self->flag_0x1B1 == 0) {
            flags |= 0x40;
        }
        if (self->flag_0x1B0 != 0) {
            draw_ok = 1;
        }
        break;

    case 1:
        if (two_page != 0) {
            flags |= 1;
        }
        flags |= 0x200;
        draw_panel = 3;
        line_kind = 2;
        span = (u8)self->action_no;
        break;

    case 2:
        switch (self->action_no) {
        case 1:
            if (two_page != 0) {
                flags |= self->field_0x1E7 ? 1 : 2;
            }
            flags |= 4;
            draw_panel = 3;
            line_kind = 2;
            span = 3;
            if (self->field_0x1E6 >= 24) {
                break;
            }
            if (two_page == 0) {
                break;
            }
            entry = menu_slot_get(self, self->field_0x1E6);
            if (entry->id == 0) {
                break;
            }
            if (GetItemData(entry->id)->kind_0x00 == 1) {
                break;
            }
            line_kind = 3;
            break;

        case 0:
            if (two_page != 0) {
                flags |= 1;
            }
            flags |= 0x200;
            draw_ok = 1;
            line_kind = 2;
            span = 4;
            break;

        case 2:
            if (two_page != 0) {
                flags |= 1;
            }
            flags |= 0x200;
            switch (self->phase) {
            case 0:
                draw_arrow = 1;
                line_kind = 2;
                span = 5;
                break;
            case 1:
                if (self->item_count != 0) {
                    draw_list = 1;
                }
                line_kind = 2;
                span = 6;
                break;
            default:
                break;
            }
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }

    if (self->flag_0x1B0 == 0) {
        menu_frame_draw_page(two_page != 0 ? self->page_0x198 : self->page_0x194, self->slots_0x06C,
                    (s8)(self->grid_w + 1), (s8)self->grid_h, (u16)self->timer_0x23A, flags);
    } else {
        menu_frame_entries_build(self->page_0x194, self->page_0x198, (s8)self->grid_h, 628, 0);
    }

    if (line_kind != 0 && (u8)span != 0xFF) {
        if (line_kind == 2) {
            get_lsp_data(732, &pos);
            fn_802E4828(pos.x, pos.y, 0);
            font_set_size(16, 16);
            font_print_ex((s16)(pos.x + 24), (s16)(pos.y + 16), 0, (s8*)fn_8029F7E0(37, (u8)span));
        } else if (line_kind == 3) {
            get_lsp_data(732, &pos);
            fn_802E4828(pos.x, pos.y, 0);
            font_set_size(16, 16);
            font_print_ex((s16)(pos.x + 24), (s16)(pos.y + 16), 0, (s8*)fn_8029F7E0(37, (u8)span));
            font_print_ex((s16)(pos.x + 24), (s16)(pos.y + 32), 2, (s8*)fn_8029F7E0(37, 8));
        }
    }
    font_flush();

    if (draw_arrow != 0) {
        get_lsp_data(803, &pos);
        fn_802A6414(self, &pos);
    }

    if (draw_list != 0) {
        get_lsp_data(748, &pos);
        draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(48), (u16)((self->item_count << 1) - 2), &pos);
        pos.x += 12;
        pos.y += 17;
        move_work = (MENU_MOVE_WORK*)get_move_work_adrs(2);
        for (i = 0; i < self->item_count; i++) {
            draw_sprite_ary((u16*)get_menu_lsp_tbl(49), &pos);
            spr_data_copy((s16*)&spr, get_lsp_data(772, 0));
            spr.color_0x1C = 0xC3C3C3FF;
            length = 0;
            if (i == self->item_cursor) {
                put_menu_cursor((u16*)get_menu_lsp_tbl(50), 0, &pos);
                spr.color_0x1C = 0xF0F0F0FF;
                length = 4;
            }
            item = self->items_0x1BE[i];
            if (self->menu_kind == 2) {
                MENU_OPTION_REC* option = &((MENU_OPTION_REC*)lbl_806BE340)[(s8)item];
                if (get_option_cfg(12) == 0) {
                    fn_802E2358(&spr, option->value_0x0D, length, &pos);
                } else {
                    draw_font(spr, option->label_0x03, length, &pos);
                }
            } else {
                MENU_MOVE_WORK* work = &move_work[(s8)item];
                if (get_option_cfg(12) == 0) {
                    fn_802E2358(&spr, work->name_0x5CA, length, &pos);
                } else {
                    draw_font(spr, work->value_0x5DB, length, &pos);
                }
            }
            pos.y += 26;
        }
    }

    if (draw_ok != 0) {
        get_lsp_data(741, &pos);
        fn_802A9F48((u8)self->field_0x18C, 2, (s8*)fn_8029F7C4(13), &pos, 0);
    }

    if (draw_panel != 0) {
        get_lsp_data(742, &pos);
        fn_802AA0DC(&self->panel_0x024, self->field_0x016, &pos);
    }
}

/* The cursor arrow: hands the player work record's cursor state to the arrow-table drawer. */
extern "C" void fn_802A6414(MENU_ITEM_W* self, const _mh_ivec2_* pos) {
    fn_802DB140(lbl_805CDFF0, self->cursor, self->cursor_max, self->move_result_0x238, pos);
}

/* The player work record's item-list test, through the Pl unit's own entry point. */
extern "C" s32 fn_802A6434(_PLW* plw) {
    return fn_80274570(plw);
}

/* Re-tests the entry the cursor sits on and records whether it may be picked. */
extern "C" u32 fn_802A6438(MENU_ITEM_W* self, s16 index) {
    if (menu_item_slot_accepts(self, menu_slot_get(self, self->field_0x1E6)->id, index) != 0) {
        self->field_0x1E7 = 1;
        return 1;
    }
    self->field_0x1E7 = 0;
    return 0;
}

/* Whether the entry the cursor sits on may be picked in the current menu kind. */
extern "C" s32 fn_802A64B0(MENU_ITEM_W* self) {
    u16 item_id;

    if (fn_802A04EC(0, self->slot_id) == 0) {
        nw4r::db::Panic(lbl_805CDFC8, 1175, lbl_805CDFD8);
        return 0;
    }

    item_id = menu_slot_get(self, (s16)((s8)self->grid_row + (s8)self->grid_col * (s8)self->grid_w))->id;

    switch ((u8)menu_list_mode_get(self)) {
    default:
        return 0;

    case 1:
        if (GetItemData(item_id)->level_0x01 >= 3) {
            return 0;
        }
        if (menu_list_fill(self, -1) == 0) {
            return 0;
        }
        break;

    case 3:
        if (GetItemData(item_id)->level_0x01 >= 3) {
            return 0;
        }
        if (menu_list_fill(self, (s8)self->slot_id) == 0) {
            return 0;
        }
        break;

    case 2:
        if (quest_move_state_valid_ck() == 0) {
            return 0;
        }
        if ((s16)fn_802D72EC(item_id) == 0) {
            return 0;
        }
        break;
    }

    return 1;
}

#include "types.h"
#include "menu/menu_item.h"
#include "Pl/pl_act.h"
#include "unsplit/unknown.h"
#include "Runtime.PPCEABI.H/memset.h"

/* 0x8029F3C8: fills a body slot from a body record (the appearance/hit state the item menu shows); `kind` picks the
 * work byte copied and its sub-code, the second argument is stored, not read. */
void body_set(_BODY_W* body, _BODY_DATA* data, u8 kind, u32 work, u8 mode)
{
    _BODY_DATA* work_rec = (_BODY_DATA*)work;

    body->field_0x005 = 1;
    body->field_0x006 = 0;
    body->kind = kind;
    body->field_0x010 = work;
    body->field_0x00A = 0;
    body->data = (u32)data;
    body->source_kind = 0;
    body->field_0x014 = 0;
    body->field_0x03F = mode;
    switch (kind) {
    case 0:
        body->source = work_rec->area;
        body->source_kind = 1;
        break;
    case 1:
        body->source = work_rec->field_0x1E1;
        body->source_kind = 2;
        break;
    case 2:
        body->source = work_rec->chunk_ofs;
        body->source_kind = 8;
        break;
    case 3:
        body->source = work_rec->field_0x1A4;
        body->source_kind = 4;
        break;
    case 4:
        body->source = get_now_areano();
        body->source_kind = 16;
        break;
    case 5:
        body->source = get_now_areano();
        body->source_kind = 32;
        break;
    }
}

/* 0x8029F4C4: flags a hit entry as populated and stores the four values the caller resolved. */
extern "C" void fn_8029F4C4(_HIT_W* hit, u16 a, u16 b, u16 c, u16 d)
{
    hit_flag_set(hit, 0x100);
    hit->field_0x018 = a;
    hit->field_0x01A = b;
    hit->field_0x01C = c;
    hit->field_0x01E = d;
}

/* 0x8029F51C: whether one of the hit entry's +0x05B bits is set. */
extern "C" u32 fn_8029F51C(_HIT_W* hit, u32 mask)
{
    return (hit->field_0x05B & (u8)mask) != 0;
}

/* 0x8029F538: clears the hit entry's flag word. */
extern "C" void hit_flags_clear(_HIT_W* hit)
{
    hit->flags = 0;
}

/* 0x8029F544: sets hit flags. */
void hit_flag_set(_HIT_W* hit, u32 flags)
{
    hit->flags |= flags;
}

/* 0x8029F554: clears hit flags. */
extern "C" void fn_8029F554(_HIT_W* hit, u32 flags)
{
    hit->flags &= ~flags;
}

/* 0x8029F564: whether any of the requested hit flags is set. */
extern "C" u32 fn_8029F564(_HIT_W* hit, u32 flags)
{
    return (hit->flags & flags) != 0;
}

/* 0x8029F57C: whether one of the hit entry's +0x031 bits is set. */
extern "C" u32 fn_8029F57C(_HIT_W* hit, u32 mask)
{
    return (hit->field_0x031 & (u8)mask) != 0;
}

/* 0x8029F598: the hit entry's result id, or 0xFF when the entry carries no result. */
u8 hit_result_check(_HIT_W* hit)
{
    if (hit->result_valid == 0) {
        return 255;
    }
    return hit->result;
}

/* 0x8029F5B4: stores the attack's damage value as the entry's +0x03C float. */
extern "C" void hit_knock_set(_HIT_W* hit, s16 value)
{
    hit->field_0x03C = (f32)value;
}

/* 0x8029F5E4: whether the hit entry's +0x00A state run is clear of the 0xD2 bit set. */
extern "C" u32 fn_8029F5E4(_HIT_W* hit)
{
    return (hit->state & 0xD2) == 0;
}

/* 0x8029F5F8: whether the hit entry's +0x00A state run is clear of the 0xB2 bit set. */
extern "C" u32 fn_8029F5F8(_HIT_W* hit)
{
    return (hit->state & 0xB2) == 0;
}

/* 0x8029F60C: the item the menu currently has selected. */
extern "C" u32 fn_8029F60C(void)
{
    return lbl_806AC8A8.field_0x004;
}

/* 0x8029F61C: the item table head, the block every accessor below reads. */
ItemDataHead* get_item_data_ptr(void)
{
    return &lbl_806AC8B8;
}

/* 0x8029F628: the display name id of an item; 0 when the id is out of the 747-entry table. */
u32 ItemName(u16 id)
{
    u32* names = lbl_806AC8B8.names;
    if (id >= 747) {
        id = 0;
    }
    return names[id];
}

/* 0x8029F654: the experience value of an item; 0 when the id is out of the table. */
u32 ItemExp(u16 id)
{
    u32* exp = lbl_806AC8B8.exp;
    if (id >= 747) {
        id = 0;
    }
    return exp[id];
}

/* 0x8029F680: reports an item's species kind to the caller, through the species table's +0x08 byte. */
extern "C" void fn_8029F680(u16 id)
{
    fn_8027EB18(fn_8029F6B4(GetItemData(id)->species)->kind);
}

/* 0x8029F6B4: the species record an item's +0x00A id selects; entry 0 when out of the 132-entry table. */
ItemSpeciesRecord* fn_8029F6B4(u16 id)
{
    if (id >= 132) {
        id = 0;
    }
    return &lbl_805DBFB8[id];
}

/* 0x8029F6DC: the 0x14-byte record of an item; entry 0 when the id is out of the table. */
ItemDataRecord* GetItemData(u16 id)
{
    ItemDataRecord* items = lbl_806AC8B8.items;
    if (id >= 747) {
        id = 0;
    }
    return &items[id];
}

/* 0x8029F704: the colour the item's +0x05 kind selects. */
extern "C" u32 fn_8029F704(u16 id)
{
    return lbl_805CDE78[GetItemData(id)->kind];
}

/* 0x8029F73C: the item record's +0x02 byte masked by the caller's value. */
extern "C" s32 item_category_ck(u16 id, s32 index)
{
    return index & GetItemData(id)->field_0x002;
}

/* 0x8029F774: whether the id is the menu's own item. */
extern "C" u32 fn_8029F774(u16 id)
{
    return id == 53;
}

/* 0x8029F788: the colour an item kind selects, without an item record. */
extern "C" u32 fn_8029F788(u16 kind)
{
    return lbl_805CDE78[kind];
}

/* 0x8029F7A0: the menu table set the three accessors below index. */
MenuTables* get_menu_tbl_ptr(void)
{
    return &lbl_806ACF28;
}

/* 0x8029F7AC: one entry of the menu table's leading pointer run. */
u32* get_menu_lsp_tbl(u16 idx)
{
    return lbl_806ACF28.lsp_tbl[idx];
}

/* 0x8029F7C4: the second table's entry for a menu index. */
extern "C" u32 fn_8029F7C4(u16 idx)
{
    return (u32)lbl_806ACF28.tab_0x66C[idx];
}

/* 0x8029F7E0: an entry of the table the second run's entry points at. */
extern "C" u32 fn_8029F7E0(u16 idx, u16 sub)
{
    return (u32)lbl_806ACF28.tab_0x66C[idx][sub];
}

/* 0x8029F808: the menu table's +0x754 word. */
extern "C" u32 fn_8029F808(void)
{
    return lbl_806ACF28.field_0x754;
}

/* 0x8029F818: the menu table's +0x758 run, indexed. */
extern "C" u32 fn_8029F818(u16 idx)
{
    return lbl_806ACF28.array_0x758[idx];
}

/* 0x8029FFB8: marks the one entry `index` names as selected in the first entry array and clears the
 * rest, then re-enters the menu's work initialiser. */
extern "C" void fn_8029FFB8(MenuSlot* slot, s32 index)
{
    s32 i;

    for (i = 0; i < (s8)slot->entry_count_a; i++) {
        if (i == index) {
            slot->entries_a[i].selected = 1;
        } else {
            slot->entries_a[i].selected = 0;
        }
    }
    return fn_8029FCFC();
}

/* 0x8029FFFC: the same selection over the second entry array. */
extern "C" void fn_8029FFFC(MenuSlot* slot, s32 index)
{
    s32 i;

    for (i = 0; i < (s8)slot->entry_count_b; i++) {
        if (i == index) {
            slot->entries_b[i].selected = 1;
        } else {
            slot->entries_b[i].selected = 0;
        }
    }
}

/* 0x802A0040: the same selection over the first slot's third entry array. */
extern "C" void fn_802A0040(s32 index)
{
    MenuSlot* slot = &lbl_806AC8C8.slot[0];
    s32 i;

    for (i = 0; i < (s8)slot->entry_count_c; i++) {
        if (i == index) {
            slot->entries_c[i].selected = 1;
        } else {
            slot->entries_c[i].selected = 0;
        }
    }
}

/* 0x802A008C: whether the slot's displayed player is in a state the item menu accepts input in. */
extern "C" u32 fn_802A008C(MenuSlot* slot)
{
    _PLW* worker = slot->worker;

    if (slot->field_0x00F == 1) {
        if (quest_select_ready_ck() != 0 && (u8)fn_802FBA60() == 0) {
            return 0;
        }
        if (Pl_motion_input_ck(0) == 1) {
            return 0;
        }
        if (fn_8027E120(worker) == 0) {
            return 0;
        }
    } else {
        if ((worker->field_0xB01 & 7) != 0) {
            return 0;
        }
        if (fn_8027D738(worker) == 1) {
            return 0;
        }
    }
    return 1;
}

/* 0x802A0148: whether the menu is in the state `move_work_state_ck` reports or a single area is loaded. */
extern "C" u32 fn_802A0148(void)
{
    if (move_work_state_ck() != 0) {
        return 1;
    }
    return quest_select_ready_ck() == 1;
}

/* 0x802A02CC: the slot-0 form of the predicate below. */
extern "C" u32 menu_busy_ck(void)
{
    return fn_802A02D4(0);
}

/* 0x802A02D4: whether a slot is in use, else what `fn_802DE670` reports for its index. */
extern "C" u32 fn_802A02D4(u8 idx)
{
    if (lbl_806AC8C8.slot[idx].active != 0) {
        return 1;
    }
    return fn_802DE670(idx);
}

/* 0x802A0304: the same predicate for the slot the frame work selects, which is the current game
 * mode's own slot. */
extern "C" u32 menu_item_frame_update(MenuFrameWork* self)
{
    MenuSlot* slot = &lbl_806AC8C8.slot[0];
    u32 idx = 0;

    if (self != 0) {
        switch ((u8)GameMode_ck()) {
        default:
            return 0;
        case 1:
            if (screen_split_mode_ck() != 0) {
                idx = self->slot_index;
                slot = &lbl_806AC8C8.slot[idx];
            }
            break;
        case 2:
            break;
        }
    }
    if (slot->active != 0) {
        return 1;
    }
    return fn_802DE670(idx);
}

/* 0x802A03A4: whether the first slot is in use in its single-player mode. */
extern "C" u32 fn_802A03A4(void)
{
    if (lbl_806AC8C8.slot[0].active != 0 && lbl_806AC8C8.slot[0].field_0x000 < 2) {
        return 1;
    }
    if (fn_802DE670(0) != 0) {
        return 1;
    }
    return 0;
}

/* 0x802A0404: the frame work's own predicate - the slot it selects, or its own two flags. */
extern "C" u32 fn_802A0404(MenuFrameWork* self)
{
    if (menu_item_frame_update(self) == 1) {
        return 1;
    }
    if (self->field_0x5BC != 0) {
        return 1;
    }
    return self->field_0x5BE != 0;
}

/* 0x802A0464: whether the first slot is in use, in mode 1, with the value 3 stored and its flag
 * cleared - the state the menu's item list accepts input in. */
extern "C" u32 fn_802A0464(void)
{
    if (lbl_806AC8C8.slot[0].active != 0 && lbl_806AC8C8.slot[0].field_0x000 == 1 &&
        lbl_806AC8C8.slot[0].field_0x014 == 3 && lbl_806AC8C8.slot[0].field_0x001 == 0) {
        return 1;
    }
    return 0;
}

/* 0x802A04B0: whether the first slot is in use in mode 2 with its flag set to 2. */
extern "C" u32 fn_802A04B0(void)
{
    if (lbl_806AC8C8.slot[0].active != 0 && lbl_806AC8C8.slot[0].field_0x000 == 2 &&
        lbl_806AC8C8.slot[0].field_0x001 == 2) {
        return 1;
    }
    return 0;
}

/* 0x802A04EC: whether a slot is in use in mode 1 and carries `value` as its stored byte. */
extern "C" u32 fn_802A04EC(s8 value, u8 idx)
{
    if (lbl_806AC8C8.slot[idx].active != 0 && lbl_806AC8C8.slot[idx].field_0x000 == 1 &&
        lbl_806AC8C8.slot[idx].field_0x014 == value) {
        return 1;
    }
    return 0;
}

/* 0x802A053C: stores the first slot's +0x021 byte. */
extern "C" void fn_802A053C(u8 value)
{
    lbl_806AC8C8.slot[0].field_0x021 = value;
}

/* 0x802A054C: the same store for the slot the caller names. */
extern "C" void fn_802A054C(u8 idx, u8 value)
{
    lbl_806AC8C8.slot[idx].field_0x021 = value;
}


/* 0x8029FA74: resets a slot's first entry array and both of its selection lists to the state the
 * caller's two stored bytes name. */
extern "C" void fn_8029FA74(MenuSlot* slot)
{
    memset(slot->entries_a, 0, sizeof(slot->entries_a));
    if (game_ready_ck() != 0) {
        slot->entry_count_a = 3;
        slot->entries_a[2].field_0x02 = 1;
    } else {
        slot->entry_count_a = 2;
    }
    slot->entries_a[0].field_0x02 = 1;
    slot->entries_a[1].field_0x02 = 1;
    fn_8029FFB8(slot, (s8)slot->field_0x019);
    fn_8029FFFC(slot, (s8)slot->field_0x01B);
}

/* 0x8029FB00: builds a slot's third entry array from the menu table the slot's state selects. */
extern "C" void fn_8029FB00(MenuSlot* slot)
{
    u32* table = 0;
    s32 i;

    memset(slot->entries_c, 0, sizeof(slot->entries_c));
    switch ((s8)slot->field_0x019) {
    case 0:
        if ((s8)slot->field_0x01B != 5) {
            break;
        }
        slot->entry_count_c = 3;
        table = (u32*)fn_8029F7C4(7);
        slot->entries_c[0].field_0x02 = 1;
        slot->entries_c[0].field_0x00 = 6;
        slot->entries_c[1].field_0x02 = 1;
        slot->entries_c[1].field_0x00 = 6;
        slot->entries_c[2].field_0x02 = 1;
        slot->entries_c[2].field_0x00 = 6;
        break;
    case 2:
        switch ((s8)slot->field_0x01B) {
        case 2:
            slot->entry_count_c = 3;
            table = (u32*)fn_8029F7C4(8);
            slot->entries_c[0].field_0x02 = 1;
            slot->entries_c[0].field_0x00 = 22;
            slot->entries_c[1].field_0x02 = 1;
            slot->entries_c[1].field_0x00 = 23;
            slot->entries_c[2].field_0x02 = 1;
            slot->entries_c[2].field_0x00 = 24;
            break;
        case 4:
            slot->entry_count_c = 3;
            table = (u32*)fn_8029F7C4(9);
            slot->entries_c[0].field_0x02 = 1;
            slot->entries_c[0].field_0x00 = 19;
            slot->entries_c[1].field_0x02 = 1;
            slot->entries_c[1].field_0x00 = 20;
            slot->entries_c[2].field_0x02 = 1;
            slot->entries_c[2].field_0x00 = 21;
            break;
        case 5:
            slot->entry_count_c = 4;
            table = (u32*)fn_8029F7C4(10);
            if (slot->field_0x00F == 2) {
                slot->entries_c[0].field_0x02 = 1;
                slot->entries_c[1].field_0x02 = 1;
                slot->entries_c[2].field_0x02 = 1;
            } else {
                slot->entries_c[0].field_0x02 = 0;
                slot->entries_c[1].field_0x02 = 0;
                slot->entries_c[2].field_0x02 = 0;
            }
            slot->entries_c[0].field_0x00 = 14;
            slot->entries_c[1].field_0x00 = 15;
            slot->entries_c[2].field_0x00 = 16;
            slot->entries_c[3].field_0x02 = 1;
            slot->entries_c[3].field_0x00 = 17;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    if (table != 0) {
        MenuEntry* entry = slot->entries_c;

        for (i = 0; i < (s8)slot->entry_count_c; i++, entry++) {
            entry->field_0x08 = table[i];
        }
    }
    s8 kind = slot->field_0x01A;
    fn_802A0040(kind);
}

/* 0x802A0188: releases both menu slots and puts the menu back into its unloaded state. */
extern "C" void fn_802A0188(void)
{
    MenuSlot* slot;
    s32 i;

    fn_802DA2D4(1);
    for (i = 0, slot = &lbl_806AC8C8.slot[0]; i < 2; slot++, i++) {
        if (slot->active != 0) {
            fn_8031A638(slot);
        }
    }
    lbl_806AC8C8.slot[1].active = 0;
    lbl_806AC8C8.slot[0].active = 0;
    if (lbl_806AC8C8.slot[0].field_0x32D != 0) {
        fn_8004082C();
        lbl_806AC8C8.slot[0].field_0x32D = 0;
        lbl_806AC8C8.slot[1].field_0x32D = 0;
    }
    fn_802DB26C();
    fn_80384380();
    if (game_ready_ck() == 1) {
        if (lbl_806AC8C8.slot[0].field_0x31E == 1 || lbl_806AC8C8.slot[1].field_0x31E == 1) {
            fn_804273EC(4, 0, 0);
        }
    }
}

/* 0x802A025C: the same release for the frame the caller's own menu state ends; returns whether the
 * display was handed back. */
extern "C" u32 fn_802A025C(void)
{
    u32 released = 0;

    fn_802DA2D4(1);
    fn_80384380();
    fn_802DE238();
    if (game_ready_ck() == 1 && lbl_806AC8C8.slot[0].field_0x31E == 1) {
        fn_804273EC(4, 0, 0);
        released = 1;
    }
    return released;
}

/* 0x802A16F8: clears the five 16-bit state words of the caller's record. */
extern "C" void fn_802A16F8(MenuEntryState* state)
{
    state->field_0x004 = 0;
    state->field_0x006 = 0;
    state->field_0x008 = 0;
    state->field_0x00C = 0;
    state->field_0x00A = 0;
}

/* 0x802A2550: copies the caller's two 16-bit coordinates. */
extern "C" void fn_802A2550(MenuEntryState* dst, const MenuEntryState* src)
{
    dst->field_0x000 = src->field_0x000;
    dst->field_0x002 = src->field_0x002;
}

/* 0x802A2C98: clears a slot's in-use flag. */
extern "C" void fn_802A2C98(u8 idx)
{
    lbl_806AC8C8.slot[idx].active = 0;
}

/* 0x802A3190: the second entry array's selector, over the slot the caller passes. */
extern "C" void fn_802A3190(MenuSlot* slot, s32 index)
{
    s32 i;

    for (i = 0; i < (s8)slot->entry_count_b; i++) {
        if (i == index) {
            slot->entries_b[i].selected = 1;
        } else {
            slot->entries_b[i].selected = 0;
        }
    }
}

/* 0x802A441C: puts the caller's record into its mode-5 state. */
extern "C" void fn_802A441C(MenuSlot* self)
{
    self->field_0x014 = 5;
    self->field_0x18C = 1;
}

/* 0x802A47F4: the mode-6 state of the same record. */
extern "C" void fn_802A47F4(MenuSlot* self)
{
    self->field_0x014 = 6;
    self->field_0x18C = 1;
    self->field_0x001 = 0;
}
