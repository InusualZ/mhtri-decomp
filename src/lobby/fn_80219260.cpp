/* lobby/fn_80219260.cpp - the lobby equipment page layer: it hands one player's `_PLW` (`self->plw_0x34`) to the
 *   `Put_equip_dtl_*` / `Put_status_equip_*` page and edits that player's equipment set.
 * RANGE. .text 0x80219260-0x8021E1EC (76 functions); .data 0x805B9DDC-0x805BA0C0, .bss 0x806AA790-0x806AA8C8, .sdata
 *   0x80791EB0-0x80791EE0, .sdata2 0x80799C40-0x80799C60, extab, extabindex.
 * FLAGS. `cflags_lobby` and file-scope `#pragma peephole off`: retail keeps `extsb` + `cmpwi` (`fn_8021CE0C`) and
 *   `clrlwi` + `cmpwi` (`fn_80219640`) unfused, and the pragma took `fn_802195E0`, `fn_8021AC8C`, `fn_8021D1D4` to
 *   byte-identical.
 * NAMES. No `__FILE__` string covers the range (the band's `enemy_control.cpp` at 0x805A1BB8 is never addressed from
 *   it) and the dump answers `zz_` throughout, so the file keeps the map's stem.  Module `lobby`: the lobby/HUD API
 *   (`LbStr`, `get_lsp_data`, `draw_sprite_ary`, `get_menu_lsp_tbl`) and the `_EQUIP` accessors (`Get_equip_rare`,
 *   `Gunner_opt_ok_ck`, `GetEquipName`, `ItemName`, `draw_weaponicon_idx`).
 *   GUESS: `userdata_award_bit_set`, `lb_talk_page_value_get`, `lb_talk_page_mode_set`,
 *   GUESS: `lb_talk_page_mode_reset`
 * RESIDUALS. 48 rows unwritten: 0x802196F0-0x8021AC24, 0x8021AD14-0x8021B5AC, 0x8021B690-0x8021B890,
 *   0x8021B94C-0x8021C900, 0x8021C938-0x8021CE0C, 0x8021CE70-0x8021D160, 0x8021D21C-0x8021D5A8, 0x8021D5D0-0x8021D704,
 *   0x8021D72C-0x8021D86C, 0x8021D884-0x8021D9E0, 0x8021D9F8-0x8021DED8, 0x8021DEFC-0x8021E1EC.
 *  - `fn_80219340`, `fn_80219640`: the two guard pairs and the `kind` byte are read from `self` in another order;
 *  - `fn_8021C900`: retail addresses the id table at 0x805C9F50 as its own base, the map's only symbol there is
 *    `lbl_805C9F40`, so ours needs one more `addi`; `fn_8021B5AC`: the same at 0x805C968C plus one `extsh`;
 *  - `fn_8021CE0C`: the base and byte registers swap (r4/r3). `fn_80218138` (owned by `lobby/fn_80212810.cpp`, which
 *    has no header) is declared here; `lb_talk_page_open` and `lb_talk_page_mode_reset`, this unit's own, are also declared in
 *    `unsplit/lobby.h`.
 *   flipcheck: `.data`/`.bss`/`.sdata`/`.sdata2` claimed, not emitted; `.text`/extab/extabindex short of the claim;
 *   `Gunner_opt_ok_ck` and `get_menu_lsp_tbl` are referenced unmangled where the map spells
 *   `Gunner_opt_ok_ck__FP6_EQUIP` and `get_menu_lsp_tbl__FUs`; `lbl_806BAB44` has no map row.
 */
#include "types.h"

#include "pl.h"
#include "unsplit/lobby.h"
#include "lobby/lb_menu_scratch.h" /* `lb_menu_scratch`, owned by lobby/lb_menu_scratch.cpp (rule 2) */

#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The retail object keeps the unfused compare forms the peephole pass folds (`extsb` + `cmpwi`
 * in `fn_8021CE0C`, `clrlwi` + `cmpwi` in `fn_80219640`), so the unit is built with the pass off. */
#pragma peephole off

