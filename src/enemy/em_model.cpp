/* enemy/em_model.cpp - the roster spawn and model band of the enemy population manager: the carve and drop points a
 *   monster leaves, the pop-file and spare gathering points, their models (bobbing, glowing, the spring and its
 *   bubbles), the 32-slot model-callback table and the per-kind roster helpers.
 * RANGE. .text 0x803B936C-0x803BE30C (57 functions); .data 0x805F8220-0x805F84E0, .bss 0x806D2A68-0x806D2AF8,
 *   .sdata 0x80793690-0x807936A8, .sbss 0x80794C50-0x80794C60, .sdata2 0x8079C5B8-0x8079C630, extab, extabindex.  It
 *   defines `em_pop_w` (0x80794C58), `em_handle_tbl` (0x806D2A78), `m_item_tbl`, `em_hagi_tbl` and `em_drop_tbl`;
 *   `enemy/em_pop.cpp` is the band's head.
 * SEAM. The range is two TUs: the `.sdata2` pool repeats 2.0f, 3.0f and 1.5f (0x8079C5BC/C0/CC and C5E4/F8/620) and
 *   the second pool starts 8-aligned after a pad word at 0x8079C5D4; the first band (0x8079C5B8-0x8079C5D8) is read
 *   only by the carve/drop functions (0x803B9588-0x803B9E50), the second and every `.data` table by
 *   0x803BA1B4-0x803BE30C.  The cut lies in 0x803B9E50-0x803BA1B4 (0x803B9E90 proposed: request enemy-model#1).
 * FLAGS. `cflags_menu`, like `enemy/em_pop.cpp`.
 * NAMES. `em_model` is a GUESS from the model-callback table and the 0x803BA6F0-0x803BE30C model helpers; the
 *   function names are `enemy/em_pop.cpp`'s scheme (its header).  `EmModelCallback` (the 0xC-byte
 *   `ICalcWorldCallback` the table holds; its vtable is 0x805F84C0) is a GUESS class name.
 *   GUESS (from each body and its callers): every `em_drop_*`, `em_pop_obj_*`, `em_pop_rec_*`, `em_pop_sub_*`,
 *   `em_pop_spare_*`, `em_pop_res_*` and `em_roster_*` name, `em_model_pool_reset`, `em_kind_res_release`, the data
 *   names (`em_pop_*_tbl`, `em_pop_*_key*`, `em_drop_default_tbl`, `em_pop_bob_tilt`, `em_pop_spare_life`,
 *   `em_pop_lure_mat_name`), and the foreign names in the registry (`mhchar_model_create`, `pop_dat_*`, ...).
 * RESIDUALS.
 *  - the `.sdata2` pool cannot match in one TU (the seam above): ours dedups the second band's 2.0f/3.0f/1.5f
 *    (0x68 bytes against 0x78); the 4-byte `.data` tail after the vtable is link padding;
 *  - foreign callees and data named through registry proposals (leaf headers beside their owners) pair by
 *    relocation name once the map takes the names; `res_mat_copy_ctor`/`res_mat_chan_copy_ctor` are the map's C
 *    names for the ResMat/ResMatChan copy constructors `em_pop_obj_tint_set` calls;
 *  - the nw4r members this unit calls (ICalcWorldCallback's ctor/dtor, ResMdl's ctor and assignment,
 *    ScnMdlSimple::SetCalcWorldCallback/SetCalcWorldNodeID/EnableScnMdlCallbackTiming,
 *    FuncObjCalcWorld::Get/SetCallbackNodeID) keep the map's generated rows: other units still call them by C names
 *    until those classes are modelled there, so the relocations differ by name only;
 *  - `em_pop_obj_callback_bind`: retail's first search jumps into the second loop's bind tail (a shared tail); without
 *    `goto` the best shape is a break-out first loop and a guarded second loop (88.3; the flag-free unrolled form
 *    measured 56.8);
 *  - `em_pop_obj_tint_set`: retail re-sign-extends the s16 channels at every use and does not fold the `max < r`
 *    test with `r > 0xFF`; its two colour words sit at 0x18/0x14 (ours 0x20/0x1C); s32 channels measured worse;
 *  - `em_pop_spare_init`: retail rebuilds each 5-byte run's base (`li; addis; addi 0x4A49; add`), ours folds it;
 *  - `em_pop_spare_spawn`: retail indexes the lists as `work + (i + 0x34A49)`, ours as `(work + i) + 0x4A49`;
 *  - `em_roster_spare_draw`, `em_pop_work_alloc`, `em_pop_obj_glow_step`, `em_pop_spare_step`: register allocation;
 *  - `em_roster_record_get`: retail returns through `bne` + `mr r3,r4` + `blr`, ours through `beqlr` (the ternary,
 *    early-return, result-variable and byte-local spellings measured the same or worse);
 *  - `em_weight_table_pick`: retail reloads the weight halfword in both loop bodies (`lhz`);
 *  - `em_pop_rec_step`: retail tail-merges the three `field_0x008 = 3` exits of step 2 into the first one;
 *  - `em_roster_pop_entry_add`: retail's switch range test is `subi`/`cmplwi` on r3, ours `cmpwi` pairs on r0;
 *  - `em_pop_spring_bubble_spawn`: retail moves `area` after `height`; `em_pop_obj_alpha_set`: the stack slots of the
 *    two handle temporaries and the 8th `mhchar_mat_tev_set` argument (f1) the header declares;
 *  - `em_drop_item_spawn`: the `em_drop_tbl` load is scheduled one instruction later in retail;
 *    `em_roster_slot_effect_set`: `quest_id_get() == 1` is `cmpwi` in retail.
 * SHAPES. `#pragma fp_contract off` and `#pragma pool_data off` file-wide (retail has no fused multiply-adds and
 *   addresses each table by its own symbol); `#pragma dont_inline on` around the callback class's ctor/dtor, the tint
 *   and the entry-add functions makes the inline nw4r handle members out-of-line calls as retail has them, and off
 *   elsewhere (on, the spare copy becomes a call).  A `switch` whose `default:` is listed first gives retail's
 *   branch-free default (`em_roster_record_result_get`, `em_drop_offset_get`).
 */


#include "types.h"
#include "nw4r/math.h"
#include "nw4r/math_triangular.h"
#include "mh3_pad/vec3.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/string.h"
#include "enemy/em_pop.h"
#include "enemy/em_model.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/em_break_part_data_get.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "quest/quest_entry.h"
#include "quest/quest_item_slot.h"
#include "lobby/lb_quest_screen.h"
#include "ef/fn_800CDB2C.h"
#include "ef/system_core.h"
#include "ef/pRoot.h"
#include "ef/eft004_scaled_spawn.h"
#include "ef/eft_rot_vec_copy.h"
#include "fn_8004CAD8.h"
#include "fn_80047398.h"
#include "camera/camera.h"
#include "draw_shape.h"
#include "draw_shape/mtx34_copy.h"
#include "rotLocalMatX.h"
#include "g3d/mtx34_inverse.h"
#include "g3d/g3d_calcworld.h"
#include "g3d/g3d_resanmchr.h"
#include "g3d/anm_chr_result_scale_get.h"
#include "g3d/g3d_scnmdl.h"
#include "g3d/g3d_scnmdlsmpl.h"
#include "g3d/g3d_resmat.h"
#include "nw_resource.h"
#include "Pl/pl_skill.h"
#include "sound/mhchar.h"
#include "sound/mhchar_motion_set.h"
#include "sound/fn_800DD1F0.h"
#include "stage/stg_w.h"
#include "unsplit/unknown.h"
#include "unsplit/enemy.h"
#include "sound/mhchar_model_create.h"
#include "sound/mhchar_mat_alpha_set.h"
#include "sound/mhchar_mat_num_get.h"
#include "sound/se_item_drop_req.h"
#include "sound/se_pop_spring_req.h"
#include "vec3_random_offset.h"
#include "res_mem_index_get.h"
#include "Pl/pl_map_kind_get.h"
#include "Pl/pl_area_ground_probe.h"
#include "stage/stage_area_cell_get.h"
#include "enemy/em033_hold_kind_get.h"
#include "menu/pop_dat_c_load.h"
#include "menu/pop_dat_f_load.h"
#include "menu/pop_dat_r_load.h"
#include "menu/pop_dat_ptrs_bind.h"
#include "menu/pop_map_kind_get.h"
#include "menu/pop_place_item_tbl.h"
#include "menu/pop_spare_item_tbl.h"
#include "math_sin_deg.h"
#include "lobby/res_mat_name_get.h"

#pragma fp_contract off
#pragma pool_data off


/* One carve point of an enemy kind (`em_hagi_tbl`'s entries).  size: 0x10 (approximation: the read fields) */
struct EmCarveEntry {
    /* +0x00 */ u16 item_0x00;        /* the drop-table index */
    /* +0x02 */ u16 count_0x02;       /* the carves */
    /* +0x04 */ f32 radius_0x04;
    /* +0x08 */ u8 pad_0x08[3];
    /* +0x0B */ u8 joint_0x0B;        /* the joint the point sits at (0: the enemy's position) */
    /* +0x0C */ u8 pad_0x0C[4];
};

/* The context a population-file load hands its completion.  size: 0x8 */
struct EmPopResLoad {
    /* +0x0 */ EmPopWork* work;
    /* +0x4 */ s32 index;             /* 0 the place file, 1 the free-hunt file */
};

/* One fixed point the 0x8 points move to (per map, the quest's or the hub's).  size: 0x10 */
struct EmPopFixedPoint {
    /* +0x00 */ nw4r::math::VEC3 pos;
    /* +0x0C */ u32 rot_y;
};

/* One glow colour.  size: 0xC */
struct EmPopColor {
    /* +0x0 */ f32 r;
    /* +0x4 */ f32 g;
    /* +0x8 */ f32 b;
};

/* The scale a bound node's key value is multiplied by.  size: 0x8 */
struct EmPopNodeScale {
    /* +0x0 */ f32 x;
    /* +0x4 */ f32 y;
};

extern "C" {

/* ---------------------------------------------------------------------------------------------- */
/* This unit's own data. */

/* Per model kind, the model's `ready` byte while it is drawn. */
u8 em_pop_model_show_tbl[20] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0};
/* The pop file's model index to the model kind (0x12 is the glow; 0xFF none). */
u8 em_pop_model_kind_tbl[28] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13,
                                14, 15, 16, 17, 18, 19, 0xFF, 0xFF, 0x12, 0, 0, 0, 0, 0};
/* The four glow materials' alpha key frames (frame, value pairs ending at -1). */
f32 em_pop_glow_key0[8] = {0.0f, 255.0f, 15.0f, 255.0f, 19.0f, 0.0f, -1.0f, 0.0f};
f32 em_pop_glow_key1[8] = {0.0f, 255.0f, 8.0f, 130.0f, 29.0f, 0.0f, -1.0f, 0.0f};
f32 em_pop_glow_key2[8] = {0.0f, 32.0f, 3.0f, 70.0f, 10.0f, 0.0f, -1.0f, 0.0f};
f32 em_pop_glow_key3[8] = {0.0f, 32.0f, 3.0f, 140.0f, 11.0f, 0.0f, -1.0f, 0.0f};
f32* em_pop_glow_key_tbl[4] = {em_pop_glow_key0, em_pop_glow_key1, em_pop_glow_key2, em_pop_glow_key3};
/* The four glow materials' colours. */
EmPopColor em_pop_glow_color_tbl[4] = {
    {255.0f, 217.0f, 103.0f}, {255.0f, 160.0f, 64.0f}, {255.0f, 207.0f, 132.0f}, {255.0f, 207.0f, 132.0f}};
