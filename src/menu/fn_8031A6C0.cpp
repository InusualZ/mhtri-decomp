/*
 * src/menu/fn_8031A6C0.cpp - the menu selection-screen band, `.text` 0x8031A6C0..0x8031EA8C.
 *
 * WHAT IT IS.  The item/equipment selection screen.  `MenuSel` (this band's view of the 0x330-byte
 * menu working record `menu_item.h` names `MenuSlot`) is the screen state: the input bits at +0x4/
 * +0x8, the selection cursor at +0x1EC, the 3+8 0x18-byte entry arrays at +0x24/+0x6C, the resolved
 * slot pairs at +0x210 and the item data at +0x1F0.  `self[1]` is the work area's second slot (the
 * +0x330 accesses).  `fn_8031A6C0` is the per-frame state step and `fn_8031B774` the state dispatch
 * (both switch on `state_0x001`); `fn_8031AE38`/`fn_8031B080`/`fn_8031BD7C` are the three draw
 * passes and the tail (0x8031D294..) drives the selected item's 3D effect model.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string covers the range: the
 * only source-file string in the band's whole `.data` run is `menu_infomation.cpp` (0x805DCCDC), and
 * it is loaded only by functions of the *previous* proposal range (0x80312F84..0x80316DDC), never by
 * this one - the flanking TU's string, not this range's (the convention `menu/fn_802E4978.cpp`'s
 * registration records).  2. `dumpmap.py lookup` answers only `zz_031a6c0_` for the code.  3. The
 * `menu` module is certain: the band calls the menu library's `get_menu_lsp_tbl`/`GetMenuFontColor`/
 * `GetItemData`/`ItemName`/`draw_font`, drives the `_PLW`/`_EQUIP` selection, and both bracketing
 * registrations are `menu`.  The file therefore keeps the map's stem (brief option 4).
 *
 * LANGUAGE.  C++: every callee is a mangled free function (`GetItemData__FUs`,
 * `get_lsp_data__FUsP10_mh_ivec2_`, ...), so the band's bodies are `extern "C"` and call the real
 * signature (rule 9).  The extab/extabindex runs the registered unit owns are
 * 0x80015F3C..0x80016074 / 0x80034D40..0x80034F14 (contiguous with the previous proposal's run,
 * which is why the seam at 0x8031A6C0 is a discovery size cap, not a TU edge).
 *
 * RESIDUALS (this pass).  The band is 59 functions / 17356 B; this pass reconstructs the 28 whose
 * bodies need no in-band helper that is not itself written here.  The remaining 31 - the two big
 * state machines (`fn_8031A6C0`, `fn_8031B774`), the three draw passes (`fn_8031AE38`,
 * `fn_8031B080`, `fn_8031BD7C`), the cursor's record machine (`fn_8031C0B8`, `fn_8031C5FC`,
 * `fn_8031CE08`, `fn_8031CAC4`) and the whole 0x8031D294.. effect tail - are transcribed from the
 * target's m2c skeletons in `.pi/notes` and are the follow-up round's work.  Every function's
 * `.text` is unaffected by the helpers, so the landed subset measures as its own functions.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked every `.text` row of
 * config/RMHE08/symbols.txt from 0x8031A6C0 to 0x8031EA8C - 59 functions, all `fn_` stems, and the
 * runtime dump answers `zz_031a6c0_`).
 */

#include "types.h"
#include "menu/fn_8031A6C0.h"
#include "menu/menu_item.h"
#include "menu/menu_message.h"
#include "sound/fn_800D7F54.h"

/* The band's unowned callees (no registered unit owns these addresses; rule 2's unsplit case). */
extern "C" {

u16* menu_slot_get(MenuSel* self, s16 index);
void menu_cursor_column_step(u16* value);
s32  fn_80274570(void* a);
s32  fn_8033AAFC(u16 a);
s32  fn_8033AC78(u16 a, u16 b, s32 c);
s16  fn_8033B380(void* self, u16 a, u16 b, s32 kind, s16 c, s16 d);
u16  item_count_find(u16 a, s32 b, s32 c);
s32  fn_8004B3A0(u16 a, s32 b, s32 c);
void fn_800DCFC0(void);
void fn_80349184(void* a);
void fn_800F886C(void* a);
extern MenuSharedSlots* lobby_world_block;

} /* extern "C" */

