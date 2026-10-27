/* Pl/player_control.cpp - the player control set: the player count, the per-player work set-up and release, the
 *   move start-up, the per-part model file loads and binds, and the parts' model release.
 * RANGE. .text 0x802673A4-0x802693C4 (28 functions); .data 0x805C5E58-0x805C5F30, .bss 0x806AB410-0x806AB810, .sdata
 *   0x80792068-0x80792140, .sbss 0x80794B28-0x80794B30, extab, extabindex.
 * NAMES. `set_max_player`, `init_player_work`, `player_control_move`, `player_init_data_load`, `player_move_start_ck`,
 *   `player_move_start`, `player_control_release` and `parts_mdl_release_all` are the runtime dump's names; the file
 *   name follows them.
 *   GUESS (from each body and its callers): `player_work_array_get` (it allocates the records and returns the works),
 *   `pl_players_ready_ck`, `pl_players_start_poll`, `pl_player_first_step`, `pl_common_motion_type_get`,
 *   `pl_players_draw`, `pl_net_slot_ready_ck`, `pl_armor_file_name_make`/`pl_weapon_file_name_make` (they build
 *   "08/m|f/..." and "09/..." `.brres` names), `pl_part_model_release`, `pl_equip_file_get`, `pl_part_model_bind`,
 *   `pl_part_state_loaded_set`, `pl_part_load_callback`, `pl_part_load_request`, `pl_parts_load_step`,
 *   `pl_weapon_file_kind`; the tables (`pl_armor_part_blocks`, `pl_weapon_part_blocks`/`_offsets`, the stem and `_e`
 *   number lists, the four name buffers) and `pl_res_loaded_flags`.
 * FLAGS. `#pragma pool_data off` for the whole file: with it on, MWCC addresses the tables through one `.data` base
 *   that `pl_part_load_request`/`pl_part_model_bind` hoist into a saved register; retail reloads each table by name.
 * SHAPES. The tables are defined before the functions (that order emits `.data`/`.sdata`/`.bss`/`.sbss` byte for byte);
 *   callers that pass a wider or narrower type than the callee's declaration go through `_c1` cast views.
 * RESIDUALS. `pl_part_model_bind` and `pl_part_load_request`: retail keeps `part * 4` and the record-plus-index
 *   pointers in saved registers for the whole body, ours recomputes them after calls (no source order found that keeps
 *   them). `player_move_start_ck`, `pl_players_ready_ck`, `pl_net_slot_ready_ck`, `pl_parts_load_step`,
 *   `player_move_start`, `init_player_work`: register numbering only (declaration-order searches exhausted).
 *   `pl_part_model_bind`'s `mat = mdl.GetResMat(i)` calls `ResCommon<ResMatData>::operator=`, retail
 *   `ResMat::operator=` (`g3d/g3d_resmat.h` declares none for `ResMat`).
 *   `Pl/player_control.h` declares `player_work_array_get(void)` because `ef/system_core.cpp` redeclares it so;
 *   the definition takes the player count. The target's `.sdata` lacks three intra-section relocations
 *   (0x807920D4/E8/F0: `pl_lance_names[1]`, `pl_long_sword_names`, `pl_switch_axe_names`), which our object carries.
 */

#include "types.h"
#include "pl.h"
#include "nw4r/math.h"
#include "Pl/plw.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_se_rig.h"
#include "Pl/fn_802693C4.h"
#include "unsplit/unknown.h"
#include "lobby/lobby_w.h"
#include "sound/fn_800D7F54.h"
#include "ef/fn_800CDB2C.h"
#include "mh3_pad.h"
#include "MSL_C/string.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Network/network_pat_control.h"
#include "nw_resource.h"
#include "nw_res_entry_get.h"
#include "nw_res_list_load.h"
#include "res_mem_index_get.h"
#include "draw_shape.h"
#include "face_work_init.h"
#include "ef/load_file_req.h"
#include "ef/work_mem_free.h"
#include "ef/system_core.h"
#include "ef/get_move_work_adrs.h"
#include "g3d/g3d_resmat.h"
#include "RVLGX/GXTexture.h"
#include "g3d/res_mdl_copy_ctor.h"
#include "lobby/res_mat_name_get.h"
#include "lobby/lb_equip_page.h"
#include "Pl/pl_net_slot_bind.h"
#include "Pl/pl_model_pos_sync.h"
#include "Pl/pl_se_frame_callback.h"
#include "Pl/pl_se_kind_dispatch.h"
#include "Pl/pl_yure.h"
#include "sound/pl_voice_bank_load.h"
#include "sound/mhchar_model_release.h"
#include "sound/mhchar_model_mtx_update.h"
#include "pl_weapon_motion_res_ids.h"
#include "pl_common_motion_res_ids.h"
#include "nw_res_file_load_entry.h"
#include "face_tex_image_get.h"
#include "face_tex_request.h"
#include "face_tex_ready_ck.h"
#include "lobby/lb_res_rec_copy.h"
#include "lobby/lb_player_frame.h"
#include "lobby/pl_rig_model_load.h"
#include "sound/pl_weapon_se_bank_load.h"
#include "sound/mhchar_file_release.h"
#include "Pl/pl_frame_step.h"
#include "Pl/pl_frame_post_step.h"
#include "Pl/pl_act_state_reset.h"

/* The 0x10-byte context a part's file read hands its completion callback. size: 0x10 */
typedef struct PlPartLoadCtx {
    /* +0x00 */ _PLOBJ* obj;     /* the player's part record */
    /* +0x04 */ s32 player;      /* the player index */
    /* +0x08 */ s32 part;        /* the part being read */
    /* +0x0C */ s32 owner;       /* the player whose resource memory receives it */
} PlPartLoadCtx;

