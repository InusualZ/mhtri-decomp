/*
 * menu/charmake.cpp - the character make (`charmake_init`, `charmake_move`, `get_charmake_param`,
 *   `set_charmake_r_no`) and the demo equipment (`set_demo_equip_data`, `demoEquip`).
 * RANGE. .text 0x80443554-0x804459DC; extab 0x8001DE04-0x8001DE9C, extabindex 0x8003EA3C-0x8003EB20, .ctors
 *   0x8056F3CC, .data 0x806049D8-0x80604D2C, .bss 0x806E3AC8-0x806E3E10, .sdata 0x80793B08-0x80793B28, .sbss
 *   0x80794D08-0x80794D18, .sdata2 0x8079C930-0x8079C960.  The left edge is `menu/menu_friendlist.cpp`'s end (the
 *   `extabindex` record of `charmake_color_pick` at 0x8003EA3C; 1.0f/0.0f re-pooled at 0x8079C930; `demo_equip_male`
 *   0x806049D8, `demoEquip` 0x806E3AC8 and `charmake_work` 0x80794D08 are read by this run alone); the right edge is
 *   `quest/arenatask.cpp`.
 * FLAGS. `cflags_menu` (configure.py); file-scope `#pragma pool_data off` (MWCC otherwise addresses the strings and
 *   tables through one pooled base); `#pragma optimization_level 4` over the whole file (`charmake_move`'s palette
 *   switches lower to range compares only under it).
 * NAMES. Module `menu`; the file name `charmake.cpp` is a GUESS (the range has no `__FILE__` string; named for its
 *   entry points' `charmake` stem).  GUESS (from each body and its callers): `charmake_*`, `demo_equip_*`, the types
 *   `CharmakeWork`, `DemoEquipSet`, and every foreign name this file spells from the name registry.
 * RESIDUALS.
 *  - `charmake_move`: retail range-checks the palette cursor on the raw byte (`cmplwi`, `subi`+`clrlwi`+`cmplwi`) and
 *    sign-extends only for the `== 12` case; MWCC sign-extends once before both switches' compares.
 *  - `charmake_move` compares `lb_cursor_box_step`'s result unsigned (`cmplwi`) while `lobby/lb_pane_ui.cpp` declares
 *    it `s32`, so the call site casts it to `u32`; `charmake_value_step` takes the pressed buttons it never reads (every
 *    call passes them in r4).
 */

#include "types.h"
#include "gx.h"
#include "gx/GXRenderModeObj.h"
#include "MTX/mtx.h"
#include "MTX/mtx44.h"
#include "OS/OSError.h"
#include "OS/OSInterrupt.h"
#include "OS/OSReset.h"
#include "OS/DCInvalidateRange.h"
#include "DVD/dvd.h"
#include "DVD/dvdfs.h"
#include "VI/vi.h"
#include "AI_SDK/ai.h"
#include "MIX/mix.h"
#include "SEQ/seq.h"
#include "SYN/syn.h"
#include "WPAD/wpad.h"
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXGeometry.h"
#include "RVLGX/GXLight.h"
#include "RVLGX/GXMisc.h"
#include "RVLGX/GXPixel.h"
#include "RVLGX/GXFrameBuf.h"
#include "RVLGX/GXTev.h"
#include "RVLGX/GXTexture.h"
#include "RVLGX/GXTransform.h"
#include "THP/THPDec.h"
#include "MSL_C/string.h"
#include "MSL_C/strcat.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "main.h"
#include "mh3_pad.h"
#include "mh3_pad/system_w.h"
#include "mh3_pad/lb_param_w.h"
#include "ef/system_core.h"
#include "ef/fn_800CDB2C.h"
#include "font/flfnt.h"
#include "fn_8004CAD8.h"
#include "menu/menu_item.h"
#include "lobby/lb_quest_screen.h"
#include "Network/network_pat_control.h"
#include "sound/fn_800D7F54.h"
#include "sound/fn_800F2A94.h"
#include "menu/Movie_open.h"
#include "dvd_drive_status.h"
#include "Rmode.h"
#include "sound/snd_master_volume_set.h"
#include "sound/snd_track_pause.h"
#include "pmic_stop.h"
#include "font/flfnt_frame_reset.h"
#include "font/flfntGetSizeX.h"
#include "font/dbg_drawgraph.h"
#include "draw_rect_set.h"
#include "draw_rect_fill.h"
#include "ef/dvd_read_wait.h"
#include "ef/hbm_enable.h"
#include "kbd_close_call.h"
#include "enemy/lobby_state_block.h"
#include "enemy/em020_prog.h"
#include "Network/net_session_close.h"
#include "hud/layout.h"
#include "hud/put_lsp_anchor_offset.h"
#include "hud/cockpit.h"
#include "menu/menu_message.h"
#include "menu/get_pop_dat_ptr.h"
#include "lobby/lb_npc.h"
#include "mh3_pad/Psw.h"
#include "nw4r/db_assert.h"
#include "MSL_C/printf.h"
#include "menu/menu_friend_list_init.h"
#include "menu/friend_card_view.h"
#include "menu/movie_page18_open.h"
#include "menu/movie_page18_step.h"
#include "menu/movie_page18_draw.h"
#include "menu/plsearch_work_init.h"
#include "lobby/lb_player_menu_area_ck.h"
#include "enemy/lobby_friend_remove.h"
#include "menu/info_gun_parts_draw.h"
#include "Pl/pl_skill_tree_get.h"
#include "menu/placeinfo_server_rows_draw.h"
#include "menu/chara_icon_tex_tbl.h"
#include "menu/card_equip_ids_build.h"
#include "menu/put_menu_cursor.h"
#include "menu/Put_equip_dtl_syo_item1.h"
#include "menu/Set_equip_column_arrangement.h"
#include "Pl/fn_8027D684.h"
#include "Pl/pl_status_sheet_get.h"
#include "userdata_item.h"
#include "hud/draw_window_box.h"
#include "hud/draw_layout_window.h"
#include "ef/get_move_work_adrs.h"
#include "menu/ck_cfg.h"
#include "menu/option_cfg_max.h"
#include "hud/draw_sprite_blink_idx.h"
#include "disp_beta.h"
#include "color_black.h"
#include "mh3_pad/control.h"
#include "menu/movie_page04_open.h"
#include "menu/movie_page04_step.h"
#include "menu/movie_page04_draw.h"
#include "menu/movie_page04_split_open.h"
#include "menu/movie_page04_split_step.h"
#include "menu/movie_page04_split_draw.h"
#include "menu/movie_page17_open.h"
#include "menu/movie_page17_step.h"
#include "menu/movie_page17_draw.h"
#include "quest/quest_types.h"
#include "quest/quest_entry.h"
#include "unsplit/stage.h"
#include "stage/stg_w.h"
#include "light/light.h"
#include "g3d/g3d_scnroot.h"
#include "g3d/g3d_camera_types.h"
#include "g3d/g3d_anmscn.h"
#include "ef/pRoot.h"
#include "ef/eft_rot_vec_copy.h"
#include "ef/work_mem_free.h"
#include "mh3_pad/Screen_w.h"
#include "Pl/fn_802693C4.h"
#include "Pl/fn_80273B14.h"
#include "Pl/fn_80262940.h"
#include "Pl/fn_80288CEC.h"
#include "Pl/pl_yure.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_net_slot_ready_ck.h"
#include "sound/snd_bank_loader.h"
#include "sound/fn_800EF7D8.h"
#include "sound/fn_800E3CBC.h"
#include "sound/fn_800E46E8.h"
#include "nw_resource.h"
#include "fn_80056F24.h"
#include "fn_80040598.h"
#include "fn_80047398.h"
#include "MSL/strlen.h"
#include "face_work_init.h"
#include "userdata_clear.h"
#include "g3d/camera_posture_get.h"
#include "sound/pl_voice_bank_load.h"
#include "Pl/pl_obj_ready_ck.h"
#include "Pl/pl_parts_set.h"
#include "sound/snd_stream_reloc_busy_ck.h"
#include "sound/se_bank_req.h"
#include "lobby/lb_cursor_box_step.h"
#include "lobby/get_change_hair_color.h"
#include "menu/movie.h"

