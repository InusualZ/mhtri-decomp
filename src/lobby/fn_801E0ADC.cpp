/* lobby/fn_801E0ADC.cpp - the effect/flag bookkeeping group that precedes the lobby menu layer.
 *
 * `.text` 0x801E0ADC..0x801E7530 (77 functions, 27220 B), extab 0x800103D4..0x800105B4 (60 unwind-only
 * 8-byte records), extabindex 0x8002C4A8..0x8002C778 (60 x 12 B).  Registered; both section gaps are exactly the space between the registered
 * `enemy/fn_801D80EC.cpp` and `lobby/fn_801E7530.cpp` claims, which is why the whole capped run is one
 * registration (the seam is unproven, see below).
 *
 * Module `lobby`.  Every foreign call is a lobby/HUD symbol (`LbStr`, `GetMenuFontColor`, `get_lsp_data`,
 * `draw_sprite_*`, `ItemName`) or a call into the registered `lobby/fn_80212810.cpp` (23 sites); the
 * range's data lives in the lobby `.sbss`/`.data` runs (`lobby_world_block`, `lbl_805B75E8`), and both link
 * neighbours are the enemy/lobby units.  Language C++: every call out is a mangled symbol.
 *
 * Name.  The map has only `fn_XXXXXXXX` for this range and the runtime dump answers only `zz_`
 * placeholders (`dumpmap.py lookup 0x801E0ADC` -> `zz_01e0adc_`, and the same for the rest of the
 * inventory), so the file keeps the map's stem.
 *
 * Seam.  The proposal was cut by `--max-bytes`, so its edge is a size cap, not a boundary.  `tudiscover
 * at 0x801E0ADC` puts a strong left cut exactly here (a `.sdata2` pool split) and finds the certain
 * match set 0x801E0ADC..0x801E2864; it also finds *separate* certain sets inside the run (0x801E2A9C..
 * 0x801E3E04 with `eft033_set` in it, 0x801E41D0..0x801E448C, 0x801E5000..0x801E5828), so the run holds
 * several original TUs and the extent is provisional: it settles as its functions match.
 *
 * Flags.  `#pragma peephole off` is required: retail keeps the unfused `clrlwi`+`slwi` index scale
 * (`fn_801E1A40`) and unfused narrow compares, where the pass folds them into `clrlslwi`/an
 * if-converted branch chain.  Per-function measurements are in the outbox.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `dumpmap.py lookup` on the range's inventory - every name is a bare `fn_XXXXXXXX`/`zz_XXXXXXXX_`
 * placeholder; the only real names in the run, `eft033_set`/`eft033_set_ofs`, are used as they are)
 */

#include "types.h"
#include "Runtime.PPCEABI.H/memset.h"

/* Foreign callees whose owners' headers do not publish them.  Bare prototypes (no `extern` keyword)
 * inside the linkage block is how the neighbouring units carry them; each is filed as a shared-file
 * request to move it to its owner's header (docs/plan.md 6.5 rule 2). */
extern "C" {
void fn_8004D334(s32 size);
void fn_8004C038(void* a, void* b);
s32 game_ready_ck(void);
u32 fn_800D0734(s32 a);
s32 fn_80217F4C(void*, u8, u16);
void fn_802190FC(void*, s32, s32, void*, void*);
void fn_80219530(void*, void*);
s32 fn_802754B4(void*);
s32 fn_8027E354(u8, u16);
s32 fn_802FB4BC(u16);
u32 fn_803768F8(void);
u32 chk_pointer(void);
void eft_res_slot_release(void* obj);
void fn_8021AA78(void*, u32, void*, void*, void*);
s32 fn_80217934(void);
}

/* The two `.data` colour tables `fn_801E1A40` reads as flat byte runs (0x805B75E8 and 0x805B75F8 are
 * 0x10 B apart, so 15 B of 3-byte entries plus padding). */
extern u8 lbl_805B75E8[];
extern u8 lbl_805B75F8[];

