/* lobby/fn_801E7530.cpp - the lobby menu-layer group.
 *
 * `.text` 0x801E7530..0x801EC9E0 (77 functions, 21680 B), extab 0x800105B4..0x8001079C (61 unwind-only
 * 8-byte records), extabindex 0x8002C778..0x8002CA54 (61 x 12 B).  Registered from
 * `proposal/801E7530_fn_801E7530.cpp`.
 *
 * Module `lobby`.  The range's callees are the lobby UI API (`lobby_w` .bss 0x806AAB44, `LbStr`,
 * `lb_param_w`, `get_lsp_data`, `GetMenuFontColor`, `set_blendmode`, `draw_font_idx`,
 * `draw_sprite_ary`) and its neighbour above is the registered `lobby/lobby_scene.c`; the .bss run
 * around `lobby_w` (`Screen_w`, `option_w`, `lb_param_w`) is the lobby/option state.  Language C++:
 * every call out of the range is a mangled symbol.
 *
 * Name.  The map has only `fn_XXXXXXXX` for this range and the runtime dump answers only `zz_`
 * placeholders (`dumpmap.py lookup 0x801E7530` -> `zz_01e7530_`), so the file keeps the map's stem.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap.py
 * lookup on the range's inventory: all 77 names are bare `.text` entries in
 * config/RMHE08/symbols.txt and the runtime dump has only `zz_XXXXXXXX_` placeholders for them)
 */
#include "types.h"

#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

#include "unsplit/lobby.h"
#include "menu/menu_item.h"   /* `put_menu_cursor` (owner: `menu/menu_item.cpp`, rule 2) */

/* `sprintf` is a libc intrinsic; `-nosyspath` means it has to be declared here (the owner unit has no
 * publishable header). */
extern "C" {
int sprintf(char* dst, const char* fmt, ...);
}

/* Foreign functions owned by other registered units: declared here as plain prototypes (no `extern`
 * keyword), which is how the existing units carry the callees their owners' headers do not yet publish.
 * Each is filed as a shared-file request so the declaration can move to its owner's header. */
extern "C" {
void fn_8004A1F8(void* p);
s32 fn_8004B0A4(u16 id, void* out);
s32 fn_8004BA3C(s16 a, void* b, s32 c, s32 d, s32 e);
void* fn_8004D134(void);
s32 fn_8004D27C(s32 a);
s32 fn_8004D334(s32 a);
void fn_800526F8(_mh_ivec2_* dst, const _mh_ivec2_* src);
u16 fn_800CEF18(u16 id);
u32 game_ready_ck(void);
s32 fn_800DBC84(s32 id);
}

/* This unit's own functions that earlier bodies call; C linkage, like their definitions. */
extern "C" {
void fn_801E7530(LbMenuWork* self, s16 list_mode);
void fn_801E78B4(LbMenuWork* self);
s32 fn_801E790C(LbMenuWork* self);
void fn_801E80FC(LbMenuWork* self);
void fn_801E89B4(LbMenuWork* self);
void fn_801E8C44(void);
void fn_801E88C4(LbStrBlock* dst, const LbStrBlock* src);
void fn_801E8CDC(u8 id, s8* str);
void fn_801E9010(void);
void fn_801E9AA8(u8 flag);
void fn_801EB464(void);
void fn_801EB744(u16 id, _mh_ivec2_* off);
}

/* Foreign C++-linkage callees (their map names are manglings); the front-end reproduces those names from
 * these declarations, and rule 9 forbids spelling the manglings at the call site. */
s32 ck_WideMode(void);
void font_set_size(s16 w, s16 h);
s32 ran_suu(s32 n);
void set_blendmode(u8 a, u8 b, u8 c);
void set_zmode(bool a, u8 b, bool c);
void subTransSet(u32 a, s32 b, u32* c);
void sysSE_req(s32 id);

/* ------------------------------------------------------------------------------------------------ */

/* The two 6-byte selection-table walks below share this: a row's flag gates it (`fn_802FB4BC`). */

/* Every map name in this range is a plain `fn_XXXXXXXX`, so the whole batch of definitions takes C
 * linkage (the map does not mangle them). */