extern "C" {

/* Owned by `lobby/fn_80212810.cpp` (whose header does not exist yet), declared with the neutral
 * object pointer both units pass. */
void fn_80218138(void* self);

/* Pl-band helpers with no registered owner (the bracketing Pl units make `unsplit/Pl.h`
 * their rule-2 home). */
u8 fn_8027E290(u8 kind);
u8 fn_8027E29C(u8 kind);
u32 fn_8026FE44(struct _PLW* plw);
s32 Gunner_opt_ok_ck(_EQUIP* equip);

/* Unsplittable callees whose bracketing registered units name different modules (rule 2's named gap:
 * `ai` below, `ef`/`hud`/`stage` above), so they are declared here. */
void fn_800D0754(u8 value);
void fn_800DCFE4(void);
const u16* get_menu_lsp_tbl(u16 id);
void lb_npc_talk_mode_set(void* slot, s32 value);
s32 fn_8021D21C(u8 a, s32 b, s32 c, s32 d);

void fn_8021B94C(void);

/* The `.sdata` sprite row `draw_sprite_ary` is handed. */
extern u16 lbl_80791EB0[4];

/* The menu-state block at 0x806AA790 (0x138 B in the map): the page's small persistent state. */
struct LbMenuState {
    /* +0x000 */ u8 unused_0x000[8];
    /* +0x008 */ u8 state_0x008;    /* the page's tick state (0 -> 1) */
    /* +0x009 */ u8 flags_0x009;    /* bit 0 = a slot is set, bit 1 = the selection timer is armed */
    /* +0x00A */ u8 unused_0x00A[2];
    /* +0x00C */ u8 unused_0x00C[4];
    /* +0x010 */ u32 word_0x010;    /* the menu value `fn_8021D170` publishes */
    /* +0x014 */ u32 word_0x014;
    /* +0x018 */ u32 word_0x018;
    /* +0x01C */ s32 word_0x01C;    /* the selection timer `fn_8021D1D4` advances */
    /* +0x020 */ s32 word_0x020;
    /* +0x024 */ u8 unused_0x024[0x0C];
    /* +0x030 */ u32 word_0x030;
    /* +0x034 */ u8 unused_0x034[0x104];
}; /* size: 0x138 */
extern LbMenuState lbl_806AA790;


/* The `stage_w` block (0x806BAB44, 0x2FE0 B in the map); only its +0x54 gate byte is read here. */
struct LbStageWork {
    /* +0x000 */ u8 unused_0x000[0x54];
    /* +0x054 */ u8 busy_0x054;
    /* +0x055 */ u8 unused_0x055[0x12D];
    /* +0x182 */ u8 flag_0x182;
}; /* size: 0x2FE0 */
extern LbStageWork lbl_806BAB44;

/* The id table `fn_8021C900` scans: `lbl_805C9F40` (0x2C B) with its u16 run at +0x10. */
struct LbIdTable {
    /* +0x00 */ u8 unused_0x00[0x10];
    /* +0x10 */ u16 ids_0x10[14];
}; /* size: 0x2C */
extern LbIdTable lbl_805C9F40;

/* The 6-byte selection-range table `fn_8021B5AC` scans: `lbl_805C9608` (0x390 B) with its rows at
 * +0x18 (id, range start, range length; 0xFFFF-terminated) and the fallback base row at +0x84. */
struct LbRangeTable {
    /* +0x000 */ u8 unused_0x000[0x18];
    /* +0x018 */ u16 rows_0x018[0x36];
    /* +0x084 */ u16 base_0x084[2];
}; /* size: 0x390 */
extern LbRangeTable lbl_805C9608;

/* A menu slot record (`fn_8021D188`'s argument and `fn_8021D704`'s only one). */
struct LbMenuSlot {
    /* +0x000 */ u8 unused_0x000[2];
    /* +0x002 */ u8 type_0x002;    /* 9 / 15 select the id-table entries below */
    /* +0x003 */ u8 unused_0x003[0x224];
    /* +0x227 */ u8 flags_0x227;   /* bit 0 = the slot is filled */
}; /* size: 0x228 (approximate: traced from the two fields this range reads) */

/* The 0x10-byte row `fn_8021DED8` copies field by field, with its float at +0x8. */
struct LbRow16 {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ f32 value_0x08;
    /* +0x0C */ u32 field_0x0C;
}; /* size: 0x10 */

/* The page's work object: the equipment kind it edits, the player whose set it stages, and the six
 * staged records (three per page side, see the `page` argument of `fn_80219340`).  Reached through
 * the `.sbss` pointer `lobby_world_block`. */
struct LbEquipWork {
    /* +0x0000 */ u8 kind_0x00;    /* the equipment kind the page edits (1-15) */
    /* +0x0001 */ u8 unused_0x01[2];
    /* +0x0003 */ u8 flag_0x03;
    /* +0x0004 */ u8 unused_0x04[4];
    /* +0x0008 */ s16 index_0x08;  /* reset to 0 once it reaches index_max_0x0A */
    /* +0x000A */ s16 index_max_0x0A;
    /* +0x000C */ u8 unused_0x0C[4];
    /* +0x0010 */ u8 flags_0x10;   /* low two bits: the staged-set flags */
    /* +0x0011 */ u8 unused_0x11[0x23];
    /* +0x0034 */ _PLW* plw_0x34;  /* the player whose equipment set this page edits */
    /* +0x0038 */ _EQUIP slots_0x38[6];
    /* +0x0080 */ u8 unused_0x80[0x4078];
    /* +0x40F8 */ u16 maskA_0x40F8; /* bits 0-11 of the owned-slot mask */
    /* +0x40FA */ u16 maskB_0x40FA; /* bits 12-19 */
    /* +0x40FC */ u8 unused_0x40FC[8];
    /* +0x4104 */ u32 bits_0x4104[4]; /* bits 0-117 of the owned-item mask */
}; /* size: 0x4114+ (approximate: traced from the fields this range reads; the object is reached through
      a pointer argument; nothing but `stage/shell.cpp` references `.bss` 0x806ADA58 (`shell_work_tbl`, 64 x 0x10C),
      so it does not live there) */

} /* extern "C" */