#pragma pool_data off
#pragma optimization_level 4

/* ---------------------------------------------------------------------------------------------------------------- */
/* The character make. */

/* size: 0x274 (the allocation `charmake_init` makes) - the character make's work: its steps, the edited item, the
 * hunter's name and look, the colour palette cursor, the camera and the model's turn. */
typedef struct CharmakeWork {
    /* +0x000 */ u8 step;
    /* +0x001 */ u8 sub;
    /* +0x002 */ u8 sub2;
    /* +0x003 */ u8 pad_0x003;
    /* +0x004 */ s16 timer;                 /* the fade-out frames, then the ending's frame count */
    /* +0x006 */ u8 pad_0x006[6];
    /* +0x00C */ u8 item;                   /* the edited item: 1..11, 12 the finish row */
    /* +0x00D */ u8 pad_0x00D[2];
    /* +0x00F */ char name[9];
    /* +0x018 */ u8 sex;                    /* the hunter model (0 or 1) */
    /* +0x019 */ s8 equip;                  /* the demo equipment set (0..3) */
    /* +0x01A */ u8 inner_rgb[3];           /* the inner-wear colour */
    /* +0x01D */ u8 pad_0x01D;
    /* +0x01E */ s8 face;
    /* +0x01F */ u8 pad_0x01F[2];
    /* +0x021 */ s8 makeup_color;
    /* +0x022 */ s8 hair;
    /* +0x023 */ u8 hair_rgb[3];            /* the hair colour */
    /* +0x026 */ s8 voice;
    /* +0x027 */ u8 makeup;
    /* +0x028 */ s8 voice_sample;           /* the next sample the voice row plays */
    /* +0x029 */ u8 pad_0x029[3];
    /* +0x02C */ u32 voice_handle;          /* the sample playing (0: none) */
    /* +0x030 */ s8 palette_cursor;         /* the colour palette's cell (12: custom) */
    /* +0x031 */ u8 palette_custom;         /* 1 while the custom colour box is open */
    /* +0x032 */ s16 custom_x;              /* the custom colour box's cursor */
    /* +0x034 */ s16 custom_y;
    /* +0x036 */ s8 left_arrow;             /* the sex arrows' blink frames (-1: off) */
    /* +0x037 */ s8 right_arrow;
    /* +0x038 */ u8 camera;                 /* the camera position (0, 1 the close-up, 2 the ending) */
    /* +0x039 */ u8 pad_0x039[0x1C4 - 0x039];
    /* +0x1C4 */ nw4r::math::VEC3 eye_goal;
    /* +0x1D0 */ nw4r::math::VEC3 eye;
    /* +0x1DC */ nw4r::math::VEC3 at_goal;
    /* +0x1E8 */ nw4r::math::VEC3 at;
    /* +0x1F4 */ u8 pad_0x1F4[4];
    /* +0x1F8 */ _CP_VECTOR turn;           /* the model's rotation */
    /* +0x204 */ u8 pad_0x204[0x218 - 0x204];
    /* +0x218 */ u32 hair_color;            /* the hair colour as an RGBA word */
    /* +0x21C */ u32 inner_color;           /* the inner-wear colour as an RGBA word */
    /* +0x220 */ s16 layout[10];            /* the palette screen's boxes */
    /* +0x234 */ u8 pad_0x234[0x270 - 0x234];
    /* +0x270 */ s16 hold;                  /* frames the item rows ignore the pad */
    /* +0x272 */ u8 confirm;                /* 1 while a confirmation is shown */
    /* +0x273 */ u8 pad_0x273;
} CharmakeWork;

/* size: 0xC - one equipment piece of a demo set, copied as three words. */
typedef struct DemoEquipPiece {
    /* +0x0 */ u32 word[3];
} DemoEquipPiece;

/* size: 0x60 - a demo set's skill list. */
typedef struct DemoEquipSkills {
    /* +0x00 */ u32 word[24];
} DemoEquipSkills;

/* size: 0x20 - a demo set's decoration list. */
typedef struct DemoEquipDecos {
    /* +0x00 */ u32 word[8];
} DemoEquipDecos;

/* size: 0x100 - one demo equipment set: the header bytes, the two colour words, nine pieces and the two lists (the
 * record `arena_userdata_apply` applies). */
typedef struct DemoEquipSet {
    /* +0x00 */ u8 head[4];
    /* +0x04 */ u32 color_a;
    /* +0x08 */ u8 kind[6];
    /* +0x0E */ u8 pad_0x0E[2];
    /* +0x10 */ u32 color_b;
    /* +0x14 */ DemoEquipPiece piece0;
    /* +0x20 */ DemoEquipPiece piece1;
    /* +0x2C */ DemoEquipPiece piece2;
    /* +0x38 */ DemoEquipPiece piece3;
    /* +0x44 */ DemoEquipPiece piece4;
    /* +0x50 */ DemoEquipPiece piece5;
    /* +0x5C */ DemoEquipPiece piece6;
    /* +0x68 */ DemoEquipPiece piece7;
    /* +0x74 */ DemoEquipPiece piece8;
    /* +0x80 */ DemoEquipSkills skills;
    /* +0xE0 */ DemoEquipDecos decos;
} DemoEquipSet;