/* The big lobby work block `lobby_world_block` points at.  Only the two 41-entry u16 flag arrays this unit
 * reaches are named; the front of the block is untouched here.  Size is an approximation - the block
 * continues past the last named field. */
typedef struct LbEftFlagBlock {
    /* +0x0000 */ u8 unused_0x0000[0x48D8];
    /* +0x48D8 */ u16 set_0x48D8[41];
    /* +0x492A */ u8 unused_0x492A[0xAE];
    /* +0x49D8 */ u16 active_0x49D8[41];
} LbEftFlagBlock; /* size: 0x4A2A (approximate) */

extern LbEftFlagBlock* lobby_world_block;

/* The player status block the item page reads its button flags from (a 0xD40-byte object in the map). */
typedef struct LbPadStatus {
    /* +0x000 */ u8 unused_0x000[0x2C8];
    /* +0x2C8 */ u16 press_0x2C8;
    /* +0x2CA */ u8 unused_0x2CA[0xA76];
} LbPadStatus; /* size: 0xD40 */

extern LbPadStatus Psw;

/* The 6-byte id/payload row of the `lbl_805CA6D4` / `lbl_805CA660` lookup tables: a u16 id that
 * terminates the table at 0 plus two u16 payload fields. */
typedef struct LbIdRow6 {
    /* +0x00 */ u16 id_0x00;
    /* +0x02 */ u16 value_0x02;
    /* +0x04 */ u16 sub_0x04;
} LbIdRow6; /* size: 0x6 */

/* The 8-byte row of the `lbl_805CA718` lookup table `fn_801E6F10` returns a pointer into. */
typedef struct LbIdRow8 {
    /* +0x00 */ u16 id_0x00;
    /* +0x02 */ u16 unused_0x02;
    /* +0x04 */ u32 value_0x04;
} LbIdRow8; /* size: 0x8 */

extern LbIdRow6 lbl_805ED210[];
extern LbIdRow6 lbl_805CA660[];
extern LbIdRow6 lbl_805CA6D4[];
extern LbIdRow8 lbl_805CA718[];

/* The lookup record `fn_8027E354` hands back for a slot and an item id - the same record the sibling
 * `lobby/fn_80212810.cpp` views as `LbRefEntry` (0x10 B, its s32 fields at +0x04/+0x08/+0x0C).  Only the
 * value at +0x08 is reached here.  The record is owned outside this range, so the durable fix is one
 * header both units include, filed as a shared-file request; this view exists because the sibling's is
 * a definition in a `.cpp` and a second definition of the same name is what rule 1 forbids. */
typedef struct LbItemRef {
    /* +0x00 */ u8 unused_0x00[8];
    /* +0x08 */ u32 value_0x08;
    /* +0x0C */ u8 unused_0x0C[4];
} LbItemRef; /* size: 0x10 (the sibling unit's view of the same record) */

/* The 0x24-byte record `spr_data_copy` copies field by field. */
typedef struct LbItemRec {
    /* +0x00 */ u32 id_0x00;
    /* +0x04 */ u32 value_0x04;
    /* +0x08 */ s16 rate_0x08;
    /* +0x0A */ s16 rate_0x0A;
    /* +0x0C */ u32 data_0x0C;
    /* +0x10 */ u16 count_0x10;
    /* +0x12 */ u8 kind_0x12;
    /* +0x13 */ u8 flag_0x13;
    /* +0x14 */ u32 value_0x14;
    /* +0x18 */ u32 value_0x18;
    /* +0x1C */ u32 value_0x1C;
    /* +0x20 */ u32 value_0x20;
} LbItemRec; /* size: 0x24 */

/* One 8-byte slot record of `LbEftWork::recs_0x134` (`fn_801E403C` marks every one, `fn_801E413C`
 * reads the selected one). */
typedef struct LbEftRec {
    /* +0x00 */ u8 mode_0x00;
    /* +0x01 */ u8 kind_0x01;
    /* +0x02 */ u16 id_0x02;
    /* +0x04 */ u8 unused_0x04[4];
} LbEftRec; /* size: 0x8 */

/* The 0xC-byte id record the work block carries at +0x430/+0x43C/+0x448 (and a page record at +0x454
 * of the menu work). */
