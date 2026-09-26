/* menu/menu_item.cpp - the item menu's data layer: the item/table accessors and the hit helpers the
 * menu's attack preview reads back.
 *
 * `.text` 0x8029F3C8..0x802A5444 (91 symbols, 0x607C B), extab 0x8001360C..0x800137A4 and
 * extabindex 0x80030F90..0x800311F4 (the 51 framed functions of the registered range - the records of
 * the functions *before* 0x8029F3C8 are the previous run's object until the seam merges, see SEAM);
 * no `.ctors`/`.dtors` word is this unit's, and the `.data` run 0x805CDE78..0x805CE00C (this file's
 * colour table, three switch tables and two strings) is left unclaimed because the bodies declare
 * those objects instead of defining them (row 29).
 *
 * MODULE AND NAME (brief section 2, evidence order).
 *   * class 1 (a `__FILE__` string) decides both.  `.data` carries `lbl_805CDFC8`, 0xE bytes =
 *     `"menu_item.cpp"`, and the dump's own local symbol for it is
 *     `_802a22a4s_menu_item.cpp_805cdfc8` - i.e. the `__FILE__` static emitted by the function at
 *     0x802A22A4, which this range owns (`fn_802A1714`, 0x802A1714-0x802A23F4).  The module is
 *     `menu`, the file name `menu_item.cpp`, the extension `.cpp` (the range's mangled callees).
 *   * class 2 confirms the range's own symbols: `python tools/symbols/dumpmap.py lookup` answers the
 *     real names `body_set(_BODY_W`, `hit_flag_set(_HIT_W`, `hit_result_check(_HIT_W`,
 *     `get_item_data_ptr`, `ItemName(unsigned`, `ItemExp(unsigned`, `GetItemData(unsigned`,
 *     `get_menu_tbl_ptr`, `get_menu_lsp_tbl(unsigned`, `put_menu_cursor(unsigned` for the 10 rows the
 *     map already names; every other row answers the `zz_<addr>_` placeholder.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with `symedit.py range
 * 0x8029F3C8 0x802A5444` - 81 of the 91 rows are `fn_XXXXXXXX` and `dumpmap.py lookup` answers
 * `zz_<addr>_` for all 81 - and the 10 real rows are written as the C++ functions their manglings
 * spell below).
 *
 * SEAM (unproven, and it is a proposal cap rather than a TU boundary).  The unit's own `.data` run
 * 0x805CDE78..0x805CE00C holds this file's colour table (0x30 B), its three switch tables and its two
 * `__FILE__`/assert strings; the dump's table labels place two of the tables in *this* range's
 * functions (`_802a0684switchdataD_805cdf10`, `_802a1010switchdataD_805cdf5c`) but the first one,
 * `_8029e4e0switchdataD_805cdea8`, belongs to 0x8029E4E0, which sits in the *previous* unclaimed run
 * (0x80295EF4..0x8029F3C8).  A TU cannot own two disjoint `.text` runs, so the TU starts at or before
 * 0x8029E4E0: the registration's left edge is the proposal cap, not the seam.  Merging the two runs
 * (and the one above, 0x802A5444..0x802AD9C0, which shares the shell/serial band) is a follow-up.
 *
 * STATUS (official `build/RMHE08/report.json`, full `ninja` in this worktree, `main.dol: OK`).
 * 53 of the 91 functions are written and every written one is above the 80 % bar; 50 are
 * byte-identical.  Unit: 13.872065 % fuzzy, 2568 / 24700 `.text` bytes matched.  The 37 unwritten
 * functions (21268 B, 86 % of the range - the menu's own per-frame/draw half) are the follow-up
 * queue, in address order: fn_8029F834 (576 B);
 * fn_8029FCFC (700 B);
 * fn_802A0568 (232 B);
 * fn_802A0650 (336 B);
 * fn_802A07A0 (3360 B);
 * fn_802A14C0 (312 B);
 * fn_802A15F8 (92 B);
 * fn_802A1654 (164 B);
 * fn_802A1714 (3296 B);
 * fn_802A23F4 (348 B);
 * put_menu_cursor__FPUsUsPC10_mh_ivec2_ (188 B);
 * fn_802A2620 (212 B);
 * fn_802A26F4 (600 B);
 * fn_802A294C (476 B);
 * fn_802A2B28 (72 B);
 * fn_802A2B70 (296 B);
 * fn_802A2CB8 (176 B);
 * fn_802A2D68 (80 B);
 * fn_802A2DB8 (940 B);
 * fn_802A3164 (44 B);
 * fn_802A31D4 (164 B);
 * fn_802A3278 (1060 B);
 * fn_802A369C (164 B);
 * fn_802A3740 (180 B);
 * fn_802A37F4 (988 B);
 * fn_802A3BD0 (1248 B);
 * fn_802A40B0 (472 B);
 * fn_802A4288 (404 B);
 * fn_802A4430 (272 B);
 * fn_802A4540 (692 B);
 * fn_802A4810 (660 B);
 * fn_802A4AA4 (596 B);
 * fn_802A4CF8 (160 B);
 * fn_802A4D98 (352 B);
 * fn_802A4EF8 (244 B);
 * fn_802A4FEC (820 B);
 * fn_802A5320 (292 B)
 *
 * Residuals, by measurement (all three are register-only, no size or instruction-count difference):
 *   * `fn_8029F73C` 99.29 - the `and` of the item record's +0x02 byte with the caller's mask; retail
 *     orders the operands `and r3,r<byte>,r<mask>`, ours `and r3,r<mask>,r<byte>` (both source
 *     spellings tried).
 *   * `fn_8029FA74` 99.43 - the two selection calls: retail loads the byte straight into the argument
 *     register (`lbz r4,0x19(r31); extsb r4,r4`), ours loads into r0 and sign-extends into r4.
 *   * `fn_8029FB00` 99.61 - the same r0-vs-argument-register shape on the trailing `fn_802A0040`
 *     call; passing the field through an `s8` local (which fixes nothing else here) is what closed
 *     the rest of that function.
 */