/* The two colour-change callbacks the character make's palette uses (one per colour kind). */
typedef struct CharmakeColorFuncs {
    /* +0x0 */ void (*set)(u8 kind, u8* rgb);
    /* +0x4 */ u32 (*name_ok)(char* name);
} CharmakeColorFuncs; /* size: 0x8 */

/* A light direction built through `setVec3`, so the file-scope table is constructed by `__sinit`.  size: 0xC */
struct CharmakeLightDir : public nw4r::math::VEC3 {
    /* Builds (x, y, z). */
    CharmakeLightDir(f32 x, f32 y, f32 z) {
        setVec3(this, x, y, z);
    }
};

extern "C" {
/* The character make's work (allocated by `charmake_init`). */
CharmakeWork* charmake_work;
/* The palette's callbacks. */
CharmakeColorFuncs charmake_func;
}

/* The male and female demo equipment sets `charmake_init` copies into `demoEquip`. */
DemoEquipSet demo_equip_male = {
    { 0x00, 0x00, 0x00, 0x00 }, 0x000000FF, { 0x00, 0x02, 0x00, 0x00, 0x00, 0x00 }, { 0, 0 }, 0x000000FF,
    { { 0x0700001D, 0x00000000, 0x00000000 } },
    { { 0x00000000, 0x00000000, 0x00000000 } },
    { { 0x00000000, 0x00000000, 0x00000000 } },
    { { 0x05000000, 0x00000000, 0x00000000 } },
    { { 0x01000000, 0x00000000, 0x00000000 } },
    { { 0x03000000, 0x00000000, 0x00000000 } },
    { { 0x02000000, 0x00000000, 0x00000000 } },
    { { 0x04000000, 0x00000000, 0x00000000 } },
    { { 0x06000000, 0x00000000, 0x00000000 } },
    { { 0 } },
    { { 0 } },
};
DemoEquipSet demo_equip_female = {
    { 0x01, 0x00, 0x00, 0x00 }, 0x000000FF, { 0x00, 0x02, 0x00, 0x00, 0x00, 0x00 }, { 0, 0 }, 0x000000FF,
    { { 0x0700001D, 0x00000000, 0x00000000 } },
    { { 0x00000000, 0x00000000, 0x00000000 } },
    { { 0x00000000, 0x00000000, 0x00000000 } },
    { { 0x05000000, 0x00000000, 0x00000000 } },
    { { 0x01000000, 0x00000000, 0x00000000 } },
    { { 0x03000000, 0x00000000, 0x00000000 } },
    { { 0x02000000, 0x00000000, 0x00000000 } },
    { { 0x04000000, 0x00000000, 0x00000000 } },
    { { 0x06000000, 0x00000000, 0x00000000 } },
    { { 0 } },
    { { 0 } },
};
/* The face and hair model ids each sex offers. */
u16 charmake_face_ids_male[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
u16 charmake_face_ids_female[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
u16 charmake_hair_ids_male[11] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
u16 charmake_hair_ids_female[11] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
u16* charmake_face_ids[2] = { charmake_face_ids_male, charmake_face_ids_female };
u8 charmake_face_count[2] = { 9, 9 };
u16* charmake_hair_ids[2] = { charmake_hair_ids_male, charmake_hair_ids_female };
u8 charmake_hair_count[2] = { 11, 11 };

static void charmake_turn_step(CharmakeWork* work);
static void demo_equip_copy(DemoEquipSet* dst, DemoEquipSet* src);

/* The two demo equipment sets the character make dresses each model in (one per sex, plus a spare). */
DemoEquipSet demoEquip[3];
/* The name the software keyboard edits. */
char charmake_name_buffer[0x18];

/* The four light directions. */
CharmakeLightDir charmake_light_dirs[4] = {
    CharmakeLightDir(0.0f, 0.0f, 50.0f),
    CharmakeLightDir(-50.0f, 70.0f, 40.0f),
    CharmakeLightDir(70.0f, -30.0f, -50.0f),
    CharmakeLightDir(0.0f, 100.0f, 0.0f),
};

/* 0x80443554 (0x4C): picks palette cell `index` of colour kind `kind` (1 the inner wear, else the hair). */
static void charmake_color_pick(u8 kind, u8 index, u32* rgba, s16* x, s16* y) {
    u16 id;

    if (kind == 1) {
        get_change_inner_color(index, rgba, &id, x, y);
    } else {
        get_change_hair_color(index, rgba, &id, x, y);
    }
}

/* 0x804435A0 (0x24C): copies character-make parameter `kind` to `out`. */
/* untyped: caller-owned payload - each kind fills its own parameter block */
void get_charmake_param(u8 kind, void* param) {
    CharmakeWork* work;
    u8* out;

    out = (u8*)param;
    work = charmake_work;
    if (work == NULL) {
        return;
    }
    switch (kind) {
    case 0:
        memcpy(out, work->name, 10);
        break;
    case 12:
        if (work->item == 1) {
            out[0] = work->sub;
        } else {
            out[0] = 0;
        }
        break;
    case 1:
        out[0] = work->sex;
        out[1] = work->left_arrow;
        out[2] = work->right_arrow;
        break;
    case 2:
        out[0] = face_type_get(work->sex);
        out[1] = face_eye_get(work->sex);
        break;
    case 3:
        out[0] = work->equip;
        break;
    case 4:
        out[0] = work->inner_rgb[0];
        out[1] = work->inner_rgb[1];
        out[2] = work->inner_rgb[2];
        out[3] = work->palette_cursor;
        out[4] = work->palette_custom;
        out[5] = work->custom_x;
        out[6] = work->custom_y;
        break;
    case 5:
        out[0] = work->face;
        break;
    case 6:
        out[0] = work->makeup;
        break;
    case 7:
        out[0] = work->makeup_color;
        break;
    case 8:
        out[0] = work->hair;
        break;
    case 9:
        out[0] = work->hair_rgb[0];
        out[1] = work->hair_rgb[1];
        out[2] = work->hair_rgb[2];
        out[3] = work->palette_cursor;
        out[4] = work->palette_custom;
        out[5] = work->custom_x;
        out[6] = work->custom_y;
        break;
    case 10:
        out[0] = work->voice;
        break;
    case 11:
        out[0] = work->item;
        if (work->step == 4) {
            if (work->item == 12 && work->sub2 == 2) {
                out[1] = 3;
            } else {
                out[1] = 1;
            }
        } else if (work->step == 3 && work->sub == 1) {
            out[1] = 2;
        } else if (work->step == 3 && work->sub == 2) {
            out[1] = 4;
        } else {
            out[1] = 0;
        }
        if (work->confirm == 0) {
            out[2] = 0;
        } else {
            out[2] = 1;
        }
        break;
    }
}

/* The four light colours of the character make's scene. */
GXColor charmake_light_colors[4] = {
    { 0xFF, 0xFF, 0xFF, 0xFF }, { 0x40, 0x50, 0x50, 0xFF }, { 0x1A, 0x1A, 0x1A, 0xFF }, { 0x80, 0x80, 0x80, 0xFF },
};
/* The camera's eye and target for each camera position. */
Vec charmake_camera_table[6] = {
    { 0.0f, 85.0f, 300.0f },   { -80.0f, 75.0f, 0.0f }, { -20.0f, 160.0f, 80.0f },
    { -20.0f, 150.0f, 0.0f },  { 0.0f, 70.0f, 350.0f }, { 0.0f, 70.0f, 0.0f },
};

/* 0x804437EC (0x10): stores `value` as byte `index` of the character make's work. */
void set_charmake_r_no(u8 index, u8 value) {
    ((u8*)charmake_work)[index] = value;
}

/* 0x804437FC (0xA8): sets the scene's ambient and four directional lights. */
static void charmake_light_set(void) {
    GXColor* color;
    s32 i;
    CharmakeLightDir* dir;

    set_amblight(0, (GXColor){ 0x1A, 0x1A, 0x1A, 0xFF });
    color = charmake_light_colors;
    make_dir_light2(0, charmake_light_dirs, *color, 0);
    for (i = 0, dir = charmake_light_dirs; i < 4; color++, dir++, i++) {
        make_dir_light2(i + 5, dir, *color, 0);
    }
}

/* 0x804438A4 (0xB4): sets the ending pose: each shown model plays the decided motion pair. */
static void charmake_finish_pose(CharmakeWork* work) {
    u16 motion[2];
    _PLW* plw;
    s32 i;
    u16 count;

    Pl_decide_mot_get(&motion[0], &motion[1]);
    work->turn.y = 0;
    plw = (_PLW*)get_move_work_adrs(2);
    count = get_move_work_max(2);
    for (i = 0; i < count; i++, plw++) {
        if (plw->slot_active != 0 && plw->field_0x001 != 0) {
            plw->field_0x058 = 0;
            Pl_chr_set_attr(plw, motion[i & 1], 10, 0, 1);
        }
    }
}

/* 0x80443958 (0x1A4): turns, colours and poses the two models; only the edited sex's model is shown. */
static void charmake_model_update(CharmakeWork* work) {
    _PLW* plw;
    s32 i;
    u16 count;

    if (work->palette_custom == 0) {
        charmake_turn_step(work);
    }
    plw = (_PLW*)get_move_work_adrs(2);
    count = get_move_work_max(2);
    for (i = 0; i < count; i++, plw++) {
        plw->motion_pos_0x3C = 0.0f;
        plw->motion_pos_0x40 = 0.0f;
        plw->motion_pos_0x44 = 0.0f;
        eft_rot_vec_copy(&plw->rot_0x54, &work->turn);
        plw->field_0x01E = 1;
        plw->field_0x258[0] = (work->hair_color & 0xFF000000) >> 24;
        plw->field_0x258[1] = (work->hair_color & 0xFF0000) >> 16;
        plw->field_0x258[2] = (work->hair_color & 0xFF00) >> 8;
        plw->field_0x258[3] = 0xFF;
        plw->field_0x25C[0] = (work->inner_color & 0xFF000000) >> 24;
        plw->field_0x25C[1] = (work->inner_color & 0xFF0000) >> 16;
        plw->field_0x25C[2] = (work->inner_color & 0xFF00) >> 8;
        plw->field_0x25C[3] = 0xFF;
        if (Get_motion_no(plw) < 700) {
            if (i & 1) {
                Pl_chr_setX(plw, 700, 0, 0);
            } else {
                Pl_chr_setX(plw, 720, 0, 0);
            }
        }
        if (work->sex == i) {
            if (pl_net_slot_ready_ck(plw) != 0) {
                plw->field_0x001 = 1;
            } else {
                plw->field_0x001 = 0;
            }
        } else {
            plw->field_0x001 = 0;
        }
    }
}

/* 0x80443AFC (0x58): aims the camera's goals at camera position `index`. */
static void charmake_camera_goal_set(CharmakeWork* work, u8 index) {
    work->eye_goal.x = charmake_camera_table[index * 2].x;
    work->eye_goal.y = charmake_camera_table[index * 2].y;
    work->eye_goal.z = charmake_camera_table[index * 2].z;
    work->at_goal.x = charmake_camera_table[index * 2 + 1].x;
    work->at_goal.y = charmake_camera_table[index * 2 + 1].y;
    work->at_goal.z = charmake_camera_table[index * 2 + 1].z;
}

/* 0x80443B54 (0xCC): places the scene camera at camera position 0. */
static void charmake_camera_init(CharmakeWork* work) {
    nw4r::g3d::Camera camera;
    s32 handle;
    nw4r::g3d::Camera::PostureInfo posture;

    handle = ((nw4r::g3d::ScnRoot*)pRoot)->GetCamera(0);
    word_copy_return_dst(&camera, &handle);
    camera_posture_info_ctor(&posture);
    work->camera = 0;
    charmake_camera_goal_set(work, 0);
    copyVec3(&work->eye, &work->eye_goal);
    copyVec3(&work->at, &work->at_goal);
    posture.mType = 2;
    copyVec3((nw4r::math::VEC3*)&posture.mTargetX, &work->at);
    posture.mUnk28 = 0.0f;
    camera.SetPosition(work->eye);
    camera.SetPosture(posture);
    camera.SetPerspective(50.0f, Screen_w.aspect, 1.0f, 10000.0f);
}

/* 0x80443C20 (0x10): switches between the full and the close-up camera. */
static void charmake_camera_toggle(CharmakeWork* work) {
    work->camera ^= 1;
    charmake_camera_goal_set(work, work->camera);
}

/* 0x80443C30 (0x110): moves the camera a tenth of the way to its goals. */
static void charmake_camera_step(CharmakeWork* work) {
    nw4r::g3d::Camera camera;
    s32 handle;
    nw4r::g3d::Camera::PostureInfo posture;

    handle = ((nw4r::g3d::ScnRoot*)pRoot)->GetCamera(0);
    word_copy_return_dst(&camera, &handle);
    camera_posture_info_ctor(&posture);
    camera_posture_get(&camera, &posture);
    work->eye.x += (work->eye_goal.x - work->eye.x) / 10.0f;
    work->eye.y += (work->eye_goal.y - work->eye.y) / 10.0f;
    work->eye.z += (work->eye_goal.z - work->eye.z) / 10.0f;
    work->at.x += (work->at_goal.x - work->at.x) / 10.0f;
    work->at.y += (work->at_goal.y - work->at.y) / 10.0f;
    work->at.z += (work->at_goal.z - work->at.z) / 10.0f;
    copyVec3((nw4r::math::VEC3*)&posture.mTargetX, &work->at);
    camera.SetPosition(work->eye);
    camera.SetPosture(posture);
}

/* 0x80443D40 (0x110): turns the model with the remote's or the classic controller's shoulder buttons and switches
 * the camera with the minus button. */
static void charmake_turn_step(CharmakeWork* work) {
    PlayerPad* pad;
    u16 pressed;
    s32 turn;

    pad = Psw;
    pressed = pad->button_0x2C0.pressed_0x04;
    turn = 0;
    switch ((u8)get_ControlType(0)) {
    case 1:
        if (pad->control_0x104 & 0x400) {
            turn = 1;
        } else if (pad->control_0x104 & 0x800) {
            turn = 2;
        }
        break;
    case 2:
        if (pad->control_0x0DE & 4) {
            turn = 1;
        } else if (pad->control_0x0DE & 8) {
            turn = 2;
        }
        break;
    }
    if (turn & 1) {
        work->turn.y = (u16)(work->turn.y + 0x400);
    }
    if (turn & 2) {
        work->turn.y = (u16)(work->turn.y - 0x400);
    }
    if (pressed & 0x80) {
        charmake_camera_toggle(work);
    }
}

/* 0x80443E50 (0x124): sets up the character make: the work, the demo sets, the two hunter models and the
 * defaults. */
void charmake_init(void) {
    CharmakeWork* work;

    GameMode_set(0);
    charmake_work = (CharmakeWork*)work_mem_alloc(sizeof(CharmakeWork));
    memset(charmake_work, 0, sizeof(CharmakeWork));
    demo_equip_copy(&demoEquip[0], &demo_equip_male);
    demo_equip_copy(&demoEquip[1], &demo_equip_female);
    create_move_work(6);
    set_move_work_max(2, 2);
    set_max_player(2);
    set_com_motion_type(3);
    quest_snd_wk_init();
    face_work_init();
    init_player_work();
    player_init_data_load();
    work = charmake_work;
    work->sex = 0;
    work->layout[4] = 0;
    work->layout[6] = 0;
    work->layout[5] = 0xBF;
    work->layout[7] = 0xBF;
    work->hair_color = 0x371A00FF;
    work->layout[0] = 20;
    work->layout[1] = 160;
    work->layout[8] = 32;
    work->layout[9] = 240;
    work->hair_rgb[0] = 0x38;
    work->hair_rgb[1] = 0x1A;
    work->hair_rgb[2] = 0;
    work->inner_color = 0xFFFFFFFF;
    work->layout[2] = 0xBF;
    work->layout[3] = 0;
    work->inner_rgb[0] = 0xFF;
    work->inner_rgb[1] = 0xFF;
    work->inner_rgb[2] = 0xFF;
    snd_bank_loader_reset();
}

/* 0x80443F74 (0x1A0): copies demo equipment set `src` to `dst`. */
static void demo_equip_copy(DemoEquipSet* dst, DemoEquipSet* src) {
    dst->head[0] = src->head[0];
    dst->head[1] = src->head[1];
    dst->head[2] = src->head[2];
    dst->head[3] = src->head[3];
    dst->color_a = src->color_a;
    dst->kind[0] = src->kind[0];
    dst->kind[1] = src->kind[1];
    dst->kind[2] = src->kind[2];
    dst->kind[3] = src->kind[3];
    dst->kind[4] = src->kind[4];
    dst->kind[5] = src->kind[5];
    dst->color_b = src->color_b;
    dst->piece0 = src->piece0;
    dst->piece1 = src->piece1;
    dst->piece2 = src->piece2;
    dst->piece3 = src->piece3;
    dst->piece4 = src->piece4;
    dst->piece5 = src->piece5;
    dst->piece6 = src->piece6;
    dst->piece7 = src->piece7;
    dst->piece8 = src->piece8;
    dst->skills = src->skills;
    dst->decos = src->decos;
}

/* 0x80444114 (0x48): selects face slot `sex` and shows that model's face, hiding the other's. */
static void charmake_face_select(u8 sex) {
    face_select_set(sex, sex);
    face_draw_on(sex);
    face_draw_off(sex ^ 1);
}

/* 0x8044415C (0x64): loads face `face` of model `sex`. */
static void charmake_face_load(u8 sex, u8 face) {
    face_tex_load(sex, charmake_face_ids[sex][face], 0);
    face_parts_tex_load(sex, 0, 0);
    face_tex_bind(sex);
}

/* 0x804441C0 (0x18): non-zero once the edited model's object is ready. */
static s32 charmake_model_ready_ck(CharmakeWork* work) {
    return pl_obj_ready_ck(&pl_global->table[work->sex]);
}

/* 0x804441D8 (0xBC): moves the item cursor up (1..12, wrapping); 1 when it moved. */
static u32 charmake_item_step(CharmakeWork* work) {
    PlayerPad* pad;
    u16 input;

    pad = Psw;
    input = pad->button_0x2C0.held_0x14 | pad->button_0x2C0.pressed_0x04;
    if (input & 1) {
        titleSE_req(3);
        if (--work->item < 1) {
            work->item = 12;
        }
        return 1;
    }
    if (input & 2) {
        titleSE_req(3);
        if (++work->item >= 13) {
            work->item = 1;
        }
        return 1;
    }
    return 0;
}

/* 0x80444294 (0xAC): applies the face choice to the edited model. */
static void charmake_face_apply(CharmakeWork* work) {
    _PLW* plw;
    u16* ids;
    u8 count;

    plw = (_PLW*)get_move_work_adrs(2);
    plw = &plw[work->sex];
    ids = charmake_face_ids[work->sex];
    count = charmake_face_count[work->sex];
    if (work->face >= count) {
        work->face = count - 1;
    }
    plw->field_0x25C[5] = work->face;
    charmake_face_load(work->sex, work->face);
    charmake_face_select(work->sex);
    pl_parts_set(work->sex, 6, ids[work->face]);
}

/* 0x80444340 (0xE4): applies the hair choice to the edited model (the previous one stays when it cannot be set). */
static void charmake_hair_apply(CharmakeWork* work) {
    u8 prev;
    u16* ids;
    _PLW* plw;
    u8 count;
    s8 ok;

    plw = (_PLW*)get_move_work_adrs(2);
    plw = &plw[work->sex];
    prev = work->hair;
    ids = charmake_hair_ids[work->sex];
    count = charmake_hair_count[work->sex];
    if (work->hair >= count) {
        work->hair = count - 1;
    }
    ok = pl_parts_set_ck(work->sex, 5, ids[work->hair]);
    if (ok == -1) {
        work->hair = prev;
        return;
    }
    if (ok != 0) {
        plw->field_0x25C[4] = work->hair;
        pl_parts_set(work->sex, 5, ids[work->hair]);
    }
}

/* 0x80444424 (0xF0): dresses the edited model in demo equipment set `equip`. */
static void charmake_equip_apply(CharmakeWork* work) {
    DemoEquipSet* set;
    _PLW* plw;

    plw = (_PLW*)get_move_work_adrs(2);
    plw = &plw[work->sex];
    set = &demoEquip[work->sex];
    plw->user_profile_0x1C = work->equip;
    pl_parts_set(work->sex, 0, pl_equip_model_id_get(plw, (_EQUIP*)&set->piece4));
    pl_parts_set(work->sex, 1, pl_equip_model_id_get(plw, (_EQUIP*)&set->piece6));
    pl_parts_set(work->sex, 2, pl_equip_model_id_get(plw, (_EQUIP*)&set->piece5));
    pl_parts_set(work->sex, 3, pl_equip_model_id_get(plw, (_EQUIP*)&set->piece7));
    pl_parts_set(work->sex, 4, pl_equip_model_id_get(plw, (_EQUIP*)&set->piece3));
}

/* 0x80444514 (0x178): starts a new user data from the character made: name, look, voice, colours and the seed. */
static void charmake_userdata_store(CharmakeWork* work) {
    Q_UserData* user;
    _PLW* plw;
    s32 i;
    u8 slot;

    user = get_userdata();
    userdata_clear();
    userdata_new_game_init(-1);
    plw = (_PLW*)get_move_work_adrs(2);
    plw = &plw[work->sex];
    for (i = strlen(work->name); i <= 0; i--) {
        if (work->name[i] == ' ') {
            work->name[i] = 0;
        }
    }
    strcpy(user->hunter_name_0x0003, work->name);
    user->hunter_sex_0x0000 = work->sex;
    user->hunter_face_0x0001 = plw->field_0x25C[5];
    user->hunter_voice_0x009F = work->voice;
    user->hunter_equip_0x009E = work->equip;
    user->inner_color_0x39BC = (work->inner_rgb[0] << 24) | (work->inner_rgb[1] << 16) | (work->inner_rgb[2] << 8) | 0xFF;
    user->hunter_hair_0x0002 = plw->field_0x25C[4];
    user->hair_color_0x0014 = (work->hair_rgb[0] << 24) | (work->hair_rgb[1] << 16) | (work->hair_rgb[2] << 8) | 0xFF;
    user->face_type_0x39A0 = face_type_get(work->sex);
    user->face_eye_0x39A1 = face_eye_get(work->sex);
    user->makeup_0x3DE6 = work->makeup;
    user->makeup_color_0x3DE7 = work->makeup_color;
    slot = system_w.vs_load_slot_0x7e9 + system_w.vs_load_player_0x7e8 * 3;
    user->seed_0x53A8 = (slot << 24) | ran_suu(1);
}

/* 0x8044468C (0x340): steps the value of the item rows with right/left (equipment, makeup, face, makeup colour,
 * hair, voice); 1 when the pad was taken. */
static u32 charmake_value_step(CharmakeWork* work, u16 caller_pressed) {
    PlayerPad* pad;
    u16 pressed;

    pad = Psw;
    pressed = pad->button_0x2C0.pressed_0x04;
    if (work->hold > 0) {
        work->hold--;
        return 0;
    }
    if (pressed & 2) {
        switch (work->item) {
        case 4:
            if (work->equip < 3) {
                work->equip++;
                charmake_equip_apply(work);
                sysSE_req(3);
            }
            return 1;
        case 7:
            if (work->makeup < 9) {
                work->makeup++;
                face_makeup_set(work->sex, work->makeup, work->makeup_color);
                sysSE_req(3);
            }
            return 1;
        case 6:
            if (work->face < charmake_face_count[work->sex] - 1) {
                work->face++;
                charmake_face_apply(work);
                sysSE_req(3);
            }
            return 1;
        case 8:
            if (work->makeup_color < 12) {
                work->makeup_color++;
                face_makeup_set(work->sex, work->makeup, work->makeup_color);
                sysSE_req(3);
            }
            return 1;
        case 9:
            if (work->hair < charmake_hair_count[work->sex] - 1) {
                work->hair++;
                charmake_hair_apply(work);
                sysSE_req(3);
            }
            return 1;
        case 11:
            if (work->voice < 19) {
                work->voice++;
                sysSE_req(3);
            }
            return 1;
        }
    }
    if (pressed & 1) {
        switch (work->item) {
        case 4:
            if (work->equip > 0) {
                work->equip--;
                charmake_equip_apply(work);
                sysSE_req(3);
            }
            return 1;
        case 7:
            if (work->makeup != 0) {
                work->makeup--;
                face_makeup_set(work->sex, work->makeup, work->makeup_color);
                sysSE_req(3);
            }
            return 1;
        case 6:
            if (work->face > 0) {
                work->face--;
                charmake_face_apply(work);
                sysSE_req(3);
            }
            return 1;
        case 8:
            if (work->makeup_color > 0) {
                work->makeup_color--;
                face_makeup_set(work->sex, work->makeup, work->makeup_color);
                sysSE_req(3);
            }
            return 1;
        case 9:
            if (work->hair > 0) {
                work->hair--;
                charmake_hair_apply(work);
                sysSE_req(3);
            }
            return 1;
        case 11:
            if (work->voice > 0) {
                work->voice--;
                sysSE_req(3);
            }
            return 1;
        }
    }
    return 0;
}

/* Steps the colour palette of `kind` (1 the inner wear at `rgb`/`color`, 0 the hair): the cells, the custom box. */
static inline void charmake_palette_step(CharmakeWork* work, u16 pressed, u16 buttons, u8 kind, u8* rgb, u32* color) {
    if (work->palette_custom == 0) {
        if (pressed & 0x20) {
            sysSE_req(1);
            work->step--;
        } else if (pressed & 8) {
            sysSE_req(3);
            work->palette_cursor++;
            if (work->palette_cursor > 12) {
                work->palette_cursor = 0;
            }
        } else if (pressed & 4) {
            sysSE_req(3);
            work->palette_cursor--;
            if (work->palette_cursor < 0) {
                work->palette_cursor = 12;
            }
        } else if (pressed & 1) {
            sysSE_req(3);
            switch (work->palette_cursor) {
            case 0:
            case 1:
            case 2:
                work->palette_cursor = 12;
                break;
            case 12:
                work->palette_cursor = 11;
                break;
            default:
                work->palette_cursor -= 3;
                break;
            }
        } else if (pressed & 2) {
            sysSE_req(3);
            switch (work->palette_cursor) {
            case 9:
            case 10:
            case 11:
                work->palette_cursor = 12;
                break;
            case 12:
                work->palette_cursor = 0;
                break;
            default:
                work->palette_cursor += 3;
                break;
            }
        } else if (pressed & 0x10) {
            titleSE_req(4);
            if (work->palette_cursor < 12) {
                charmake_color_pick(kind, work->palette_cursor, color, &work->custom_x, &work->custom_y);
                rgb[0] = (*color & 0xFF000000) >> 24;
                rgb[1] = (*color & 0xFF0000) >> 16;
                rgb[2] = (*color & 0xFF00) >> 8;
            } else {
                work->palette_custom = 1;
            }
        }
    } else if (pressed & 0x20) {
        sysSE_req(1);
        work->palette_custom = 0;
    } else if ((u32)lb_cursor_box_step(&work->custom_x, &work->custom_y, 1, 199, 1, 87, buttons) == 1) {
        charmake_func.set(kind ^ 1, rgb);
        *color = (rgb[0] << 24) | (rgb[1] << 16) | (rgb[2] << 8) | 0xFF;
    }
}

/* 0x804449CC (0xF78): the character make's frame: set-up, the item menu, the name keyboard, the item editors, the
 * confirmation and the ending; returns -1 without a work, -3 when cancelled, -2 when finished, else the screen. */
s32 charmake_move(void) {
    CharmakeWork* work;
    PlayerPad* pad;
    s32 ret;
    u16 pressed;
    u16 held;
    u16 buttons;
    s32 key;
    _PLW* plw;

    work = charmake_work;
    pad = Psw;
    pressed = pad->button_0x2C0.pressed_0x04;
    held = pad->button_0x2C0.hold_0x00;
    buttons = pad->button_0x2C0.press_0x08;
    if (work == NULL) {
        return -1;
    }
    ret = 0;
    switch (work->step) {
    case 0:
        ret = 1;
        work->step++;
        charmake_camera_init(work);
        title_se_load();
        break;
    case 1:
        ret = 1;
        if (file_loading_ck(NULL, NULL) != 1) {
            pl_motion_set();
            ((StageMapView*)stage_w)->mapno = 0xFF;
            ((StageMapView*)stage_w)->areano = 0;
            filter_reset();
            light_init();
            charmake_light_set();
            work->name[0] = 0;
            work->step++;
        }
        break;
    case 2:
        ret = 1;
        if (player_move_start_ck() == 1) {
            player_move_start(-1);
            work->step++;
            work->sub = 0;
            GlareFilter_on();
            work->item = 1;
            charmake_face_select(work->sex);
            get_move_work_adrs(2);
            charmake_equip_apply(work);
            charmake_face_apply(work);
            charmake_hair_apply(work);
        }
        break;
    case 3:
        switch (work->sub) {
        case 0:
            if (charmake_item_step(work) == 1) {
                break;
            }
            if (pressed & 0x20) {
                if (work->item != 12) {
                    work->item = 12;
                    sysSE_req(1);
                } else {
                    work->sub++;
                    work->confirm = 1;
                    sysSE_req(1);
                }
            } else if ((pressed & 0x10) && work->item >= 1) {
                if (work->item == 11) {
                    work->step++;
                    plw = (_PLW*)get_move_work_adrs(2);
    plw = &plw[work->sex];
                    plw->se_name_idx = work->voice;
                    pl_voice_bank_load(plw);
                    work->voice_sample = 0;
                } else if (work->item == 12) {
                    work->sub = 2;
                    work->sub2 = 0;
                    work->confirm = 0;
                } else {
                    if (work->item == 1) {
                        kbd_open(3);
                        strcpy(charmake_name_buffer, work->name);
                        set_kbd_param(charmake_name_buffer, 16);
                    }
                    work->step++;
                    work->sub = 0;
                }
                sysSE_req(0);
                work->palette_cursor = 0;
                work->palette_custom = 0;
                work->custom_x = 100;
                work->custom_y = 45;
                work->left_arrow = -1;
                work->right_arrow = -1;
            } else {
                charmake_model_update(work);
            }
            break;
        case 1:
            switch (work->sub2) {
            case 0:
                if (work->confirm == 0) {
                    if (pressed & 0x10) {
                        sysSE_req(0);
                        fade_set(0, 1);
                        work->timer = 32;
                        work->sub2++;
                    } else if (pressed & 8) {
                        work->confirm = 1;
                        sysSE_req(3);
                    } else if (pressed & 0x20) {
                        work->confirm = 1;
                        sysSE_req(1);
                    }
                } else if (pressed & 0x10) {
                    work->sub = 0;
                    work->sub2 = 0;
                    sysSE_req(1);
                } else if (pressed & 4) {
                    work->confirm = 0;
                    sysSE_req(3);
                }
                break;
            case 1:
                if (--work->timer <= 0) {
                    work_mem_free(charmake_work);
                    return -3;
                }
                break;
            }
            break;
        case 2:
            if (work->confirm == 0) {
                if (pressed & 0x10) {
                    if (strlen(work->name) != 0) {
                        sysSE_req(0);
                        work->step++;
                        charmake_userdata_store(work);
                        work->sub = 0;
                        work->sub2 = 0;
                        ret = 1;
                    } else {
                        sysSE_req(2);
                    }
                } else if (pressed & 8) {
                    work->confirm = 1;
                    sysSE_req(3);
                } else if (pressed & 0x20) {
                    work->confirm = 1;
                    sysSE_req(1);
                }
            } else if (pressed & 0x10) {
                work->sub = 0;
                work->sub2 = 0;
                sysSE_req(1);
            } else if (pressed & 4) {
                work->confirm = 0;
                sysSE_req(3);
            }
            break;
        }
        break;
    case 4:
        switch (work->item) {
        case 0:
            work->step--;
            break;
        case 1:
            switch (work->sub) {
            case 0:
                key = kbd_move();
                if (key != 0) {
                    if (key == 1) {
                        if (charmake_func.name_ok(charmake_name_buffer) == 1) {
                            strcpy(work->name, charmake_name_buffer);
                            work->step--;
                        } else {
                            work->sub = 1;
                        }
                    } else {
                        work->step--;
                    }
                }
                break;
            case 1:
                if (pressed & 0x10) {
                    sysSE_req(0);
                    work->sub = 0;
                }
                break;
            }
            break;
        case 2:
            if (work->left_arrow >= 0) {
                work->left_arrow++;
                if (work->left_arrow > 30) {
                    work->left_arrow = -1;
                }
            }
            if (work->right_arrow >= 0) {
                work->right_arrow++;
                if (work->right_arrow > 30) {
                    work->right_arrow = -1;
                }
            }
            if (charmake_model_ready_ck(work) != 0) {
                if ((pressed & 8) || (pressed & 4)) {
                    sysSE_req(3);
                    work->sex ^= 1;
                    charmake_face_load(work->sex, work->sex);
                    charmake_face_select(work->sex);
                    get_move_work_adrs(2);
                    charmake_equip_apply(work);
                    charmake_face_apply(work);
                    charmake_hair_apply(work);
                    face_makeup_set(work->sex, work->makeup, work->makeup_color);
                    if (pressed & 4) {
                        work->left_arrow = 0;
                    } else {
                        work->right_arrow = 0;
                    }
                } else if (pressed & 0x20) {
                    sysSE_req(1);
                    work->step--;
                }
            }
            break;
        case 4:
            if (charmake_model_ready_ck(work) != 0 && charmake_value_step(work, pressed) != 1 && (pressed & 0x20)) {
                sysSE_req(1);
                work->step--;
            }
            break;
        case 5:
            charmake_palette_step(work, pressed, buttons, 1, work->inner_rgb, &work->inner_color);
            break;
        case 6:
            if (charmake_model_ready_ck(work) != 0 && charmake_value_step(work, pressed) != 1 && (pressed & 0x20)) {
                sysSE_req(1);
                work->step--;
            }
            break;
        case 3:
            if (pressed & 0x20) {
                sysSE_req(1);
                work->step--;
            } else {
                if (held & 8) {
                    face_type_next(0);
                    face_type_next(1);
                }
                if (held & 4) {
                    face_type_prev(0);
                    face_type_prev(1);
                }
            }
            break;
        case 8:
            if (charmake_value_step(work, pressed) != 1 && (pressed & 0x20)) {
                sysSE_req(1);
                work->step--;
            }
            break;
        case 7:
            if (charmake_value_step(work, pressed) != 1 && (pressed & 0x20)) {
                sysSE_req(1);
                work->step--;
            }
            break;
        case 9:
            if (charmake_model_ready_ck(work) != 0 && charmake_value_step(work, pressed) != 1 && (pressed & 0x20)) {
                sysSE_req(1);
                work->step--;
            }
            break;
        case 10:
            charmake_palette_step(work, pressed, buttons, 0, work->hair_rgb, &work->hair_color);
            break;
        case 11:
            if (file_loading_ck(NULL, NULL) != 1) {
                plw = (_PLW*)get_move_work_adrs(2);
    plw = &plw[work->sex];
                if (charmake_value_step(work, pressed) == 1) {
                    if (plw->se_name_idx != work->voice) {
                        if (work->voice_handle != 0 && snd_stream_reloc_busy_ck(work->voice_handle) != 0) {
                            snd_stream_reloc_stop(work->voice_handle);
                            work->voice_handle = 0;
                        }
                        plw->se_name_idx = work->voice;
                        pl_voice_bank_load(plw);
                        work->voice_sample = 0;
                    }
                } else if (pressed & 0x20) {
                    work->step--;
                    sysSE_req(1);
                } else if (pressed & 0x10) {
                    work->voice_handle = se_bank_req(work->sex + 0x14, work->voice_sample);
                    work->voice_sample = work->voice_sample + 1;
                    if (work->voice_sample >= 12) {
                        work->voice_sample = 0;
                    }
                }
            }
            break;
        case 12:
            switch (work->sub2) {
            case 0:
                ret = 2;
                break;
            case 1:
                system_w.field_0x890 = 0;
                charmake_userdata_store(work);
                ret = 3;
                titleSE_req(5);
                PlayStream(1, 1);
                work->camera = 0;
                charmake_camera_goal_set(work, 2);
                charmake_finish_pose(work);
                work->sub2++;
                work->timer = 0;
                break;
            case 2:
                if (++work->timer > 90) {
                    work->step = 100;
                    ret = -2;
                } else {
                    ret = 3;
                }
                break;
            }
            break;
        }
        if (work->item != 12) {
            charmake_model_update(work);
        }
        break;
    case 10:
        work->step = 2;
        break;
    case 11:
        work->step++;
        break;
    case 12:
        charmake_face_select(1);
        charmake_face_load(1, 1);
        work->step++;
        break;
    case 13:
        charmake_face_select(work->sex);
        charmake_face_load(work->sex, work->sex);
        work->step = 2;
        break;
    }
    nwMoveStart();
    prim_init_all();
    player_control_move();
    yure_move();
    charmake_camera_step(work);
    light_move();
    ((nw4r::g3d::ScnRoot*)pRoot)->SetCurrentCamera(0);
    nwMoveEnd();
    return ret;
}

/* 0x80445944 (0x1C): applies the demo equipment set of `plw`'s slot to it. */
void set_demo_equip_data(_PLW* plw) {
    arena_userdata_apply((u8*)&demoEquip[plw->chunk_ofs], (u8*)plw);
}