typedef struct LbEftInfo {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u16 id_0x02;
    /* +0x04 */ u8 unused_0x04[8];
} LbEftInfo; /* size: 0xC */

/* The effect work block this unit drives.  Only the fields these bodies reach are named; the rest is
 * padding to the next named offset.  Size is an approximation - the block continues past the last
 * named field. */
typedef struct LbEftWork {
    /* +0x000 */ u8 unused_0x000[2];
    /* +0x002 */ u16 id_0x002;
    /* +0x004 */ u16 id_0x004;
    /* +0x006 */ u8 unused_0x006[0x2C];
    /* +0x032 */ u16 timer_0x032;
    /* +0x034 */ u8 unused_0x034[0xB8];
    /* +0x0EC */ s16 sel_0x0EC;
    /* +0x0EE */ u8 unused_0x0EE[4];
    /* +0x0F2 */ s16 value_0x0F2;
    /* +0x0F4 */ u8 unused_0x0F4[2];
    /* +0x0F6 */ u16 slot_0x0F6;
    /* +0x0F8 */ u16 flag_0x0F8;
    /* +0x0FA */ u8 unused_0x0FA[6];
    /* +0x100 */ u8 unused_0x100[1];
    /* +0x101 */ u8 flag_0x101;
    /* +0x102 */ u8 unused_0x102[2];
    /* +0x104 */ s16 slot_0x104;
    /* +0x106 */ u8 unused_0x106[0x26];
    /* +0x12C */ s32 handle_0x12C;
    /* +0x130 */ s32 handle_0x130;
    /* +0x134 */ LbEftRec recs_0x134[8];
    /* +0x174 */ u8 field_0x174[0x234];
    /* +0x3A8 */ u8 field_0x3A8[0x80];
    /* +0x428 */ u8 kind_0x428[8];
    /* +0x430 */ LbEftInfo info_0x430;
    /* +0x43C */ LbEftInfo sub_0x43C;
    /* +0x448 */ LbEftInfo sub_0x448;
    /* +0x454 */ u8 unused_0x454[0x6C];
    /* +0x4C0 */ u16 time_0x4C0;
} LbEftWork; /* size: 0x4C2 (approximate) */

/* Foreign callees whose owners' headers do not publish them.  Bare prototypes (no `extern` keyword)
 * inside the linkage block is how the neighbouring units carry them; each is filed as a shared-file
 * request to move it to its owner's header (docs/plan.md 6.5 rule 2). */
extern "C" {
void fn_8004D334(s32 size);
void fn_8004C038(void* a, void* b);
s32 game_ready_ck(void);
u32 fn_800D0734(s32 a);
s32 fn_80217F4C(void*, u8, u16);
void fn_802190FC(void*, s32, s32, void*, void*);
void fn_80219530(void*, void*);
s32 fn_802754B4(void*);
s32 fn_8027E354(u8, u16);
s32 fn_802FB4BC(u16);
u32 fn_803768F8(void);
u32 chk_pointer(void);
void eft_res_slot_release(void* obj);
void fn_8021AA78(void*, u32, void*, void*, void*);
s32 fn_80217934(void);
}

#pragma peephole off

/* ------------------------------------------------------------------------------------------------ */