extern "C" {
void fn_801E7530(LbMenuWork* self, s16 list_mode)
{
    const u16* row;
    s16 count;
    s16 i;

    self->list_mode_0x0C = list_mode;
    self->count_0x10 = 0;
    count = 0;
    row = lbl_805BA6D4;
    for (;;) {
        if (row[0] == 0) {
            break;
        }
        if (fn_802FB4BC(row[2]) == 0) {
            break;
        }
        count++;
        row += 3;
    }
    if (fn_802FB4BC(0xA) != 0) {
        self->stage_0x0E = (count > 6) ? 6 : 5;
    } else {
        self->stage_0x0E = (count > 6) ? 5 : 4;
    }
    memset(self->entries_0xD0, 0, 0x30);
    if ((u32)self->list_mode_0x0C > 2U) {
        switch (self->list_mode_0x0C) {
        case 3:
            row = lbl_805BA6D4;
            for (;;) {
                if (row[0] == 0) {
                    break;
                }
                if (fn_802FB4BC(row[2]) == 0) {
                    break;
                }
                self->entries_0xD0[self->count_0x10].kind_0x00 = (u8)row[0];
                self->entries_0xD0[self->count_0x10].id_0x02 = row[1];
                self->entries_0xD0[self->count_0x10].icon_kind_0x01 = 0;
                i = self->count_0x10;
                self->entries_0xD0[i].value_0x04 = *(s32*)(lbex_zaco_str + i * 4);
                self->count_0x10 = i + 1;
                if (self->count_0x10 >= 6) {
                    break;
                }
                row += 3;
            }
            break;
        case 4:
            if (self->stage_0x0E != 3 || fn_802FB4BC(0xA) != 1U) {
                row = lbl_805BA6D4 + 18;
                for (;;) {
                    if (row[0] == 0) {
                        break;
                    }
                    if (fn_802FB4BC(row[2]) == 0) {
                        break;
                    }
                    self->entries_0xD0[self->count_0x10].kind_0x00 = (u8)row[0];
                    self->entries_0xD0[self->count_0x10].id_0x02 = row[1];
                    self->entries_0xD0[self->count_0x10].icon_kind_0x01 = 0;
                    i = self->count_0x10;
                    { u8* base18 = lbex_zaco_str + 0x18; self->entries_0xD0[i].value_0x04 = *(s32*)(base18 + i * 4); }
                    self->count_0x10 = i + 1;
                    if (self->count_0x10 >= 6) {
                        break;
                    }
                    row += 3;
                }
            } else {
                row = lbl_805BA718;
                for (;;) {
                    if (row[0] == 0) {
                        break;
                    }
                    if (fn_802FB4BC(row[3]) == 0) {
                        break;
                    }
                    self->entries_0xD0[self->count_0x10].kind_0x00 = (u8)row[0];
                    self->entries_0xD0[self->count_0x10].id_0x02 = row[1];
                    self->entries_0xD0[self->count_0x10].icon_kind_0x01 = 1;
                    i = self->count_0x10;
                    self->entries_0xD0[i].value_0x04 = *(s32*)(lbex_main_str + i * 4);
                    self->count_0x10 = i + 1;
                    if (self->count_0x10 >= 6) {
                        break;
                    }
                    row += 4;
                }
            }
            break;
        case 5:
            row = lbl_805BA718;
            for (;;) {
                if (row[0] == 0) {
                    break;
                }
                if (fn_802FB4BC(row[3]) == 0) {
                    break;
                }
                self->entries_0xD0[self->count_0x10].kind_0x00 = (u8)row[0];
                self->entries_0xD0[self->count_0x10].id_0x02 = row[1];
                self->entries_0xD0[self->count_0x10].icon_kind_0x01 = 1;
                i = self->count_0x10;
                self->entries_0xD0[i].value_0x04 = *(s32*)(lbex_main_str + i * 4);
                self->count_0x10 = i + 1;
                if (self->count_0x10 >= 6) {
                    break;
                }
                row += 4;
            }
            break;
        }
    } else {
        const u16* p = lbl_805BA660 + (u16)self->list_mode_0x0C * 18;

        for (;;) {
            if (p[0] == 0) {
                break;
            }
            self->count_0x10 = self->count_0x10 + 1;
            if (self->count_0x10 >= 6) {
                break;
            }
            p += 3;
        }
        fn_801E74B0(self, self->list_mode_0x0C);
    }
    if (self->count_0x10 <= self->selected_0x08) {
        self->selected_0x08 = self->count_0x10 - 1;
    }
}

void fn_801E78B4(LbMenuWork* self)
{
    if (fn_802FB4BC(0xA) != 0) {
        self->stage_0x0E = 3;
    } else {
        self->stage_0x0E = 2;
    }
    fn_801E7530(self, 0);
}

s32 fn_801E790C(LbMenuWork* self)
{
    s16 value;
    s32 result = 0;

    if (fn_8021213C(0x20, 0) != 0) {
        result = 2;
        sysSE_req(1);
    } else if (fn_802121F4(0xC) != 0) {
        /* The string block goes through the owner's `s32` tail parameter; the cast emits nothing. */
        value = fn_802A8EC0(self->list_mode_0x0C, self->stage_0x0E, fn_802122AC(0), 4, 8, (s32)self->str_0x24);
        self->list_mode_0x0C = value;
        fn_801E7530(self, value);
    } else if (fn_802121F4(3) != 0) {
        self->selected_0x08 = fn_802A8EFC(self->selected_0x08, self->count_0x10, fn_802122AC(0), 1, 2);
    }
    return result;
}

void fn_801E79E4(void)
{
    LbMenuWork* self;
    u16 flags;
    u8 state;

    self = lobby_w.menu_0xAC;
    if (self->open_0x05 != 0) {
        u8 timer = self->timer_0x06 + 1;

        self->timer_0x06 = timer;
        if (timer > 5U) {
            self->timer_0x06 = 5U;
        }
    } else if (self->timer_0x06 != 0) {
        self->timer_0x06 = self->timer_0x06 - 1;
    }
    self->item_id_0x12 = 0;
    state = self->state_0x00;
    switch (state) {
    case 0:
        if ((u32)(fn_8021D5BC() - 1) <= 1U) {
            self->state_0x00 = 1U;
            fn_801E6F80(self);
            flags = 0;
            if (fn_802FB4BC(5U) == 0 && fn_802089F4(self->data_0x1C) == 0xFFFFFFFFU) {
                flags = 4;
            }
            if (fn_802FB4BC(6U) == 0) {
                flags |= 1;
            }
            if (fn_8033B6C0(self->data_0x1C, self->data_0x20) > 0 && fn_802FB4F4(4) == 0) {
                fn_802146F0(self->str_0x24, lbl_805B7CB0 + 20, flags, 0x10);
            } else {
                fn_802146F0(self->str_0x24, lbl_805B7CB0, flags, 0);
            }
            self->value_0x38 = -1;
            self->value_0x3C = 3;
            fn_802BBA64(1);
            fn_800DBC84(0x29);
        }
        /* fall through */
    default:
        for (;;) {
            state = self->state_0x00;
            if (state == 0 || state == 8) {
                break;
            }
            subTransSet((u32)fn_801E9010, 0, 0);
            break;
        }
        break;
    case 1:
        fn_802DE224();
        switch (fn_80214798(self->str_0x24)) {
        case 1:
            self->mode_0x01 = 0;
            self->unused_0x02[1] = 0;
            self->list_mode_0x0C = 0;
            switch (self->str_0x24[0]) {
            case 0:
                self->state_0x00 = 2U;
                self->list_mode_0x0C = 0;
                self->stage_0x0E = 3;
                self->open_0x05 = 0;
                self->timer_0x06 = 0;
                sysSE_req(0x1B);
                break;
            case 1:
                self->state_0x00 = 3U;
                fn_801E6F80(self);
                sysSE_req(0x1B);
                break;
            case 2:
                self->state_0x00 = 4U;
                {
                    LbSeParam param;

                    param.value_0x00 = 0x63;
                    param.mode_0x04 = 0xB;
                    param.id_0x06 = -1;
                    param.limit_0x08 = 0xC;
                    param.max_0x0A = 0xF;
                    param.cur_0x0C = fn_801E74B0(self, 0);
                    param.time_0x10 = 0x13E6;
                    param.count_0x12 = 0;
                    param.data_0x14 = self->str_0x24;
                    param.flag_0x18 = 1;
                    eft052_hold_entry_set((s32*)&param, 0);
                }
                sysSE_req(5);
                break;
            case 3:
                self->state_0x00 = 7U;
                fn_801E78B4(self);
                sysSE_req(5);
                break;
            case 4:
                self->state_0x00 = 6U;
                fn_8033B6C0(self->data_0x1C, self->data_0x20);
                sysSE_req(0x1B);
                break;
            }
            break;
        case 2:
            self->state_0x00 = 8U;
            fn_8021CBB0(1);
            sysSE_req(1);
            fn_802BBA64(0);
            break;
        }
        return;
    case 7:
        fn_802DE224();
        if ((u32)(fn_801E790C(self) - 1) <= 1U) {
            self->state_0x00 = 1U;
        }
        return;
    case 6:
        fn_802DE224();
        switch (fn_8033B990()) {
        case 1:
            break;
        case 2:
            self->state_0x00 = 1U;
            return;
        }
        return;
    case 2:
        fn_802DE224();
        if (fn_8021213C(0x20, 0) != 0) {
            self->state_0x00 = 1U;
            sysSE_req(1);
        } else if (fn_8021213C(0x80, 0) != 0) {
            self->open_0x05 = self->open_0x05 ^ 1;
            if (self->open_0x05 != 0) {
                sysSE_req(0x1B);
            } else {
                sysSE_req(1);
            }
        } else if (fn_802121F4(0xC) != 0) {
            self->list_mode_0x0C = fn_802A8ED8(self->list_mode_0x0C, self->stage_0x0E, fn_802122AC(0), 4, 8, 6);
        }
        return;
    case 3:
        fn_802DE224();
        switch (fn_801E7178(self)) {
        case 1:
            self->state_0x00 = 5U;
            fn_80359D98(lbl_80794880 + 0x4860, lb_item_get_data + 0x20);
            break;
        case 2:
            self->state_0x00 = 1U;
            break;
        }
        return;
    case 4:
        fn_802DE224();
        if ((u32)(fn_801E73A4(self) - 1) <= 1U) {
            self->state_0x00 = 1U;
        }
        return;
    case 5:
        fn_802DE224();
        if (fn_8035A034() == 1) {
            self->state_0x00 = 1U;
        }
        return;
    case 8:
        if ((u32)(fn_8021D5BC() - 1) <= 1U) {
            fn_801E6DCC();
        }
        return;
    }
}

void fn_801E7EA8(u16 id, const _mh_ivec2_* pos)
{
    LbItemData* data;
    u16 a;
    u16 b;
    u8 buf[8];

    data = (LbItemData*)fn_801E6F10(id);
    draw_sprite_ary(&lbl_805B7D18[0], pos);
    draw_font_idx(0x1DD9U, (s8*)fn_802DFACC((u8)id), 4, pos);
    a = fn_801E6EA8(id, 0);
    b = fn_801E6EA8(id, 1);
    sprintf((char*)buf, (char*)lbl_80791B88, a);
    draw_font_idx(0x1DDAU, (s8*)buf, 2, pos);
    sprintf((char*)buf, (char*)lbl_80791B88, b);
    draw_font_idx(0x1DDBU, (s8*)buf, 2, pos);
    sprintf((char*)buf, (char*)lbl_80791B88, data->rate_0x02 * a + data->rate_0x04 * b);
    draw_font_idx(0x1DDCU, (s8*)buf, 2, pos);
    draw_font_idx(0x1DDDU, (s8*)LbStr(1, 0x2CU), 0, pos);
    draw_monstericon_idx(0x1DCA, (u8)id, pos);
}

void fn_801E7FEC(u16 id, const _mh_ivec2_* pos)
{
    LbItemData* data;
    u16 a;
    u8 buf[8];

    data = (LbItemData*)fn_801E6F48(id);
    draw_sprite_ary(&lbl_805B7D38[0], pos);
    draw_font_idx(0x1DEFU, (s8*)fn_802DFACC((u8)id), 4, pos);
    a = fn_801E6EA8(id, 0);
    sprintf((char*)buf, (char*)lbl_80791B88, a);
    draw_font_idx(0x1DF0U, (s8*)buf, 2, pos);
    sprintf((char*)buf, (char*)lbl_80791B88, data->rate_0x02 * a);
    draw_font_idx(0x1DF1U, (s8*)buf, 2, pos);
    draw_font_idx(0x1DF2U, (s8*)LbStr(1, 0x2CU), 0, pos);
    draw_monstericon_idx(0x1DE6, (u8)id, pos);
}

void fn_801E80FC(LbMenuWork* self)
{
    _mh_ivec2_ anchor;
    _mh_ivec2_ pos;
    char text[16];
    s16 bound;
    s32 step;
    s32 mode;
    s32 i;
    u16* rate;
    const u16* idp;
    const u16* idp2;

    get_lsp_data(0x1DB1U, &anchor);
    draw_sprite_ary((const u16*)lbl_805B7CFC, &anchor);
    draw_font_idx(0x1DB4U, (s8*)LbStr(0, 0x132U), 0, &anchor);
    bound = self->slots_0x44[self->list_mode_0x0C];
    if (bound < self->bound_0x4C) {
        step = 3;
        mode = 0;
        draw_sprite_ary((const u16*)lbl_80791B70, &anchor);
        draw_font_idx(0x1DB7U, (s8*)LbStr(0, 0x130U), 4, &anchor);
    } else {
        step = 5;
        mode = 1;
        draw_sprite_ary((const u16*)lbl_80791B78, &anchor);
        draw_font_idx(0x1DB8U, (s8*)LbStr(0, 0x131U), 4, &anchor);
    }
    sprintf(text, (char*)lbl_80791B88, self->value_0x100);
    draw_font_idx(0x1DB9U, (s8*)text, 2, &anchor);
    draw_font_idx(0x1DBAU, (s8*)LbStr(1, 0x2CU), 0, &anchor);
    i = 0;
    idp = lbl_80791B80;
    idp2 = lbl_805B7D0C;
    rate = &self->flags_0x50[bound];
    for (; i < step; i++) {
        if (mode == 0) {
            get_lsp_data(idp[0], &pos);
            pos.x += anchor.x;
            pos.y += anchor.y;
            draw_sprite_ary((const u16*)lbl_805B7D4C, &pos);
            if (rate[0] != 0) {
                fn_801E7EA8(rate[0], &pos);
            }
        } else {
            get_lsp_data(idp2[0], &pos);
            pos.x += anchor.x;
            pos.y += anchor.y;
            draw_sprite_ary((const u16*)lbl_805B7D5C, &pos);
            if (rate[0] != 0) {
                fn_801E7FEC(rate[0], &pos);
            }
        }
        rate++;
        idp++;
        idp2++;
    }
    fn_802159F0(0x1D93, self->list_mode_0x0C, self->stage_0x0E, self->item_id_0x12,
                (u32)((self->mode_0x01 - 1) << __cntlzw((self->mode_0x01 - 1) ^ 1)) >> 0x1FU);
}
void fn_801E8348(LbMenuWork* self)
{
    _mh_ivec2_ pos;
    u16 a = 0xFFFF;
    u16 b = 0xFFFF;
    u8 mode;

    get_lsp_data(0x1D99U, &pos);
    draw_sprite_ary((const u16*)lbl_805B7D70, &pos);
    fn_801E80FC(self);
    mode = self->mode_0x01;
    switch ((s32)mode) {
    case 0:
        a = 7;
        if ((s32)(self->bound_0x4C + self->bound_0x4E) == 0) {
            b = 8;
        }
        break;
    case 1:
        a = 9;
        fn_80215170(0x1879, self->data_0x1C);
        break;
    case 2:
        a = 0xA;
        break;
    }
    if (a != 0xFFFFU) {
        fn_80214EF0(0x1877, (s16)a);
        if (b != 0xFFFFU) {
            fn_8021505C(0x1877, (s16)b, 2);
        }
    }
}

void fn_801E843C(LbMenuWork* self)
{
    LbBigBlock* big;
    _mh_ivec2_ anchor;
    _mh_ivec2_ pos;
    u8 name[0x20];
    u8* np;
    u16* idp;
    u16 type;
    s32 t;
    s16 blink;
    s16 item;
    u8 idx;
    u8 kind;
    s32 i;
    u16 shown;

    big = (LbBigBlock*)lbl_80794880;
    if (chk_pointer() == 0) {
        get_lsp_data(0x1D8BU, &anchor);
        draw_sprite_ary((const u16*)lbl_80791B90, &anchor);
        draw_sprite_idx((u16)(-((get_option_cfg(7) == 0) & 1) + 0x1D8D), &anchor);
        draw_sprite_idx((u16)(-((self->open_0x05 == 0) & 1) + 0x1D8F), &anchor);
    }
    get_lsp_data(0x1D3EU, &anchor);
    draw_sprite_anim_ary((const u16*)lbl_805B7DA0, (u16)self->timer_0x06, &anchor);
    get_lsp_data(0x1D60U, &anchor);
    draw_sprite_ary((const u16*)lbl_805B7DC4, &anchor);
    draw_font_idx(0x1D7DU, (s8*)LbStr(0, 0x11BU), 0, &anchor);
    draw_font_idx(0x1D7EU, (s8*)LbStr(0, 0x11CU), 0, &anchor);
    if (self->list_mode_0x0C - 1U > 1U) {
        type = ((fn_8021F238() - 1) == 0) + 0x1D6F;
        memcpy(name, big->name_0x4068, 2);
        idx = big->index_0x406C;
        kind = big->tail_0x6010[0];
    } else {
        type = 0x1D71;
        { u8* entry = lbl_80794880 + 0x4850; fn_801E88C4((LbStrBlock*)name, (const LbStrBlock*)(entry + (self->list_mode_0x0C - 1) * 8)); }
        idx = name[4];
        kind = name[6];
    }
    draw_sprite_idx(type, &anchor);
    draw_font_idx(0x1D83U, (s8*)LbStr(0, 0x118U), (s16)((s16)self->list_mode_0x0C == 0 ? 5 : 1), &anchor);
    draw_font_idx(0x1D84U, (s8*)LbStr(0, 0x119U), (s16)((s16)self->list_mode_0x0C == 1 ? 5 : 1), &anchor);
    draw_font_idx(0x1D85U, (s8*)LbStr(0, 0x11AU), (s16)((s16)self->list_mode_0x0C == 2 ? 5 : 1), &anchor);
    draw_font_idx(0x1D7FU, (s8*)LbStr(0, (u16)(kind + 0x128)), 0, &anchor);
    t = idx * 4 - idx;
    draw_font_idx(0x1D82U, (s8*)LbStr(0, (u16)(lbl_805B7E18[t] + 0x11D)), 0, &anchor);
    draw_font_idx(0x1D81U, (s8*)LbStr(0, (u16)(lbl_805B7E18[t + 1] + 0x11F)), 0, &anchor);
    draw_font_idx(0x1D80U, (s8*)LbStr(0, (u16)(lbl_805B7E18[t + 2] + 0x122)), 0, &anchor);
    shown = 0;
    np = name;
    idp = &lbl_80791BA8[0];
    for (i = 0; i < 2; i++) {
        u16 id = *(u16*)np;

        if (lbl_805B7A88[id] != 0) {
            get_lsp_data(idp[0], &pos);
            pos.x += anchor.x;
            pos.y += anchor.y;
            draw_sprite_ary((const u16*)lbl_805B7E0C, &pos);
            draw_monstericon_idx(0x1D59, lbl_805B7A88[id], &pos);
            draw_font_idx(0x1D5FU, (s8*)fn_802DFACC(lbl_805B7A88[id]), 0, &pos);
            shown++;
        }
        np += 2;
        idp += 2;
    }
    if ((s32)shown == 0) {
        fn_801E6850((s16*)&pos, (void*)get_lsp_data(0x1D7DU, NULL));
        pos.y += 0x32;
        draw_font((const _SPR_DATA_&)pos, (s8*)LbStr(0, 0x13CU), 0, &anchor);
    }
    i = 0;
    idp = &lbl_80791B98[0];
    for (; i < 3; i++) {
        get_lsp_data(idp[0], &pos);
        pos.x += anchor.x;
        pos.y += anchor.y;
        draw_sprite_ary((const u16*)lbl_80791BA0, &pos);
        idp += 2;
    }
    get_lsp_data(lbl_80791B98[self->list_mode_0x0C], &pos);
    pos.x += anchor.x;
    pos.y += anchor.y;
    put_menu_cursor((u16*)lbl_805B7E00, 0, &pos);
}

void fn_801E88C4(LbStrBlock* dst, const LbStrBlock* src)
{
    dst->code_0x00 = src->code_0x00;
    dst->code_0x02 = src->code_0x02;
    dst->code_0x04 = src->code_0x04;
    dst->code_0x06 = src->code_0x06;
}

void fn_801E88E8(u8 mode)
{
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(0x1D27U, &pos);
        draw_sprite_ary((const u16*)lbl_805B7E5C, &pos);
    } else {
        get_lsp_data(0x1D22U, &pos);
        draw_sprite_ary((const u16*)lbl_805B7E50, &pos);
    }
    if (mode == 0) {
        fn_80215C04(0x1D11);
    } else {
        fn_802DF6E4(0x14FC);
    }
    get_lsp_data(0x1D12U, &pos);
    draw_sprite_ary((const u16*)lbl_805B7E30, &pos);
}

