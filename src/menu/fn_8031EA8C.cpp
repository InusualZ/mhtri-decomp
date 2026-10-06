/*
 * menu/fn_8031EA8C.cpp - the continuation of the item/equipment selection screen: the head (0x8031EA8C-0x8031FBE8) is
 *   the two four-state updater families that drive the selected entry's effect instance (`_EFT`, dispatched on
 *   `state_0x05`, each with its own `+0x38` work record); the tail (from 0x803203A0) is the screen's label/icon draw
 *   passes and the keyboard name/message edit screen (a 4-character name at +0x43 and a 0x90-character one at +0x48,
 *   edited through `_kbd_open_`/`_kbd_move` and copied back by `fn_80321A14`).  C++ callees (`res_eft_model_create`,
 *   `setVector3`, `rotVecZXY`, `ran_suu`), `extern "C"` for the plain stems.
 * RANGE. .text 0x8031EA8C-0x80324F7C (67 functions); extab, extabindex, .data 0x805DD598-0x805DD9C8, .bss
 *   0x806BE120-0x806BE2C8, .sdata 0x80792C50-0x80792CC8, .sdata2 0x8079AEA8-0x8079AF48.
 * FLAGS. `cflags_menu` (configure.py); `infer.py` on the target: `fn_80324CC4` keeps 0 record forms with 2 fold-shaped
 *   pairs (the peephole off).
 * NAMES. Module `menu` from the menu/HUD 2D callees and the `menu` band below; no `__FILE__` string covers the range
 *   (its `.data` reads are mask/sprite tables, `jumptable_805DD598` and pool floats) and the dump answers `zz_`, so the
 *   file keeps the map's stem.
 * RESIDUALS. 38 rows unwritten (objdiff scores them zero), including the two state-machine updaters `fn_8031ECF0`/
 *   `fn_8031EFEC`: `eft045_set` (0x8031EB54), 0x8031ECF0-0x8031F2F0, `fn_8031F510`, `fn_8031F7DC`,
 *   0x8031FBE8-0x80320D20, 0x80320D24-0x803210B8, `fn_80321130`, 0x8032145C-0x8032194C, 0x80321A14-0x80322C68,
 *   0x80322CEC-0x80323318, 0x80323428-0x80323874, `fn_80323884`, 0x80323A24-0x80323C4C, 0x80323CD4-0x80323F08,
 *   `fn_80323F0C`, 0x80324274-0x80324F7C.  The 29 written rows are byte-identical.
 *   flipcheck: `.bss` (0x1A8), `.data` (0x430), `.sdata` (0x78) and `.sdata2` (0xA0) claimed but not emitted; short
 *   `.text` 0x894 of 0x64F0, extab 0x78 of 0x1A8, extabindex 0xB4 of 0x27C; the bytes of all three differ; the pools are
 *   partial (a low-confidence fold candidate with `enemy/em_action`).
 */

#include "ef/eft_state_flags_set.h" /* eft_state_flags_set (rule 2: the owner's header) */
#include "ef/eft_rot_vec_copy.h" /* eft_rot_vec_copy (rule 2: the owner's header) */
#include "types.h"
#include "menu/fn_8031EA8C.h"
#include "unsplit/lobby.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "ai/fn_802D44F4.h" /* ai_slots_clear (rule 2: its owner) */
#include "lobby/lb_npc.h" /* lb_party_state_reset (rule 2: its owner) */
#include "enemy/em_pop.h" /* quest_flag_2000000_ck (rule 2: its owner) */
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define eft_state_flags_set_c1 ((void (*)(_EFT*, u32, u32))eft_state_flags_set)

/* Callees other units own (`eft_res_slot_get` `ef/eft_res.cpp`, ...); their owners' headers do not declare them
 * yet. */