/* The rising bubble's speed factors. */
f32 em_pop_bubble_speed_tbl[4] = {4.0f, 5.0f, 6.0f, 7.0f};
/* The glow's bound nodes' scale key frames (frame, value pairs ending at -1). */
f32 em_pop_node_key0[10] = {0.0f, 0.75f, 5.0f, 1.0f, 12.0f, 0.9f, 19.0f, 0.5f, -1.0f, 0.5f};
f32 em_pop_node_key1[10] = {0.0f, 0.75f, 8.0f, 1.0f, 18.0f, 0.9f, 29.0f, 0.5f, -1.0f, 0.5f};
f32 em_pop_node_key2[8] = {0.0f, 0.1f, 2.0f, 0.2f, 10.0f, 1.4f, -1.0f, 1.4f};
f32 em_pop_node_key3[8] = {0.0f, 0.1f, 5.0f, 0.47f, 11.0f, 2.5f, -1.0f, 2.5f};
f32* em_pop_node_key_tbl[4] = {em_pop_node_key0, em_pop_node_key1, em_pop_node_key2, em_pop_node_key3};
EmPopNodeScale em_pop_node_scale_tbl[4] = {{2.5f, 1.25f}, {1.0f, 1.0f}, {0.75f, 0.75f}, {0.375f, 0.75f}};
/* Per map, the fixed point the 0x8 points take. */
EmPopFixedPoint em_pop_fixed_point_tbl[13] = {
    {{0.0f, 0.0f, 0.0f}, 0x0},
    {{135.0f, 263.0f, -45.0f}, 0x693F},
    {{260.0f, -84.0f, 165.0f}, 0x0},
    {{-2380.0f, 60.0f, -1120.0f}, 0x7777},
    {{-358.0f, 56.0f, -564.0f}, 0x871C},
    {{-1385.0f, 0.0f, 678.0f}, 0xDBBC},
    {{0.0f, 0.0f, 0.0f}, 0x0},
    {{-1962.0f, 838.0f, -134.0f}, 0xED83},
    {{173.0f, 0.0f, -1830.0f}, 0x8667},
    {{10056.0f, 0.0f, -16274.0f}, 0x8667},
    {{4336.0f, 0.0f, -15912.0f}, 0x8667},
    {{-751.0f, 60.0f, -57.0f}, 0xBC73},
    {{173.0f, 0.0f, -1830.0f}, 0x8667},
};
/* The lure material the 0x12 models hide (`mat_0x21D`). */
char em_pop_lure_mat_name[] = "B-L-p02-ankou";

/* The drop table a carve of the 0x1B kind uses in quest 1. */
EmWeightEntry em_drop_default_tbl[2] = {{100, 0x1D}, {0xFFFF, 0}};
/* The bobbing model's tilt, the spare's life frames and the bubbles' wait frames. */
s32 em_pop_bob_tilt = 0xF000;
s32 em_pop_spare_life = 5400;
s16 em_pop_bubble_wait_tbl[4] = {1, 6, 10, 14};

/* ---------------------------------------------------------------------------------------------- */
/* The model-callback table. */

/* The world-matrix callback one table slot carries: it swings or scales the node of the record it is bound to.
 * size: 0xC */
class EmModelCallback : public nw4r::g3d::ICalcWorldCallback {
public:
    EmModelCallback();
    virtual ~EmModelCallback();
    virtual void ExecCallbackA(nw4r::g3d::ChrAnmResult* result, nw4r::g3d::ResMdl mdl,
                               nw4r::g3d::FuncObjCalcWorld* func);
    virtual void bind(u8 node);

    /* +0x4 */ u8 in_use_0x04;
    /* +0x8 */ EmPopObj* owner_0x08;
};

/* .sbss/.bss: the per-kind carve and drop tables, the quest slots' drop tables and the model-callback table (defined
 * at the foot). */
extern EmCarveEntry** em_hagi_tbl;
extern u16* em_drop_tbl;
extern const EmWeightEntry** m_item_tbl[4];
extern EmModelCallback* em_handle_tbl[0x20];

/* This unit's functions used before their definition. */
void em_drop_rec_step(EmPopObj* obj);
u32 em_roster_spare_draw(EmPopRec* rec);
void em_pop_obj_model_show(EmPopObj* obj);
/* untyped: byte range - the loaded file image `nwAddResource` registers */
void em_pop_res_loaded(char* name, void* data, s32 size, EmPopResLoad* load);
void em_pop_obj_tint_set(EmPopRec* rec);
void em_pop_rec_bob_step(EmPopRec* rec);
void em_pop_obj_glow_step(EmPopObj* obj);
void em_pop_rec_step(EmPopObj* obj);
void em_pop_spare_step(EmPopObj* obj);
void em_pop_spring_bubble_spawn(nw4r::math::VEC3* pos, u8 area, f32 height);
void em_pop_bubble_offset_get(nw4r::math::VEC3* out, f32 radius);
void em_pop_sub_bubble_step(EmPopObj* obj);
void em_pop_obj_callback_bind(EmPopObj* obj, u8 mode);
void em_pop_mote_spawn(nw4r::math::VEC3* pos, u8 area);
void em_pop_sub_mote_step(EmPopObj* obj);
void em_pop_obj_alpha_set(EmPopObj* obj, f32 alpha);

/* ---------------------------------------------------------------------------------------------- */
/* Carve and drop points. */

/* Where a carve/drop point of `enemy` goes: its joint `joint`'s position (at the enemy's +0x20C height for flag 1),
 * its break part `part`'s origin (flag 2), or the enemy's own position when `joint` is 0. */
void em_drop_pos_get(_ENEMY_WORK* enemy, u32 joint, nw4r::math::VEC3* out, u8 flags, u8 part) {
    if (joint != 0) {
        if (flags & 1) {
            get_joint_wpos_em(enemy, joint, out);
            out->y = enemy->field_0x20C;
        } else if (flags & 2) {
            EmBreakPart* bp = &enemy->break_parts_0x488[part];
            copyVec3(out, &bp->origin);
            if (bp->kind != 1) {
                EmBreakPartData* data = em_break_part_data_get(enemy, part);
                out->y -= data->lift * get_em_scale(enemy);
            }
        } else {
            get_joint_wpos_em(enemy, joint, out);
        }
    } else {
        copyVec3(out, &enemy->pos);
    }
}

/* Maps the enemy's kind and variant to the offset of its drop table (0 when it has none). */
u8 em_roster_record_result_get(const _ENEMY_WORK* enemy) {
    u8 variant;

    switch (enemy->team) {
    case 0x20:
        variant = enemy->field_0x00A;
        switch (variant) {
        default:
            return variant == 6 ? 5 : 0;
        case 1:
            return 1;
        case 2:
            return 2;
        case 3:
        case 4:
            return 3;
        case 5:
            return 4;
        }
    case 0x22:
        return (enemy->field_0x00A & 2) != 0;
    case 0x1b:
        return (u8)(enemy->field_0x00A == 3);
    }
    return 0;
}

/* The drop-table offset of `enemy`: em033's variant for kind 0x21, none for kind 0x1B, else the record's. */
u8 em_drop_offset_get(_ENEMY_WORK* enemy) {
    u8 variant;

    switch (enemy->team) {
    case 0x21:
        variant = em033_hold_kind_get(enemy);
        switch (variant) {
        default:
            return variant == 4 ? 3 : 0;
        case 1:
            return 1;
        case 2:
            return 2;
        }
    case 0x1b:
        return 0;
    }
    return em_roster_record_result_get(enemy);
}

/* Places the carve point `index` of `enemy` (its carve table entry) as a roster record; `slot` gets the record's
 * index, or -1 when nothing was placed.  Returns 1 when placed. */
s32 em_roster_slot_effect_set(_ENEMY_WORK* self, s32 index, s32* slot, s32 a, s32 b) {
    nw4r::math::VEC3 pos;
    EmCarveEntry* entry;
    const EmWeightEntry* drops;
    const EmWeightEntry** tbl;
    EmPopRec* rec;
    s32 bonus;
    _PLW* pl;

    VEC3_ctor(&pos);
    for (;;) {
        if (em_hagi_tbl[self->team] != NULL) {
            entry = &em_hagi_tbl[self->team][index];
            if (quest_select_ready_ck() == 1) {
                if (m_item_tbl[0] == NULL) {
                    break;
                }
                drops = m_item_tbl[0][entry->item_0x00];
                if ((u16)quest_id_get() == 1 && self->team == 0x1b) {
                    drops = em_drop_default_tbl;
                }
            } else if (move_work_state_ck() != 0) {
                if (m_item_tbl[0] == NULL) {
                    break;
                }
                drops = m_item_tbl[0][entry->item_0x00];
            } else {
                tbl = m_item_tbl[quest_slot_progress_get(0)];
                if (tbl == NULL) {
                    break;
                }
                drops = tbl[entry->item_0x00];
            }
            if (drops != NULL && (rec = em_roster_free_record_get()) != NULL) {
                bonus = 0;
                if (PlayMode_ck() != 2 && (pl = (_PLW*)get_move_work_adrs(2)) != NULL) {
                    pl += (s8)my_player_no();
                    if (Pl_Skill_ck(pl, 0xB3) == 1 && (ran_suu(1) & 1) != 0) {
                        bonus = 1;
                    } else if (Pl_cat_skill_ck(pl, 1) == 1 && (ran_suu(1) & 1) != 0) {
                        bonus = 1;
                    } else if (Pl_cat_skill_ck(pl, 2) == 1 && (ran_suu(1) & 3) == 0) {
                        bonus = 1;
                    }
                }
                em_drop_pos_get(self, entry->joint_0x0B, &pos, (u8)a, (u8)b);
                if ((u8)a == 0 && self->field_0x1E2 == 2 && pos.y > self->field_0x210) {
                    pos.y = self->field_0x210;
                }
                rec->state_0x000 = 1;
                rec->in_area_0x001 = 0;
                rec->action_0x003 = 2;
                rec->field_0x006 = 0;
                rec->kind_0x004 = self->area_no;
                copyVec3(&rec->pos_0x014, &pos);
                copyVec3(&rec->aim_0x1D0, &pos);
                rec->field_0x005 = 0;
                rec->radius_0x1F0 = entry->radius_0x04;
                rec->height_0x1F4 = 200.0f;
                if (((u8)a & 2) == 0 && self->field_0x1E2 == 2) {
                    rec->height_0x1F4 *= 2.0f;
                }
                if ((u8)a & 4) {
                    rec->height_0x1F4 *= 2.0f;
                }
                if ((u8)a & 0x10) {
                    rec->height_0x1F4 *= 3.0f;
                }
                if ((u8)a & 8) {
                    rec->height_0x1F4 *= 4.0f;
                }
                if (self->field_0x1C8 & 0x800) {
                    rec->state_0x1F8 = 0;
                    rec->state_0x1FC = 0;
                } else {
                    rec->state_0x1F8 = 2;
                    rec->state_0x1FC = 2;
                }
                rec->slot_id_0x1FA = entry->count_0x02 + bonus;
                rec->bonus_0x1FE = entry->count_0x02 + bonus;
                rec->drops_0x1EC = drops;
                rec->life_0x1B8 = 0x708;
                rec->step_0x1BC = em_drop_rec_step;
                rec->uses_0x204 = 1;
                rec->uses_max_0x206 = 1;
                if (slot != NULL) {
                    *slot = rec->index_0x002;
                }
                return 1;
            }
        }
        break;
    }
    if (slot != NULL) {
        *slot = -1;
    }
    return 0;
}

/* Places the record's position and its aim position on the same vector. */
void em_roster_record_pos_set(s32 index, nw4r::math::VEC3* pos, u8 area) {
    EmPopRec* rec = em_roster_record_get(index);
    if (rec != NULL) {
        copyVec3(&rec->pos_0x014, pos);
        copyVec3(&rec->aim_0x1D0, pos);
    }
}

/* Hands one live record back to the pool. */
void em_roster_record_release(u32 index) {
    EmPopRec* rec = em_roster_record_get(index);
    if (rec != NULL) {
        em_roster_record_clear(rec);
    }
}

/* Releases the roster records an enemy holds (its own and each break part's) and resets the indices to -1. */
void em_roster_records_release(_ENEMY_WORK* enemy) {
    EmBreakPart* bp;
    s32 i;

    if (enemy->field_0x888 != -1) {
        em_roster_record_release(enemy->field_0x888);
        enemy->field_0x888 = -1;
    }
    bp = enemy->break_parts_0x488;
    for (i = 0; i < 2; i++, bp++) {
        if (bp->effect != -1) {
            em_roster_record_release(bp->effect);
            bp->effect = -1;
        }
    }
}

/* The record's population id, or -1 when the slot is empty or the id is unset. */
s32 em_roster_record_slot_id_get(u32 index) {
    EmPopRec* rec = em_roster_record_get(index);
    s32 id = 0;
    if (rec != NULL) {
        id = rec->slot_id_0x1FA;
    }
    if (id > 0) {
        return id;
    }
    return -1;
}

/* The drop/carve point's step: it counts its life down, sparkles every 32 frames in the local player's area and
 * clears itself once spent. */
void em_drop_rec_step(EmPopObj* obj) {
    EmPopRec* rec = (EmPopRec*)obj;
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (rec->field_0x008) {
    case 0:
        rec->field_0x008++;
        rec->counter_0x010 = 0;
    case 1:
        if (rec->state_0x1FC != 0) {
            break;
        }
        if (rec->life_0x1B8 == 0 || rec->slot_id_0x1FA == 0) {
            rec->field_0x008 = 2;
            break;
        }
        rec->life_0x1B8--;
        rec->counter_0x010++;
        if (rec->kind_0x004 == get_now_areano() && rec->slot_id_0x1FA != 0 && (rec->counter_0x010 & 0x1F) == 1) {
            copyVec3(&pos, &rec->pos_0x014);
            pos.y += 40.0f;
            eft004_scaled_spawn(&pos, rec->kind_0x004, 1.5f);
        }
        break;
    case 2:
        rec->field_0x008++;
    case 3:
        em_roster_record_clear(rec);
        break;
    }
}