void fn_801E8994(void)
{
    fn_80214948((u8*)0x1D2D, lbl_805B7E68, lbl_805B7E80, (void*)0x1877, 0);
}

void fn_801E89B4(LbMenuWork* self)
{
    _mh_ivec2_ pos;
    _mh_ivec2_ p0;
    _mh_ivec2_ p1;
    _mh_ivec2_ p2;
    _mh_ivec2_ m;
    _mh_ivec2_ a;
    s32 i;
    s32 found;
    s32 off;

    get_lsp_data(0x1E16U, &pos);
    draw_sprite_ary((const u16*)lbl_805BAA20, &pos);
    found = 0;
    fn_801E6850((s16*)&p0, (s16*)get_lsp_data(0x1307U, NULL));
    fn_800526F8(&a, &p0);
    off = 0;
    for (i = 0; i < 0x1E; i++) {
        fn_80222848(i, (s16*)&m);
        fn_801E6850((s16*)&p2, (s16*)get_lsp_data(0x12F9U, NULL));
        fn_800526F8(&p2, &m);
        p0.x = m.x + a.x;
        p0.y = m.y + a.y;
        draw_sprite((const _SPR_DATA_&)p2, &pos);
        if (self->stage_0x0E == i) {
            found = 1;
            fn_800526F8(&p1, &m);
        }
        if ((u8)(self->state_0x00 + 0xFF) <= 1U && chk_pointer() == 0 && self->count_0x10 == i) {
            fn_801E6850((s16*)&p2, (s16*)get_lsp_data(0x1304U, NULL));
            p2.x += m.x;
            p2.y += m.y;
            set_blendmode(4, 1, 1);
            fn_802E0DA8((s16*)&p2, self->selected_0x08, (s16*)&pos);
            set_blendmode(4, 5, 1);
        }
        if (*(s32*)(self->data_0x1C + off) != 0) {
            draw_itemicon_item_id((const _SPR_DATA_&)p0, self->item_id_0x12, &pos);
        }
        off += 4;
    }
    if (self->list_mode_0x0C != 0 && found != 0) {
        set_blendmode(4, 1, 1);
        fn_801E6850((s16*)&p2, (s16*)get_lsp_data(0x1305U, NULL));
        p2.x += p1.x;
        p2.y += p1.y;
        fn_802E0DA8((s16*)&p2, (u16)self->stage_0x0E, (s16*)&pos);
        set_blendmode(4, 5, 1);
        fn_801E6850((s16*)&p2, (s16*)get_lsp_data(0x1306U, NULL));
        p2.x += p1.x;
        p2.y += p1.y;
        fn_802E0DA8((s16*)&p2, (u16)self->stage_0x0E, (s16*)&pos);
    }
}

