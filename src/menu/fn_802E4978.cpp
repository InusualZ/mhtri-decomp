/*
 * menu/fn_802E4978.cpp - the 0x802E4978-0x802E7408 cockpit/HUD band (29 functions, 10896 B).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * tools/symbols/symedit.py find/at + the Dolphin dump map at D:/WiiExperiment/DumpSymbols.zip: every
 * defined name in the range is a bare `fn_`/`zz_` entry, and the only `__FILE__` strings in the
 * neighbourhood are `cockpit.cpp` (emitted at 0x802DBC48, below this range) and `cockpit_quest.cpp`
 * (emitted at 0x802E74E0, above it), neither of which this range references).
 *
 * Home and type, from evidence: the band's entry points are the menu/UI library's - it calls
 * `get_menu_lsp_tbl`, `put_menu_cursor`, `GetMenuFontColor`, `ItemName`, `GetItemData` and the
 * `draw_sprite`/`draw_font`/`drawshape_*` family - and the registered sibling `menu/fn_802A6624.cpp`
 * owns `GetMenuFontColor`, so the module is `menu`.  The file keeps the map's stem (brief section 2,
 * class 4: nothing in the object names the original source file).  Its own data is the two
 * 0x194-byte per-player cockpit work records at `lbl_806BDCC8` (2 entries) and the sub-screen state
 * at `lbl_806BDFF0`.
 *
 * Sections: .text 0x802E4978..0x802E7408, extab 0x80014FFC..0x800150C4 and extabindex
 * 0x80033660..0x8003378C (dtk derived both from the range's own prologues).  The range's `.data`
 * jump table `jumptable_805D5B48` (0x4C = 19 entries, the `fn_802E4C5C` switch) is declared
 * `extern` and used, never defined (playbook 29); claiming it as a range is a config_request.
 *
 * Flags: `cflags_menu` (the registered sibling's lib flags), -O3 -inline noauto with the peephole
 * pass off.
 *
 * Work: 23 of the 29 functions are at or above the 80 % bar (7 byte-identical).  Residuals, by
 * function, measured with the report metric (`tools/units/recompile.py menu/fn_802E4978 --measure
 * <symbol>`, target = the split object of the range):
 *   * NOT RECONSTRUCTED (empty body, 0 %): `fn_802E4C5C` (the 0x5C4 switch, needs its `.data` jump
 *     table `jumptable_805D5B48` claimed - playbook 53/56); `fn_802E5764`, `fn_802E5E90`,
 *     `fn_802E646C`, `fn_802E6AAC` (VMX/paired-single bodies, `m2c` flags them);
 *     `fn_802E6EB4` is partial (39.8 %) - the `drawshape` colour/rect tail does not reproduce.
 *   * `fn_802E4AD4` 80.3 %, `fn_802E555C` 81.3 %, `fn_802E5220` 87.2 %, `fn_802E5284` 88.9 %,
 *     `fn_802E53C4` 89.3 %, `fn_802E4978` 93.3 %, `fn_802E5A14` 95.6 %, `fn_802E5D68` 95.7 %,
 *     `fn_802E5C3C` 95.8 % - register-allocation/argument-width residuals (first divergence is an
 *     ARG row, the instruction set is equal; several already sit at the 100 %-minus-a-few-points
 *     level from the `{lval}` casts the compiler had to insert).
 *   * a 0-byte-identical tail: none of the residuals is a flag problem - all 23 matched functions
 *     are compiled with the sibling `menu` lib's own `cflags_menu` command line.
 *
 * Rule 2 (an extern lives with the TU that owns it).  `stylelint --diff main` reports 0 sites: the 54
 * declarations this band used to carry in `include/unsplit/menu.h` are re-homed - 16 to their owners'
 * headers, which this file now includes (`fn_8004CAD8.h`, `ef/fn_800CDB2C.h`,
 * `Runtime.PPCEABI.H/memset.h`, `Pl/pl_act.h`, `menu/menu_item.h`, the new `ai/fn_802D44F4.h` and
 * `enemy/fn_80382310.h`), 14 dropped (nothing in this file referenced them) and 24 left as this
 * unit's own view in `include/menu/fn_802E4978.h` because the owner's header cannot be included from
 * here (the clash that blocks each is named there; every one is a `shared-file` config_request).  The
 * band header now declares only symbols no registered unit owns.
 *
 * `.data` request: the range's own jump table is `jumptable_805D5B48` (0x4C = 19 entries, the
 * `fn_802E4C5C` switch); it is declared `extern` here and never defined (playbook 29), and a `range`
 * config_request asks for 0x805D5B48..0x805D5B94 so the switch can be read from `main.elf`.
 * Functions reconstructed in address order.
 */

