/* lobby/fn_80212810.cpp - the lobby item/equipment page layer.
 * RANGE. .text 0x80212810-0x80219260 (105 functions); .data 0x805B97BC-0x805B9DDC, .sdata 0x80791DC0-0x80791EB0,
 *   .sdata2 0x80799C18-0x80799C40, extab, extabindex.  `tudiscover at 0x80212810` puts the left edge here with strong
 *   evidence (the `jumptable_805B9770` -> `jumptable_805B97BC` cut) and the right edge only weakly (0x80213290 /
 *   0x80213870).
 * NAMES. The map and the dump give only placeholders (`LbPutAnaPageArrow__FUsUsssUsP10_mh_ivec2_bb` is the range's one
 *   real name), so the file keeps the map's stem.  Module `lobby`: `LbStr`, `draw_sprite_ary`, `get_lsp_data`,
 *   `GetMenuFontColor`, `put_menu_cursor`, and the `.bss` it reads (`lobby_w`, `lb_npc`).
 * RESIDUALS. 68 rows unwritten: 0x80212B1C-0x80214EF0, 0x80214F30-0x80215A74, 0x80215AE4-0x80215E6C,
 *   0x80215E84-0x802164F0, 0x8021677C-0x80216A08, 0x80216A68-0x80217934, 0x802179D4-0x80217B04, 0x80217C68-0x80217DA0,
 *   0x80217DD8-0x80217F4C, 0x80217FA0-0x802180D8, 0x80218138-0x80219080.  `fn_80214948` among them: `unsplit/lobby.h`
 *   declares five parameters where retail reads a sixth (`clrlwi r8,r8,24`), so its real definition would clash.
 *  - `fn_802128A8`: retail zero-extends `kind` (`clrlwi r3,r3,24`) before the switch; a `u8` parameter drops the mask,
 *    `s32` + `switch ((u8)kind)` gives a signed tree, `kind = (u8)kind` moves the copy to r31; the `u8` form is kept;
 *  - `fn_80215E6C`: retail keeps `clrlwi r0,r3,24; slwi r0,r0,2`, ours fuses them into one `rlwinm` (the unit does not
 *    set the `#pragma peephole off` its siblings use);
 *  - `fn_80212A2C`: ours fuses `clrlwi.` where retail keeps `clrlwi` + `cmpwi` (the same missing pragma);
 *  - `fn_80216560`, `fn_80216678`, `fn_80217B04`: retail narrows the `u8`/`u16` values (`clrlwi`) where ours passes
 *    them through; `fn_80217944`, `fn_80217988`: the narrowed copy lands in r4 where ours uses r3;
 *  - `fn_80215A74`: `li r0,1` is scheduled earlier in retail; `fn_80217BE4`: its two counters swap r28/r29;
 *  - `fn_802180D8`: the compare tree is laid out differently (ours 16 B longer);
 *  - `fn_802190FC`: `addi r3` and `li r5` swap (the copy's argument setup order).
 *   flipcheck: `.data` 0x38 against 0x620; `.sdata`/`.sdata2` claimed, not emitted; `.text`/extab/extabindex short of
 *   the claim; `GetItemData`, `LbPutAnaPageArrow` and `PutPageArrow` are referenced unmangled where the map spells
 *   `GetItemData__FUs`, `LbPutAnaPageArrow__FUsUsssUsP10_mh_ivec2_bb` and `PutPageArrow__FPUsssUsPC10_mh_ivec2_Uc`.
 */
#include "types.h"

#include "unsplit/lobby.h"

#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The 0xFFFF-masked byte/word table `fn_80212810` and `fn_802128A8` index by the item kind. */
extern const u8 lbl_805B97F4[];

/* The pointer run `fn_80215E6C` indexes by a byte id (the `.data` run the discovery surfaced). */
extern u8* lbl_805B9B50[];

/* Foreign unsplit callees whose bracketing registered units name different modules (rule 2's named
 * gap - Pl below, stage/sound above), so no `unsplit/<module>.h` is sound for them. */
extern "C" {
u8 equip_kind_table_class(u8 id);
void* fn_8027EC50(u8 id, u16 sel);
void* fn_8027E2A8(u8 id, u16 sel);
void* fn_8027E344(u8 id);
u8 fn_8027F0F4(u16 id);
}

/* The record `fn_8027EC50` returns: the item-kind byte the table is compared against and its signed
 * availability counter.  Only the two byte pairs this unit reads are named. size: 0x12 (approximate:
 * the allocation size is not in this range). */
typedef struct LbEquipCheck {
    /* +0x00 */ u8 unused_0x00[0x0E];
    /* +0x0E */ u8 field_0x0E;
    /* +0x0F */ s8 value_0x0F;
    /* +0x10 */ u8 field_0x10;
    /* +0x11 */ s8 value_0x11;
} LbEquipCheck; /* size: 0x12 */

