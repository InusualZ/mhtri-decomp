/*
 * menu/fn_8031A6C0.cpp - the item/equipment selection screen.  `MenuSel` (this unit's view of the 0x330-byte working
 *   record `menu/menu_item.h` names `MenuSlot`) is the screen state: the input bits at +0x4/+0x8, the cursor at +0x1EC,
 *   the 3+8 0x18-byte entry arrays at +0x24/+0x6C, the resolved slot pairs at +0x210, the item data at +0x1F0;
 *   `self[1]` is the work area's second slot.  `fn_8031A6C0` is the per-frame state step and `fn_8031B774` the state
 *   dispatch (both switch on `state_0x001`); `fn_8031AE38`/`fn_8031B080`/`fn_8031BD7C` are the three draw passes.
 *   C++: the callees are mangled free functions (`GetItemData__FUs`, `get_lsp_data__FUsP10_mh_ivec2_`), the bodies
 *   `extern "C"`.
 * RANGE. .text 0x8031A6C0-0x8031DAA8 (45 functions); extab, extabindex, .ctors 0x8056F398, .data 0x805DCE98-0x805DCF38,
 *   .bss 0x806BE108-0x806BE120, .sdata 0x80792C20-0x80792C48, .sdata2 0x8079AE50-0x8079AE70.  The extab/extabindex runs
 *   continue `menu/menu_infomation.cpp`'s, so the left edge is a discovery size cap, not a measured TU edge; the effect
 *   tail from 0x8031DAA8 is `menu/menu_item_effect.cpp`.
 * FLAGS. `cflags_menu` (configure.py).
 * NAMES. Module `menu` from the callees (`get_menu_lsp_tbl`, `GetMenuFontColor`, `GetItemData`, `ItemName`, `draw_font`)
 *   and the bracketing menu units; no `__FILE__` string covers the range (`menu_infomation.cpp`'s is the neighbour's) and
 *   the dump answers `zz_`, so the file keeps the map's stem.
 * RESIDUALS. 23 rows unwritten (objdiff scores them zero): `fn_8031A6C0`, 0x8031AE38-0x8031B2D0, `fn_8031B39C`,
 *   `fn_8031B4C4`, 0x8031B624-0x8031BD50, `fn_8031BD7C`, `fn_8031C0B8`, `fn_8031C5FC`, `fn_8031C934`,
 *   0x8031CAC4-0x8031DA50, `fn_8031DA54` (the state machines, the draw passes, the cursor's record machine and the
 *   effect-model tail).  The 15 partial rows (0x8031AD30-0x8031AE38, 0x8031B2D0-0x8031B39C, 0x8031BFEC-0x8031C0B8,
 *   0x8031C338-0x8031C5FC, `fn_8031C8EC`, `fn_8031CA3C`), by cause:
 *  - indexed-access fold: retail `add r3,r3,r0` + `lhz`/`sth 0x39a2(r3)`, ours `lhzx`/`sthx` (`fn_8031B2D0`,
 *    `fn_8031B2FC`, `fn_8031B31C`);
 *  - an extra narrowing (`clrlwi`/`extsh`) ours applies to a value retail stores or compares raw (`fn_8031AD30`,
 *    `fn_8031AD9C`, `fn_8031B2FC`, `fn_8031BFEC`, `fn_8031C01C`, `fn_8031C028`, `fn_8031C390`, `fn_8031CA3C`);
 *  - 0xFFFF built as `lis r6,1; subi r0,r6,1` in retail, `li r0,-1` in ours (`fn_8031BFEC`, `fn_8031C028`);
 *  - a signed byte test (`extsb` + `cmpwi`) where ours emits `cmplwi` (`fn_8031C338`, `fn_8031C8EC`), and `lhz`
 *    where ours loads `lha` (`fn_8031C514`);
 *  - ours repeats a `li r3,0; blr` tail where retail branches to a shared one (`fn_8031C338`, `fn_8031C390`,
 *    `fn_8031C408`), and an inverted branch with a dropped `b` (`fn_8031AD9C`, `fn_8031C514`);
 *  - register allocation only (`fn_8031B344`; `fn_8031C408` with r30/r31 swapped).
 *   flipcheck: `.bss` (0x18), `.ctors` (0x4), `.data` (0xA0), `.sdata` (0x28) and `.sdata2` (0x20) claimed but not
 *   emitted; short `.text` 0x868 of 0x33E8, extab 0x48 of 0xF8, extabindex 0x6C of 0x174; the bytes of all three differ.
 */

#include "types.h"
#include "ef/eft_res.h"
#include "menu/menu_effect_slot.h"
#include "menu/fn_8031A6C0.h"
#include "menu/menu_item.h"
#include "menu/menu_message.h"
#include "sound/fn_800D7F54.h"

/* Callees other units own (`menu_slot_get` `menu/menu_message.cpp`, `fn_80274570` `Pl/pl_act.cpp`, `fn_8033AAFC`
 * `lobby/lb_companion_ui.cpp`, ...); their owners' headers do not declare them yet. */

extern "C" {

u16* menu_slot_get(MenuSel* self, s16 index);
s32  fn_80274570(void* a);
s32  fn_8033AAFC(u16 a);
s32  fn_8033AC78(u16 a, u16 b, s32 c);
s16  fn_8033B380(void* self, u16 a, u16 b, s32 kind, s16 c, s16 d);
u16  item_count_find(u16 a, s32 b, s32 c);
s32  fn_8004B3A0(u16 a, s32 b, s32 c);
void fn_800DCFC0(void);
extern MenuSharedSlots* lobby_world_block;
}

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

extern "C" void fn_8031DA50(MenuEff* self) {
    eft_res_slot_release(self);
}