/* ============================================================================================== */
/* 0x8031AC04 - hand the display worker to its update routine                                     */
/* ============================================================================================== */
extern "C" void fn_8031AC04(MenuSel* self) {
    fn_80274570(self->worker_0x190);
}

/* 0x8031AC0C - does item id `a` resolve to the slot `b`? */
extern "C" s32 fn_8031AC0C(MenuSel* self, u16 a, u16 b) {
    if (a == 0xFFFF) return 0;
    u16* rec = menu_slot_get(self, (s16)a);
    if ((s16)rec[1] == 0) return 0;
    if (rec[0] != b) return 0;
    return fn_8033AAFC(b);
}

/* 0x8031AC80 - resolve the pair `(a, b)` into the cursor's record */
extern "C" s32 fn_8031AC80(MenuSel* self, u16 a, u16 b) {
    if (a == 0xFFFF) return 0;
    if (a == b) return 0;
    u16* rec = menu_slot_get(self, (s16)a);
    if ((s16)rec[1] == 0) return 0;
    u16* rec2 = menu_slot_get(self, (s16)b);
    s32 r = fn_8033AC78(rec2[0], rec[0], 0);
    self->cursor_0x1EC.record_0x00 = (void*)r;
    return r != 0;
}

/* 0x8031AD30 - the item's base value plus the second slot's */
extern "C" s32 fn_8031AD30(u16 id, s32 b, s32 c) {
    s32 r = item_count_find(id, b, 0x18);
    if (c != 0) r += item_count_find(id, c, 8);
    return r;
}

/* 0x8031AD9C - apply or clear the item's bonus */
extern "C" void fn_8031AD9C(u16 id, s32 a, s32 b) {
    if (item_count_find(id, a, 0x18) == 0 && b != 0 &&
        GetItemData(id)->kind_0x00 == 1 && fn_8004B3A0(id, b, 8) >= 0) {
        return;
    }
    fn_8004B3A0(id, a, 0x18);
}

/* ============================================================================================== */
/* 0x8031B2D0 / 0x8031B2FC / 0x8031B31C - the eight-slot id table the screen shares with the menu  */
/* ============================================================================================== */
extern "C" u16 fn_8031B2D0(s16 idx) {
    if (idx >= 8) return 0xFFFF;
    return lobby_world_block->ids_0x39A2[idx];
}

extern "C" void fn_8031B2FC(s16 idx, s16 value) {
    if (idx < 8) lobby_world_block->ids_0x39A2[idx] = (u16)value;
}

extern "C" void fn_8031B31C(s16 idx) {
    if (idx < 8) lobby_world_block->ids_0x39A2[idx] = 0xFFFF;
}

/* 0x8031B344 - mark the visible window's entries available */
extern "C" void fn_8031B344(MenuSel* self) {
    MenuSelEntry* cell = self->entries_b_0x06C;
    s16 slot = (s16)((s8)self->col_0x1A0 * (s8)self->count_0x1A3);
    for (s32 i = 0; i < (s8)self->count_0x1A3; i++) {
        cell->available_0x02 = 1;
        cell->slot_0x06 = slot;
        cell->color_a_0x0C = 0;
        cell++;
        slot++;
    }
}

/* 0x8031B45C - clear one column and its slot pair */
extern "C" void fn_8031B45C(MenuSel* self, u8 idx) {
    fn_8031B31C(self->entries_b_0x06C[idx].slot_0x06);
    self->slots_0x210[idx].id_0x00 = 0;
    self->slots_0x210[idx].amount_0x02 = 0;
    fn_8031B344(self);
}