/* The staged-record lookup `fn_80219530` calls; defined below so MWCC keeps the retail `bl`
 * instead of inlining the whole switch into its caller. */
extern "C" _EQUIP* fn_80219260(LbEquipWork* self, s32 kind);

/* Stages `src` on the page side `page` selects: its triple of the six staged records is filled from
 * the source record, from the player's own set, or from a pair of guard records. */
extern "C" void fn_80219340(LbEquipWork* self, _EQUIP* src, s32 page) {
    u8 kind = self->kind_0x00;

    _EQUIP* a;
    _EQUIP* b;
    _EQUIP* c;
    _PLW* plw = self->plw_0x34;

    if ((u8)page == 0) {
        a = &self->slots_0x38[3];
        b = &self->slots_0x38[4];
        c = &self->slots_0x38[5];
    } else {
        a = &self->slots_0x38[0];
        b = &self->slots_0x38[1];
        c = &self->slots_0x38[2];
    }

    switch (kind) {
    case 1:
    case 2:
    case 5:
    case 6:
    case 10:
        memcpy(a, src, 12);
        break;
    default:
        if (fn_8026FE44(plw) == 1) {
            switch (src->kind) {
            case 11:
                memcpy(a, src, 12);
                if (Gunner_opt_ok_ck(src) == 0) {
                    memset(b, 0, 12);
                    memset(c, 0, 12);
                    b->kind = 12;
                    b->item_id = src->item_id;
                    c->kind = 13;
                    c->item_id = src->item_id;
                } else {
                    memcpy(b, &plw->equipC, 12);
                    memcpy(c, &plw->equipD, 12);
                }
                break;
            case 12:
                memcpy(a, &plw->equipB, 12);
                memcpy(b, src, 12);
                memcpy(c, &plw->equipD, 12);
                break;
            case 13:
                memcpy(a, &plw->equipB, 12);
                memcpy(b, &plw->equipC, 12);
                memcpy(c, src, 12);
                break;
            default:
                memcpy(a, src, 12);
                break;
            }
        } else {
            memcpy(a, src, 12);
        }
        break;
    }
}

/* Stages `src` on the page's second side and refreshes the page, resetting the index when it ran
 * past its maximum. */
extern "C" void fn_80219530(LbEquipWork* self, _EQUIP* src) {
    _EQUIP* equip = fn_80219260(self, src->kind);

    fn_80219340(self, equip, 1);
    fn_80218138(self);
    if (self->index_0x08 >= self->index_max_0x0A) {
        self->index_0x08 = 0;
    }
}

/* Stages `src` on the page's first side and refreshes the page, resetting the index when it ran past
 * its maximum. */
extern "C" void fn_80219590(LbEquipWork* self, _EQUIP* src) {
    fn_80219340(self, src, 0);
    fn_80218138(self);
    if (self->index_0x08 >= self->index_max_0x0A) {
        self->index_0x08 = 0;
    }
}

/* Returns the player's equipment record for `kind`: weapon slots 1-6 through fn_8027E290, the player's own armour for
 * 7-11/13, and the player's or the gunner copy for 12/14/15. */