#include "types.h"
#include "menu/fn_802E4978.h"
#include "menu/menu_item.h"
#include "menu/menu_message.h"
#include "ef/fn_800CDB2C.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "ai/fn_802D44F4.h"
#include "enemy/fn_80382310.h"
#include "unsplit/lobby.h"
#include "unsplit/Pl.h"
#include "unsplit/menu.h"
#include "enemy/em_pop.h"
#include "Pl/pl_act.h"
#include "Pl/pl_master.h"
#include "fn_80047398.h"

/* The two player records and the sub-screen state the band drives.  The map spells them `lbl_*`;
 * this file defines them (they are this band's own .bss, 0x806BDCC8-0x806BE078). */
CockpitWork lbl_806BDCC8[2];
CockpitState lbl_806BDFF0;

extern "C" {

/* ---- pooled constants and tables: declared, never defined (playbook 29) ---- */
extern f32 lbl_8079A8E0;
extern f32 lbl_8079A8E4;
extern f32 lbl_8079A8F0;
extern f32 lbl_8079A8F4;
extern f32 lbl_8079A8F8;
extern f32 lbl_8079A8FC;
extern f32 lbl_8079A900;
extern f32 lbl_8079A904;
extern f32 lbl_8079A908;
extern f32 lbl_8079A90C;
extern f32 lbl_8079A918;
extern f32 lbl_8079A91C;
extern f32 lbl_8079A920;
extern f32 lbl_8079A924;
extern u16 lbl_80792030[];
extern u16 lbl_80792038[];
extern u16 lbl_805BFFE0[];
extern u32 lbl_805CDE78[];
extern u16 lbl_805D5B98[];
extern u8 lbl_805D5BD8[];
extern u8 lbl_805D5C68[];
extern u16 lbl_805E707C[];
extern u16 lbl_805E70C8[];
extern u16 lbl_805E70D4[];
extern u16 lbl_805E70E8[];
extern u32* jumptable_805D5B48[];
extern char lbl_807927C0[];
extern char lbl_807927C4[];

int sprintf(char*, const char*, ...);
u32 fn_800AB658(u8, f32);
void fn_802E4978(CockpitWork*, CockpitMove*);
void fn_802E4AD4(void);
void fn_802E4B8C(void);
void fn_802E4C24(void);
void fn_802E5220(CockpitWork*);
void fn_802E5284(void);
void fn_802E53C4(void);
void fn_802E5400(void);
void fn_802E5414(u8);
u32 fn_802E54A8(CockpitMove*);
u32 fn_802E54F8(CockpitMove*, u8);
void fn_802E555C(void);
void fn_802E56B4(void);
void fn_802E5764(void*, u8);
void fn_802E5A14(void);
void fn_802E5BB4(void);
void fn_802E5C3C(_mh_ivec2_*, s8);
void fn_802E5CFC(s8);
void fn_802E5D68(s16, u8);
void fn_802E5E90(void);
void fn_802E646C(void);
void fn_802E6AAC(void);
void fn_802E6EB4(void);
void fn_802E704C(u16, s32, _mh_ivec2_*);
void fn_802E70F0(s32, _mh_ivec2_*);
void fn_802E71C4(void);
u32 fn_802E73B4(void);
s32 quest_bar_id_keep(s32, s32);
void fn_802E4C5C(CockpitMove*, CockpitText*);

}