/* 0x8031B5D0 - reset the work area's second record */
extern "C" void fn_8031B5D0(MenuSel* self) {
    MenuSel* second = self + 1;
    fn_80349184(second);
    second->state_0x001 = 1;
    second->field_0x002 = 1;
    second->field_0x01C = 2;
    second->fade_0x23A = 0x29;
    second->field_0x23B = 5;
}

/* 0x8031B6D0 - reset this record to its item-screen defaults (kept for the follow-up round that
 * writes the in-band `fn_8031B39C`/`fn_8031B4C4` it calls). */
#if 0
extern "C" void fn_8031B6D0(MenuSel* self) {
    self->mode_0x014 = 2;
    self->flag_0x1B0 = 0;
    self->value_0x19E = 0;
    self->value_0x017 = 8;
    self->count_0x1A3 = 8;
    self->col_0x1A0 = 0;
    self->row_0x1A1 = menu_page_count(8, 8);
    self->sel_0x1A2 = 0;
    self->fade_0x23A = 0;
    self->state_0x001 = 0;
    fn_8031B39C(self);
    fn_8029FFFC((MenuSlot*)self, (s8)self->sel_0x1A2);
    fn_8031BFEC(&self->cursor_0x1EC, 1, self);
    fn_8031B4C4(self);
}
#endif

/* 0x8031BD50 - copy the five halfwords of one cursor record */
extern "C" void fn_8031BD50(u16* dst, u16* src) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
    dst[4] = src[4];
}

/* ============================================================================================== */
/* 0x8031BFEC..0x8031CA3C - the cursor's own machine                                               */
/* ============================================================================================== */
extern "C" void fn_8031BFEC(MenuSelCursor* cur, s8 kind, MenuSel* owner) {
    cur->item_b_0x06 = 0;
    cur->item_a_0x04 = 0;
    cur->slot_b_0x0A = -1;
    cur->slot_a_0x08 = -1;
    cur->mode_0x16 = 0;
    cur->record_0x00 = NULL;
    cur->kind_0x0E = (u8)kind;
    cur->owner_0x1C = owner;
}

extern "C" void fn_8031C01C(MenuSelCursor* cur, u16 a, u16 b) {
    cur->slot_a_0x08 = a;
    cur->item_a_0x04 = b;
}

extern "C" s32 fn_8031C028(MenuSelCursor* cur, u16 a, u16 b) {
    cur->slot_b_0x0A = (s16)a;
    cur->item_b_0x06 = b;
    if (a == 0xFFFF) cur->record_0x00 = NULL;
    else cur->record_0x00 = (void*)fn_8033AC78(cur->item_a_0x04, b, 0);
    cur->mode_0x16 = 0;
    cur->state_0x18 = 0;
    if (cur->record_0x00 == NULL) {
        cur->slot_b_0x0A = -1;
        cur->item_b_0x06 = 0;
        return 0;
    }
    return 1;
}

extern "C" s32 fn_8031C338(MenuSelCursor* cur, u16 bits) {
    if (cur->state_0x18 == 2) {
        if (bits == 0x10) {
            u8 t = (u8)(cur->timer_0x17 - 1);
            cur->timer_0x17 = t;
            if ((s8)t <= 0) {
                cur->state_0x18 = 0;
                return 1;
            }
            return 0;
        }
        cur->state_0x18 = 0;
    }
    return 0;
}

extern "C" s32 fn_8031C390(MenuSelCursor* cur, u16 a, u16 b) {
    if (a & 0x10) {
        cur->state_0x18 = 0;
        cur->timer_0x17 = 0x10;
        return 0;
    }
    if (b & 0x10) {
        u8 t = (u8)(cur->timer_0x17 - 1);
        cur->timer_0x17 = t;
        if ((s8)t <= 0) {
            cur->state_0x18 = 1;
            cur->timer_0x17 = 5;
            return 1;
        }
        return 0;
    }
    cur->state_0x18 = 0;
    return 0;
}