extern "C" {
s32 pl_player_count_get(void);
void pl_res_loaded_set(u8 index);
s32 pl_players_ready_ck(void);
_PLW* player_work_array_get(s32 count);
u32 pl_res_buffer_get(u8 index);
void pl_players_start_poll(_PLGLOBAL* global, _PLW* plw);
void pl_player_first_step(_PLW* plw);
s32 pl_common_motion_type_get(_PLGLOBAL* global);
void pl_players_draw(void);
u32 pl_net_slot_ready_ck(_PLW* self);
char* pl_armor_file_name_make(u8 sex, u8 part, s32 number);
char* pl_weapon_file_name_make(u8 part, u32 id);
void pl_part_model_release(_PLOBJ* obj, MHchar* model, u8 part);
u32 pl_equip_file_get(s32 player, u8 part);
void pl_part_model_bind(_PLW* plw, _PLOBJ* obj, u8 part, s32 unused);
void pl_part_state_loaded_set(_PLW* plw, _PLOBJ* obj, u8 part);
void pl_part_load_callback(s32 result, u32 unused, s32 mode, PlPartLoadCtx* ctx);
void pl_part_load_request(_PLW* plw, _PLOBJ* obj, u8 part);
void pl_parts_load_step(_PLGLOBAL* global);
s32 pl_obj_ready_ck(_PLOBJ* obj);
}

/* `pl_net_slot_bind` with the slot passed as a full word, as these callers pass it. */
#define pl_net_slot_bind_c1 ((void (*)(_PLW*, s32))pl_net_slot_bind)
/* `isServerSlotOccupied` read back as an unsigned flag, as these callers compare it. */
#define isServerSlotOccupied_c1 ((u32 (*)(u8))isServerSlotOccupied)
/* `isCityMode` read back as an unsigned flag, as this caller compares it. */
#define isCityMode_c1 ((u32 (*)(void))isCityMode)
/* `se_entry_request` with the player work as its sound source. */
#define se_entry_request_c1 ((struct SeEntry* (*)(s32, _PLW*, void (*)(_PLW*, s32)))se_entry_request)

void set_max_player(s32 value);
void init_player_work(void);
void player_control_move(void);
void player_init_data_load(void);
u32 player_move_start_ck(void);
void player_move_start(s32 index);
void player_control_release(void);
void parts_mdl_release_all(_PLW* plw);

#pragma pool_data off

/* .data */
/* Per armour part: the size of its file buffer in 0x2800-byte blocks. */
s16 pl_armor_part_blocks[8] = { 0x11, 0xD, 0xD, 0xF, 6, 7, 4, 0 };
/* Per weapon file kind: the size of its file in 0x5000-byte blocks. */
s16 pl_weapon_part_blocks[14] = { 0xC, 6, 0xC, 7, 6, 6, 6, 0xC, 0xC, 6, 5, 3, 3, 0 };
/* Per weapon file kind: its offset inside the weapon's buffer in 0x5000-byte blocks. */
s16 pl_weapon_part_offsets[14] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 7, 6, 9, 0 };
/* Per armour part: the file name stem. */
char* pl_armor_part_names[7] = { "body", "arm", "wst", "leg", "helm", "hair", "face" };
/* The body file numbers that have an `_e` variant. */
u8 pl_body_e_numbers[3] = { 0x40, 0x41, 0xFF };
/* Per armour part: the file numbers that have an `_e` variant (0xFF-terminated), NULL none. */
u8* pl_armor_part_e_numbers[7] = { pl_body_e_numbers, NULL, NULL, NULL, NULL, NULL, NULL };
/* Per weapon kind and weapon part (part - 7): the file name stem. */
char* pl_great_sword_names[1] = { "two" };
char* pl_sword_names[2] = { "one", "slds" };
char* pl_hammer_names[1] = { "ham" };
char* pl_lance_names[2] = { "lan", "sldl" };
char* pl_bowgun_names[3] = { "frm", "brl", "stk" };
char* pl_long_sword_names[1] = { "swo" };
char* pl_switch_axe_names[1] = { "axe" };
char** pl_weapon_part_names[9] = {
    pl_great_sword_names, pl_sword_names, pl_hammer_names, pl_lance_names, pl_bowgun_names,
    pl_bowgun_names, pl_bowgun_names, pl_long_sword_names, pl_switch_axe_names,
};
/* Per weapon kind: the file numbers that have an `_e` variant (0xFF-terminated). */
u8 pl_great_sword_e_numbers[2] = { 0x19, 0xFF };
u8 pl_lance_e_numbers[2] = { 0x17, 0xFF };
u8 pl_long_sword_e_numbers[3] = { 0x08, 0x09, 0xFF };
u8 pl_switch_axe_e_numbers[2] = { 0x0B, 0xFF };
u8* pl_weapon_e_numbers[10] = {
    pl_great_sword_e_numbers, NULL, NULL, pl_lance_e_numbers, NULL,
    NULL, NULL, pl_long_sword_e_numbers, pl_switch_axe_e_numbers, NULL,
};

/* .bss: the file-name buffers the two name builders return. */
char pl_armor_file_name[0x100];
char pl_armor_file_stem[0x100];
char pl_weapon_file_name[0x100];
char pl_weapon_file_stem[0x100];

/* .sbss */
_PLGLOBAL* pl_global;
u8 pl_res_loaded_flags[4];

/* 0x802673A4 - stores the maximum player count. */
void set_max_player(s32 value) {
    if (pl_global == NULL) {
        return;
    }
    pl_global->count = value;
}