void fn_801E8C44(void)
{
    _mh_ivec2_ pos;

    get_lsp_data(0x1DFBU, &pos);
    draw_sprite_ary((const u16*)lbl_805B7E8C, &pos);
    fn_801E89B4((LbMenuWork*)lbl_806BF310);
    fn_80222BC4(lbl_806BF310, 0x1E17, 0U);
}

void fn_801E8CA0(void)
{
    fn_801E8C44();
    fn_8035A7D8(0x1E26, lbl_805B7EC8, lbl_80791BAC, 0x1DFA, 0);
}

void fn_801E8CDC(u8 id, s8* str)
{
    _mh_ivec2_ pos;

    get_lsp_data(0x1E36U, &pos);
    draw_sprite_ary((const u16*)lbl_805B7F00, &pos);
    draw_font_idx(0x1E37U, (s8*)fn_802DFACC(id), 1, &pos);
    draw_font_idx(0x1E38U, str, 0, &pos);
}

void fn_801E8D5C(LbMenuWork* self)
{
    _mh_ivec2_ anchor;
    _mh_ivec2_ pos;
    LbMenuEntry* entry;
    LbItemData* data;
    const u16* idp;
    s16 i;
    s16 selected;
    u16 id;
    u8 buf[0x40];
    const u16* base;
    u8 mode;

    get_lsp_data(0x1E4BU, &anchor);
    draw_sprite_ary((const u16*)lbl_805B7EE0, &anchor);
    draw_sprite_ary((const u16*)lbl_80791BB8, &anchor);
    fn_802159F0(0x1E6D, self->list_mode_0x0C, self->stage_0x0E, self->item_id_0x12, 1U);
    if ((u32)self->list_mode_0x0C <= 2U) {
        base = lbl_805BA660 + self->list_mode_0x0C * 18;
        draw_font_idx(0x1E59U, (s8*)LbStr(0, 0x139U), 1, &anchor);
        i = 0;
        idp = &lbl_805B7EF4[0];
        for (; i < self->count_0x10; i++) {
            id = base[(u16)i * 3];
            if ((s16)self->selected_0x08 == i) {
                *(u16*)&pos.x = id;
                pos.y = 0;
                fn_802A7C04(0x1E35, (u16*)&pos);
                selected = 1;
            } else {
                selected = 0;
            }
            get_lsp_data(idp[0], &pos);
            pos.x += anchor.x;
            pos.y += anchor.y;
            fn_80216560(id, base[(u16)i * 3 + 1], (s16*)&pos, 1, selected, 1, 0x10);
            idp += 2;
        }
        return;
    }
    if (self->entries_0xD0[0].icon_kind_0x01 != 0) {
        draw_font_idx(0x1E59U, (s8*)LbStr(0, 0x13BU), 1, &anchor);
    } else {
        draw_font_idx(0x1E59U, (s8*)LbStr(0, 0x13AU), 1, &anchor);
    }
    entry = self->entries_0xD0;
    idp = &lbl_805B7EF4[0];
    for (i = 0; i < self->count_0x10; i++) {
        if ((s16)self->selected_0x08 == i) {
            fn_801E8CDC((u8)entry->kind_0x00, (s8*)entry->value_0x04);
            selected = 1;
        } else {
            selected = 0;
        }
        get_lsp_data(idp[0], &pos);
        pos.x += anchor.x;
        pos.y += anchor.y;
        {
            s32 color = GetMenuFontColor(1, selected != 0, 1, 0);

            fn_802DFACC(entry->kind_0x00);
            fn_80215C98(selected, (s16*)&pos, color, 0xC);
            sprintf((char*)buf, (char*)lbl_80791BC0, entry->id_0x02, LbStr(1, 0x2CU));
            fn_801E6850((s16*)&data, (s16*)get_lsp_data(0x1E62U, NULL));
            draw_font((const _SPR_DATA_&)data, (s8*)buf, 2, &pos);
        }
        entry++;
        idp += 2;
    }
}
u16 fn_801E936C(u16 a, u16 b)
{
    u16 v = a + b;

    if (v > 0x63U) {
        v = 0x63;
    }
    return v;
}