extern "C" {

void* eft_res_slot_get(s32 size);
void* eft_res_model_get(void);
void  eft_res_slot_release(_EFT* self);
void  fn_800F8A44(MHchar** models, s32 count);
f32   fn_800513F0(VEC3* v, f32 s);
u32   fn_8004D70C(u32 value);
s32   fn_800CEF18(u32 value);
s32   fn_80217934(void);
void  setCockpitTransferMode(u8 index, s32 value);
void  fn_802DFCD4(void);
void* memset(void* dst, int value, u32 size);
void  sysSE_stop(s32 value);
char* quest_result_field_text_get(MenuQuestWork* self, s32 index);
s32   fn_803223B0(MenuQuestWork* self, u8 value);
void  fn_80323204(MenuQuestWork* self, u16 a, s16 b);
u32   game_ready_ck(void);
void  fn_80321130(MenuQuestWork* self, s32 value, u8 mode);

/* This unit's own forward declarations (defined below). */
void fn_8031EADC(_EFT* self, u32 index);
void fn_8031ECF0(_EFT* self);
void fn_8031EFEC(_EFT* self);
void fn_8031F2F0(_EFT* self);
void fn_8031F300(_EFT* self);
void fn_8031F3E4(_EFT* self);
void fn_8031F420(_EFT* self);
void fn_8031F45C(_EFT* self);
void fn_8031F510(_EFT* self);
void fn_8031F768(_EFT* self);
void fn_8031F778(_EFT* self);
void fn_803234EC(MenuQuestWork* self, s32 value);

} /* extern "C" */

/* The mangled callees (real C++ signatures - rule 9). */
struct _CP_VECTOR;
void* res_eft_model_create(MHchar* model, u16 id, u32 arg);
s32 ran_suu(s32 range);
void setVector3(nw4r::math::VEC3* v, f32 x, f32 y, f32 z);
void rotVecZXY(nw4r::math::VEC3* v, _CP_VECTOR* rot);

/* The mangled screen callees (real C++ signatures - rule 9). */
struct _CP_VECTOR;
s32 ck_WideMode(void);
s32 sysSE_req(s32 value);

/* The screen's own sprite tables (`.data` 0x805DD744.., unsplit; declared, never defined). */
extern u16 lbl_805DD814[];
extern u16 lbl_805DD834[];
extern u16 lbl_805DD840[];
extern u16 lbl_805DD744[];
extern u16 lbl_805DD8B0[];

/* Pooled constants (`.sdata2` 0x8079AEA8.., this band's own run; declared, never defined - brief 5d). */
extern f32 lbl_8079AEA8; /* 0.0f  */
extern f32 lbl_8079AED8; /* 10.0f */

extern MenuLabelTable lbl_806BE120;

/* ---------------------------------------------------------------------------------------------- */
/* the effect family wrappers                                                                      */
/* ---------------------------------------------------------------------------------------------- */

/* 0x8031EA8C - reset the effect for `mode`: clear its model's y-scale, clear the timer and set the
 * two flag bytes, then show the `mode`-selected model. */
extern "C" void fn_8031EA8C(_EFT* self, u8 mode) {
    MenuFxWork* work = (MenuFxWork*)self->work_0x38;

    work->models[0]->scale_0x1C.y = lbl_8079AEA8;
    self->timer_0x0C = 0;
    self->flag_0x01 = 1;
    self->field_0x06 = 1;

    switch (mode) {
    case 0:
        fn_8031EADC(self, 1);
        break;
    case 1:
        fn_8031EADC(self, 2);
        break;
    }
}

/* 0x8031EADC - show model `index` and hide the other of the two model slots. */
extern "C" void fn_8031EADC(_EFT* self, u32 index) {
    MenuFxWork* work = (MenuFxWork*)self->work_0x38;
    s32 i;

    for (i = 1; i <= 2; i++) {
        if ((u32)i == index) {
            work->models[0]->setVisibility(i, true);
        } else {
            work->models[0]->setVisibility(i, false);
        }
    }
}

/* 0x8031EC78 - release the effect's model list. */
extern "C" void fn_8031EC78(_EFT* self) {
    MenuFxWork* work = (MenuFxWork*)self->work_0x38;

    fn_800F8A44(&work->models[0], work->count);
    work->count = 0;
}

/* 0x8031ECB4 - the first family's four-state dispatch on `state_0x05`. */
extern "C" void fn_8031ECB4(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        fn_8031ECF0(self);
        return;
    case 1:
        fn_8031EFEC(self);
        return;
    case 2:
        fn_8031F2F0(self);
        return;
    case 3:
        fn_8031F300(self);
        return;
    }
}

/* 0x8031F2F0 - advance the first family's state. */
extern "C" void fn_8031F2F0(_EFT* self) {
    self->state_0x05++;
}

/* 0x8031F300 - retire the first family's effect. */
extern "C" void fn_8031F300(_EFT* self) {
    eft_res_slot_release(self);
}