/* 0x802673B8 - reads the maximum player count back. */
s32 pl_player_count_get(void) {
    if (pl_global == NULL) {
        return 0;
    }
    return pl_global->count;
}

/* 0x802673D4 - marks one player slot as ready. */
void pl_res_loaded_set(u8 index) {
    pl_res_loaded_flags[index] = 1;
}

/* 0x802673E8 - every player the session expects has finished its first lobby frame. */
s32 pl_players_ready_ck(void) {
    if (pl_global == NULL) {
        return 0;
    }
    if (isServerSelectState() == 0) {
        s32 ready;
        s32 expected = NetCtrlWk::countValidServerIndexes();
        s32 i;
        u8* flags = pl_res_loaded_flags;
        s32 loaded = 0;
        _PLW* plw;

        if (flags[0] == 1) {
            loaded = 1;
        }
        if (flags[1] == 1) {
            loaded++;
        }
        if (flags[2] == 1) {
            loaded++;
        }
        if (flags[3] == 1) {
            loaded++;
        }
        if (loaded != expected) {
            return 0;
        }
        ready = 0;
        plw = (_PLW*)get_move_work_adrs(2);
        for (i = 0; i < expected; i++, plw++) {
            if (plw->field_0x267 != 0) {
                ready++;
            }
        }
        if (ready == expected) {
            return 1;
        }
    } else {
        s32 ready = 0;
        _PLW* plw = (_PLW*)get_move_work_adrs(2);
        s32 i;

        for (i = 0; i < 4; i++, plw++) {
            if (plw->slot_active == 0 || isServerSlotOccupied(i) == 0) {
                ready++;
            } else if (plw->field_0x267 != 0) {
                ready++;
            }
        }
        if (ready == 4) {
            return 1;
        }
    }
    return 0;
}

/* 0x80267548 - allocates the player-global record, the player works and the part records, reserves every
 * player's part and motion resource memory, and returns the player works. */
_PLW* player_work_array_get(s32 count) {
    s32 i;
    s32 part;
    _PLGLOBAL* global;
    _PLOBJ* objs;
    _PLW* works;
    u32 size;

    if (count <= 0) {
        return NULL;
    }
    global = (_PLGLOBAL*)work_mem_alloc(sizeof(_PLGLOBAL));
    pl_global = global;
    if (global == NULL) {
        return NULL;
    }
    memset(global, 0, sizeof(_PLGLOBAL));
    pl_res_loaded_flags[3] = 0;
    pl_res_loaded_flags[2] = 0;
    pl_res_loaded_flags[1] = 0;
    pl_res_loaded_flags[0] = 0;
    size = count * sizeof(_PLW);
    works = (_PLW*)work_mem_alloc(size);
    if (works == NULL) {
        return NULL;
    }
    memset(works, 0, size);
    if (isServerSelectState() == 1 || GameMode_ck() == 3) {
        size = count * sizeof(_PLOBJ);
        objs = (_PLOBJ*)work_mem_alloc(size);
        if (objs == NULL) {
            return NULL;
        }
        memset(objs, 0, size);
        global->limit479 = count;
        global->limit47A = count;
    } else {
        size = (count + 1) * sizeof(_PLOBJ);
        objs = (_PLOBJ*)work_mem_alloc(size);
        if (objs == NULL) {
            return NULL;
        }
        memset(objs, 0, size);
        global->limit479 = count;
        global->limit47A = count + 1;
    }
    global->state = 1;
    global->count = count;
    global->limit478 = count;
    global->objects = (_PLWORK*)works;
    global->table = objs;
    for (i = 0; i < count + 1; i++, objs++) {
        if ((isServerSelectState() == 1 || GameMode_ck() == 3) && i == count) {
            break;
        }
        for (part = 0; part < 7; part++) {
            objs->state[part] = 0;
            objs->req_id[part] = -1;
            objs->load_mem[part] = -1;
            objs->aux_id[part] = -1;
            objs->bound_id[part] = -1;
            objs->bound_mem[part] = -1;
            objs->res_entry[part] = -1;
            objs->gate[part] = NULL;
            global->part_mem[i][part] = pull_res_mem(NULL, pl_armor_part_blocks[part] * 0x2800, 1);
            global->part_adrs[i][part] = getResMemAdrs(global->part_mem[i][part]);
            global->loaded_0x200[part] = 0;
        }
        for (; part < 11; part++) {
            objs->state[part] = 0;
            objs->req_id[part] = -1;
            objs->load_mem[part] = -1;
            objs->aux_id[part] = -1;
            objs->bound_id[part] = -1;
            objs->bound_mem[part] = -1;
            objs->res_entry[part] = -1;
            objs->gate[part] = NULL;
            if (part == 7) {
                global->part_mem[i][part] = pull_res_mem(NULL, 0x3C000, 1);
                global->part_adrs[i][part] = getResMemAdrs(global->part_mem[i][part]);
            } else {
                global->part_mem[i][part] = global->part_mem[i][7];
                global->part_adrs[i][part] = global->part_adrs[i][7];
            }
            global->loaded_0x200[part] = 0;
        }
        if (GameMode_ck() != 2 && i < count) {
            global->player_mem[i] = pull_res_mem(NULL, 0x90000, 1);
            global->player_adrs[i] = getResMemAdrs(global->player_mem[i]);
        }
        global->load_state[i] = 0;
    }
    global->last_player = i - 1;
    face_work_init();
    return (_PLW*)global->objects;
}