extern "C" {

/* This unit's own bodies that an earlier body calls. */
u32 fn_801E448C(LbEftWork* self);

/* Bumps the counter byte at +0x5 of the record the caller passes. */
void fn_801E1A2C(u8* rec)
{
    rec[5]++;
}

/* Forwards to the effect release helper (a one-instruction tail call). */
void fn_801E1A3C(void* obj)
{
    eft_res_slot_release(obj);
}

/* Scatters the entry-0 colour of both tables over the first output pair and the indexed colour over the
 * second, or the other way round for kind 1. */
void fn_801E1A40(u8 kind, u8 idx, u8* out0, u8* out1, u8* out2, u8* out3)
{
    s32 i;
    s32 j;
    s32 k;

    switch (kind) {
    case 0:
        out0[0] = lbl_805B75E8[0];
        out0[1] = lbl_805B75E8[1];
        out0[2] = lbl_805B75E8[2];
        out1[0] = lbl_805B75F8[0];
        out1[1] = lbl_805B75F8[1];
        out1[2] = lbl_805B75F8[2];
        i = idx * 3;
        j = i + 1;
        k = i + 2;
        out2[0] = lbl_805B75E8[i];
        out2[1] = lbl_805B75E8[j];
        out2[2] = lbl_805B75E8[k];
        out3[0] = lbl_805B75F8[i];
        out3[1] = lbl_805B75F8[j];
        out3[2] = lbl_805B75F8[k];
        break;
    case 1:
        out2[0] = lbl_805B75E8[0];
        out2[1] = lbl_805B75E8[1];
        out2[2] = lbl_805B75E8[2];
        out3[0] = lbl_805B75F8[0];
        out3[1] = lbl_805B75F8[1];
        out3[2] = lbl_805B75F8[2];
        i = idx * 3;
        j = i + 1;
        k = i + 2;
        out0[0] = lbl_805B75E8[i];
        out0[1] = lbl_805B75E8[j];
        out0[2] = lbl_805B75E8[k];
        out1[0] = lbl_805B75F8[i];
        out1[1] = lbl_805B75F8[j];
        out1[2] = lbl_805B75F8[k];
        break;
    }
}

/* Forwards to the effect release helper (a one-instruction tail call). */
void fn_801E3EDC(void* obj)
{
    eft_res_slot_release(obj);
}

/* Forwards one record plus its three sub-records to the page handler. */
void fn_801E3EE0(void* a, u8* rec, u8 kind)
{
    fn_8021AA78(a, kind, rec + 0x454, rec + 0x49C, rec + 0x4A8);
}

/* Clears the work block's 16-bit timer at +0x32. */
void fn_801E4E38(LbEftWork* self)
{
    self->timer_0x032 = 0;
}

/* Clears the 16-bit timer at +0x4C0 when the slot the record selects holds a kind-3 entry. */
void fn_801E4FE0(LbEftWork* self)
{
    if (self->kind_0x428[self->slot_0x104] == 3) {
        self->time_0x4C0 = 15;
    }
}

/* Copies one 0x24-byte item record onto another. */
void spr_data_copy(LbItemRec* dst, const LbItemRec* src)
{
    *dst = *src;
}

/* Returns whether the id is present in the 6-byte-row table. */
s32 fn_801E6DD0(s32 id)
{
    const LbIdRow6* row = lbl_805CA6D4;

    while (row->id_0x00 != 0) {
        if (row->id_0x00 == id) {
            return 1;
        }
        row++;
    }
    return 0;
}

/* Returns the 16-bit flag a slot id carries: kind 0 the set table, 1 the active table, 2 their sum. */
s32 fn_801E6EA8(u16 idx, u8 kind)
{
    LbEftFlagBlock* blk = lobby_world_block;

    switch (kind) {
    case 2:
        return blk->set_0x48D8[idx] + blk->active_0x49D8[idx];
    case 0:
        return blk->set_0x48D8[idx];
    case 1:
        return blk->active_0x49D8[idx];
    }
}

/* Forwards to the lobby item-table rebuild helper (a one-instruction tail call). */
s32 fn_801E6DCC(void)
{
    return fn_80217934();
}

/* Returns the 8-byte table row for an id, or the null pointer. */
LbIdRow8* fn_801E6F10(u16 id)
{
    LbIdRow8* row = lbl_805CA718;

    while (row->id_0x00 != 0) {
        if (id == row->id_0x00) {
            return row;
        }
        row++;
    }
    return 0;
}

/* Returns the 6-byte table row for an id, or the null pointer. */
LbIdRow6* fn_801E6F48(u16 id)
{
    LbIdRow6* row = lbl_805CA6D4;

    while (row->id_0x00 != 0) {
        if (id == row->id_0x00) {
            return row;
        }
        row++;
    }
    return 0;
}

/* Returns the payload the 6-byte table carries for an id, or 0. */
u16 fn_801E72F4(u16 id)
{
    LbIdRow6* row = lbl_805CA660;

    while (row->id_0x00 != 0) {
        if (id == row->id_0x00) {
            return row->value_0x02;
        }
        row++;
    }
    return 0;
}

/* Marks every slot record of the work block open (mode 0x40) and clears the two 16-bit fields. */
void fn_801E403C(LbEftWork* self, s16 value)
{
    self->flag_0x0F8 = 0;
    self->value_0x0F2 = value;
    memset(self->recs_0x134, 0, sizeof(self->recs_0x134));
    self->recs_0x134[0].mode_0x00 = 0x40;
    self->recs_0x134[1].mode_0x00 = 0x40;
    self->recs_0x134[2].mode_0x00 = 0x40;
    self->recs_0x134[3].mode_0x00 = 0x40;
    self->recs_0x134[4].mode_0x00 = 0x40;
    self->recs_0x134[5].mode_0x00 = 0x40;
    self->recs_0x134[6].mode_0x00 = 0x40;
    self->recs_0x134[7].mode_0x00 = 0x40;
}

/* Whether the work block's active id is present in the lobby item table. */
s32 fn_801E40A4(LbEftWork* self)
{
    if (game_ready_ck() == 0) {
        if (fn_802FB4BC(self->id_0x002) == 0) {
            return 0;
        }
    } else {
        if (fn_802FB4BC(self->id_0x004) == 0) {
            return 0;
        }
    }
    return 1;
}

/* The sprite index the weapon record carries for a slot and an item id. */
u32 fn_801E410C(u8 slot, u16 id)
{
    LbItemRef* rec = (LbItemRef*)fn_8027E354(slot, id);

    return rec->value_0x08 << 1;
}

/* Fills the work block's info record from the selected slot record, then feeds it to the page. */
void fn_801E413C(LbEftWork* self)
{
    s16 sel = self->sel_0x0EC;

    fn_80217F4C(&self->info_0x430, self->recs_0x134[sel].kind_0x01,
                self->recs_0x134[sel].id_0x02);
    if (fn_801E448C(self) == 1) {
        fn_802190FC(&self->field_0x3A8, 0, 0, &self->info_0x430, &self->sub_0x43C);
    } else {
        fn_802190FC(&self->field_0x3A8, 0, 0, &self->info_0x430, 0);
    }
    fn_80219530(&self->field_0x3A8, &self->info_0x430);
}

/* Returns whether the info record's id is in the page table, filling the two sub-records when it is. */
u32 fn_801E448C(LbEftWork* self)
{
    const LbIdRow6* row;

    if (self->info_0x430.kind_0x00 == 0x0B) {
        row = lbl_805ED210;
        while (row->id_0x00 != 0) {
            if (row->id_0x00 == self->info_0x430.id_0x02) {
                fn_80217F4C(&self->sub_0x43C, 0xC, row->value_0x02);
                fn_80217F4C(&self->sub_0x448, 0xD, row->sub_0x04);
                return 1;
            }
            row++;
        }
    }
    return 0;
}

/* Nudges the work block's timer by the pointer's horizontal press flags. */
void fn_801E4E44(LbEftWork* self)
{
    u16 flags;

    if (chk_pointer() != 1) {
        if (fn_803768F8() != 1) {
            flags = Psw.press_0x2C8;
            if (flags & 0x0C00) {
                if (flags & 0x0800) {
                    self->timer_0x032 -= 0x400;
                }
                if (flags & 0x0400) {
                    self->timer_0x032 += 0x400;
                }
            }
        }
    }
}

/* Frees the work block's effect handle and drives it to its next state. */
void fn_801E4F78(LbEftWork* self)
{
    fn_8004C038((void*)self->handle_0x12C, lobby_world_block);
    if (self->kind_0x428[self->slot_0x104] == 3) {
        fn_802754B4((void*)self->handle_0x12C);
    } else if (self->flag_0x101 != 0) {
        fn_802754B4((void*)self->handle_0x12C);
    }
}

} /* extern "C" */