s32 fn_801E9600(u8* list, u8 count, u8 id)
{
    u8 i = 0;
    u8 n = count;

    if (count > 0U) {
        do {
            if (id == list[i]) {
                return 1;
            }
            i++;
            n--;
        } while (n != 0);
    }
    return 0;
}

void fn_801EC804(s16* value)
{
    s16 next = *value + 1;

    *value = next;
    if (next >= 0x28) {
        *value = 0;
    }
}

s32 fn_801EC7E4(void)
{
    return lobby_w.menu_0xAC->active_0x04 != 0;
}

void fn_801EC7AC(void)
{
    if (game_ready_ck() == 1U) {
        lobby_w.param_0x12D = 1;
    }
}

void fn_801EC828(u8* self)
{
    self[3] = 1;
    fn_802BBAC4(1);
    sysSE_req(1);
}

void fn_801EC194(void)
{
    fn_80359B00(0);
}

void fn_801E9FC8(void)
{
    fn_801E9AA8(1U);
    fn_80217934();
}

void fn_801EB524(void)
{
    fn_801EB464();
    fn_8035A7D8(0x1718, lbl_805B83F4, lbl_80791BE8, 0x16D6, 0);
}

void fn_801EAC30(LbParam* dst, const LbParam* src)
{
    dst->kind_0x00 = src->kind_0x00;
    dst->type_0x01 = src->type_0x01;
    dst->sub_0x02 = src->sub_0x02;
    dst->flag_0x03 = src->flag_0x03;
    dst->level_0x04 = src->level_0x04;
    dst->count_0x05 = src->count_0x05;
    dst->id_0x06 = src->id_0x06;
    dst->value_0x08 = src->value_0x08;
    dst->rate_0x0A = src->rate_0x0A;
    dst->mode_0x0B = src->mode_0x0B;
}