/* 0x80267874 - clears every player work and binds it to its network slot and model rig. */
void init_player_work(void) {
    s32 i;
    _PLGLOBAL* global = pl_global;
    _PLW* plw = (_PLW*)get_move_work_adrs(2);
    u16 max = get_move_work_max(2);

    if (system_w.field_0x7d3 == 0) {
        for (i = 0; i < max; i++, plw++) {
            memset(plw, 0, sizeof(_PLW));
            pl_net_slot_bind_c1(plw, i);
            pl_physics_rebind(plw->physics_0x13C, plw);
        }
        return;
    }
    if (isServerSelectState() == 1 || isCityMode_c1() == 1) {
        for (i = 0; i < max; i++, plw++) {
            memset(plw, 0, sizeof(_PLW));
            if (GameMode_ck() == 2) {
                if (i == (s8)my_player_no()) {
                    pl_net_slot_bind_c1(plw, i);
                    if (plw->physics_0x13C != NULL) {
                        pl_physics_rebind(plw->physics_0x13C, plw);
                    }
                }
            } else if (GameMode_ck() == 3 || isServerSlotOccupied_c1(i) == 1 || i == (s8)my_player_no()) {
                pl_net_slot_bind_c1(plw, i);
                if (plw->physics_0x13C != NULL) {
                    pl_physics_rebind(plw->physics_0x13C, plw);
                }
                pl_res_loaded_set(i);
            }
        }
        if (GameMode_ck() != 2 && GameMode_ck() != 3 && global != NULL) {
            global->count = 4;
            global->limit478 = 4;
        }
    } else {
        for (i = 0; i < max; i++, plw++) {
            memset(plw, 0, sizeof(_PLW));
            pl_net_slot_bind_c1(plw, i);
            pl_physics_rebind(plw->physics_0x13C, plw);
        }
    }
}

/* 0x80267A78 - one player's per-player work word. */
u32 pl_res_buffer_get(u8 index) {
    if (pl_global == NULL) {
        return 0;
    }
    return pl_global->players_0x44C[index];
}

/* 0x80267AA0 - in the lobby, starts every player whose part files are bound, and holds the lobby act layer while
 * the local player's are not. */
void pl_players_start_poll(_PLGLOBAL* global, _PLW* plw) {
    s32 i;

    if (GameMode_ck() == 2 && global->state > 2) {
        lobby_w.field_0x12C = 0;
        for (i = 0; i < global->count; i++, plw++) {
            if (plw->slot_active == 0) {
                continue;
            }
            if (isServerSelectState() == 1 && isServerSlotOccupied(i) == 0) {
                continue;
            }
            if (global->ready > global->count) {
                continue;
            }
            if (pl_net_slot_ready_ck(plw) == 1) {
                player_move_start(i);
                if (global->ready < global->count) {
                    ((void (*)(_PLW*))pl_parts_refresh)(plw);
                }
                global->ready++;
            }
            if (i == (s8)my_player_no() && pl_net_slot_ready_ck(plw) == 0) {
                lobby_w.field_0x12C = 1;
            }
        }
    }
}

/* 0x80267BB8 - the first frames of a started player: the SE entries once, then the model follows the work. */
void pl_player_first_step(_PLW* plw) {
    MHchar* model = &((PlSeRig*)plw->physics_0x13C)->models_0x004[0].model;

    switch (plw->field_0x004) {
    case 0:
        plw->field_0x004++;
        pl_act_state_reset(plw);
        plw->field_0xAF4 = (struct _se_w*)se_entry_request_c1(1, plw, pl_se_frame_callback);
        plw->field_0xAF8 = (struct _se_w*)se_entry_request_c1(2, plw, NULL);
        plw->field_0xAFC = (struct _se_w*)se_entry_request_c1(3, plw, pl_se_kind_dispatch);
    case 1:
        plw->drift_on_0x582 = 0;
        pl_model_pos_sync(plw);
        pl_rig_model_move((PlSeRig*)plw->physics_0x13C);
        copyVec3(&plw->vec_0x03C, &model->pos_0x04);
        mhchar_model_mtx_update(model);
        break;
    }
}

/* 0x80267C84 - the common motion set the current game mode uses. */
s32 pl_common_motion_type_get(_PLGLOBAL* global) {
    u8 type = 0;
    u8 mode = GameMode_ck();

    if (mode == 2) {
        if (PlayMode_ck() == 5) {
            type = 1;
        } else {
            type = 2;
        }
    } else if (mode == 0 || mode == 3) {
        type = global->com_motion_type;
    }
    return type;
}