/* Rolls `percent` and places one of the enemy's three drops at `pos` in a free drop slot (its drop table at the
 * enemy's variant offset); returns 1 when placed. */
s32 em_drop_item_spawn(_ENEMY_WORK* self, nw4r::math::VEC3* pos, u8 percent) {
    u32 slot = 0xFF;
    nw4r::math::VEC3 at;
    u16 item;
    const EmWeightEntry* drops;
    const EmWeightEntry** tbl;
    EmPopRec* rec;

    VEC3_ctor(&at);
    for (;;) {
        if (self->drop_left_0x89E <= 0 || percent < ran_suu(1) % 100) {
            break;
        }
        if (self->drop_rec_0x88C[0] == -1 && self->drop_timer_0x898[0] <= 0) {
            slot = 0;
        } else if (self->drop_rec_0x88C[1] == -1 && self->drop_timer_0x898[1] <= 0) {
            slot = 1;
        } else if (self->drop_rec_0x88C[2] == -1 && self->drop_timer_0x898[2] <= 0) {
            slot = 2;
        }
        if (slot == 0xFF) {
            break;
        }
        item = em_drop_tbl[self->team];
        if (item == 0) {
            break;
        }
        item += em_drop_offset_get(self);
        if (move_work_state_ck() != 0) {
            if (m_item_tbl[0] == NULL) {
                break;
            }
            drops = m_item_tbl[0][item];
        } else {
            tbl = m_item_tbl[quest_slot_progress_get(0)];
            if (tbl == NULL) {
                break;
            }
            drops = tbl[item];
        }
        if (drops == NULL) {
            break;
        }
        rec = em_roster_free_record_get();
        if (rec == NULL) {
            break;
        }
        copyVec3(&at, pos);
        if (self->field_0x1E2 == 1) {
            at.y = self->field_0x20C;
        }
        rec->state_0x000 = 1;
        rec->in_area_0x001 = 0;
        rec->action_0x003 = 2;
        rec->field_0x006 = 0;
        rec->kind_0x004 = self->area_no;
        copyVec3(&rec->pos_0x014, &at);
        copyVec3(&rec->aim_0x1D0, &at);
        rec->field_0x005 = 0;
        rec->radius_0x1F0 = 150.0f;
        rec->height_0x1F4 = 200.0f;
        if (self->field_0x1E2 == 2) {
            rec->radius_0x1F0 *= 2.0f;
            rec->height_0x1F4 *= 2.0f;
        }
        rec->state_0x1F8 = 0;
        rec->slot_id_0x1FA = 1;
        rec->state_0x1FC = 0;
        rec->bonus_0x1FE = 1;
        rec->drops_0x1EC = drops;
        rec->life_0x1B8 = 0x708;
        rec->step_0x1BC = em_drop_rec_step;
        rec->uses_0x204 = 0;
        rec->uses_max_0x206 = 0;
        self->drop_rec_0x88C[slot] = rec->index_0x002;
        self->drop_timer_0x898[slot] = rec->life_0x1B8;
        self->drop_left_0x89E--;
        if (em_area_ck(self) == 1) {
            se_item_drop_req(&at);
        }
        return 1;
    }
    return 0;
}