void fn_802E4978(CockpitWork* self, CockpitMove* move) {
    s32 r;
    s32 i;

    memset(self, 0, 0x194);
    self->move = move;
    self->field_0x048 = (MenuWorkView*)fn_802D27E0();
    self->field_0x0C6 = -1;
    self->field_0x0C4 = -1;
    self->field_0x0C2 = -1;
    self->field_0x0C0 = -1;
    self->field_0x0BE = -1;
    self->field_0x0CE = 0;
    self->field_0x0CF = 0;
    self->field_0x134 = 0;
    self->field_0x136 = 0;
    self->field_0x138 = 0;
    self->field_0x137 = 0;
    self->field_0x13A = 0;
    self->field_0x13C = 0;
    self->field_0x139 = 0;
    self->text.field_0x014 = 0;
    self->text.field_0x00C = 0;
    fn_802EDAF0(move, &self->field_0x16B, &self->field_0x16F);
    r = fn_802EC6C4(move);
    self->field_0x16A = (s8)(((s32)(-r | r) >> 31) & 5);
    self->field_0x168 = move->field_0x36C;
    self->field_0x164 = 0xFF;
    self->field_0x166 = 0;
    self->field_0x167 = 0;
    self->field_0x176 = 0;
    self->field_0x17C = 0;
    self->field_0x17B = 0;
    self->field_0x178 = 0;
    self->field_0x17D = 0;
    self->field_0x17E = 0;
    self->field_0x019 = 0xFF;
    self->field_0x021 = 0xFF;
    self->field_0x01B = 0xFF;
    self->field_0x023 = 0xFF;
    self->field_0x01D = 0xFF;
    self->field_0x025 = 0xFF;
    self->field_0x01F = 0xFF;
    self->field_0x027 = 0xFF;
    self->field_0x029 = 0xFF;
    self->field_0x02B = 0xFF;
    self->byte_0x180 = 0;
    self->field_0x17F = 0;
    self->field_0x18C = 0;
    self->field_0x18E = 0;
    self->field_0x184 = 0;
    self->field_0x188 = 0;
    for (i = 0; i < 10; i++) {
        fn_802E0468((u8)i, 0);
    }
}

void fn_802E4AD4(void) {
    CockpitMove* move;
    CockpitState* st;
    s8 idx;

    st = &lbl_806BDFF0;
    move = (CockpitMove*)get_move_work_adrs(2);
    st->field_0x000 = 1;
    st->field_0x059 = 0;
    st->field_0x058 = 0;
    if (move_work_state_ck() != 0) {
        st->field_0x05C = 0;
        st->field_0x05A = 0;
        st->field_0x070 = em_set_work_state_get();
    } else {
        st->field_0x05C = 0;
        st->field_0x05A = 0;
        st->field_0x070 = fn_803A9690();
    }
    st->field_0x074 = 0;
    idx = (s8)my_player_no();
    fn_802E4978(&lbl_806BDCC8[0], &move[idx]);
    fn_802DFCD4();
    fn_802DA1B0(NULL);
}

void fn_802E4B8C(void) {
    CockpitMove* move;

    move = (CockpitMove*)get_move_work_adrs(2);
    lbl_806BDFF0.field_0x000 = 2;
    lbl_806BDFF0.field_0x059 = 0;
    lbl_806BDFF0.field_0x058 = 0;
    fn_802E4978(&lbl_806BDCC8[0], &move[0]);
    fn_802E4978(&lbl_806BDCC8[1], &move[1]);
    lbl_806BDFF0.field_0x078 = 0;
    lbl_806BDFF0.field_0x002 = 100;
    lbl_806BDFF0.field_0x004 = 0;
    fn_802DA1B0(NULL);
}

void fn_802E4C24(void) {
    lbl_806BDFF0.field_0x078++;
    lbl_806BDCC8[0].field_0x190++;
    lbl_806BDCC8[1].field_0x190++;
}