/* 0x80267D00 - the player-control state machine: the motion loads, the part loads, then every player's frame. */
void player_control_move(void) {
    _PLGLOBAL* global = pl_global;
    _PLW* plw;
    LbResRec rec;
    s32 i;

    if (global == NULL) {
        return;
    }
    plw = (_PLW*)global->objects;
    pl_players_start_poll(global, plw);
    switch (global->state) {
    case 0:
        return;
    case 1:
        i = pl_common_motion_type_get(global);
        lb_res_rec_copy(&rec, &pl_common_motion_res_ids[i]);
        rec.c_0x08 = 0;
        nw_res_list_load((LbResId*)&rec);
        if (GameMode_ck() != 2 && GameMode_ck() != 3) {
            for (i = 0; i < global->count; i++, plw++) {
                if (plw->slot_active == 0) {
                    continue;
                }
                if (isServerSelectState() == 1 && isServerSlotOccupied(i) == 0 && i != (s8)my_player_no()) {
                    continue;
                }
                plw->field_0x002 = Get_pl_type(&plw->equipB, &plw->equipC);
                lb_res_rec_copy(&rec, &pl_weapon_motion_res_ids[plw->field_0x002]);
                nw_res_file_load_entry((char*)global->player_adrs[i], (u32*)&rec);
                pl_weapon_se_bank_load(plw);
            }
        }
        global->state = 2;
        break;
    case 2:
        if (file_loading_ck(NULL, NULL) == 1) {
            break;
        }
        for (i = 0; i < global->count; i++, plw++) {
            if (plw->slot_active == 0) {
                continue;
            }
            if (isServerSelectState() == 1 && GameMode_ck() != 3 && isServerSlotOccupied(i) == 0 &&
                i != (s8)my_player_no()) {
                continue;
            }
            plw->field_0x002 = Get_pl_type(&plw->equipB, &plw->equipC);
            pl_weapon_parts_refresh(plw->chunk_ofs, &plw->equipB, &plw->equipC);
            if (GameMode_ck() == 2) {
                ((void (*)(_PLW*))pl_parts_refresh)(plw);
            }
        }
        global->state = 3;
        break;
    case 3:
        pl_parts_load_step(global);
        break;
    case 4:
        pl_parts_load_step(global);
        if (GameMode_ck() == 0 || GameMode_ck() == 3) {
            for (i = 0; i < global->count; i++, plw++) {
                if (plw->slot_active != 0 && plw->field_0x001 != 0 && pl_net_slot_ready_ck(plw) == 1) {
                    pl_player_first_step(plw);
                }
            }
        } else if (GameMode_ck() == 2) {
            for (i = 0; i < global->count; i++, plw++) {
                if (plw->slot_active != 0) {
                    lb_player_frame(plw);
                }
            }
        } else if (PlayMode_ck() == 2) {
            for (i = 0; i < global->count; i++, plw++) {
                my_player_no_set((u8)i);
                pl_frame_step(plw);
            }
            my_player_no_set(0);
        } else {
            u8* system = (u8*)get_move_work_adrs(0);

            for (i = 0; i < global->count; i++, plw++) {
                if (pl_net_slot_ready_ck(plw) == 1 &&
                    (plw->slot_active != 1 || plw->field_0x267 == 0 || system == NULL || system[0x22E5] != 1)) {
                    pl_frame_step(plw);
                }
            }
        }
        break;
    case 5:
        player_control_release();
        break;
    }
}

/* 0x802680C4 - draws every live player. */
void pl_players_draw(void) {
    _PLGLOBAL* global = pl_global;
    _PLW* plw;
    s32 i;

    if (global == NULL) {
        return;
    }
    plw = (_PLW*)global->objects;
    if (global->state != 4) {
        return;
    }
    if (GameMode_ck() == 2) {
        for (i = 0; i < global->count; i++, plw++) {
            if (plw->slot_active != 0) {
                lb_player_draw(plw);
            }
        }
    } else if (PlayMode_ck() == 2) {
        for (i = 0; i < global->count; i++, plw++) {
            my_player_no_set((u8)i);
            pl_frame_post_step(plw);
        }
        my_player_no_set(0);
    } else {
        for (i = 0; i < global->count; i++, plw++) {
            if (plw->slot_active != 0) {
                pl_frame_post_step(plw);
            }
        }
    }
}

/* 0x802681D4 - arms the player data load. */
void player_init_data_load(void) {
    if (pl_global == NULL) {
        return;
    }
    pl_global->state = 1;
}

/* 0x802681EC - every live player's 11 state slots have left the "not yet started" value. */
u32 player_move_start_ck(void) {
    _PLWORK* work = pl_global->objects;
    _PLOBJ* obj = pl_global->table;
    s32 pending = 0;
    s32 i;
    s32 slot;

    for (i = 0; i < pl_global->count; i++, work++, obj++) {
        if (work->active != 0) {
            for (slot = 0; slot < 11; slot++) {
                if (obj->state[slot] != 1) {
                    pending++;
                }
            }
        }
    }
    return pending == 0;
}

/* 0x80268260 - starts the move for one player (`-1` = every live player). */
void player_move_start(s32 index) {
    _PLGLOBAL* global = pl_global;
    _PLWORK* work;
    s32 i;

    if (global == NULL) {
        return;
    }
    if (index == -1) {
        work = global->objects;
        for (i = 0; i < global->count; i++, work++) {
            if (work->active != 0) {
                pl_rig_weapon_kind_set((PlSeRig*)work->model_0x13C);
            }
        }
    } else {
        _PLWORK* one = &global->objects[index];

        if (one->active != 0) {
            pl_rig_weapon_kind_set((PlSeRig*)one->model_0x13C);
        }
    }
    global->state = 4;
}

/* 0x80268308 - the player's parts are loading and every one of its 11 part states is back to idle. */
u32 pl_net_slot_ready_ck(_PLW* self) {
    u8 index;
    _PLOBJ* obj;
    s32 busy;
    s32 i;

    if (pl_global == NULL) {
        return 0;
    }
    index = self->chunk_ofs;
    if (pl_global->load_state[index] != 2) {
        return 0;
    }
    obj = &pl_global->table[index];
    busy = 0;
    for (i = 0; i < 11; i++) {
        if (obj->state[i] != 1) {
            busy = 1;
        }
    }
    return busy != 1;
}