/* 0x8031F304 - spawn the second effect family over `arg`. */
extern "C" void fn_8031F304(void* arg) {
    _EFT* self = (_EFT*)eft_res_slot_get(0xC);
    MenuFxWork* work;
    s32 i;

    if (self == NULL) {
        return;
    }

    work = (MenuFxWork*)self->work_0x38;
    work->count = 1;
    ((MenuFxWorkB*)work)->value_0x08 = 0;
    self->type_0x02 = 0;
    self->release_0x40 = fn_8031F3E4;
    self->dispatch_0x34 = fn_8031F420;

    for (i = 0; i < work->count; i++) {
        work->models[i] = (MHchar*)eft_res_model_get();
        if (work->models[i] == NULL) {
            eft_res_slot_release(self);
            return;
        }
    }

    self->rot_0x24.x = 0;
    self->rot_0x24.y = 0;
    self->rot_0x24.z = 0;
    eft_state_flags_set_c1(self, 1, 0);
    self->field_0x03 = 0x2E;
    self->source_0x30 = arg;
}

/* 0x8031F3E4 - release the second family's model list. */
extern "C" void fn_8031F3E4(_EFT* self) {
    MenuFxWork* work = (MenuFxWork*)self->work_0x38;

    fn_800F8A44(&work->models[0], work->count);
    work->count = 0;
}

/* 0x8031F420 - the second family's four-state dispatch on `state_0x05`. */
extern "C" void fn_8031F420(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        fn_8031F45C(self);
        return;
    case 1:
        fn_8031F510(self);
        return;
    case 2:
        fn_8031F768(self);
        return;
    case 3:
        fn_8031F778(self);
        return;
    }
}

/* 0x8031F45C - the second family's state 0: create each model and seed its scale. */
extern "C" void fn_8031F45C(_EFT* self) {
    s32 i;
    MenuFxWork* work = (MenuFxWork*)self->work_0x38;

    self->state_0x05++;

    for (i = 0; i < work->count; i++) {
        if (res_eft_model_create(work->models[i], 0x3B, 0) == NULL) {
            fn_8031F778(self);
            return;
        }
        setVector3(&work->models[i]->scale_0x1C, lbl_8079AED8, lbl_8079AED8, lbl_8079AED8);
    }

    fn_8031F510(self);
}

/* 0x8031F768 - advance the second family's state. */
extern "C" void fn_8031F768(_EFT* self) {
    self->state_0x05++;
}

/* 0x8031F778 - retire the second family's effect. */
extern "C" void fn_8031F778(_EFT* self) {
    eft_res_slot_release(self);
}

/* 0x8031F77C - count the non-zero `u16` entries of a string table whose value `fn_8004D70C`
 * accepts. */
extern "C" s16 fn_8031F77C(u16* table) {
    u16* p = table;
    s16 count = 0;

    while (*p != 0) {
        if (fn_8004D70C(*p) == 1) {
            count++;
        }
        p++;
    }
    return count;
}

/* 0x8031FB84 - walk `n` links of the `lobby_w + 0x48` chain, returning the last value. */
extern "C" s32 fn_8031FB84(s16 n) {
    s32 value = ((MenuLobbyView*)&lobby_w)->field_0x48;
    s16 i = 0;

    while (i < n) {
        value = fn_800CEF18((u16)value);
        i++;
    }
    return value;
}

/* ---------------------------------------------------------------------------------------------- */
/* the screen's small helpers                                                                       */
/* ---------------------------------------------------------------------------------------------- */

extern "C" u16 fn_803210B8(u16 index, u16 value) {
    lbl_806BE120.labels_0x0EE[index] = value;
    return index + 1;
}

/* 0x803210DC - clear the sixteen label words of `lbl_806BE120` from +0xEE. */
extern "C" void fn_803210DC(void) {
    s32 i;

    for (i = 0; i < 17; i++) {
        lbl_806BE120.labels_0x0EE[i] = 0;
    }
}

/* 0x80320D20 - the screen's teardown tail. */
extern "C" void fn_80320D20(void) {
    fn_80217934();
}

/* 0x80323874 - hand `state_0x01 == 0` to `fn_803234EC`. */
extern "C" void fn_80323874(MenuQuestWork* self) {
    fn_803234EC(self, self->state_0x001 == 0 ? 1 : 0);
}