void fn_802E5220(CockpitWork* self) {
    switch (self->field_0x048->field_0x3C4) {
    case 0:
        self->field_0x04C = 0;
        self->field_0x050 = 0;
        break;
    case 1:
        self->field_0x04C = 0x22;
        self->field_0x050 = 0x00FF00FF;
        break;
    case 2:
        self->field_0x04C = 2;
        self->field_0x050 = 0xFF0000FF;
        break;
    }
}

void fn_802E53C4(void) {
    fn_80047058();
    fn_802DA344();
    subTransSetPrio(6, (u32)&fn_802DA3CC, 0, NULL);
}

void fn_802E5400(void) {
    lbl_806BDCC8[0].field_0x0CF = 1;
    fn_802DA1B0(&lbl_806BDCC8[0]);
}

void fn_802E5414(u8 arg0) {
    _mh_ivec2_ pos;
    u16 lsp;
    s32 idx;

    switch (arg0) {
    case 4:
        lsp = 0xEC8;
        idx = 0;
        break;
    case 6:
        lsp = 0xEE5;
        idx = 1;
        break;
    case 7:
        lsp = 0xEED;
        idx = 2;
        break;
    default:
        return;
    }
    get_lsp_data(lsp, &pos);
    fn_802DA454(2, idx, pos.x, pos.y, 1, 1, 0);
}

u32 fn_802E54A8(CockpitMove* move) {
    u32 r;

    r = get_cfg((u8)move->field_0x008, 0xD) == 0;
    if (move->field_0x308 == 1) {
        r = 1;
    }
    return r;
}

u32 fn_802E54F8(CockpitMove* move, u8 id) {
    u32 r;

    if (id == 0xFF) {
        return fn_802E54A8(move);
    }
    r = get_arena_cfg(id, 0xD) == 0;
    if (move->field_0x308 == 1) {
        r = 1;
    }
    return r;
}

void fn_802E56B4(void) {
    CockpitMove* move;
    u16 max;
    s32 i;

    if (game_ready_ck() != 0) {
        move = (CockpitMove*)get_move_work_adrs(2);
        max = get_move_work_max(2);
        for (i = 0; i < (s32)max; i++) {
            if (move[i].field_0x000 != 0 && Pl_master_ck((struct _PLW*)&move[i]) == 0) {
                draw_lsp_element(&move[i], 0xFF, 0xFF, 0xFFFF);
            }
        }
    }
}

void fn_802E5BB4(void) {
    CockpitWork* self;
    CockpitMove* move;

    self = &lbl_806BDCC8[0];
    move = self->move;
    if (self->field_0x17E == 1) {
        fn_8027B918((struct _PLW*)move);
    }
    if (move->field_0x5BD == 0xFF) {
        if (menu_item_frame_update((MenuFrameWork*)move) == 0) {
            self->field_0x17E = 1;
            return;
        }
        self->field_0x17E = 0;
        return;
    }
    self->field_0x17E = 0;
}

void fn_802E5C3C(_mh_ivec2_* pos, s8 idx) {
    _mh_ivec2_* src;

    get_lsp_data(0xE15, pos);
    src = (_mh_ivec2_*)get_lsp_data(lbl_805E70C8[idx >> 3], NULL);
    pos->x += src->x;
    pos->y = (s16)(pos->y + src->y);
    src = (_mh_ivec2_*)get_lsp_data(lbl_805E70D4[idx & 7], NULL);
    pos->x += src->x;
    pos->y = (s16)(pos->y + src->y);
}

void fn_802E5CFC(s8 idx) {
    _mh_ivec2_ pos;
    CockpitWork* self;

    self = &lbl_806BDCC8[0];
    fn_802E5C3C(&pos, idx);
    if (fn_802DA454(0, 6, pos.x, pos.y, 1, 1, 0) != 0) {
        fn_802DA1A8(&self->field_0x17A);
    }
}