/* 0x8026837C - releases the player models, the works and every reserved resource memory, then the record. */
void player_control_release(void) {
    _PLGLOBAL* global = pl_global;
    s32 i;
    s32 part;

    if (global == NULL) {
        return;
    }
    if (global->state != 0) {
        _PLW* plw = (_PLW*)global->objects;

        for (i = 0; i < global->limit479; i++, plw++) {
            pl_model_release(plw);
        }
        global->state = 0;
        global->count = 0;
        work_mem_free(global->table);
        work_mem_free(global->objects);
        global->table = NULL;
        global->objects = NULL;
    }
    for (i = 0; i < global->limit47A; i++) {
        for (part = 0; part < 7; part++) {
            push_res_mem(global->part_mem[i][part]);
        }
        for (; part < 11; part++) {
            if (part == 7) {
                push_res_mem(global->part_mem[i][part]);
            }
        }
        if (GameMode_ck() != 2 && i < global->limit478) {
            push_res_mem(global->player_mem[i]);
        }
    }
    {
        s32 type = pl_common_motion_type_get(global);

        i = res_mem_index_get((char*)pl_common_motion_res_ids[type].b_0x04);
    }
    if (i != -1) {
        push_res_mem(i);
    }
    work_mem_free(pl_global);
    pl_global = NULL;
}

/* The weapon file kind of part `part` (7 the weapon body) of the weapon file id `id`: the sword's and the
 * lance's shields and the bowgun's barrel and stock have files of their own. */
inline s32 pl_weapon_file_kind(u32 id, u8 part) {
    s32 kind = (id & 0xFF000000) >> 24;
    s32 sub = part - 7;

    switch (kind) {
    case 1:
        if (sub == 0) {
            return kind;
        }
        return 9;
    case 3:
        if (sub == 0) {
            return kind;
        }
        return 10;
    case 4:
    case 5:
    case 6:
        if (sub == 0) {
            return kind;
        }
        return sub + 10;
    }
    return kind;
}

/* 0x802684EC - builds an armour part's file name: "08/m/m_" or "08/f/f_", the stem and number, "_e" where the
 * number has the variant, and ".brres". */
char* pl_armor_file_name_make(u8 sex, u8 part, s32 number) {
    u8* list;

    sprintf(pl_armor_file_stem, "%s%03d", pl_armor_part_names[part], number);
    list = pl_armor_part_e_numbers[part];
    if (list != NULL) {
        while (*list != 0xFF) {
            if (*list == number) {
                strcat(pl_armor_file_stem, "_e");
                break;
            }
            list++;
        }
    }
    if (sex == 0) {
        strcpy(pl_armor_file_name, "08/m/");
        strcat(pl_armor_file_name, "m_");
    } else {
        strcpy(pl_armor_file_name, "08/f/");
        strcat(pl_armor_file_name, "f_");
    }
    strcat(pl_armor_file_name, pl_armor_file_stem);
    strcat(pl_armor_file_name, ".brres");
    return pl_armor_file_name;
}

/* 0x80268604 - builds a weapon part's file name: "09/", the kind's stem for the part and the number, "_e" where
 * the number has the variant, and ".brres". */
char* pl_weapon_file_name_make(u8 part, u32 id) {
    s32 number = id & 0xFFFFFF;
    u32 kind = (id & 0xFF000000) >> 24;
    u8* list;

    sprintf(pl_weapon_file_stem, "%s%03d", pl_weapon_part_names[kind][part - 7], number);
    list = pl_weapon_e_numbers[kind];
    if (list != NULL) {
        while (*list != 0xFF) {
            if (*list == number) {
                strcat(pl_weapon_file_stem, "_e");
                break;
            }
            list++;
        }
    }
    strcpy(pl_weapon_file_name, "09/");
    strcat(pl_weapon_file_name, pl_weapon_file_stem);
    strcat(pl_weapon_file_name, ".brres");
    return pl_weapon_file_name;
}

/* 0x802686EC - releases one part's model and its resource entry. */
void pl_part_model_release(_PLOBJ* obj, MHchar* model, u8 part) {
    if (model->g3d_0x10C != NULL) {
        mhchar_model_release(model);
        mhchar_file_release(model);
        push_g3d_wk(model->g3d_0x10C);
        model->g3d_0x10C = NULL;
        model->field_0x118 = 0;
    }
    nwDelResource(obj->res_entry[part]);
}

/* 0x80268770 - releases every bound part model of the player. */
void parts_mdl_release_all(_PLW* plw) {
    s32 part;
    _PLOBJ* obj = pl_global->table;
    PlSeRig* rig = (PlSeRig*)plw->physics_0x13C;

    obj += plw->chunk_ofs;

    for (part = 0; part < 11; part++) {
        if (obj->bound_id[part] != -1) {
            pl_part_model_release(obj, part < 7 ? &rig->models_0x004[part].model : &rig->sub_0x0A14[part - 7].model,
                                  part);
        }
    }
}

/* 0x8026880C - the resource file of one player's part model. */
u32 pl_equip_file_get(s32 player, u8 part) {
    u32 file;
    u32 tmp;
    ResEntry* entry;

    res_file_ctor(&file, 0);
    if (pl_global == NULL) {
        return file;
    }
    entry = nw_res_entry_get(pl_global->table[player].res_entry[part]);
    if (entry == NULL) {
        return file;
    }
    res_file_assign(&file, res_file_ctor(&tmp, (u32)entry->data));
    return file;
}

/* 0x802688B0 - binds a part's read file to the player's model: the old model goes, the file moves into the
 * player's own memory, the model is built from it, and the face/makeup textures are pointed at the face work. */