#include "types.h"
#include "menu/menu_item.h"
#include "Pl/pl_act.h"
#include "unsplit/unknown.h"
#include "Runtime.PPCEABI.H/memset.h"

/* 0x8029F3C8: fills a body slot in from a body record - the appearance/hit state the item menu shows.
 * `kind` selects which byte of the work record it copies and with which sub-code; the second
 * argument is stored, not read. */
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
extern "C" void fn_8029F538(_HIT_W* hit)
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
extern "C" void fn_8029F5B4(_HIT_W* hit, s16 value)
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
extern "C" s32 fn_8029F73C(u16 id, s32 index)
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
        if (fn_803AAEC0() != 0 && (u8)fn_802FBA60() == 0) {
            return 0;
        }
        if (fn_8027BC48(0) == 1) {
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

/* 0x802A0148: whether the menu is in the state `fn_800CF280` reports or a single area is loaded. */
extern "C" u32 fn_802A0148(void)
{
    if (fn_800CF280() != 0) {
        return 1;
    }
    return fn_803AAEC0() == 1;
}

/* 0x802A02CC: the slot-0 form of the predicate below. */
extern "C" u32 fn_802A02CC(void)
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
extern "C" u32 fn_802A0304(MenuFrameWork* self)
{
    MenuSlot* slot = &lbl_806AC8C8.slot[0];
    u32 idx = 0;

    if (self != 0) {
        switch ((u8)fn_800CF208()) {
        default:
            return 0;
        case 1:
            if (fn_80047058() != 0) {
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
    if (fn_802A0304(self) == 1) {
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
    if (fn_800D0708() != 0) {
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
    if (fn_800D0708() == 1) {
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
    if (fn_800D0708() == 1 && lbl_806AC8C8.slot[0].field_0x31E == 1) {
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