/* The record `fn_8027E2A8` returns; only its flag byte is read here. size: 0x7 (approximate). */
typedef struct LbItemFlags {
    /* +0x00 */ u8 unused_0x00[6];
    /* +0x06 */ u8 flags_0x06;
} LbItemFlags; /* size: 0x7 */

/* The 0x80-byte scroll-list work block the tail of this range operates on: a byte kind/mode pair,
 * two 16-bit cursors, four copied tables and the owner id.  Layout read off `fn_80219080`'s memset
 * (0x80) and the memcpy lengths in `fn_802190FC`. */
typedef struct LbScrollWork {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 flag_0x01;
    /* +0x02 */ u8 mode_0x02;
    /* +0x03 */ u8 unused_0x03[5];
    /* +0x08 */ s16 cursor_0x08;
    /* +0x0A */ s16 count_0x0A;
    /* +0x0C */ u16 value_0x0C;
    /* +0x0E */ u16 value_0x0E;
    /* +0x10 */ u8 value_0x10;
    /* +0x11 */ u8 unused_0x11[0x1B];
    /* +0x2C */ s32 value_0x2C;
    /* +0x30 */ s32 value_0x30;
    /* +0x34 */ s32 owner_0x34;
    /* +0x38 */ u8 table1_0x38[12];
    /* +0x44 */ u8 table2_0x44[24];
    /* +0x5C */ u8 table3_0x5C[12];
    /* +0x68 */ u8 table4_0x68[24];
} LbScrollWork; /* size: 0x80 */

extern "C" {

s32 fn_80212810(s32 id)
{
    s32 value;

    switch ((u8)id) {
    case 0:
    default:
        value = 249;
        break;
    case 1:
        value = 250;
        break;
    case 2:
        value = 251;
        break;
    case 3:
        value = 252;
        break;
    case 4:
        value = 253;
        break;
    case 5:
        value = 254;
        break;
    case 6:
        value = 255;
        break;
    case 7:
        value = 256;
        break;
    case 8:
        value = 257;
        break;
    case 9:
        value = 269;
        break;
    case 10:
        value = 270;
        break;
    case 11:
        value = 235;
        break;
    case 12:
        value = 236;
        break;
    case 13:
        value = 237;
        break;
    }
    return (s32)LbStr(0, (u16)value);
}

s32 fn_802128A8(u8 kind, s32 arg, s32 sel)
{
    LbEquipCheck* equip;
    LbItemFlags* flags;

    switch (kind) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        if (equip_kind_table_class((u8)arg) != 1) {
            return 0;
        }
        if ((u8)arg == 11) {
            return 0;
        }
        equip = (LbEquipCheck*)fn_8027EC50((u8)arg, (u16)sel);
        if (lbl_805B97F4[(u8)kind] != equip->field_0x0E) {
            return 0;
        }
        if (equip->value_0x0F > 0) {
            return 1;
        }
        return 0;
    case 6:
    case 7:
    case 8:
        if (equip_kind_table_class((u8)arg) != 1) {
            return 0;
        }
        if ((u8)arg == 11) {
            return 0;
        }
        equip = (LbEquipCheck*)fn_8027EC50((u8)arg, (u16)sel);
        if (lbl_805B97F4[(u8)kind] != equip->field_0x10) {
            return 0;
        }
        if (equip->value_0x11 > 0) {
            return 1;
        }
        return 0;
    case 0:
        return 1;
    case 9:
        flags = (LbItemFlags*)fn_8027E2A8((u8)arg, (u16)sel);
        if ((flags->flags_0x06 & 4) != 0) {
            return 1;
        }
        return 0;
    case 10:
        flags = (LbItemFlags*)fn_8027E2A8((u8)arg, (u16)sel);
        if ((flags->flags_0x06 & 8) != 0) {
            return 1;
        }
        return 0;
    }
    return 0;
}