void fn_801EB0B8(void)
{
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(0x16BEU, &pos);
        draw_sprite_ary((const u16*)lbl_805B8354, &pos);
    } else {
        get_lsp_data(0x16B9U, &pos);
        draw_sprite_ary((const u16*)lbl_805B8348, &pos);
    }
    get_lsp_data(0x16A8U, &pos);
    draw_sprite_ary((const u16*)lbl_805B8360, &pos);
}

void fn_801EB6F4(void)
{
    _mh_ivec2_ pos;

    get_lsp_data(0x171BU, &pos);
    draw_sprite_ary((const u16*)lbl_805B84B0, &pos);
    fn_801EB744(0x1749U, &pos);
    fn_801EB744(0x174AU, &pos);
}

void fn_801EB744(u16 id, _mh_ivec2_* off)
{
    _mh_ivec2_ pos;

    get_lsp_data(id, &pos);
    pos.x += off->x;
    pos.y += off->y;
    draw_sprite_ary((const u16*)lbl_80791BF0, &pos);
}

void fn_801EB138(s32 idx, _mh_ivec2_* out)
{
    _mh_ivec2_ pos;

    get_lsp_data(lbl_805B8384[idx % 10], &pos);
    out->x = pos.x + 4;
    out->y = pos.y + 3;
}