void pl_part_model_bind(_PLW* plw, _PLOBJ* obj, u8 part, s32 unused) {
    nw4r::g3d::ResMdl mdl(NULL);
    nw4r::g3d::ResMat mat(NULL);
    nw4r::g3d::ResTexObj tex(NULL);
    char name[0x100];
    PlSeRig* rig;
    MHchar* model;
    void* image;
    u16 size;
    u32 count;
    u32 i;
    s32 p = part;
    s32 id;

    obj->gate[p] = NULL;
    rig = (PlSeRig*)plw->physics_0x13C;
    if (part < 7) {
        model = &rig->models_0x004[part].model;
    } else {
        model = &rig->sub_0x0A14[part - 7].model;
    }
    if (obj->bound_id[p] != -1) {
        pl_part_model_release(obj, model, part);
        obj->bound_mem[p] = -1;
        obj->res_entry[p] = -1;
        if (part == 7) {
            if (obj->bound_id[8] != -1) {
                pl_part_model_release(obj, &rig->sub_0x0A14[1].model, 8);
                obj->bound_mem[8] = -1;
                obj->res_entry[8] = -1;
            }
            if (obj->bound_id[9] != -1) {
                pl_part_model_release(obj, &rig->sub_0x0A14[2].model, 9);
                obj->bound_mem[9] = -1;
                obj->res_entry[9] = -1;
            }
        }
    }
    if (obj->req_id[p] == -1) {
        obj->bound_id[p] = -1;
        obj->bound_mem[p] = -1;
        obj->res_entry[p] = -1;
        if (part < 7) {
            pl_rig_model_reset(rig, part);
            rig->field_0x9C0[part] = 0;
        } else {
            pl_rig_sub_reset(rig, part - 7);
            rig->field_0x9C7[part - 7] = 0;
        }
    } else {
        obj->bound_mem[p] = pl_global->part_mem[plw->chunk_ofs][part];
        obj->bound_id[p] = obj->req_id[p];
        if (part < 7) {
            void* src = getResMemAdrs(obj->load_mem[p]);
            void* dst = getResMemAdrs(obj->bound_mem[p]);

            if (dst != src) {
                memcpy(dst, src, pl_armor_part_blocks[part] * 0x2800);
                memset(src, 0, pl_armor_part_blocks[part] * 0x2800);
            }
            strcpy(name, pl_armor_file_name_make(plw->se_name_set, part, obj->req_id[p]));
            obj->res_entry[p] = nw_res_entry_add(name, dst);
            pl_rig_model_load(rig, part);
            if (model->g3d_0x10C != NULL) {
                model->g3d_0x10C->stage = 4;
            }
            obj->gate[p] = &rig->field_0x9C0[part];
            if (part == 4) {
                if (obj->bound_id[6] != -1) {
                    pl_rig_face_parts_apply(rig);
                }
                if (obj->bound_id[5] != -1) {
                    pl_rig_hair_parts_apply(rig);
                }
            }
        } else {
            u8* dst;
            s32 offset;

            id = pl_weapon_file_kind(obj->req_id[p], part);
            {
                u8* src = (u8*)getResMemAdrs(obj->load_mem[p]);

                dst = (u8*)getResMemAdrs(obj->bound_mem[p]);
                offset = pl_weapon_part_offsets[id] * 0x5000;
                dst += offset;
                if (dst != src + offset) {
                    memcpy(dst, src + offset, pl_weapon_part_blocks[id] * 0x5000);
                }
            }
            strcpy(name, pl_weapon_file_name_make(part, obj->req_id[p]));
            obj->res_entry[p] = nw_res_entry_add(name, dst);
            pl_rig_weapon_load(rig, part - 7, part);
            if (model->g3d_0x10C != NULL) {
                model->g3d_0x10C->stage = 4;
            }
            obj->gate[p] = &rig->field_0x9C7[part - 7];
        }
    }
    if ((part == 0 || part >= 7) && obj->part_flag_0x0B[part] != 0) {
        pl_rig_callbacks_rebind(rig);
    }
    if ((u32)(part - 4) <= 1) {
        yure_player_init_kind((YurePlayerWork*)plw, part);
    }
    obj->part_flag_0x0B[part] = 0;
    obj->req_id[p] = -1;
    obj->load_mem[p] = -1;
    obj->aux_id[p] = -1;
    obj->state[part] = 8;
    pl_global->loaded_0x200[part] = 0;
    if (part < 7 && part != 5 && obj->bound_id[p] != -1) {
        if (part != 6) {
            image = face_tex_image_get(plw->chunk_ofs, 0);
            size = 0x80;
        } else {
            image = face_tex_image_get(plw->chunk_ofs, 1);
            size = 0x80;
        }
        if (image != NULL) {
            res_mdl_copy_ctor((struct ResHandle*)&mdl, (struct ResHandle*)&rig->models_0x004[part].model.field_0x114);
            count = mdl.GetResMatNumEntries();
            for (i = 0; i < count; i++) {
                const char* mat_name;

                mat = mdl.GetResMat(i);
                mat_name = res_mat_name_get(&mat);
                if (mat_name == NULL) {
                    continue;
                }
                if (part != 6) {
                    if (strcmp(mat_name, "skin") == 0) {
                    } else {
                        continue;
                    }
                } else if (strcmp(mat_name, "face") != 0) {
                    continue;
                }
                {
                    u32 map;

                    tex = mat.GetResTexObj();
                    for (map = 0; map < 8; map++) {
                        GXTexObj* obj_tex = tex.GetTexObj((_GXTexMapID)map);

                        if (obj_tex != NULL) {
                            GXInitTexObj(obj_tex, image, size, size, 6, 0, 0, 0);
                        }
                    }
                }
            }
        }
    }
}

/* 0x80268E38 - marks one part's file as read. */
void pl_part_state_loaded_set(_PLW* plw, _PLOBJ* obj, u8 part) {
    obj->state[part] = 3;
}