s32 fn_80212A2C(s32 kind, s32 arg)
{
    switch ((u8)kind) {
    case 0:
        return 1;
    case 11:
        if (fn_8027F0F4((u16)arg) == 1) {
            return 1;
        }
        break;
    case 12:
        if (fn_8027F0F4((u16)arg) == 2) {
            return 1;
        }
        break;
    case 13:
        if (fn_8027F0F4((u16)arg) == 3) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 fn_80212AD0(s32 id)
{
    id = (u8)id;
    switch (id) {
    case 0:
    case 16:
        return 0;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        return 1000;
    case 17:
        return 0;
    default:
        return 0;
    }
}



/* ------------------------------------------------------------------------------------------------ */

/* The arrow helper this unit defines further down, used by the forwarder just below. */
void LbPutAnaPageArrow(u16 a, u16 b, s16 c, s16 d, u16 e, _mh_ivec2_* pos, bool f, bool g);

/* The 9-argument page drawer the small wrappers below forward to; the 9th argument travels on the
 * stack and selects which item table the page uses (0..11). */
void fn_80215F8C(u16 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g, u16 h, u8 kind);

/* The 9-argument variant the `fn_80216Axx` wrappers forward to. */
void fn_8021677C(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, u8 g, s32 h, u8 kind);

/* Forwarders around LbPutAnaPageArrow that fix one of the trailing flags. */
void fn_80215AC8(s32 a, s32 b, s32 c, s32 d, s32 e, _mh_ivec2_* pos, bool side)
{
    LbPutAnaPageArrow((u16)a, (u16)b, (s16)c, (s16)d, (u16)e, pos, side, true);
}

/* The 0x805B9B50 pointer run, indexed by a byte id. */
void* fn_80215E6C(u8 id)
{
    return lbl_805B9B50[id];
}

/* The page drawers, one per table kind. */
void fn_802164F0(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g)
{
    fn_80215F8C((u16)a, b, c, d, e, f, 0, (u16)g, 7);
}

void fn_80216528(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g)
{
    fn_80215F8C((u16)a, b, c, d, e, f, 0, (u16)g, 0);
}

s32 fn_80216560(u16 a, u16 b, s16* c, s32 d, s32 e, s32 f, s32 g)
{
    fn_80215F8C(a, b, (s32)c, d, e, f, 0, (u16)g, 1);
}

void fn_80216598(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g)
{
    fn_80215F8C((u16)a, b, c, d, e, f, 0, (u16)g, 2);
}

void fn_802165D0(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g)
{
    fn_80215F8C((u16)a, b, c, d, e, f, 0, (u16)g, 3);
}

void fn_80216608(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g)
{
    fn_80215F8C((u16)a, b, c, d, e, f, 0, (u16)g, 5);
}

void fn_80216640(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g)
{
    fn_80215F8C((u16)a, b, c, d, e, f, 0, (u16)g, 6);
}

s32 fn_80216678(u16 a, s32 b, s16* c, s32 d, s32 e, s32 f, s32 g)
{
    fn_80215F8C(a, b, (s32)c, d, e, f, 0, (u16)g, 10);
}

void fn_802166B0(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f)
{
    fn_80215F8C((u16)a, 0, b, c, d, e, 0, (u16)f, 8);
}

void fn_80216714(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g)
{
    fn_80215F8C((u16)a, b, c, d, e, f, 0, (u16)g, 11);
}

void fn_8021674C(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g, s32 h)
{
    fn_80215F8C((u16)a, b, c, d, e, f, g, (u16)h, 1);
}

void fn_80216A08(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g, s32 h)
{
    fn_8021677C(a, b, c, d, e, f, (u8)g, 3, (u8)h);
}

void fn_80216A38(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g, s32 h)
{
    fn_8021677C(a, b, c, d, e, f, (u8)g, 5, (u8)h);
}

/* ------------------------------------------------------------------------------------------------ */

/* The page-arrow setter family. */
s32 fn_802178B8(s32 on);
s32 fn_80217B04(u8 a, u16 b);

s32 fn_80217934(void)
{
    return fn_802178B8(0);
}

s32 fn_8021793C(void)
{
    return fn_802178B8(1);
}

s32 fn_80217DA0(s32 a, s32 b)
{
    return fn_80217B04((u8)a, (u16)b);
}

s32 fn_80217DAC(s32 a, s32 b)
{
    return fn_80217B04((u8)a, (u16)b) * 2;
}

s32 fn_80217944(s32 a, s32 bits)
{
    return (((LbItemFlags*)fn_8027E344(a))->flags_0x06 & (u8)bits) == (u8)bits;
}

s32 fn_80217988(s32 a, s32 b, s32 bits)
{
    return (((LbItemFlags*)fn_8027E2A8((u8)a, (u16)b))->flags_0x06 & (u8)bits) == (u8)bits;
}

s32 fn_80214F30(u16 a, void* str);

s32 fn_80214EF0(s32 a, s16 b)
{
    return fn_80214F30((u16)a, LbStr(3, (u16)b));
}

/* The page-arrow table pointer the `PutPageArrow` helper reads. */
extern u16 lbl_805B9924[];

void PutPageArrow(u16* table, s16 c, s16 d, u16 e, const _mh_ivec2_* pos, u8 flags);

void fn_80219080(s32 owner, LbScrollWork* self, u8 kind, u8 mode)
{
    memset(self, 0, 0x80);
    self->owner_0x34 = owner;
    self->kind_0x00 = kind;
    self->mode_0x02 = mode;
    self->value_0x10 = 0;
    self->value_0x2C = 0;
}

void fn_80218138(LbScrollWork* self);

void fn_802190FC(LbScrollWork* self, void* a, void* b, void* c, void* d)
{
    if (a != NULL) {
        memcpy(self->table1_0x38, a, 12);
    }
    if (b != NULL) {
        memcpy(self->table2_0x44, b, 24);
    }
    if (c != NULL) {
        memcpy(self->table3_0x5C, c, 12);
    }
    if (d != NULL) {
        memcpy(self->table4_0x68, d, 24);
    }
    fn_80218138(self);
    if (self->cursor_0x08 >= self->count_0x0A) {
        self->cursor_0x08 = 0;
    }
}

void fn_802191C4(LbScrollWork* self, u8 kind)
{
    self->kind_0x00 = kind;
    fn_80218138(self);
    if (self->cursor_0x08 >= self->count_0x0A) {
        self->cursor_0x08 = 0;
    }
}

void fn_8021920C(LbScrollWork* self, u16 value)
{
    self->value_0x0E = value;
    fn_80218138(self);
    if (self->cursor_0x08 >= self->count_0x0A) {
        self->cursor_0x08 = 0;
    }
}

void fn_802190F4(LbScrollWork* self, u8 flag)
{
    self->flag_0x01 = flag;
}

void fn_80219254(LbScrollWork* self, s32 value, u16 index)
{
    self->value_0x30 = value;
    self->value_0x0C = index;
}

void* fn_8029F6B4(u16 id);
void* GetItemData(u16 id);

/* A 12-byte row `fn_80217F4C` clears and fills. size: 0xC. */
typedef struct LbRow12 {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u16 value_0x02;
    /* +0x04 */ u8 unused_0x04[8];
} LbRow12; /* size: 0xC */

void fn_80217F4C(LbRow12* row, u8 kind, u16 value)
{
    memset(row, 0, 0xC);
    row->kind_0x00 = kind;
    row->value_0x02 = value;
}

s32 fn_80217BE4(u16* list, s32 count)
{
    s32 sum;
    s32 i;

    sum = 0;
    for (i = 0; i < (u8)count; i++) {
        if (list[i + 3] != 0) {
            u16* entry = (u16*)fn_8029F6B4(list[i + 3]);
            u32* item = (u32*)GetItemData(entry[1]);
            sum += (s32)item[4];
        }
    }
    return sum;
}

s32 fn_802180D8(s32 kind)
{
    s32 k;

    k = (u8)kind;
    if (k < 11) {
        if (k == 6) {
            return 4;
        }
        if (k >= 7) {
            return 1;
        }
        if (k >= 1) {
            return 3;
        }
        return 0;
    }
    if (k >= 16) {
        return 0;
    }
    if (k >= 14) {
        return 1;
    }
    return 2;
}

/* The pointer `fn_8027ED6C`/`fn_8027E354` hand back; the two fields this unit reads are named.
 * size: 0x10 (approximate: the record is owned outside this range). */
typedef struct LbRefEntry {
    /* +0x00 */ u8 unused_0x00[4];
    /* +0x04 */ s32 value_0x04;
    /* +0x08 */ s32 value_0x08;
    /* +0x0C */ s32 value_0x0C;
} LbRefEntry; /* size: 0x10 */

void fn_8027ED6C(u8 id, u16 sel, LbRefEntry** a, LbRefEntry** b);
LbRefEntry* fn_8027E354(u8 id, u16 sel);

s32 fn_80217B04(u8 kind, u16 sel)
{
    LbRefEntry* a;
    LbRefEntry* b;
    LbRefEntry* p;
    s32 result;
    u8 k;

    result = 0;
    k = (u8)kind;
    switch (equip_kind_table_class((u8)kind)) {
    case 0:
        p = fn_8027E354((u8)kind, (u16)sel);
        if (p != NULL) {
            result = p->value_0x08;
        }
        break;
    case 1:
    case 2:
        fn_8027ED6C((u8)kind, (u16)sel, &a, &b);
        switch (k) {
        case 7:
        case 8:
        case 9:
        case 10:
        case 14:
        case 15:
            p = a;
            if (p != NULL) {
                result = p->value_0x04;
            }
            break;
        case 11:
        case 12:
        case 13:
            p = b;
            if (p != NULL) {
                result = p->value_0x0C;
            }
            break;
        }
        break;
    }
    return result;
}

void fn_80215A74(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f)
{
    _mh_ivec2_ pos;
    u8 flags;

    pos.x = (u16)a;
    pos.y = (u16)b;
    flags = 1;
    if (f == 0) {
        flags |= 0x80;
    }
    PutPageArrow(lbl_805B9924, (s16)c, (s16)d, (u16)e, &pos, flags);
}

} /* extern "C" */