u32 fn_802E54A8(CockpitMove* move);

void fn_802E555C(void) {
    CockpitMove0* m0;
    CockpitMove* move;

    m0 = (CockpitMove0*)get_move_work_adrs(0);
    move = lbl_806BDCC8[0].move;
    if (get_now_areano() != 0xFF) {
        set_zmode(0, 0, 0);
        lbl_806BDCC8[0].field_0x0CD = 0;
        fn_802E56B4();
        draw_lsp_parts();
        fn_802EC700();
        if (fn_802E54A8(move) == 1) {
            fn_802E71C4();
            fn_802E7FA0();
            fn_802ED480();
        }
        fn_802EE82C(&lbl_806BDCC8[0]);
        if (m0->field_0x0EF != 0) {
            fn_802EF0FC(&lbl_806BDCC8[0]);
        }
        if (menu_item_frame_update((MenuFrameWork*)move) == 0) {
            if (move->field_0x5BC != 0) {
            } else if (move->field_0x5BE != 0) {
            } else if (move->field_0x5BD != 0) {
                quest_marker_arm();
                if (fn_802E54A8(move) == 1) {
                    fn_802EE65C(move, &lbl_806BDCC8[0].field_0x16F);
                    fn_802EDCE4(move, &lbl_806BDCC8[0].field_0x16B);
                }
            }
        }
        fn_802EA33C(&lbl_806BDCC8[0]);
        fn_802E6EB4();
        note_box_draw();
        if (move_work_state_ck() != 0) {
            fn_802EF424();
        }
        fn_802EF730();
    }
}

void fn_802E5A14(void) {
    CockpitWork* self;
    CockpitMove* move;
    u8 v;

    self = &lbl_806BDCC8[0];
    move = self->move;
    if (self->field_0x176 != 0) {
        fn_8027B0BC((struct _PLW*)move);
    }
    if (self->field_0x17D == 1) {
        fn_8027B358((struct _PLW*)move);
    }
    if (move->field_0x5BC != 0) {
        if (menu_item_frame_update((MenuFrameWork*)move) == 0) {
            self->field_0x176 = 1;
        } else {
            self->field_0x176 = 0;
        }
        self->field_0x17B++;
        if (fn_802E0B54(0xE53) < (s32)self->field_0x17B) {
            self->field_0x17B = 0;
        }
        if (self->field_0x17C != 0) {
            self->field_0x17C++;
            if (fn_802E0B54(0xE33) < (s32)self->field_0x17C) {
                self->field_0x17C = 0;
            }
        }
        if (self->field_0x178 != 0) {
            v = self->field_0x179 - 1;
            self->field_0x179 = v;
            if (v == 0) {
                self->field_0x178 = 0;
                self->field_0x17C = 1;
            }
        }
        if (menu_item_frame_update((MenuFrameWork*)move) == 0) {
            self->field_0x17A = 0;
        } else {
            self->field_0x17A = 1;
        }
    } else {
        self->field_0x176 = 0;
        self->field_0x17A = 1;
        self->field_0x178 = 0;
        self->field_0x17C = 0;
    }
    if (move->field_0x5BE != 0) {
        if (menu_item_frame_update((MenuFrameWork*)move) == 0) {
            self->field_0x17D = 1;
            return;
        }
        self->field_0x17D = 0;
        return;
    }
    self->field_0x17D = 0;
}

u32 fn_802E73B4(void) {
    return move_work_state_ck();
}

s32 quest_bar_id_keep(s32 arg0, s32 arg1) {
    s32 r;
    s32 mask;
    u16 v;

    r = Pl_dm_condition_ck((struct _PLW*)arg1, 0x100000);
    mask = (s32)~((r - 1) | (1 - r)) >> 31;
    v = (u16)(arg0 + 0x3C0);
    return mask & v;
}