/* 0x80323F08 - tail call into the quest record's 0x2000000 flag test. */
extern "C" void fn_80323F08(QuestRecord* rec) {
    quest_flag_2000000_ck(rec);
}

/* 0x8032422C - reset the ten slots and run the screen's two post steps. */
extern "C" void fn_8032422C(void) {
    s32 i;

    for (i = 0; i < 10; i++) {
        setCockpitTransferMode((u8)i, 0);
    }
    fn_802DFCD4();
    ai_slots_clear();
}

/* 0x80324224 - the screen's stored word at +0x348. */
extern "C" u32 fn_80324224(MenuQuestWork* self) {
    return self->field_0x348;
}


/* 0x80321444 - reset the row counter and hand `value` to the list advance. */
extern "C" void fn_80321444(MenuQuestWork* self, u8 value) {
    self->value_0x022 = 0;
    fn_80321130(self, 0, value);
}

/* 0x8032194C - when the option block is live, play the confirm sound and run the two post steps. */
extern "C" void fn_8032194C(void) {
    if (lb_param_w.field_0x04 != 0) {
        sysSE_req(9);
    }
    lb_party_state_reset();
    sysSE_stop(1);
}

/* 0x80321990 - reset the edit screen for `mode`: seed the row count, mirror the label, clear both
 * editable buffers. */
extern "C" void fn_80321990(MenuQuestWork* self, u8 mode) {
    if (mode == 0) {
        self->field_0x166 = 4;
    } else {
        self->field_0x166 = 2;
    }
    self->row_0x042 = (s8)self->field_0x166;
    self->field_0x040 = self->field_0x036;
    self->field_0x164 = 0;
    memset(self->name_0x043, 0, 5);
    memset(self->text_0x048, 0, 0x91);
}

/* 0x80322C68 - draw the two label rows of one entry. */
extern "C" void fn_80322C68(MenuQuestWork* self) {
    _mh_ivec2_ pos;
    char* text;

    get_lsp_data(0x15F0, &pos);
    draw_sprite_ary(lbl_805DD744, &pos);
    text = quest_result_field_text_get(self, 7);
    draw_font_idx(0x15F8, (s8*)text, 1, &pos);
    text = quest_result_field_text_get(self, 8);
    draw_font_idx(0x15FA, (s8*)text, 0, &pos);
}

/* 0x80323318 - the entry draw pass: a per-state body then the shared frame row. */
extern "C" void fn_80323318(MenuQuestWork* self, u8 arg) {
    font_set_size(0x12, 0x12);
    switch (self->state_0x001) {
    case 0:
        fn_803223B0(self, arg);
        return;
    case 2:
        fn_80214EF0(0x1877, 0x15B);
        fn_80215170(0x1879, self->field_0x168);
        break;
    }
    fn_80323204(self, self->field_0x036, self->field_0x03C);
}

/* 0x803233A8 - the two-mode backdrop. */
extern "C" void fn_803233A8(void) {
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(0x1511, &pos);
        draw_sprite_ary(lbl_805DD840, &pos);
    } else {
        get_lsp_data(0x150C, &pos);
        draw_sprite_ary(lbl_805DD834, &pos);
    }
    get_lsp_data(0x14FD, &pos);
    draw_sprite_ary(lbl_805DD814, &pos);
}

/* 0x803239A4 - the same backdrop for the second screen's frame. */
extern "C" void fn_803239A4(void) {
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(0x1511, &pos);
        draw_sprite_ary(lbl_805DD840, &pos);
    } else {
        get_lsp_data(0x150C, &pos);
        draw_sprite_ary(lbl_805DD834, &pos);
    }
    get_lsp_data(0x27FA, &pos);
    draw_sprite_ary(lbl_805DD8B0, &pos);
}

/* 0x80323C4C - whether the id names a displayable entry in the current mode. */
extern "C" s32 fn_80323C4C(s32 id) {
    if ((u32)((u16)id - 0x3E80) <= 1) {
        return 0;
    }
    if (game_ready_ck() == 1) {
        if ((u16)id >= 0x2710) {
            if ((u16)id < 0xEA60) {
                return 1;
            }
        }
    } else {
        if ((u16)id >= 0x3E8) {
            if ((u16)id < 0x2328) {
                return 1;
            }
        }
    }
    return 0;
}
