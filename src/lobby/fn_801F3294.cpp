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
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap.py lookup
 * over the range's inventory: all 48 names are bare `.text` entries in config/RMHE08/symbols.txt and the
 * runtime dump answers only `zz_XXXXXXXX_` placeholders for them)
 */
#include "types.h"

#include "Runtime.PPCEABI.H/memset.h"

#include "lobby/fn_801F3294.h"

extern "C" {

#pragma peephole off
/* 0x801F3294 - tick one lobby page request: a three-state machine that reads the pad mask, advances the
 * page cursor and pushes the new selection through `fn_802738E8`. */
u32 fn_801F3294(LbPageOwner* self, LbPage* req)
{
    u32 ret = 0;
    LbWorldBlock* world = lbl_80794880;
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
                    fn_800DBC84(3);
                    fn_8004D0D8(world, (u8)req->value_0x004);
                    sub->flag_0x01C = world->count_0x009E;
                    memset(&arg, 0, sizeof(arg));
                    arg.kind_0x00 = 1;
                    fn_802738E8(sub, &arg);
                    arg.kind_0x00 = 2;
                    fn_802738E8(sub, &arg);
                    arg.kind_0x00 = 3;
                    fn_802738E8(sub, &arg);
                    arg.kind_0x00 = 4;
                    fn_802738E8(sub, &arg);
                    arg.kind_0x00 = 5;
                    fn_802738E8(sub, &arg);
                    fn_801EC7AC(self);
                } else {
                    sysSE_req(2);
                }
                break;
            case 1:
                if (world->count_0x0002 != req->value_0x004) {
                    req->state_0x000++;
                    fn_800DBC84(0x28);
                    fn_8004D0E0(world, (u8)req->value_0x004);
                    fn_80273998(sub, 5, world->count_0x0002);
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
                req->value_0x004 = fn_802A8EFC(req->value_0x004, req->count_0x006, fn_802122AC(), 1, 2);
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
    self->mode_0x002 = lbl_80794880->field_0x39BA;
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
            fn_80223258(self->next_0x028, slot);
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

    if (chk_pointer() == 1) {
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
        if ((u16)self->count_0x006 <= 1 && self->field_0x008 == 1 && fn_8004AEC0(lbl_80794880) == 0) {
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
    if ((s16)id != -1 && chk_pointer() == 0) {
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
        fn_802E0AD4(&spr, 6943, (u8)extra, 0);
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
    LbIconRec* src = lbl_80794880->entries_0x0E00;
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