/* 0x80268E48 - the part file read's completion callback: "reading" becomes "read". */
void pl_part_load_callback(s32 result, u32 unused, s32 mode, PlPartLoadCtx* ctx) {
    if (mode != 4) {
        return;
    }
    {
        u8* state = ctx->obj->state;

        if (state[ctx->part] == 4) {
            state[ctx->part] = 5;
        }
    }
}

/* 0x80268E70 - starts reading one part's requested file into resource memory (the weapon parts wait for the
 * weapon body's read). */
void pl_part_load_request(_PLW* plw, _PLOBJ* obj, u8 part) {
    _PLGLOBAL* global = pl_global;
    PlPartLoadCtx ctx;
    char name[0x100];
    s32 owner;
    s32 mem;
    u8* dst;
    s32 size;

    if (global == NULL) {
        return;
    }
    if (global->loaded_0x200[part] == 1) {
        return;
    }
    if (part == 7) {
        if (global->loaded_0x200[8] == 1) {
            return;
        }
        if (global->loaded_0x200[9] == 1) {
            return;
        }
    } else if (part > 7) {
        if (global->loaded_0x200[7] == 1 && (u32)(obj->state[7] - 4) > 1) {
            return;
        }
        if (global->loaded_0x200[8] == 1 && (u32)(obj->state[8] - 4) > 1) {
            return;
        }
        if (global->loaded_0x200[9] == 1 && (u32)(obj->state[9] - 4) > 1) {
            return;
        }
    }
    if (obj->req_id[part] == -1) {
        obj->state[part] = 3;
        return;
    }
    global->loaded_0x200[part] = 1;
    if (obj->res_entry[part] == -1) {
        owner = plw->chunk_ofs;
    } else {
        owner = global->last_player;
    }
    mem = global->part_mem[owner][part];
    if (mem < 0) {
        return;
    }
    obj->load_mem[part] = mem;
    ctx.obj = obj;
    ctx.player = plw->chunk_ofs;
    ctx.part = part;
    ctx.owner = owner;
    if (part < 7) {
        strcpy(name, pl_armor_file_name_make(plw->se_name_set, part, obj->req_id[part]));
        dst = (u8*)getResMemAdrs(mem);
        size = pl_armor_part_blocks[part] * 0x2800;
        memset(dst, 0, size);
    } else {
        u32 id = obj->req_id[part];
        s32 kind = pl_weapon_file_kind(id, part);

        strcpy(name, pl_weapon_file_name_make(part, id));
        dst = (u8*)getResMemAdrs(mem) + pl_weapon_part_offsets[kind] * 0x5000;
        size = pl_weapon_part_blocks[kind] * 0x5000;
        memset(dst, 0, size);
    }
    load_file_req(name, (u32)dst, size, (u32)pl_part_load_callback, 4, (u32*)&ctx);
    obj->state[part] = 4;
}

/* 0x8026910C - steps every live player's loads: the face textures, the voice bank, then each part's file state
 * (request, read, bind, show). */
void pl_parts_load_step(_PLGLOBAL* global) {
    s32 i;
    _PLOBJ* obj;
    _PLW* plw;

    plw = (_PLW*)global->objects;
    obj = global->table;

    for (i = 0; i < global->count; i++, plw++, obj++) {
        if (plw->slot_active == 0) {
            continue;
        }
        switch (pl_global->load_state[plw->chunk_ofs]) {
        case 0: {
            s32 j;
            s32 busy = 0;
            _PLW* other = (_PLW*)global->objects;

            for (j = 0; j < global->count; j++, other++) {
                if (other->slot_active != 0 && pl_global->load_state[j] < 2) {
                    busy = 1;
                    break;
                }
            }
            if (j == i || busy != 1) {
                face_tex_request(plw->chunk_ofs, plw->se_name_set, plw->field_0x25C[5], plw->user_profile_0xB1C[0],
                                 plw->user_profile_0xB1C[1]);
                face_makeup_set(plw->chunk_ofs, plw->user_profile_0xB1C[2], plw->user_profile_0xB1C[3]);
                face_select_set(plw->chunk_ofs, plw->se_name_set);
                pl_global->load_state[plw->chunk_ofs]++;
            }
            break;
        }
        case 1:
            face_tex_bind(plw->chunk_ofs);
            if (face_tex_ready_ck(plw->chunk_ofs) != 0) {
                pl_global->load_state[plw->chunk_ofs]++;
                pl_voice_bank_load(plw);
            }
            break;
        }
        if (pl_global->load_state[plw->chunk_ofs] == 2) {
            s32 part;

            for (part = 0; part < 11; part++) {
                switch (obj->state[part]) {
                case 0:
                    obj->state[part] = 1;
                    break;
                case 3:
                    pl_part_model_bind(plw, obj, part, 0);
                    break;
                case 2:
                    if ((part == 0 || obj->state[0] != 2) && ((u32)(part - 8) > 1 || obj->state[7] != 2)) {
                        pl_part_load_request(plw, obj, part);
                    }
                    break;
                case 5:
                    pl_part_state_loaded_set(plw, obj, part);
                    pl_part_model_bind(plw, obj, part, 1);
                    break;
                case 8:
                    obj->state[part]++;
                    break;
                case 9:
                    if (obj->gate[part] != NULL) {
                        *obj->gate[part] = 1;
                    }
                    obj->gate[part] = NULL;
                    obj->state[part] = 1;
                    break;
                }
            }
        }
    }
}

/* 0x80269394 - no slot of this record has advanced past "empty". */
s32 pl_obj_ready_ck(_PLOBJ* obj) {
    s32 count = 0;
    s32 i;

    for (i = 0; i < 11; i++) {
        if (obj->state[i] > 1) {
            count++;
        }
    }
    return count == 0;
}