extern "C" s32 fn_8031C408(MenuSelCursor* cur, s32* flag, u16 a, u16 b, u16 c) {
    u32 r = (u32)fn_8031C338(cur, a);
    if (r == 1 || (b & 0x30)) {
        if ((r != 0 || (b & 0x10)) && *flag == 0) return 1;
        if ((b & 0x20) || ((b & 0x10) && *flag == 1)) {
            sysSE_req(1);
            return 2;
        }
        return 0;
    }
    if (cur->status_0x10 < 0 && (c & 3)) {
        *flag = menu_cursor_step(*flag, 2, c, 1, 2);
    }
    return 0;
}

/* 0x8031C514 - start the value count-up for the resolved pair */
extern "C" void fn_8031C514(MenuSelCursor* cur, u8 which) {
    if (which == 0) {
        cur->state_0x18 = 0;
        cur->timer_0x17 = 0x10;
    } else {
        cur->state_0x18 = 1;
        cur->timer_0x17 = 5;
    }
    if (cur->flag_0x11 == 0) {
        cur->timer_0x0F = 0x1E;
        fn_800DCFC0();
    } else {
        cur->timer_0x0F = 1;
    }
    u8 t = cur->kind_0x0E;
    if (t > 1) {
        if (t == 2) {
            cur->value_0x0C = fn_8033B380(&cur->anim_0x15, cur->item_a_0x04, cur->item_b_0x06, 2,
                                          cur->slot_a_0x08, cur->slot_b_0x0A);
        }
    } else {
        cur->value_0x0C = fn_8033B380(&cur->anim_0x15, cur->item_a_0x04, cur->item_b_0x06, 0,
                                      cur->slot_a_0x08, cur->slot_b_0x0A);
    }
    cur->mode_0x16 = 1;
}

/* 0x8031C8EC - begin the value-apply phase */
extern "C" void fn_8031C8EC(MenuSelCursor* cur) {
    cur->counter_0x23 = 0;
    if (cur->flag_0x12 != 0) {
        cur->timer_0x0F = 0x1E;
    } else {
        cur->timer_0x0F = 0;
        if (cur->state_0x18 == 1) cur->counter_0x23 = 0x11;
    }
    cur->mode_0x16 = 2;
}

/* 0x8031CA3C - the per-frame blend counters */
extern "C" void fn_8031CA3C(MenuSelCursor* cur) {
    if (cur->record_0x00 == NULL) {
        u8 t = cur->blend_0x20;
        if (t != 0) cur->blend_0x20 = (u8)(t - 1);
    } else {
        u8 t = cur->blend_0x20;
        if (t < 3) cur->blend_0x20 = (u8)(t + 1);
    }
    if (cur->mode_0x16 <= 1) {
        u8 t = cur->blend_0x21;
        if (t != 0) cur->blend_0x21 = (u8)(t - 1);
    } else {
        u8 t = cur->blend_0x21;
        if (t < 3) cur->blend_0x21 = (u8)(t + 1);
    }
    u8 c = cur->counter_0x22;
    if (c < 2) cur->counter_0x22 = (u8)(c + 1);
}

/* ============================================================================================== */
/* 0x8031E568 / 0x8031EA78 - step counters; 0x8031E578 / 0x8031EA88 / 0x8031DA50 - tail calls    */
/* ============================================================================================== */
extern "C" void fn_8031E568(MenuEff* self) {
    self->step_0x005 = (u8)(self->step_0x005 + 1);
}

extern "C" void fn_8031EA78(MenuEff* self) {
    self->step_0x005 = (u8)(self->step_0x005 + 1);
}

extern "C" void fn_8031E578(MenuEff* self) {
    fn_800F886C(self);
}

extern "C" void fn_8031EA88(MenuEff* self) {
    fn_800F886C(self);
}

extern "C" void fn_8031DA50(MenuEff* self) {
    fn_800F886C(self);
}