extern "C" _EQUIP* fn_80219260(LbEquipWork* self, s32 kind) {
    switch ((u8)kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 14:
    case 15:
        return &self->plw_0x34->equipB;
    case 12:
        if (fn_8026FE44(self->plw_0x34) == 1) {
            return &self->plw_0x34->equipC;
        }
        return &self->plw_0x34->equipB;
    case 13:
        if (fn_8026FE44(self->plw_0x34) == 1) {
            return &self->plw_0x34->equipD;
        }
        return &self->plw_0x34->equipB;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        return &self->plw_0x34->equipA[fn_8027E290(kind)];
    default:
        return 0;
    }
}

/* Draws the page's sprite row: side 0 sits under the weapon-icon row, side 1 under the menu table
 * fetched by id. */
extern "C" void fn_802195E0(s32 page) {
    _mh_ivec2_ pos;

    if ((u8)page == 0) {
        get_lsp_data(0x19A9, &pos);
        draw_sprite_ary(lbl_80791EB0, &pos);
    } else {
        get_lsp_data(0x1C3D, &pos);
        draw_sprite_ary(get_menu_lsp_tbl(137), &pos);
    }
}

/* Whether the page's staged side holds the three guard records (a body piece, then its two upgrades)
 * with both lower pieces filled and the top one empty. */
extern "C" s32 fn_80219640(LbEquipWork* self, s32 page, s32 kind) {
    _EQUIP* a;
    _EQUIP* b;
    _EQUIP* c;

    if ((u8)page == 1) {
        a = &self->slots_0x38[0];
        b = &self->slots_0x38[1];
        c = &self->slots_0x38[2];
    } else {
        a = &self->slots_0x38[3];
        b = &self->slots_0x38[4];
        c = &self->slots_0x38[5];
    }

    return (self->flags_0x10 & 3) != 0
        && (u8)(self->kind_0x00 - 5) <= 1
        && (u8)kind == 2
        && a->kind == 11
        && b->kind == 12
        && c->kind == 13
        && a->item_id != 0
        && b->item_id != 0
        && c->item_id == 0;
}

/* Publishes the sound the given equipment kind wants. */
extern "C" void fn_8021AC24(LbEquipWork* self) {
    u8 kind = self->kind_0x00;

    switch (kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        fn_800D0754(fn_8027E29C(kind));
        break;
    case 11:
    case 12:
    case 13:
        fn_800D0754(4);
        break;
    default:
        fn_800D0754(0);
        break;
    }
}

/* Reads bit `index` of the page's owned-slot mask. */
extern "C" u16 fn_8021AC8C(LbEquipWork* self, s32 index) {
    if ((s16)index < 12) {
        return (u16)(self->maskA_0x40F8 & (1 << (s16)index));
    }
    return (u16)(self->maskB_0x40FA & (1 << ((s16)index - 12)));
}

/* Sets bit `index` of the page's owned-slot mask. */
extern "C" void userdata_award_bit_set(LbEquipWork* self, s32 index) {
    if ((s16)index < 12) {
        self->maskA_0x40F8 |= (u16)(1 << (s16)index);
        return;
    }
    self->maskB_0x40FA |= (u16)(1 << ((s16)index - 12));
}

/* Returns the entry of the 6-byte range table whose [start, start + length) interval holds `value`,
 * or the index past the table's base row when no interval does. */
extern "C" s16 fn_8021B5AC(s32 value) {
    s16 found = -1;
    s32 i = 0;
    const u16* row = lbl_805C9608.rows_0x018;
    s16 v = (s16)value;

    while (row[0] != 0xFFFF) {
        if ((s32)row[1] <= (s32)v && (s32)v < (s32)row[1] + (s32)row[2]) {
            found = i;
        }
        i++;
        row += 3;
    }
    if (found < 0) {
        found = i + (v - (s32)lbl_805C9608.base_0x084[1]);
    }
    return found;
}

/* Whether item index 0-117 is in the page's owned-item bit mask. */
extern "C" u32 fn_8021B624(LbEquipWork* self, s32 index) {
    s16 i = (s16)index;

    if (i > 117) {
        return 0;
    }
    return self->bits_0x4104[i >> 5] & (1 << (i & 31));
}

/* Adds item index 0-117 to the page's owned-item bit mask. */
extern "C" void fn_8021B65C(LbEquipWork* self, s32 index) {
    s16 i = (s16)index;

    if (i > 117) {
        return;
    }
    self->bits_0x4104[i >> 5] |= 1 << (i & 31);
}

/* Whether the stage work block is idle. */
extern "C" s32 fn_8021B890(void) {
    return lbl_806BAB44.busy_0x054 == 0;
}