void fn_801E9318(LbPartRec* dst, const LbPartRec* src)
{
    dst->kind_0x00 = src->kind_0x00;
    dst->flag_0x01 = src->flag_0x01;
    dst->sub_0x02 = src->sub_0x02;
    dst->mode_0x03 = src->mode_0x03;
    dst->level_0x04 = src->level_0x04;
    dst->count_0x05 = src->count_0x05;
    dst->id_0x06 = src->id_0x06;
    dst->value_0x08 = src->value_0x08;
    dst->rate_0x0A = src->rate_0x0A;
    dst->data_0x0C = src->data_0x0C;
}

void fn_801E92B0(void)
{
    LbPartRec* dst;
    const LbPartRec* src;
    s32 i;

    dst = (LbPartRec*)(lbl_80794880 + 0x3F98);
    src = (const LbPartRec*)lbl_805B7F28;
    for (i = 0; i < 0xD; i++) {
        fn_801E9318(dst, src);
        dst++;
        src++;
    }
}

void fn_801E9AA8(u8 arg0)
{
    LbPartSlot* slot;

    slot = (LbPartSlot*)(lbl_80794880 + 0x3F38);
    if (slot[0].kind_0x00 != 2) {
        lobby_w.slots_0x07D[0] = 0U;
    } else if (lobby_w.slots_0x07D[0] == 0 && arg0 != 0) {
        lobby_w.slots_0x07D[0] = 1U;
    }
    if (slot[1].kind_0x00 != 2) {
        lobby_w.slots_0x07D[1] = 0U;
    } else if (lobby_w.slots_0x07D[1] == 0 && arg0 != 0) {
        lobby_w.slots_0x07D[1] = 1U;
    }
    if (slot[2].kind_0x00 != 2) {
        lobby_w.slots_0x07D[2] = 0U;
        return;
    }
    if (lobby_w.slots_0x07D[2] == 0 && arg0 != 0) {
        lobby_w.slots_0x07D[2] = 1U;
    }
}

} /* extern "C" */