/* Whether the record is live and still in its first two states. */
u32 em_roster_record_alive_ck(u32 index) {
    EmPopRec* rec = em_roster_record_get(index);
    if (rec != NULL) {
        if (rec->field_0x008 <= 1) {
            return 1;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------------------------------- */
/* The work and the model-callback pool. */

#pragma dont_inline on
/* Allocates and zeroes the population work, its record arrays and the model-callback table. */
void em_pop_work_alloc(void) {
    s32 i;
    EmPopWork* work;

    work = (EmPopWork*)work_mem_alloc(sizeof(EmPopWork));
    em_pop_w.work = work;
    memset(work, 0, sizeof(EmPopWork));
    work->recs = (EmPopRec*)work_mem_alloc(0x80 * sizeof(EmPopRec));
    memset(work->recs, 0, 0x80 * sizeof(EmPopRec));
    work->rec_num = 0x80;
    work->subs = (EmPopSubRec*)work_mem_alloc(0x80 * sizeof(EmPopSubRec));
    memset(work->subs, 0, 0x80 * sizeof(EmPopSubRec));
    work->sub_num = 0x80;
    for (i = 0; i < 0x20; i++) {
        em_handle_tbl[i] = new EmModelCallback;
        if (em_handle_tbl[i] != NULL) {
            em_handle_tbl[i]->in_use_0x04 = 0;
            em_handle_tbl[i]->owner_0x08 = NULL;
        }
    }
    pop_dat_ptrs_bind();
}

EmModelCallback::EmModelCallback() {}

/* Deletes the 32 model callbacks. */
void em_model_pool_reset(void) {
    s32 i;
    for (i = 0; i < 0x20; i++) {
        if (em_handle_tbl[i] != NULL) {
            delete em_handle_tbl[i];
            em_handle_tbl[i] = NULL;
        }
    }
}

EmModelCallback::~EmModelCallback() {}
#pragma dont_inline off

/* The first free roster slot, stamped with its own index; NULL when the pool is full. */
EmPopRec* em_roster_free_record_get(void) {
    EmPopWork* work = em_pop_w.work;
    EmPopRec* rec = work->recs;
    s32 index = 0;
    s32 num = work->rec_num;
    for (; index < num; index++, rec++) {
        if (rec->state_0x000 == 0) {
            rec->index_0x002 = index;
            return rec;
        }
    }
    return NULL;
}

/* Drops every handle that still names `rec`, releases its model and zeroes the record. */
EmPopRec* em_roster_record_clear(EmPopRec* rec) {
    em_roster_record_unlink(rec);
    if (rec->model_0x038.g3d_0x10C != NULL) {
        push_g3d_wk(rec->model_0x038.g3d_0x10C);
    }
    return (EmPopRec*)memset(rec, 0, sizeof(EmPopRec));
}

/* The first free sub-record slot; NULL when the pool is full. */
EmPopSubRec* em_roster_sub_free_get(void) {
    EmPopWork* work = em_pop_w.work;
    EmPopSubRec* rec = work->subs;
    s32 num = work->sub_num;
    while (num > 0) {
        if (rec->state_0x000 == 0) {
            return rec;
        }
        rec++;
        num--;
    }
    return NULL;
}

/* The same recycle path for one of the manager's sub-records. */
u32 em_roster_sub_record_clear(EmPopSubRec* rec) {
    em_roster_record_unlink(rec);
    if (rec->model_0x038.g3d_0x10C != NULL) {
        push_g3d_wk(rec->model_0x038.g3d_0x10C);
    }
    return (u32)memset(rec, 0, sizeof(EmPopSubRec));
}

/* Draws the record's placement and kind from its spare record, by count and by weight; returns 1 when one was
 * drawn. */
u32 em_roster_spare_draw(EmPopRec* rec) {
    u8 map;
    s32 num;
    EmPopSparePlace* place;
    s32 total;
    s32 roll;
    s32 i;
    s32 acc;
    EmPopSpareKind* kind;
    u16 model;

    if (rec->spare_0x1E8 != NULL) {
        map = pl_map_kind_get();
        num = 0;
        if (rec->spare_0x1E8->places_0x004[0].valid_0x00 != 0) {
            num = 1;
        }
        if (rec->spare_0x1E8->places_0x004[1].valid_0x00 != 0) {
            num++;
        }
        if (rec->spare_0x1E8->places_0x004[2].valid_0x00 != 0) {
            num++;
        }
        if (rec->spare_0x1E8->places_0x004[3].valid_0x00 != 0) {
            num++;
        }
        if (rec->spare_0x1E8->places_0x004[4].valid_0x00 != 0) {
            num++;
        }
        if (rec->spare_0x1E8->places_0x004[5].valid_0x00 != 0) {
            num++;
        }
        if (rec->spare_0x1E8->places_0x004[6].valid_0x00 != 0) {
            num++;
        }
        if (rec->spare_0x1E8->places_0x004[7].valid_0x00 != 0) {
            num++;
        }
        if (num != 0) {
            roll = ran_suu(0) % num;
            place = &rec->spare_0x1E8->places_0x004[roll];
            total = 0;
            if (rec->spare_0x1E8->kinds_0x124[0].valid_0x07 != 0) {
                total = rec->spare_0x1E8->kinds_0x124[0].weight_0x00;
            }
            if (rec->spare_0x1E8->kinds_0x124[1].valid_0x07 != 0) {
                total += rec->spare_0x1E8->kinds_0x124[1].weight_0x00;
            }
            if (rec->spare_0x1E8->kinds_0x124[2].valid_0x07 != 0) {
                total += rec->spare_0x1E8->kinds_0x124[2].weight_0x00;
            }
            if (rec->spare_0x1E8->kinds_0x124[3].valid_0x07 != 0) {
                total += rec->spare_0x1E8->kinds_0x124[3].weight_0x00;
            }
            if (rec->spare_0x1E8->kinds_0x124[4].valid_0x07 != 0) {
                total += rec->spare_0x1E8->kinds_0x124[4].weight_0x00;
            }
            if (rec->spare_0x1E8->kinds_0x124[5].valid_0x07 != 0) {
                total += rec->spare_0x1E8->kinds_0x124[5].weight_0x00;
            }
            if (rec->spare_0x1E8->kinds_0x124[6].valid_0x07 != 0) {
                total += rec->spare_0x1E8->kinds_0x124[6].weight_0x00;
            }
            if (rec->spare_0x1E8->kinds_0x124[7].valid_0x07 != 0) {
                total += rec->spare_0x1E8->kinds_0x124[7].weight_0x00;
            }
            if (total != 0) {
                roll = ran_suu(0) % total;
                kind = rec->spare_0x1E8->kinds_0x124;
                acc = 0;
                for (i = 0; i < 8; i++) {
                    if (kind[i].valid_0x07 != 0) {
                        acc += kind[i].weight_0x00;
                        if (roll <= acc) {
                            break;
                        }
                    }
                }
                if (i < 8) {
                    kind = &kind[i];
                    copyVec3(&rec->aim_0x1D0, &place->pos_0x04);
                    eft_rot_vec_copy(&rec->aim_rot_0x1DC, &place->rot_0x10);
                    copyVec3(&rec->pos_0x014, &place->pos_0x04);
                    eft_rot_vec_copy(&rec->rot_0x020, &place->rot_0x10);
                    rec->radius_0x1F0 = place->radius_0x1C;
                    rec->height_0x1F4 = place->height_0x20;
                    rec->state_0x1F8 = kind->point_0x06;
                    rec->state_0x1FC = kind->point_0x06;
                    rec->slot_id_0x1FA = kind->count_0x08;
                    rec->bonus_0x1FE = kind->bonus_0x0A;
                    rec->uses_0x204 = kind->uses_0x0C;
                    rec->uses_max_0x206 = kind->uses_max_0x0E;
                    switch (kind->point_0x06) {
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                    case 0xA:
                    case 0x10:
                    case 0x11:
                    case 0x12:
                        rec->field_0x005 = 0xFF;
                        break;
                    default:
                        model = kind->model_0x04;
                        if (model == 0xFF) {
                            rec->field_0x005 = 0xFF;
                        } else if (map <= 1) {
                            if (model == 0x12) {
                                rec->field_0x005 = 0x81;
                            } else {
                                rec->field_0x005 = em_pop_model_kind_tbl[model];
                            }
                        } else {
                            rec->field_0x005 = em_pop_model_kind_tbl[model];
                        }
                        break;
                    }
                    if (rec->field_0x005 == 0xD) {
                        rec->field_0x005 = 0xC;
                    }
                    if (rec->field_0x005 == 0x11) {
                        rec->field_0x005 = 0x10;
                    }
                    if (rec->field_0x005 == 0xB) {
                        rec->field_0x005 = 0xA;
                    }
                    rec->drops_0x1EC = pop_spare_item_tbl[map][kind->item_0x02];
                    rec->life_0x1B8 = 0;
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* Steps the population work: once its files are loaded it clears the spare table and loads the pop files; while
 * running it runs every live record's and sub-record's step. */
void em_pop_work_step(void) {
    EmPopWork* work = em_pop_w.work;
    EmPopRec* rec;
    EmPopSubRec* sub;
    s32 i;

    if (work != NULL) {
        switch (work->state_0x000) {
        case 2:
            work->state_0x000++;
            memset(work->spares_0x034, 0, sizeof(work->spares_0x034));
            pop_dat_c_load();
            pop_dat_f_load();
            pop_dat_r_load();
            break;
        case 3:
            rec = work->recs;
            for (i = 0; i < (s32)work->rec_num; i++, rec++) {
                if (rec->state_0x000 != 0) {
                    rec->step_0x1BC(rec);
                }
            }
            sub = work->subs;
            for (i = 0; i < (s32)work->sub_num; i++, sub++) {
                if (sub->state_0x000 != 0) {
                    sub->step_0x1BC(sub);
                }
            }
            break;
        }
    }
}

/* Whether the local slot's sub index is `sub`. */
u8 quest_move_area_sub_ck(u8 sub) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work != NULL && work->sub_0xEA == sub) {
        return 1;
    }
    return 0;
}

/* Shows the record's model: binds the node callback (the 0x81..0x84 kinds), or shows the joint of its kind and
 * starts the bobbing (0xC/0x10) or tints it to the ground. */
void em_pop_obj_model_show(EmPopObj* obj) {
    s32 num;
    s32 i;

    if (obj->field_0x005 == 0xFF) {
        return;
    }
    if ((u8)(obj->field_0x005 + 0x7F) <= 3) {
        em_pop_obj_callback_bind(obj, 1);
        return;
    }
    num = obj->model_0x038.get_joint_num();
    for (i = 0; i < num; i++) {
        obj->model_0x038.setVisibility(i, i == obj->field_0x005 + 1);
    }
    if ((u8)(obj->field_0x005 + 0xF6) <= 1) {
        return;
    }
    if (obj->field_0x005 == 0xC || obj->field_0x005 == 0x10) {
        obj->model_0x038.setVisibility(obj->field_0x005 + 2, 1);
        obj->pos_0x014.y += 50.0f;
        em_pop_obj_callback_bind(obj, 0);
        return;
    }
    em_pop_obj_tint_set((EmPopRec*)obj);
    if (obj->field_0x005 == 0x12 && ((EmPopRec*)obj)->mat_0x21D >= 0) {
        mhchar_mat_hide_set(&obj->model_0x038, ((EmPopRec*)obj)->mat_0x21D, 2);
    }
}

/* Loads map `map`'s population files (the second only in the free-hunt state) into resource memory. */
void em_pop_res_load(u8 map) {
    EmPopResFiles* files = stage_pop_res_tbl[stage_map_kind_get(map)];
    EmPopResLoad load;
    s32 index;

    index = pull_res_mem(files->name_0x04, files->size_0x00, 1);
    if (index >= 0) {
        load.work = em_pop_w.work;
        load.index = 0;
        load_file_req(files->name_0x04, (u32)getResMemAdrs(index), files->size_0x00, (u32)em_pop_res_loaded, 2,
                      (u32*)&load);
        if (move_work_state_ck() == 1) {
            index = pull_res_mem(files->name_0x0C, files->size_0x08, 1);
            if (index >= 0) {
                load.work = em_pop_w.work;
                load.index = 1;
                load_file_req(files->name_0x0C, (u32)getResMemAdrs(index), files->size_0x08, (u32)em_pop_res_loaded,
                              2, (u32*)&load);
            }
        }
    }
}

/* The load completion of a population file: registers it and keeps its `ResFile` handle in the work. */
void em_pop_res_loaded(char* name, void* data /* untyped: byte range - the loaded file image */, s32 size,
                       EmPopResLoad* load) {
    EmPopWork* work = load->work;
    u32 file;

    if (load->index == 0) {
        nwAddResource(name, data);
        work->res_0x008 = ckResourceName(name);
        if (work->res_0x008 != NULL) {
            res_file_assign(&work->res_file_0x00C, res_file_ctor(&file, (u32)work->res_0x008->data));
        }
        work->state_0x000 = 2;
        return;
    }
    nwAddResource(name, data);
    work->res_0x014 = ckResourceName(name);
    if (work->res_0x014 != NULL) {
        res_file_assign(&work->res_file_0x018, res_file_ctor(&file, (u32)work->res_0x014->data));
    }
}

/* The point of `area` whose cylinder holds `pos` (searched from the last record): its id, or 0xFFFF; the kind,
 * centre, rotation and drop table go out through the pointers. */
u16 em_roster_area_hit_find(u8 player, u8 area, nw4r::math::VEC3* pos, s16* out_kind, nw4r::math::VEC3* out_pos,
                            _CP_VECTOR* out_rot, s32* out_word) {
    EmPopWork* work = em_pop_w.work;
    EmPopRec* rec;
    s32 i;
    f32 half;
    u16 id;

    if (work == NULL) {
        return 0xFFFF;
    }
    if (player == (s8)my_player_no()) {
        my_player_no();
        work->frame_prev_0x030 = work->frame_0x02C;
        work->frame_0x02C = system_w.field_0x0c;
    }
    rec = work->recs;
    i = work->rec_num - 1;
    rec += i;
    half = 0.5f;
    for (; i >= 0; i--, rec--) {
        if (rec->state_0x000 == 0) {
            continue;
        }
        if (rec->kind_0x004 != area) {
            continue;
        }
        if (rec->state_0x1FC == 0xFFFF) {
            continue;
        }
        if (rec->field_0x006 == 1 && rec->life_0x1B8 == 0) {
            continue;
        }
        if (rec->action_0x003 != 2 && rec->slot_id_0x1FA == 0) {
            continue;
        }
        if (pos->y >= rec->aim_0x1D0.y - rec->height_0x1F4 * half &&
            pos->y < rec->aim_0x1D0.y + rec->height_0x1F4 * half &&
            calcVecDistXZ(pos, &rec->aim_0x1D0) <= rec->radius_0x1F0) {
            id = rec->state_0x1F8;
            *out_kind = rec->state_0x1FC;
            copyVec3(out_pos, &rec->aim_0x1D0);
            eft_rot_vec_copy(out_rot, &rec->aim_rot_0x1DC);
            if (out_word != NULL) {
                *out_word = (s32)rec->drops_0x1EC;
            } else if (rec->state_0x1FC == 0x11) {
                return rec->field_0x21E;
            }
            if (rec->slot_id_0x1FA == 0) {
                return rec->action_0x003 == 2 ? (u16)0xFFFF : (u16)0;
            }
            return id;
        }
    }
    return 0xFFFF;
}

/* Gathers from the point of `area` under `pos` whose drop table is `word`: 0 when none, 1 when gathered, 2 when it
 * ran out (the gathering skills can end it early). */
u8 em_roster_area_gather_take(u8 area, nw4r::math::VEC3* pos, u32 word) {
    EmPopWork* work;
    EmPopRec* rec;
    s32 i;
    _PLW* pl;
    f32 half;
    u16 odds;
    u16 cut;

    if (word == 0) {
        return 0;
    }
    work = em_pop_w.work;
    rec = work->recs;
    rec += work->rec_num - 1;
    pl = (_PLW*)get_move_work_adrs(2);
    if (pl != NULL) {
        pl += (s8)my_player_no();
    }
    i = work->rec_num - 1;
    half = 0.5f;
    for (; i >= 0; i--, rec--) {
        if (rec->state_0x000 == 0) {
            continue;
        }
        if (rec->kind_0x004 != area) {
            continue;
        }
        if (rec->state_0x1FC == 0xFFFF) {
            continue;
        }
        if (rec->field_0x006 == 1 && rec->life_0x1B8 == 0) {
            continue;
        }
        if (rec->field_0x006 == 0 && rec->field_0x008 != 1) {
            continue;
        }
        if (rec->field_0x006 == 2 && rec->field_0x008 != 1) {
            continue;
        }
        if (pos->y >= rec->aim_0x1D0.y - rec->height_0x1F4 * half &&
            pos->y < rec->aim_0x1D0.y + rec->height_0x1F4 * half &&
            calcVecDistXZ(pos, &rec->aim_0x1D0) <= rec->radius_0x1F0 && word == (u32)rec->drops_0x1EC) {
            if (rec->slot_id_0x1FA == 0) {
                return rec->action_0x003 == 2 ? (u8)0 : (u8)0xFF;
            }
            if (rec->slot_id_0x1FA != 0xFFFF) {
                if (rec->bonus_0x1FE == 0) {
                    odds = 5;
                    if (Pl_Skill_slot_item_get(pl, 0x31) != 0) {
                        if (Pl_Skill_ck(pl, 0x89) == 1) {
                            odds = 1;
                        } else if (Pl_Skill_ck(pl, 0x88) == 1) {
                            odds = 3;
                        } else if (Pl_Skill_ck(pl, 0x8A) == 1) {
                            odds = 9;
                        }
                    }
                    if (Pl_cat_skill_ck(pl, 0xE) == 1) {
                        if (odds > 4) {
                            cut = odds - 3;
                        } else {
                            cut = 1;
                        }
                        odds = cut;
                    }
                    if ((s32)(ran_suu(0) % 32) < odds) {
                        rec->slot_id_0x1FA = 0;
                        return 2;
                    }
                } else {
                    rec->bonus_0x1FE--;
                }
                rec->slot_id_0x1FA--;
                if (rec->slot_id_0x1FA == 0) {
                    return 2;
                }
                return 1;
            }
            return 1;
        }
    }
    return 0;
}

/* Picks one entry of a 0xFFFF-terminated weight table at random, by weight; returns its value (0xFFFF when the
 * table is empty). */
u16 em_weight_table_pick(const EmWeightEntry* table) {
    const EmWeightEntry* entry;
    u16 total;
    u16 acc;
    u16 roll;

    total = 0;
    for (entry = table; entry->weight != 0xFFFF; entry++) {
        total += entry->weight;
    }
    if (total == 0) {
        return 0xFFFF;
    }
    roll = ran_suu(0) % total;
    acc = 0;
    for (entry = table; entry->weight != 0xFFFF; entry++) {
        acc += entry->weight;
        if (roll < acc) {
            return entry->value;
        }
    }
    return 0xFFFF;
}

/* The nearest point of `area` within `range` of `pos` (searched from the last record): its id, or 0xFFFF when it is
 * used up; its kind, centre, drop table and radius go out through the pointers. */
u16 em_roster_area_near_find(u8 area, nw4r::math::VEC3* pos, f32 range, s16* out_kind, nw4r::math::VEC3* out_pos,
                             u32* out_word, f32* out_radius) {
    EmPopWork* work;
    EmPopRec* rec;
    s32 i;
    u16 id;

    my_player_no();
    work = em_pop_w.work;
    work->frame_prev_0x030 = work->frame_0x02C;
    work->frame_0x02C = system_w.field_0x0c;
    rec = work->recs;
    i = work->rec_num - 1;
    rec += i;
    for (; i >= 0; i--, rec--) {
        if (rec->state_0x000 == 0) {
            continue;
        }
        if (rec->kind_0x004 != area) {
            continue;
        }
        if (rec->state_0x1FC == 0xFFFF) {
            continue;
        }
        if (rec->field_0x006 == 1 && rec->life_0x1B8 == 0) {
            continue;
        }
        if (calcVecDistXZ(pos, &rec->aim_0x1D0) <= range) {
            id = rec->state_0x1F8;
            *out_kind = rec->state_0x1FC;
            copyVec3(out_pos, &rec->aim_0x1D0);
            *out_word = (u32)rec->drops_0x1EC;
            *out_radius = rec->radius_0x1F0;
            if (rec->uses_0x204 != 0) {
                return id;
            }
            return 0xFFFF;
        }
    }
    return 0xFFFF;
}

/* Uses the point of `area` under `pos` whose drop table is `word`: 0 when none or used up, 1 when used. */
s32 em_roster_area_use_take(u8 area, nw4r::math::VEC3* pos, u32 word) {
    EmPopRec* rec;
    s32 i;
    f32 half;

    if (word == 0) {
        return 0;
    }
    rec = em_pop_w.work->recs;
    i = em_pop_w.work->rec_num - 1;
    rec += i;
    half = 0.5f;
    for (; i >= 0; i--, rec--) {
        if (rec->state_0x000 == 0) {
            continue;
        }
        if (rec->kind_0x004 != area) {
            continue;
        }
        if (rec->state_0x1FC == 0xFFFF) {
            continue;
        }
        if (rec->field_0x006 == 1 && rec->life_0x1B8 == 0) {
            continue;
        }
        if (pos->y >= rec->aim_0x1D0.y - rec->height_0x1F4 * half &&
            pos->y < rec->aim_0x1D0.y + rec->height_0x1F4 * half &&
            calcVecDistXZ(pos, &rec->aim_0x1D0) <= rec->radius_0x1F0 && word == (u32)rec->drops_0x1EC) {
            if (rec->uses_0x204 == 0) {
                return 0;
            }
            if (rec->uses_0x204 != 0xFFFF) {
                rec->uses_0x204--;
                return 1;
            }
            return 1;
        }
    }
    return 0;
}

#pragma dont_inline on
/* Tints the record's model by the stage colour of its ground: every material's ambient colour is scaled by the
 * ratio of the two stage colours and clamped to 0x3C..0xFF. */
void em_pop_obj_tint_set(EmPopRec* rec) {
    nw4r::math::VEC3 pos;
    u8 base[4];
    u8 tint[4];
    u8 amb[4];
    s32 ofs;
    u8 index;
    s32 num;
    s32 i;
    s16 r;
    s16 g;
    s16 b;
    s16 max;
    u32 color0;
    u32 color1;
    s32 model;

    VEC3_ctor(&pos);
    model = rec->field_0x005;
    if ((u32)(model - 0xA) <= 3 || (u32)(model - 0x10) <= 3 || model == 0xFF) {
        return;
    }
    copyVec3(&pos, &rec->pos_0x014);
    pos.y += 50.0f;
    rec->on_ground_0x21C = pl_area_ground_probe(&pos, rec->kind_0x004, &rec->ground_0x208, -5);
    if (rec->on_ground_0x21C == 0) {
        return;
    }
    ofs = 0;
    if (rec->ground_0x208.kind_0x00 == 1 && rec->pos_0x014.y < 0.0f) {
        ofs = 3;
    }
    index = ofs + rec->ground_0x208.tint_0x07;
    color0 = stage_area_cell_get(ofs, 1, rec->kind_0x004);
    color_rgba_copy(base, (u8*)&color0);
    color1 = stage_area_cell_get(index, 1, rec->kind_0x004);
    color_rgba_copy(tint, (u8*)&color1);
    num = ((nw4r::g3d::ResMdl*)&rec->model_0x038.field_0x114)->GetResMatNumEntries();
    for (i = 0; i < num; i++) {
        nw4r::g3d::ResMat mat(((nw4r::g3d::ResMdl*)&rec->model_0x038.field_0x114)->GetResMat((u32)i));
        nw4r::g3d::ResMatChan chan(mat.GetResMatChan());
        chan.GXGetChanMatColor(4, (GXColor*)amb);
        r = (s16)((tint[0] * amb[0]) / base[0]);
        g = (s16)((tint[1] * amb[1]) / base[1]);
        b = (s16)((tint[2] * amb[2]) / base[2]);
        max = 0;
        if (r > 0xFF && max < r) {
            max = r;
        }
        if (g > 0xFF && max < g) {
            max = g;
        }
        if (b > 0xFF && max < b) {
            max = b;
        }
        if (max > 0xFF) {
            r = (s16)((r * 0xFF) / max);
        }
        if (r > 0xFF) {
            r = 0xFF;
        } else if (r < 0x3C) {
            r = 0x3C;
        }
        if (max > 0xFF) {
            g = (s16)((g * 0xFF) / max);
        }
        if (g > 0xFF) {
            g = 0xFF;
        } else if (g < 0x3C) {
            g = 0x3C;
        }
        if (max > 0xFF) {
            b = (s16)((b * 0xFF) / max);
        }
        if (b > 0xFF) {
            b = 0xFF;
        } else if (b < 0x3C) {
            b = 0x3C;
        }
        amb[0] = (u8)r;
        amb[1] = (u8)g;
        amb[2] = (u8)b;
        rec->model_0x038.setMatColor(i, GX_COLOR0, *(GXColor*)amb, 0);
    }
}

#pragma dont_inline off
/* The bobbing step of the 0xC/0x10 models: it swings the node back and forth, turns and sways it, and floats it
 * up and down around its aim position. */
void em_pop_rec_bob_step(EmPopRec* rec) {
    u16 angle;
    f32 s;
    f32 c;

    if (rec->field_0x005 != 0xC && rec->field_0x005 != 0x10) {
        return;
    }
    if (rec->spin_dir_0x1B6 == 0) {
        angle = rec->spin_0x19C[0].z - rec->spin_speed_0x1B4;
        rec->spin_0x19C[0].z = angle;
        if (angle >= 0x8000) {
            rec->spin_0x19C[0].z = 0;
            rec->spin_dir_0x1B6 = 1;
            if (ran_suu(0) & 1) {
                rec->rise_0x1C8 = 2.0f;
            }
        }
    } else {
        angle = rec->spin_0x19C[0].z + rec->spin_speed_0x1B4;
        rec->spin_0x19C[0].z = angle;
        if (angle >= 0x8000) {
            rec->spin_0x19C[0].z = 0x8000;
            rec->spin_dir_0x1B6 = 0;
            if (ran_suu(0) & 1) {
                rec->rise_0x1C8 = -2.0f;
            }
        }
    }
    rec->spin_0x19C[1].z = (u16)(0x10000 - rec->spin_0x19C[0].z);
    rec->spin_index_0x1B7 = 0;
    if ((ran_suu(0) & 3) == 0) {
        rec->sway_0x1CC = (u16)(rec->sway_0x1CC + 0x1000);
    }
    if ((ran_suu(0) & 0x1F) == 0) {
        rec->rot_0x020.x = em_pop_bob_tilt;
        rec->rot_0x020.y = ran_suu(0);
        rec->rot_0x020.z = 0;
    }
    s = math_sin_deg((360.0f * (f32)rec->sway_0x1CC) / 65536.0f);
    c = nw4r::math::CosDeg((360.0f * (f32)rec->sway_0x1CC) / 65536.0f);
    rec->pos_0x014.x = rec->aim_0x1D0.x + 20.0f * s;
    rec->pos_0x014.z = rec->aim_0x1D0.z + 20.0f * c;
    rec->pos_0x014.y += rec->rise_0x1C8;
    if (rec->pos_0x014.y < rec->aim_0x1D0.y + rec->height_0x1F4 / 3.0f && rec->rise_0x1C8 < 0.0f) {
        rec->rise_0x1C8 *= -1.0f;
    }
    if (rec->pos_0x014.y > rec->aim_0x1D0.y + rec->height_0x1F4 && rec->rise_0x1C8 > 0.0f) {
        rec->rise_0x1C8 *= -1.0f;
    }
}

} /* extern "C" */

/* The cosine of `deg` degrees. */
f32 nw4r::math::CosDeg(f32 deg) {
    return CosFIdx(deg * (256.0f / 360.0f));
}

extern "C" {

/* The glow step of the 0x81 models: it fades the glow in and out along its key frames and resets the scale. */
void em_pop_obj_glow_step(EmPopObj* obj) {
    s32 num;
    u32 i;
    f32** keys;
    EmPopColor* color;
    s8 rgba[4];

    if (obj->field_0x005 == 0x81) {
        u16 angle;

        obj->spin_0x19C[0].z = (u16)(obj->spin_0x19C[0].z - 0x400);
        angle = obj->spin_0x19C[1].z + obj->spin_speed_0x1B4;
        obj->spin_0x19C[1].z = angle;
        obj->scale_0x02C.z = 1.0f;
        obj->scale_0x02C.y = 1.0f;
        obj->scale_0x02C.x = 1.0f;
        if (angle >= 0x4000) {
            obj->visible_0x1C5 = 0;
            return;
        }
        obj->visible_0x1C5 = 1;
        num = ((nw4r::g3d::ResMdl*)&obj->model_0x038.field_0x114)->GetResMatNumEntries();
        i = 0;
        keys = em_pop_glow_key_tbl;
        color = em_pop_glow_color_tbl;
        for (; i < (u32)num; keys++, color++, i++) {
            rgba[3] = getKeyData(*keys, (f32)obj->frame_0x1C6);
            rgba[0] = color->r;
            rgba[1] = color->g;
            rgba[2] = color->b;
            obj->model_0x038.setMatColor(i, GX_COLOR0A0, *(GXColor*)rgba, 0);
        }
        return;
    }
    obj->visible_0x1C5 = 1;
    obj->scale_0x02C.z = 1.0f;
    obj->scale_0x02C.y = 1.0f;
    obj->scale_0x02C.x = 1.0f;
}

/* The pop-file point's step: it fades the model in, runs the spring or the bobbing/glowing model while the local
 * player is in its area, and fades it out once gathered. */
void em_pop_rec_step(EmPopObj* obj) {
    EmPopRec* rec = (EmPopRec*)obj;
    f32 alpha;

    switch (rec->field_0x008) {
    case 0:
        rec->field_0x008 = 1;
        rec->field_0x009 = 0;
        rec->field_0x00A = 0;
        rec->fade_0x00E = 0x14;
        rec->timer_0x220 = 0;
        if (rec->field_0x005 == 0xA) {
            rec->timer_0x00C = 1;
            break;
        }
        em_pop_obj_model_show(rec);
        break;
    case 1:
        if (rec->field_0x005 == 0xFF) {
            break;
        }
        if (rec->field_0x00A == 0) {
            if (--rec->fade_0x00E < 0) {
                rec->fade_0x00E = 0;
                rec->field_0x00A = 1;
            }
            alpha = 1.0f - (f32)rec->fade_0x00E / 20.0f;
            mhchar_mat_alpha_set(&rec->model_0x038, 2, (u8)(255.0f * alpha), 0);
            em_pop_obj_alpha_set(rec, alpha);
        }
        rec->in_area_0x001 = quest_move_area_sub_ck(rec->kind_0x004);
        if (rec->in_area_0x001 != 0) {
            switch (rec->field_0x005) {
            case 0xA:
                if (--rec->timer_0x00C <= 0) {
                    em_pop_spring_bubble_spawn(&rec->aim_0x1D0, rec->kind_0x004, rec->height_0x1F4);
                    rec->timer_0x00C = 0x14;
                }
                if ((++rec->timer_0x220 & 0x1F) == 0) {
                    se_pop_spring_req(&rec->pos_0x014);
                }
                break;
            case 0xFF:
                break;
            default:
                if (rec->model_0x038.field_0x118 != 0) {
                    em_pop_rec_bob_step(rec);
                    em_pop_obj_glow_step(rec);
                    if (rec->visible_0x1C5 != 0) {
                        copyVec3(&rec->model_0x038.pos_0x04, &rec->pos_0x014);
                        eft_rot_vec_copy(&rec->model_0x038.rot_0x28, &rec->rot_0x020);
                        copyVec3(&rec->model_0x038.scale_0x1C, &rec->scale_0x02C);
                        if (rec->field_0x005 == 0x81) {
                            rec->model_0x038.ready = 1;
                        } else {
                            rec->model_0x038.ready = em_pop_model_show_tbl[rec->field_0x005];
                        }
                        rec->model_0x038.move(0);
                        g3d_root_model_bind(pRoot, rec->model_0x038.field_0x118);
                    }
                }
                break;
            }
        }
        if (rec->slot_id_0x1FA == 0) {
            rec->field_0x008 = 2;
            rec->counter_0x010 = 0x5A;
            rec->field_0x00A = 0;
            rec->fade_0x00E = 0x14;
        }
        break;
    case 2:
        for (;;) {
            if (rec->field_0x005 == 0xA) {
                break;
            }
            rec->in_area_0x001 = quest_move_area_sub_ck(rec->kind_0x004);
            if (rec->in_area_0x001 == 0 || rec->model_0x038.field_0x118 == 0) {
                break;
            }
            if (--rec->counter_0x010 > 0) {
                if (rec->field_0x005 == 0xFF) {
                    return;
                }
                rec->fade_0x00E = 0x14;
                em_pop_rec_bob_step(rec);
                em_pop_obj_glow_step(rec);
                if (rec->visible_0x1C5 != 0) {
                    copyVec3(&rec->model_0x038.pos_0x04, &rec->pos_0x014);
                    eft_rot_vec_copy(&rec->model_0x038.rot_0x28, &rec->rot_0x020);
                    copyVec3(&rec->model_0x038.scale_0x1C, &rec->scale_0x02C);
                    if (rec->field_0x005 == 0x81) {
                        rec->model_0x038.ready = 1;
                    } else {
                        rec->model_0x038.ready = em_pop_model_show_tbl[rec->field_0x005];
                    }
                    rec->model_0x038.move(0);
                    g3d_root_model_bind(pRoot, rec->model_0x038.field_0x118);
                }
                return;
            }
            if (rec->field_0x005 == 0x81) {
                break;
            }
            if (--rec->fade_0x00E < 0) {
                rec->field_0x008 = 3;
                rec->fade_0x00E = 0;
                rec->field_0x00A = 1;
            }
            alpha = (f32)rec->fade_0x00E / 20.0f;
            mhchar_mat_alpha_set(&rec->model_0x038, 2, (u8)(255.0f * alpha), 0);
            em_pop_obj_alpha_set(rec, alpha);
            rec->model_0x038.ready = 1;
            rec->model_0x038.move(0);
            g3d_root_model_bind(pRoot, rec->model_0x038.field_0x118);
            return;
        }
        rec->field_0x008 = 3;
        break;
    }
}

/* The spare point's step: it waits out its respawn time, draws a placement and kind, fades the model in, runs it
 * while gathered from and fades it out again. */
void em_pop_spare_step(EmPopObj* obj) {
    EmPopRec* rec = (EmPopRec*)obj;
    f32 alpha = 1.0f;

    if (quest_move_area_sub_ck(rec->kind_0x004) == 0) {
        return;
    }
    switch (rec->field_0x008) {
    case 0:
        if (rec->field_0x21E == 0) {
            if (rec->timer_0x220 > 0) {
                rec->timer_0x220--;
                break;
            }
            rec->timer_0x220 = 0x708;
            if ((s32)ran_suu(0) % 100 < 50) {
                rec->field_0x21E = 1;
            } else {
                rec->field_0x21E = 0;
            }
            if (rec->field_0x21E == 0) {
                break;
            }
        }
        rec->field_0x008 = 1;
        rec->field_0x009 = 0;
        rec->field_0x00A = 0;
        rec->counter_0x010 = em_pop_spare_life;
        rec->fade_0x00E = 0x14;
        if (em_roster_spare_draw(rec) == 1) {
            switch (rec->field_0x005) {
            case 0xA:
                rec->timer_0x00C = 1;
                break;
            case 0x81:
                rec->timer_0x00C = (s16)(((s32)ran_suu(0) % 10) * 10);
                em_pop_mote_spawn(&rec->pos_0x014, rec->kind_0x004);
                rec->state_0x1F8 = 0;
                rec->state_0x1FC = 0;
                break;
            default:
                em_pop_obj_model_show(rec);
                break;
            }
        } else {
            rec->field_0x009 = 2;
        }
        break;
    case 1:
        switch (rec->field_0x009) {
        case 0:
            if (rec->field_0x00A == 0) {
                if (--rec->fade_0x00E < 0) {
                    rec->fade_0x00E = 0;
                    rec->field_0x00A = 1;
                }
                alpha = 1.0f - (f32)rec->fade_0x00E / 20.0f;
                mhchar_mat_alpha_set(&rec->model_0x038, 2, (u8)(255.0f * alpha), 0);
                em_pop_obj_alpha_set(rec, alpha);
            }
            if (rec->slot_id_0x1FA == 0) {
                rec->field_0x009 = 1;
                rec->field_0x00A = 0;
                rec->fade_0x00E = 0x14;
            }
            rec->life_0x1B8 = rec->counter_0x010;
            if (rec->field_0x005 != 0x81) {
                if (--rec->counter_0x010 < 0) {
                    rec->field_0x009 = 1;
                    rec->field_0x00A = 0;
                    rec->fade_0x00E = 0x14;
                    rec->life_0x1B8 = 0;
                }
                if (rec->field_0x005 == 0xA && (rec->timer_0x220 & 0x1F) == 0) {
                    se_pop_spring_req(&rec->pos_0x014);
                }
            }
            rec->in_area_0x001 = quest_move_area_sub_ck(rec->kind_0x004);
            if (rec->in_area_0x001 == 0) {
                break;
            }
            switch (rec->field_0x005) {
            case 0xA:
                if (--rec->timer_0x00C <= 0) {
                    em_pop_spring_bubble_spawn(&rec->aim_0x1D0, rec->kind_0x004, rec->height_0x1F4);
                    rec->timer_0x00C = (s16)((s32)ran_suu(0) % 10 + 20);
                }
                break;
            case 0x81:
                if (--rec->timer_0x00C <= 0) {
                    rec->timer_0x00C = (s16)((s32)ran_suu(0) % 8 + 8);
                    em_pop_mote_spawn(&rec->pos_0x014, rec->kind_0x004);
                }
                break;
            case 0xFF:
                break;
            default:
                if (rec->model_0x038.field_0x118 != 0) {
                    em_pop_rec_bob_step(rec);
                    em_pop_obj_glow_step(rec);
                    if (rec->visible_0x1C5 != 0) {
                        copyVec3(&rec->model_0x038.pos_0x04, &rec->pos_0x014);
                        eft_rot_vec_copy(&rec->model_0x038.rot_0x28, &rec->rot_0x020);
                        copyVec3(&rec->model_0x038.scale_0x1C, &rec->scale_0x02C);
                        if (rec->field_0x005 == 0x81) {
                            rec->model_0x038.ready = 1;
                        } else if (alpha < 1.0f) {
                            rec->model_0x038.ready = 1;
                        } else {
                            rec->model_0x038.ready = em_pop_model_show_tbl[rec->field_0x005];
                        }
                        rec->model_0x038.move(0);
                        g3d_root_model_bind(pRoot, rec->model_0x038.field_0x118);
                    }
                }
                break;
            }
            break;
        case 1:
            rec->counter_0x010--;
            rec->life_0x1B8 = 0;
            if (--rec->fade_0x00E < 0) {
                rec->fade_0x00E = 0;
                rec->field_0x009 = 2;
            }
            if (rec->field_0x005 == 0x81) {
                em_pop_obj_glow_step(rec);
                if (rec->visible_0x1C5 != 0) {
                    copyVec3(&rec->model_0x038.pos_0x04, &rec->pos_0x014);
                    eft_rot_vec_copy(&rec->model_0x038.rot_0x28, &rec->rot_0x020);
                    copyVec3(&rec->model_0x038.scale_0x1C, &rec->scale_0x02C);
                    rec->model_0x038.ready = 1;
                    rec->model_0x038.move(0);
                    g3d_root_model_bind(pRoot, rec->model_0x038.field_0x118);
                }
            } else {
                alpha = (f32)rec->fade_0x00E / 20.0f;
                mhchar_mat_alpha_set(&rec->model_0x038, 2, (u8)(255.0f * alpha), 0);
                em_pop_obj_alpha_set(rec, alpha);
                rec->model_0x038.ready = 1;
                rec->model_0x038.move(0);
                g3d_root_model_bind(pRoot, rec->model_0x038.field_0x118);
            }
            break;
        case 2:
            if (rec->field_0x005 != 0x81) {
                if (--rec->counter_0x010 < 0) {
                    rec->field_0x008 = 0;
                }
            }
            break;
        }
        break;
    }
}

/* Spawns the spring's bubbles at `pos`: one rising from below the spring (half its height down) and one orbiting
 * it. */
void em_pop_spring_bubble_spawn(nw4r::math::VEC3* pos, u8 area, f32 height) {
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 at;
    nw4r::math::VEC3 at2;
    EmPopWork* work;
    EmPopSubRec* sub;
    s32 i;

    VEC3_ctor(&ofs);
    work = em_pop_w.work;
    if (work == NULL) {
        return;
    }
    for (i = 0; i < 1; i++) {
        sub = em_roster_sub_free_get();
        if (sub != NULL) {
            em_pop_bubble_offset_get(&ofs, 50.0f);
            sub->state_0x000 = 1;
            sub->action_0x003 = 0;
            sub->kind_0x004 = area;
            sub->field_0x005 = 0xA;
            addVec3(&at, pos, &ofs);
            copyVec3(&sub->pos_0x014, &at);
            sub->pos_0x014.y -= height * 0.5f;
            mhchar_reset(&sub->model_0x038);
            mhchar_model_create(&sub->model_0x038, &work->res_file_0x00C, 0x68, 7);
            sub->step_0x1BC = em_pop_sub_bubble_step;
        }
    }
    for (; i < 2; i++) {
        sub = em_roster_sub_free_get();
        if (sub != NULL) {
            em_pop_bubble_offset_get(&ofs, 50.0f);
            sub->state_0x000 = 1;
            sub->action_0x003 = 1;
            sub->kind_0x004 = area;
            sub->field_0x005 = 0xB;
            addVec3(&at2, pos, &ofs);
            copyVec3(&sub->pos_0x014, &at2);
            mhchar_reset(&sub->model_0x038);
            mhchar_model_create(&sub->model_0x038, &work->res_file_0x00C, 0x68, 7);
            sub->step_0x1BC = em_pop_sub_bubble_step;
        }
    }
}

/* A random offset within `radius` of the origin. */
void em_pop_bubble_offset_get(nw4r::math::VEC3* out, f32 radius) {
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    v.z = 0.0f;
    v.y = 0.0f;
    v.x = 0.0f;
    vec3_random_offset(&v, &v, radius, 0);
    copyVec3(out, &v);
}

/* A spring bubble's step: it picks its rise speed, direction and wobble, then rises (orbiting for the second
 * bubble) until its time runs out. */
void em_pop_sub_bubble_step(EmPopObj* obj) {
    EmPopSubRec* sub = (EmPopSubRec*)obj;
    nw4r::math::VEC3 scale;
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 at;
    f32 half;
    f32 speed;

    switch (sub->field_0x008) {
    case 0:
        if (sub->model_0x038.field_0x118 == 0) {
            em_roster_sub_record_clear(sub);
            break;
        }
        sub->field_0x008++;
        if (sub->action_0x003 == 0) {
            sub->speed_0x1D4 = 2.0f * (0.5f + 0.1f * (f32)((s32)ran_suu(0) % 6));
            sub->tilt_0x1E8 = ((s32)ran_suu(0) % 16) * 0xE0 - 0x700;
            sub->turn_0x1EC = (u16)(((s32)ran_suu(0) % 32) * 0x800);
            sub->vel_0x1C8.y = sub->speed_0x1D4 * em_pop_bubble_speed_tbl[(s32)ran_suu(0) % 4];
            sub->vel_0x1C8.z = 0.0f;
            sub->vel_0x1C8.x = 0.0f;
        } else {
            sub->speed_0x1D4 = 1.5f;
            sub->tilt_0x1E8 = ((s32)ran_suu(0) % 16) * 0x300 - 0x1800;
            sub->turn_0x1EC = (u16)(((s32)ran_suu(0) % 32) * 0x800);
            sub->vel_0x1C8.y = 12.0f;
            sub->vel_0x1C8.z = 0.0f;
            sub->vel_0x1C8.x = 0.0f;
        }
        sub->wobble_x_0x1D8 = (u16)(((s32)ran_suu(0) % 32) * 0x800);
        sub->wobble_z_0x1DC = (u16)(((s32)ran_suu(0) % 32) * 0x800);
        rotVecX(&sub->vel_0x1C8, sub->tilt_0x1E8);
        rotVecY(&sub->vel_0x1C8, sub->turn_0x1EC);
        sub->orbit_0x1F0 = 0;
        sub->counter_0x010 = em_pop_bubble_wait_tbl[(s32)ran_suu(0) % 4];
        sub->field_0x009 = 0;
        em_pop_obj_model_show(sub);
        break;
    case 1:
        sub->in_area_0x001 = quest_move_area_sub_ck(sub->kind_0x004);
        if (sub->in_area_0x001 == 0) {
            sub->field_0x008++;
            break;
        }
        switch (sub->field_0x009) {
        case 0:
            if (--sub->counter_0x010 > 0) {
                break;
            }
            sub->field_0x009++;
            sub->counter_0x010 = em_pop_bubble_wait_tbl[(s32)ran_suu(0) % 4] + 0x23;
            if (sub->action_0x003 == 0) {
                sub->counter_0x010 += 5;
            }
        case 1:
            if (--sub->counter_0x010 > 0) {
                if (sub->field_0x005 != 0xFF) {
                    VEC3_ctor(&scale);
                    VEC3_ctor(&ofs);
                    scale.x = math_sin_deg((360.0f * (f32)sub->wobble_x_0x1D8) / 65536.0f);
                    scale.y = math_sin_deg((360.0f * (f32)sub->wobble_z_0x1DC) / 65536.0f);
                    scale.z = sub->speed_0x1D4;
                    if (scale.x < 0.0f) {
                        scale.x *= -1.0f;
                    }
                    if (scale.y < 0.0f) {
                        scale.y *= -1.0f;
                    }
                    speed = sub->speed_0x1D4;
                    half = speed * 0.5f;
                    scale.x = speed - scale.x * half;
                    scale.y = speed - scale.y * half;
                    sub->wobble_x_0x1D8 = (u16)(sub->wobble_x_0x1D8 + 0x1000);
                    sub->wobble_z_0x1DC = (u16)(sub->wobble_z_0x1DC + 0x1000);
                    addVec3To(&sub->pos_0x014, &sub->vel_0x1C8);
                    if (sub->field_0x005 == 0x81) {
                        sub->model_0x038.ready = 1;
                    } else {
                        sub->model_0x038.ready += em_pop_model_show_tbl[sub->field_0x005];
                    }
                    if (sub->action_0x003 == 1) {
                        sub->orbit_0x1F0 = (u16)(sub->orbit_0x1F0 + 0x2000);
                        ofs.x = 5.0f;
                        ofs.z = 0.0f;
                        ofs.y = 0.0f;
                        rotVecY(&ofs, sub->orbit_0x1F0);
                        rotVecX(&ofs, sub->tilt_0x1E8);
                        rotVecY(&ofs, sub->turn_0x1EC);
                        addVec3(&at, &sub->pos_0x014, &ofs);
                        copyVec3(&sub->model_0x038.pos_0x04, &at);
                    } else {
                        copyVec3(&sub->model_0x038.pos_0x04, &sub->pos_0x014);
                    }
                    copyVec3(&sub->model_0x038.scale_0x1C, &scale);
                    sub->model_0x038.ready = em_pop_model_show_tbl[sub->field_0x005];
                }
                sub->model_0x038.move(0);
                g3d_root_model_bind(pRoot, sub->model_0x038.field_0x118);
                break;
            }
            sub->field_0x008++;
            break;
        }
        break;
    case 2:
        push_g3d_wk(sub->model_0x038.g3d_0x10C);
        sub->field_0x008++;
    case 3:
        em_roster_sub_record_clear(sub);
        break;
    }
}

} /* extern "C" */

/* Binds the owner's scene model to this callback at node `node` (0x80 and up: node 1). */
void EmModelCallback::bind(u8 node) {
    nw4r::g3d::ScnMdlSimple* scn = (nw4r::g3d::ScnMdlSimple*)owner_0x08->model_0x038.field_0x118;

    if (scn != NULL) {
        scn->EnableScnMdlCallbackTiming((nw4r::g3d::ScnObj::Timing)1);
        if (node & 0x80) {
            node = 1;
        }
        scn->SetCalcWorldNodeID(node);
    }
}

/* Before each of the bound nodes: swings the node by the owner's spin (mode 0), or turns it to the camera and
 * scales it along the node's key frames (the glow). */
void EmModelCallback::ExecCallbackA(nw4r::g3d::ChrAnmResult* result, nw4r::g3d::ResMdl mdl,
                                    nw4r::g3d::FuncObjCalcWorld* func) {
    EmPopObj* obj = owner_0x08;
    nw4r::math::MTX34 rot;
    nw4r::math::MTX34 view;
    nw4r::math::MTX33 rot33;
    nw4r::math::VEC3 scale;
    u32 node;
    f32* keys;

    MTX34_ctor(&rot);
    MTX34_ctor(&view);
    MTX33_ctor(&rot33);
    VEC3_ctor(&scale);
    node = func->GetCallbackNodeID();
    if (obj->callback_mode_0x1C4 == 0) {
        cpSetRotMatrix(&obj->spin_0x19C[obj->spin_index_0x1B7], &rot);
        nw4r::math::MTX34ToMTX33(&rot33, &rot);
        anm_chr_result_rot_mul(result, (MTX34*)&rot33);
        if (obj->spin_index_0x1B7 == 0) {
            obj->spin_index_0x1B7++;
            func->SetCallbackNodeID(obj->field_0x005 + 2);
        }
        return;
    }
    if (node == 1) {
        const nw4r::math::MTX34& cam = get_current_view_mtx();
        mtx34_copy(&view, (nw4r::math::MTX34*)&cam);
        mtx34_inverse(&view, &view);
        mtx34_identity(&rot);
        mtx34_concat_assign(&rot, &view);
        rotLocalMatZ(obj->spin_0x19C[0].z, &rot);
        nw4r::math::MTX34ToMTX33(&rot33, &rot);
        anm_chr_result_rot_mul(result, (MTX34*)&rot33);
    }
    keys = em_pop_node_key_tbl[node - 1];
    if (node == 4) {
        scale.x = getKeyData(keys, (f32)obj->frame_0x1C6);
        scale.z = 1.0f;
        scale.y = 1.0f;
    } else {
        scale.y = scale.x = getKeyData(keys, (f32)obj->frame_0x1C6);
        scale.z = 1.0f;
    }
    scale.x *= em_pop_node_scale_tbl[node - 1].x;
    scale.y *= em_pop_node_scale_tbl[node - 1].y;
    anm_chr_result_scale_set((struct ResAnmChrObj*)result, &scale.x);
    if (node < 4) {
        func->SetCallbackNodeID(node + 1);
    }
}

extern "C" {

/* Binds a free model callback to `obj` (mode 0 reuses the one already bound to it) and starts its swing (mode 0)
 * or its glow scale. */
void em_pop_obj_callback_bind(EmPopObj* obj, u8 mode) {
    s32 i;
    nw4r::g3d::ScnMdlSimple* scn;

    if (mode == 0) {
        for (i = 0; i < 0x20; i++) {
            if (em_handle_tbl[i] != NULL && em_handle_tbl[i]->in_use_0x04 != 0 && em_handle_tbl[i]->owner_0x08 == obj) {
                break;
            }
        }
    }
    if (mode != 0 || i >= 0x20) {
        for (i = 0; i < 0x20; i++) {
            if (em_handle_tbl[i] != NULL && em_handle_tbl[i]->in_use_0x04 == 0) {
                scn = (nw4r::g3d::ScnMdlSimple*)obj->model_0x038.field_0x118;
                scn->SetCalcWorldCallback(em_handle_tbl[i]);
                em_handle_tbl[i]->in_use_0x04 = 1;
                em_handle_tbl[i]->owner_0x08 = obj;
                obj->callback_mode_0x1C4 = mode;
                break;
            }
        }
        if (i >= 0x20) {
            return;
        }
    }
    if (obj->callback_mode_0x1C4 == 0) {
        em_handle_tbl[i]->bind(obj->field_0x005 + 1);
        obj->spin_index_0x1B7 = 0;
        obj->spin_dir_0x1B6 = 0;
        obj->spin_speed_0x1B4 = 0x1800;
        obj->spin_0x19C[0].x = 0;
        obj->spin_0x19C[0].y = 0;
        if ((s32)ran_suu(0) % 2 != 0) {
            obj->spin_0x19C[0].z = 0x8000;
            obj->spin_dir_0x1B6 = 1;
        }
    } else {
        em_handle_tbl[i]->bind(obj->field_0x005);
        obj->spin_speed_0x1B4 = 0x400;
        obj->spin_0x19C[0].x = 0;
        obj->spin_0x19C[0].y = 0;
        obj->spin_0x19C[0].z = 0;
        obj->spin_0x19C[1].z = 0;
    }
}

/* Drops every handle-table slot that still names `obj`. */
void em_roster_record_unlink(EmPopObj* obj) {
    u32 i;
    for (i = 0; i < 0x20; i++) {
        if (em_handle_tbl[i] != NULL && em_handle_tbl[i]->in_use_0x04 != 0 && em_handle_tbl[i]->owner_0x08 == obj) {
            em_handle_tbl[i]->in_use_0x04 = 0;
            em_handle_tbl[i]->owner_0x08 = NULL;
        }
    }
}

/* Spawns a glow mote of area `area` at `pos`. */
void em_pop_mote_spawn(nw4r::math::VEC3* pos, u8 area) {
    EmPopWork* work = em_pop_w.work;
    EmPopSubRec* sub;

    if (work != NULL) {
        sub = em_roster_sub_free_get();
        if (sub != NULL) {
            sub->state_0x000 = 1;
            sub->action_0x003 = 0;
            sub->kind_0x004 = area;
            sub->field_0x005 = 0x81;
            copyVec3(&sub->pos_0x014, pos);
            copyVec3(&sub->base_0x200, pos);
            sub->frame_0x1C6 = 0;
            mhchar_reset(&sub->model_0x038);
            mhchar_model_create(&sub->model_0x038, &work->res_file_0x018, 0x64, 7);
            sub->step_0x1BC = em_pop_sub_mote_step;
        }
    }
}

/* A glow mote's step: it scatters around its spawn point, then glows along its key frames until the glow ends. */
void em_pop_sub_mote_step(EmPopObj* obj) {
    EmPopSubRec* sub = (EmPopSubRec*)obj;
    f32 x;
    f32 y;
    f32 z;

    switch (sub->field_0x008) {
    case 0:
        if (sub->model_0x038.field_0x118 == 0) {
            em_roster_sub_record_clear(sub);
            break;
        }
        sub->field_0x008++;
        copyVec3(&sub->pos_0x014, &sub->base_0x200);
        x = (f32)((s32)ran_suu(0) % 50);
        if (ran_suu(0) & 1) {
            x *= -1.0f;
        }
        y = (f32)((s32)ran_suu(0) % 100);
        z = (f32)((s32)ran_suu(0) % 50);
        if (ran_suu(0) & 1) {
            z *= -1.0f;
        }
        sub->pos_0x014.x += x;
        sub->pos_0x014.y += y;
        sub->pos_0x014.z += z;
        em_pop_obj_model_show(sub);
        sub->frame_0x1C6 = 0;
        sub->field_0x009 = 0;
        break;
    case 1:
        sub->in_area_0x001 = quest_move_area_sub_ck(sub->kind_0x004);
        if (sub->in_area_0x001 == 0) {
            sub->field_0x008 = 2;
            break;
        }
        em_pop_obj_glow_step(sub);
        if (sub->visible_0x1C5 == 0) {
            sub->field_0x008 = 2;
            break;
        }
        sub->frame_0x1C6++;
        copyVec3(&sub->model_0x038.pos_0x04, &sub->pos_0x014);
        eft_rot_vec_copy(&sub->model_0x038.rot_0x28, &sub->rot_0x020);
        copyVec3(&sub->model_0x038.scale_0x1C, &sub->scale_0x02C);
        sub->model_0x038.ready = 1;
        sub->model_0x038.move(0);
        g3d_root_model_bind(pRoot, sub->model_0x038.field_0x118);
        break;
    case 2:
        sub->field_0x008 = 3;
    case 3:
        em_roster_sub_record_clear(sub);
        break;
    }
}

#pragma dont_inline on
/* Adds a roster record for the pop-file point `entry` of `area`; NULL when the work or the pool is unavailable. */
EmPopRec* em_roster_pop_entry_add(const EmPopEntry* entry, u8 area) {
    nw4r::g3d::ResMdl mdl(NULL);
    nw4r::g3d::ResMat mat(NULL);
    EmPopWork* work;
    EmPopRec* rec;
    u8 map;
    const EmPopFixedPoint* fixed;
    nw4r::math::VEC3 at;
    s32 num;
    s32 i;
    const char* name;
    QuestRecord* quest;

    work = em_pop_w.work;
    if (work == NULL) {
        return NULL;
    }
    rec = em_roster_free_record_get();
    if (rec == NULL) {
        return NULL;
    }
    get_move_work_adrs(0);
    map = pop_map_kind_get();
    rec->state_0x000 = 1;
    rec->in_area_0x001 = 0;
    rec->action_0x003 = 0;
    rec->field_0x006 = 0;
    rec->kind_0x004 = area;
    copyVec3(&rec->aim_0x1D0, &entry->pos_0x00);
    eft_rot_vec_copy(&rec->aim_rot_0x1DC, (_CP_VECTOR*)&entry->rot_0x0C);
    copyVec3(&rec->pos_0x014, &entry->pos_0x00);
    eft_rot_vec_copy(&rec->rot_0x020, (_CP_VECTOR*)&entry->rot_0x0C);
    rec->radius_0x1F0 = entry->radius_0x18;
    rec->height_0x1F4 = entry->height_0x1C;
    rec->state_0x1F8 = entry->point_0x20;
    rec->slot_id_0x1FA = entry->count_0x24;
    rec->state_0x1FC = entry->point_kind_0x22;
    rec->bonus_0x1FE = entry->bonus_0x26;
    rec->uses_0x204 = entry->uses_0x28;
    rec->uses_max_0x206 = entry->uses_max_0x2A;
    rec->drops_0x1EC = pop_place_item_tbl[map][entry->item_0x30];
    switch (rec->state_0x1F8) {
    case 5:
    case 6:
    case 7:
        rec->field_0x005 = 0xFF;
        break;
    case 8:
        if (move_work_state_ck() == 0) {
            quest = quest_record_get();
            if (quest != NULL) {
                fixed = &em_pop_fixed_point_tbl[quest->field_0x08B];
            } else {
                fixed = &em_pop_fixed_point_tbl[map];
            }
        } else {
            fixed = &em_pop_fixed_point_tbl[1];
        }
        copyVec3(&rec->aim_0x1D0, setVec3(&at, fixed->pos.x, fixed->pos.y, fixed->pos.z));
        copyVec3(&rec->pos_0x014, &rec->aim_0x1D0);
        rec->aim_rot_0x1DC.z = 0;
        rec->aim_rot_0x1DC.x = 0;
        rec->aim_rot_0x1DC.y = fixed->rot_y;
        eft_rot_vec_copy(&rec->rot_0x020, &rec->aim_rot_0x1DC);
        rec->field_0x005 = 0xFF;
        break;
    case 0xA:
    case 0x10:
        rec->field_0x005 = 0xFF;
        break;
    case 0x11:
        rec->field_0x21E = work->field_0x34A46;
        work->field_0x34A46++;
        rec->field_0x005 = 0xFF;
        break;
    default:
        if (entry->model_0x2C == 0xFF) {
            rec->field_0x005 = 0xFF;
        } else if (map <= 1) {
            if (entry->model_0x2C == 0x12) {
                rec->field_0x005 = 0x81;
            } else {
                rec->field_0x005 = em_pop_model_kind_tbl[entry->model_0x2C];
            }
        } else if (map == 6 && entry->model_0x2C == 0x13) {
            rec->field_0x005 = 0xFF;
        } else {
            rec->field_0x005 = em_pop_model_kind_tbl[entry->model_0x2C];
        }
        break;
    }
    if (rec->field_0x005 != 0xFF) {
        mhchar_reset(&rec->model_0x038);
        if (rec->field_0x005 == 0xD) {
            rec->field_0x005 = 0xC;
        }
        if (rec->field_0x005 == 0x11) {
            rec->field_0x005 = 0x10;
        }
        if (rec->field_0x005 == 0xB) {
            rec->field_0x005 = 0xA;
        }
        if (rec->field_0x005 == 0x81) {
            mhchar_model_create(&rec->model_0x038, &work->res_file_0x018, 0x64, 7);
        } else {
            mhchar_model_create(&rec->model_0x038, &work->res_file_0x00C, 0x6C, 7);
            mdl = *(nw4r::g3d::ResMdl*)&rec->model_0x038.field_0x114;
            num = mdl.GetResMatNumEntries();
            for (i = 0; i < num; i++) {
                mat = mdl.GetResMat((u32)i);
                name = res_mat_name_get(&mat);
                if (name != NULL && strcmp(name, em_pop_lure_mat_name) == 0) {
                    rec->mat_0x21D = i;
                    break;
                }
            }
        }
    }
    rec->step_0x1BC = em_pop_rec_step;
    rec->on_ground_0x21C = 0;
    return rec;
}

/* Adds a roster record for the spare record `src` of `area`, kept in the work's spare table at `slot` (slots 0x14
 * and up are the glow motes). */
EmPopRec* em_roster_spare_entry_add(EmPopSpare* src, s32 area, s32 slot) {
    nw4r::g3d::ResMdl mdl(NULL);
    nw4r::g3d::ResMat mat(NULL);
    EmPopWork* work;
    EmPopRec* rec;
    EmPopSpare* spare;
    s32 num;
    s32 i;
    const char* name;

    work = em_pop_w.work;
    if (work == NULL) {
        return NULL;
    }
    work->spare_num_0x34834[area][slot]++;
    rec = em_roster_free_record_get();
    if (rec == NULL) {
        return NULL;
    }
    spare = &work->spares_0x034[area][slot];
    em_roster_record_copy(spare, src);
    rec->state_0x000 = 1;
    rec->in_area_0x001 = 0;
    rec->action_0x003 = 0;
    rec->field_0x006 = 1;
    rec->kind_0x004 = area;
    rec->spare_0x1E8 = spare;
    rec->mat_0x21D = -1;
    if (rec->field_0x005 != 0xFF) {
        mhchar_reset(&rec->model_0x038);
        if (slot >= 0x14) {
            mhchar_model_create(&rec->model_0x038, &work->res_file_0x018, 0x64, 7);
        } else {
            mhchar_model_create(&rec->model_0x038, &work->res_file_0x00C, 0x6C, 7);
            mdl = *(nw4r::g3d::ResMdl*)&rec->model_0x038.field_0x114;
            num = mdl.GetResMatNumEntries();
            for (i = 0; i < num; i++) {
                mat = mdl.GetResMat((u32)i);
                name = res_mat_name_get(&mat);
                if (name != NULL && strcmp(name, em_pop_lure_mat_name) == 0) {
                    rec->mat_0x21D = i;
                    break;
                }
            }
        }
    }
    rec->step_0x1BC = em_pop_spare_step;
    if ((s32)ran_suu(0) % 100 < 50) {
        rec->field_0x21E = 1;
    } else {
        rec->field_0x21E = 0;
    }
    rec->timer_0x220 = 0x708;
    return rec;
}

#pragma dont_inline off
/* Copies a spare record (the part the takeover path moves). */
EmPopSpare* em_roster_record_copy(EmPopSpare* dst, const EmPopSpare* src) {
    *dst = *src;
    return dst;
}

/* Resets the spare-record pool and, in the free hunt, draws how many spares of each list will spawn. */
void em_pop_spare_init(void) {
    EmPopWork* work = em_pop_w.work;
    s32 i;
    s32 j;
    s32 k;

    if (work != NULL) {
        for (i = 0; i < 5; i++) {
            work->field_0x34A34[i] = 0;
            work->field_0x34A39[i] = 0;
        }
        work->spare_tbl_0x34A40 = (EmPopSpare**)work_mem_alloc(0x200);
        work->field_0x34A44 = 0;
        work->field_0x34A45 = 0;
        work->field_0x34A47 = 0;
        work->field_0x34A48 = 0;
        for (i = 0; i < 2; i++) {
            for (j = 0; j < 3; j++) {
                for (k = 0; k < 5; k++) {
                    work->field_0x34A49[i][j * 5 + k] = -1;
                }
            }
        }
        if (move_work_state_ck() != 0) {
            work->field_0x34A47 = ran_suu(0) % 3 + 1;
            if ((u32)em_set_userdata_flag_ck() == 1) {
                work->field_0x34A48 = (s32)ran_suu(0) % 2 + 1;
            }
        }
    }
}

/* Pushes the spare record `spare` into the work's spare table and its index into the free-hunt list of `slot`. */
void em_pop_spare_push(EmPopSpare* spare, u8 area, s32 slot) {
    EmPopWork* work;
    s32 list;
    s32 n;

    if (move_work_state_ck() != 0 && em_pop_w.work->field_0x34A44 < 0x7F) {
        work = em_pop_w.work;
        list = (u32)(slot - 0x14) > 2;
        for (n = 0; n < 15; n++) {
            if (work->field_0x34A49[list][n] == -1) {
                break;
            }
        }
        if (n < 15) {
            work->field_0x34A49[list][n] = work->field_0x34A44;
            work->spare_tbl_0x34A40[work->field_0x34A44] = spare;
            work->field_0x34A44++;
        }
    }
}

/* Spawns the drawn spares of both lists, each at a random pushed spare of its list. */
void em_pop_spare_spawn(void) {
    EmPopWork* work;
    EmPopSpare** tbl;
    EmPopSpare* spare;
    s32 i;

    if (move_work_state_ck() != 0) {
        work = em_pop_w.work;
        tbl = work->spare_tbl_0x34A40;
        if (tbl == NULL) {
            return;
        }
        while (work->field_0x34A47 > 0) {
            for (i = 0; i < 15; i++) {
                if (work->field_0x34A49[0][i] != -1 && (ran_suu(0) & 1) == 0) {
                    spare = tbl[work->field_0x34A49[0][i]];
                    if (em_roster_spare_entry_add(spare, spare->area_0x002, work->field_0x34A45 + 0x14) != NULL) {
                        work->field_0x34A45++;
                        work->field_0x34A47--;
                        work->field_0x34A49[0][i] = -1;
                        break;
                    }
                }
            }
        }
        while (work->field_0x34A48 > 0) {
            for (i = 0; i < 15; i++) {
                if (work->field_0x34A49[1][i] != -1 && (ran_suu(0) & 1) == 0) {
                    spare = tbl[work->field_0x34A49[1][i]];
                    if (em_roster_spare_entry_add(spare, spare->area_0x002, work->field_0x34A45 + 0x14) != NULL) {
                        work->field_0x34A45++;
                        work->field_0x34A48--;
                        work->field_0x34A49[1][i] = -1;
                        break;
                    }
                }
            }
        }
    }
}

/* Returns the live roster record at `index`, or NULL when the slot is out of range or not live. */
EmPopRec* em_roster_record_get(u32 index) {
    EmPopWork* work = em_pop_w.work;
    if ((s32)index >= (s32)work->rec_num) {
        return NULL;
    }
    EmPopRec* rec = &work->recs[index];
    if (rec->state_0x000 == 1) {
        return rec;
    }
    return NULL;
}

/* The index of the `kind` records (live, in an accepted state) into `out` until `max` are found; returns how
 * many matched. */
u8 em_roster_kind_collect(u8 kind, u8* out, u8 max) {
    EmPopWork* work = em_pop_w.work;
    EmPopRec* rec = work->recs;
    s32 i;
    s32 count = 0;
    for (i = 0; i < (s32)work->rec_num; rec++, i++) {
        if (rec->state_0x000 == 0) {
            continue;
        }
        if (rec->kind_0x004 != kind) {
            continue;
        }
        if (rec->field_0x006 == 0 && rec->field_0x008 != 1) {
            continue;
        }
        if (rec->field_0x006 == 1 && (rec->field_0x008 != 1 || rec->field_0x009 != 0)) {
            continue;
        }
        if (rec->field_0x005 > 1 && (u8)(rec->field_0x005 + 252) > 1 && (u8)(rec->field_0x005 + 242) > 1) {
            continue;
        }
        if (out != NULL) {
            *out = (u8)i;
            out++;
            count++;
            if (count >= max) {
                return (u8)count;
            }
        } else {
            count++;
        }
    }
    return (u8)count;
}

/* The aim position of record `index` when it is live, of `kind` and in an accepted state; else NULL. */
nw4r::math::VEC3* em_roster_kind_aim_pos_get(u8 index, u8 kind) {
    EmPopRec* rec = &em_pop_w.work->recs[index];
    if (rec->state_0x000 == 0) {
        return NULL;
    }
    if (rec->kind_0x004 != kind) {
        return NULL;
    }
    if (rec->field_0x006 == 0 && rec->field_0x008 != 1) {
        return NULL;
    }
    if (rec->field_0x006 == 1 && (rec->field_0x008 != 1 || rec->field_0x009 != 0)) {
        return NULL;
    }
    return &rec->aim_0x1D0;
}

/* The +0x005 byte of record `index` under the same acceptance test; -1 when it fails. */
s32 em_roster_kind_field5_get(u8 index, u8 kind) {
    EmPopRec* rec = &em_pop_w.work->recs[index];
    if (rec->state_0x000 == 0) {
        return -1;
    }
    if (rec->kind_0x004 != kind) {
        return -1;
    }
    if (rec->field_0x006 == 0 && rec->field_0x008 != 1) {
        return -1;
    }
    if (rec->field_0x006 == 1 && (rec->field_0x008 != 1 || rec->field_0x009 != 0)) {
        return -1;
    }
    return (s8)rec->field_0x005;
}

/* Releases map kind `kind`'s population files from resource memory (the second only in the free hunt). */
void em_kind_res_release(u8 kind) {
    EmPopResFiles* files;
    s32 index;

    if (PlayMode_ck() != 2) {
        files = stage_pop_res_tbl[kind];
        index = res_mem_index_get(files->name_0x04);
        if (index != -1) {
            push_res_mem(index);
        }
        if (move_work_state_ck() == 1) {
            index = res_mem_index_get(files->name_0x0C);
            if (index != -1) {
                push_res_mem(index);
            }
        }
    }
}

/* Scales every material's alpha-compare references by `alpha`. */
void em_pop_obj_alpha_set(EmPopObj* obj, f32 alpha) {
    nw4r::g3d::ResMatPix pix(NULL);
    MHchar* model;
    u32 num;
    u32 i;
    _GXCompare comp0;
    u8 ref0;
    _GXAlphaOp op;
    _GXCompare comp1;
    u8 ref1;

    model = &obj->model_0x038;
    num = mhchar_mat_num_get(model);
    for (i = 0; i < num; i++) {
        nw4r::g3d::ResMat mat = ((nw4r::g3d::ResMdl*)&model->field_0x114)->GetResMat(i);
        pix = mat.GetResMatPix();
        pix.GXGetAlphaCompare(&comp0, &ref0, &op, &comp1, &ref1);
        if (ref0 != 0 || ref1 != 0xFF) {
            mhchar_mat_tev_set(model, i, comp0, (u8)((f32)ref0 * alpha), op, comp1, (u8)((f32)ref1 * alpha), 0.0f);
        }
    }
}

} /* extern "C" */

extern "C" {

/* .sbss 0x80794C50/0x80794C54 - per enemy kind, the carve-point table and the drop-table base index. */
EmCarveEntry** em_hagi_tbl;
u16* em_drop_tbl;

/* .sbss 0x80794C58 - the manager work pointer, written whole by the work allocator. */
EmPopWorkSlot em_pop_w;

/* .bss 0x806D2A68 - per quest slot, the drop tables by item index. */
const EmWeightEntry** m_item_tbl[4];

/* .bss 0x806D2A78 - the 32-slot model-callback table. */
EmModelCallback* em_handle_tbl[0x20];

} /* extern "C" */