/* Whether the two menu kinds belong together (kind 22 with 1, kind 21 with the decorations 5-7). */
extern "C" s32 fn_8021B8A8(s32 type, s32 value) {
    switch ((u8)type) {
    case 22:
        if ((u8)value == 1) {
            return 1;
        }
        break;
    case 21:
        if ((u32)(u8)value - 5U <= 2U) {
            return 1;
        }
        break;
    }
    return 0;
}

/* Whether menu kind 21 carries the value 4. */
extern "C" s32 fn_8021B8F4(s32 type, s32 value) {
    if ((u8)type == 21) {
        if ((u8)value == 4) {
            return 1;
        }
    }
    return 0;
}

/* Whether the menu-state block's first flag bit is set. */
extern "C" s32 fn_8021B91C(void) {
    LbMenuState* work = &lbl_806AA790;
    u8 slot_set;

    if (work == 0) {
        return 0;
    }
    slot_set = work->flags_0x009 & 1;
    return slot_set != 0;
}

/* Whether `id` is one of the ids in the page's own id table. */
extern "C" s32 fn_8021C900(u16 id) {
    const u16* p = lbl_805C9F40.ids_0x10;

    while (*p != 0) {
        if (id == *p) {
            return 1;
        }
        p++;
    }
    return 0;
}

/* Returns the page's current menu value. */
extern "C" u32 lb_talk_page_value_get(void) {
    return lbl_806AA790.word_0x014;
}

/* Sets the page's menu value and its previous value. */
extern "C" void fn_8021D170(s32 value) {
    lbl_806AA790.word_0x010 = (u8)value;
    lbl_806AA790.word_0x014 = (u8)value;
}

/* Hands the page's selection record to the handler its type asks for. */
extern "C" void fn_8021D188(LbMenuSlot* slot, s32 id) {
    if (slot == 0) {
        return;
    }
    switch (slot->type_0x002) {
    case 9:
        if ((u16)id != 184) {
            return;
        }
        lb_npc_talk_mode_set(slot, 2);
        return;
    case 15:
        if ((u16)id != 208) {
            return;
        }
        lb_npc_talk_mode_set(slot, 0);
        return;
    default:
        return;
    }
}

/* Advances the page's selection timer once and stores the value the next step starts from. */
extern "C" s32 fn_8021D1D4(void) {
    if (lbl_806AA790.flags_0x009 & 2) {
        return 0;
    }
    lbl_806AA790.word_0x020 = lbl_806AA790.word_0x01C / 2 + 1;
    lbl_806AA790.word_0x01C = lbl_806AA790.word_0x01C + 1;
    return 1;
}

/* Switches the page to the given menu mode. */
extern "C" s32 lb_talk_page_mode_set(s32 mode) {
    return fn_8021D21C((u8)mode, 0, 0, 1);
}

/* Switches the page back to menu mode 1. */
extern "C" s32 lb_talk_page_mode_reset(void) {
    return fn_8021D21C(1, 0, 0, 1);
}

/* Whether the selection record is filled. */
extern "C" s32 fn_8021D704(LbMenuSlot* slot) {
    u8 filled;

    if (slot == 0) {
        return 0;
    }
    filled = slot->flags_0x227 & 1;
    return filled != 0;
}

/* Publishes one selected slot's byte in the selection table. */
extern "C" void fn_8021D86C(s32 index, u8 value) {
    lb_menu_scratch.flags_0x156[(u8)index] = value;
}

/* Clears the selection table. */
extern "C" void fn_8021D9E0(void) {
    memset(lb_menu_scratch.flags_0x156, 0, 8);
}

/* Copies a 0x10-byte menu row field by field (its float through `lfs`/`stfs`). */
extern "C" void fn_8021DED8(LbRow16* dst, const LbRow16* src) {
    dst->field_0x00 = src->field_0x00;
    dst->field_0x04 = src->field_0x04;
    dst->value_0x08 = src->value_0x08;
    dst->field_0x0C = src->field_0x0C;
}

/* Runs the page's tick state: the first tick starts the sound update, the second rebuilds the page
 * once the menu-mode switch has settled. */
extern "C" void fn_8021CE0C(void) {
    s8 state = lbl_806AA790.state_0x008;

    switch (state) {
    case 0:
        lbl_806AA790.state_0x008++;
        fn_800DCFE4();
        break;
    case 1:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1U) {
            fn_8021B94C();
        }
        break;
    }
}