void fn_802E5284(void) {
    CockpitMove0* m0;
    CockpitMove* move;

    m0 = (CockpitMove0*)get_move_work_adrs(0);
    move = lbl_806BDCC8[0].move;
    if (fn_80046F0C(&lbl_806BDCC8[0]) != 1) {
        lbl_806BDCC8[0].field_0x0D1 = 0;
        fn_802E4C24();
        fn_803839EC();
        fn_802DFD38();
        fn_802E796C(&lbl_806BDCC8[0], 0xFF);
        fn_802E4C5C(move, &lbl_806BDCC8[0].text);
        if (m0->field_0x0EF != 0) {
            fn_802E5220(&lbl_806BDCC8[0]);
        }
        fn_802EDB0C(move, &lbl_806BDCC8[0].field_0x16B, &lbl_806BDCC8[0].field_0x16F);
        quest_targets_update_b(&lbl_806BDCC8[0]);
        fn_802EC4F0(&lbl_806BDCC8[0]);
        fn_802E5A14();
        fn_802E5BB4();
        fn_802ED88C();
        fn_802A2620(0);
        if (move_work_state_ck() != 0) {
            fn_802EF230();
        }
        fn_802EF6B0();
        fn_802DA344();
    }
    if (event_demo_ck() != 1) {
        subTransSet((u32)(void*)fn_802E555C, 0, NULL);
        if (lb_quest_work_active_ck() == 0) {
            subTransSetPrio(6, (u32)(void*)&fn_802DA3CC, 0, NULL);
        }
        subTransSetPrio(6, (u32)(void*)&menu_slot_panel_draw, 0, NULL);
    }
}

struct SprView {
    /* +0x000 */ u8 unused_0x000[0x010];
    /* +0x010 */ s16 field_0x010;
    /* +0x012 */ u8 unused_0x012[0x020 - 0x012];
}; /* size: 0x20 */

void fn_802E704C(u16 arg0, s32 arg1, _mh_ivec2_* pos) {
    SprView spr;
    f32 f;

    fn_801E6850((s16*)&spr, get_lsp_data(arg0, NULL));
    f = (f32)(arg1 / 5);
    spr.field_0x010 = (s16)(lbl_8079A918 * f);
    draw_sprite(*(_SPR_DATA_*)&spr, (_mh_ivec2_*)pos);
}

void fn_802E70F0(s32 arg0, _mh_ivec2_* pos) {
    s32 n;
    u8 tex;

    n = arg0;
    if (arg0 > 0x37) {
        return;
    }
    if (n < 0) {
        n = 0;
    }
    tex = lbl_805D5C68[n / 5];
    drawshape_init(3, (u16)(((0x4A - n) / 15) * 4));
    drawshape_set_vertex_array((_mh_ivec2_*)lbl_805D5B98);
    drawshape_set_flat_color(0xFFFFFFFF);
    drawshape_set_texture_array(0xD, (_mh_tex_uv_*)(lbl_805D5BD8 + tex * 4), 0);
    fn_80054178((s16*)pos);
    drawshape_exec();
}

void fn_802E6EB4(void) {
    CockpitMove* move;
    _mh_ivec2_ pos;
    _mh_ivec2_* p;

    move = lbl_806BDCC8[0].move;
    if (move->field_0x00A == 7 && move->field_0x3A4 != 0) {
        set_blendmode(4, 5, 1);
        get_lsp_data(0xBCE, &pos);
        draw_sprite_anim_ary(lbl_805E70E8, lbl_806BDCC8[0].half_0x180, &pos);
        p = (_mh_ivec2_*)get_lsp_data(0xBD2, NULL);
        {
            u8 lo = move->field_0x3A5;
            u8 hi = move->field_0x3A4;
            f32 f2 = (f32)p->x * (f32)hi;
            s32 col = color_lerp(0xC50A83FF, 0xD813BEFF, lo, (f32)hi * (lbl_8079A8FC / (f32)lo), f2);
            drawshape_init(3, 0xFFFF);
            drawshape_set_vertex_rect((s16)(p->x + pos.x), (s16)(p->y + pos.y), (s16)(f2 / (f32)lo), p->y);
            fn_80053960(0xC50A83FF, col, col, 0xC50A83FF);
            drawshape_exec();
        }
    }
}

/* ---- not yet reconstructed (residual) ---- */
void fn_802E4C5C(CockpitMove* self, CockpitText* text) {
}

void fn_802E5764(void* arg0, u8 arg1) {
}

void fn_802E5D68(s16 arg0, u8 arg1) {
    CockpitWork* self;
    _mh_ivec2_ pos;
    _mh_ivec2_ anchor;
    _mh_ivec2_* p;
    s16 dx;
    s16 dy;
    s32 sq;
    s8 v;

    self = &lbl_806BDCC8[0];
    self->field_0x174 = arg0;
    self->field_0x177 = arg1;
    fn_802E5C3C(&pos, (s8)arg1);
    p = (_mh_ivec2_*)get_lsp_data(0xE56, NULL);
    pos.x += p->x;
    pos.y += p->y;
    get_lsp_data(0xE15, &anchor);
    p = (_mh_ivec2_*)get_lsp_data(0xE33, NULL);
    pos.x -= (s16)(p->x + anchor.x);
    pos.y -= (s16)(p->y + anchor.y);
    dx = pos.x;
    dy = pos.y;
    sq = dx * dx;
    v = (s8)(lbl_8079A90C * fn_80050BC0(sq, dx, (f32)(sq + dy * dy)));
    self->field_0x178 = v;
    if ((u8)v == 0) {
        self->field_0x178 = 1;
    }
    self->field_0x179 = self->field_0x178;
}

void fn_802E5E90(void) {
}

void fn_802E646C(void) {
}

void fn_802E6AAC(void) {
}

/* `Screen_w` and its `ScreenGeomView` view come from `include/unsplit/menu.h` (rule 1: this unit
 * was the first user, `menu/arena_result.cpp` the second). */

struct SprDataView {
    /* +0x000 */ u8 unused_0x000[0x1C];
    /* +0x01C */ u32 colour_0x01C;
}; /* size: 0x20 */

void fn_802E71C4(void) {
    _mh_ivec2_ pos;
    SprDataView spr;
    s32 colour;
    s32 limit;
    s32 frame;
    s32 value;
    s32 total;
    s32 bars;

    set_blendmode(4, 5, 1);
    get_lsp_data(0xB90, &pos);
    colour = 0x6DAAA3FF;
    if (fn_802E73B4() != 0) {
        frame = 0;
        bars = 0;
    } else {
        f32 f;
        limit = (s32)(lbl_8079A91C * Screen_w.field_0x14);
        if (fn_803AA41C(1, lbl_8079A91C) == 0) {
            total = fn_803A87E0();
        } else {
            total = fn_803A881C();
        }
        if (total <= limit * 5) {
            if (total > limit) {
                f = (f32)((lbl_806BDCC8[0].field_0x190 & 0x1F) << 0xB);
            } else {
                f = (f32)((lbl_806BDCC8[0].field_0x190 & 0xF) << 0xC);
            }
            colour = color_lerp(0xF50C23FF, 0xFF8C9BFF, (u8)fn_800AB658(0, lbl_8079A920 * f),
                                 lbl_8079A8FC + 0.0f, 0.0f);
        }
        value = fn_803A8858();
        frame = (value - total) / limit;
        bars = value / limit;
    }
    fn_801E6850((s16*)&spr, get_lsp_data(0xB91, NULL));
    spr.colour_0x01C = colour;
    draw_sprite(*(_SPR_DATA_*)&spr, &pos);
    draw_sprite_idx(0xB92, &pos);
    if (bars != 0) {
        fn_802E70F0(bars, &pos);
        fn_802E704C(0xB93, bars, &pos);
        fn_802E704C(0xB94, frame, &pos);
        return;
    }
    fn_802E70F0(0, &pos);
    draw_sprite_idx(0xB95, &pos);
}
