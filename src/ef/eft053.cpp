/* ef/eft053.cpp - the eft053 effect family: one `_EFT` instance driven through its `work_0x38` model block, the
 *   shell-actor lookups it reads from a `_PLW`, its per-state machine and its four slot programs (swing, lever, rotation,
 *   rank gimmick) with their two movement helpers.
 * RANGE. .text 0x80366618-0x8036A828 (18 functions); extab 0x800177D4-0x80017844, extabindex 0x80037224-0x800372CC,
 *   .rodata 0x80570A10-0x80570A20 (`eft053_slot_gimmick_step`'s shot angle initialiser), .data 0x805EDAE0-0x805EE078,
 *   .sdata2 0x8079B740-0x8079B820 (the range's own pool).  The right edge 0x8036A828, where `menu/menu_item_sub.cpp`
 *   starts, is the registered cut, not a proven seam.
 * FLAGS. `cflags_main` with `#pragma optimization_level 4`, `peephole off` and `fp_contract off` for the whole file:
 *   retail keeps the masked narrow stores (`clrlwi` before `sth`), the unfused `clrlwi` + `cmpwi` and `fmuls` + `fsubs`,
 *   with the three no row scores lower than without them (10 rows gain); `pool_data off` keeps each `.data` table's
 *   own `lis`/`addi` (with the tables defined above their users, MWCC otherwise pools one `.data` base register).
 * NAMES. `eft053_get_shell_data` and `eft053_get_model_ang` are the runtime dump's own names; every other definition is
 *   a GUESS named `eft053_<what the body does>` in `ef/eft052.cpp`'s scheme (`eft053_set` seeds `_EFT::field_0x03 = 53`):
 *   `eft053_shell_pos_project` (projects the shell frame into a `VEC3`; `hud/cockpit_quest.cpp`'s `fn_802EF0A0` uses
 *   the result as a position), `eft053_model_list_get` (map and area to the `eft053_model_lists` model list and count),
 *   `eft053_work_act_ck` (`em_act_ck(work, 13, 0|2|3|4)`), `eft053_set` (the creation entry), `eft053_release`/
 *   `eft053_dispatch` (its `+0x40` and `+0x34` hooks), `eft053_init`/`eft053_run`/`eft053_state_advance`/
 *   `eft053_destroy` (the dispatch's four states), `eft053_slot_swing_step` (sub-kind 1: the timed swing program),
 *   `eft053_slot_lever_step` (sub-kinds 2-4: the player-turned lever), `eft053_slot_rot_step` (sub-kind 6),
 *   `eft053_slot_gimmick_step` (sub-kind 7: the em025 rank gimmick), `eft053_slot_move_step`/`eft053_slot_steer_step`
 *   (one slot's straight and steered move toward a `Vec` table entry).  Foreign GUESSes proposed in the name registry:
 *   `pl_act_held_class_get` (0x8027DFE4, `Pl/pl_act.cpp`), `eft026_ramp_place_set` (0x80117EFC, `ef/eft026_fx.cpp`).
 *   Data GUESSes from their readers: `eft053_models_map<M>_area<A>` (the model list `eft053_model_list_get` picks for
 *   map M, area A), `eft053_model_lists`, `eft053_lever_offset` (the lever head offset), `eft053_place_table` (the rank
 *   gimmicks' stations).
 * RESIDUALS. Every row is written; 11 of 18 match.  The open rows:
 *  - `eft053_slot_move_step`: in states 1 and 4 retail keeps `0.5f` and the conversion constant in `f2`/`f3` across
 *    the position store where ours reloads them, and converts the counter before dividing the distance by the step;
 *  - `eft053_slot_lever_step`, `eft053_slot_gimmick_step`, `eft053_init`: register colouring; part of it is
 *    `ef/get_move_work_adrs.h` declaring `get_move_work_max` as returning `u32` where its owner returns `u16` (with a
 *    `u16` view the three rows measure 96.71 / 97.82 / 99.57 instead of 96.28 / 96.57 / 99.01; request filed);
 *  - `eft053_shell_pos_project`: frame 0x70 against retail's 0x80 (retail gives each arm of the angle test its own
 *    conversion temporaries) and `mr r3,r31` scheduled before the first float argument;
 *  - `eft053_slot_swing_step`: retail evaluates `eft028_set_scaled`'s `variant` before its `scale` (the
 *    `lobby/lb_quest_ui.cpp` call sites show the same), which the owner header's parameter order (scale, variant)
 *    does not reproduce; one `fmuls` operand order in state 2;
 *  - `eft053_get_shell_data`: the `vec3_scale_in_place` pointer argument is copied before the scale, retail after.
 *   Data: `.data` is emitted and matches (the 7 model lists, their pointer table, the two `Vec` tables, the six jump
 *   tables in retail order; the claim's last 4 bytes are dtk's trailing gap); `.rodata` is the shot angle initialiser
 *   (12 of 16 B, retail pads the section); the `.sdata2` pool is declared `extern` (`lbl_8079B740`..), not emitted, so
 *   the object carries its own 0x30-byte pool and is short of the claim there; extab/extabindex follow the function
 *   sizes.  Not flippable until the open rows and the pool close.
 * SHAPES. `eft053_shell_pos_project` keeps both arms of the angle test: with one shared arm MWCC if-converts them.
 *   `eft053_slot_swing_step`'s state 4 falls into state 5 and `eft053_slot_rot_step`'s state 5 into state 6 once their
 *   waits end (retail has no branch between); the slot programs reload `work->models[index]` at every use (a local
 *   copy costs 6-8 points); `eft053_slot_gimmick_step` declares its shot angles after the matrix constructor call.
 */
#include "types.h"
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "ef.h"
#include "pl.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8012BDF4.h" /* em_act_ck, em_work_die_ck */
#include "fn_8004CAD8.h"       /* setVector3, mulVecMatAddTrans, rotVecY, VEC3_ctor, MTX34_ctor */
#include "ef/eft053.h"         /* this unit's header (the helpers whose owner header cannot carry them) */
#include "unsplit/unknown.h"
#include "enemy/em_pop.h"   /* the band's unnamed callees */
#include "ef/eft_res.h"        /* eft_res_slot_get/eft_res_model_get/eft_res_models_release (the effect pool) */
#include "ef/effect.h"         /* eft_state_flags_set (the effect's two state bytes) */
#include "ef/eft019.h"         /* eft019_set_core (the family's shared placement entry) */
#include "draw_shape.h"        /* fn_80056A84 (the shape request the state machine fires) */
#include "sound/fn_800DD1F0.h"
#include "sound/se_req.h" /* map_se_req (the map SE the same state fires) */
#include "unsplit/Pl.h"        /* PlSlotGate lbl_806BB7A0 (the family's timing block) */
#include "ef/fn_800CDB2C.h"    /* ran_suu (the family's frame randomiser) */
#include "ef/eft001.h"         /* eft_rot_vec_copy (the rotation copy) */
#include "g3d/g3d_calcworld.h"  /* addVec3To (the vector add) */
#include "quest/quest_entry.h"   /* quest_grade_rank_get */
#include "enemy/em025_prog.h"    /* fn_8019E70C */
#include "Pl/Pl_master_ck.h"     /* Pl_master_ck */
#include "Pl/pl_act.h"           /* Pl_bari_ck */
#include "sound/fn_800D7F54.h"   /* shell_se_req */
#include "stage/stg_w.h"         /* get_now_areano */
#include "Runtime.PPCEABI.H/memset.h" /* memset */
#include "ef/get_move_work_adrs.h" /* get_move_work_adrs / get_move_work_max */
#include "stage/get_now_mapno.h" /* get_now_mapno */
#include "Pl/pl_coll.h"           /* GetGroundHit */
#include "ef/eft028_set_scaled.h" /* eft028_set_scaled */
#include "enemy/fn_8012EC74.h"   /* get_joint_wmat_em */
#include "stage/shell_set_func_ptr.h" /* shell_set_func_ptr */
#include "mh3_pad/system_w.h"    /* system_w */
#include "Pl/pl_act_held_class_get.h" /* pl_act_held_class_get */
#include "ef/eft026_ramp_place_set.h" /* eft026_ramp_place_set */
#include "MSL_C/alloc.h"          /* abs */

/* ---------------------------------------------------------------------------------------------------
 * this range's own data (declared, never defined - playbook 29)
 * ------------------------------------------------------------------------------------------------- */

/* the range's private `.sdata2` pool, 0x8079B740..0x8079B820.  Values read out of the DOL:
 *   0x8079B740 0.0f       0x8079B744 0.5f       0x8079B748 65536.0f   0x8079B74C 0.01f
 *   0x8079B750 45.0f      0x8079B754 360.0f     0x8079B758 35.0f      0x8079B75C 50.0f
 *   0x8079B760 75.0f      0x8079B764 3000.0f    0x8079B768 130.0f     0x8079B76C 2000.0f
 *   0x8079B770 176.0f     0x8079B774 -0.0f      0x8079B778 -2600.0f   0x8079B77C -4800.0f
 *   0x8079B780 -1600.0f   0x8079B784 1000.0f    0x8079B788 300.0f     0x8079B78C 800.0f
 *   0x8079B790 1.0f       0x8079B794 -1400.0f   0x8079B798 -2675.0f   0x8079B79C -2275.0f
 *   0x8079B7A0 -1300.0f   0x8079B7A4 -1200.0f   0x8079B7A8 -200.0f    0x8079B7AC 0.25f
 *   0x8079B7B0 5.0f       0x8079B7B4 -3200.0f   0x8079B7B8 10.0f      0x8079B7BC -980.0f
 *   0x8079B7C0 -1240.0f   0x8079B7C4 -50.0f     0x8079B7C8 150.0f     0x8079B7CC -500.0f
 *   0x8079B7D0 60.0f      0x8079B7D4 0.001f     0x8079B7D8 40000.0f   0x8079B7DC 850.0f
 *   0x8079B7E0 1250.0f    0x8079B7E4 9973.0f    0x8079B7E8 1320.0f    0x8079B7EC 0.9f
 *   0x8079B7F0 500.0f     0x8079B7F4 20.0f      0x8079B7F8 176.0f     0x8079B7FC 0.0f
 *   0x8079B800 -0.1f      0x8079B804 0.1f       0x8079B808 270400.0f  0x8079B80C -30.0f
 *   0x8079B810 80.0f      0x8079B814 -60.0f     0x8079B818 -1500.0f   0x8079B81C -400.0f */
extern "C" f32 lbl_8079B740; /* 0.0f */
extern "C" f32 lbl_8079B744; /* 0.5f */
extern "C" f32 lbl_8079B748; /* 65536.0f */
extern "C" f32 lbl_8079B74C; /* 0.01f */
extern "C" f32 lbl_8079B750; /* 45.0f */
extern "C" f32 lbl_8079B754; /* 360.0f */
extern "C" f32 lbl_8079B758; /* 35.0f */
extern "C" f32 lbl_8079B75C; /* 50.0f */
extern "C" f32 lbl_8079B760; /* 75.0f */
extern "C" f32 lbl_8079B764; /* 3000.0f */
extern "C" f32 lbl_8079B768; /* 130.0f */
extern "C" f32 lbl_8079B76C; /* 2000.0f */
extern "C" f32 lbl_8079B770; /* 176.0f */
extern "C" f32 lbl_8079B774; /* -0.0f */
extern "C" f32 lbl_8079B778; /* -2600.0f */
extern "C" f32 lbl_8079B77C; /* -4800.0f */
extern "C" f32 lbl_8079B780; /* -1600.0f */
extern "C" f32 lbl_8079B784; /* 1000.0f */
extern "C" f32 lbl_8079B788; /* 300.0f */
extern "C" f32 lbl_8079B78C; /* 800.0f */
extern "C" f32 lbl_8079B790; /* 1.0f */
extern "C" f32 lbl_8079B794; /* -1400.0f */
extern "C" f32 lbl_8079B798; /* -2675.0f */
extern "C" f32 lbl_8079B79C; /* -2275.0f */
extern "C" f32 lbl_8079B7A0; /* -1300.0f */
extern "C" f32 lbl_8079B7A4; /* -1200.0f */
extern "C" f32 lbl_8079B7A8; /* -200.0f */
extern "C" f32 lbl_8079B7AC; /* 0.25f */
extern "C" f32 lbl_8079B7B0; /* 5.0f */
extern "C" f32 lbl_8079B7B4; /* -3200.0f */
extern "C" f32 lbl_8079B7B8; /* 10.0f */
extern "C" f32 lbl_8079B7BC; /* -980.0f */
extern "C" f32 lbl_8079B7C0; /* -1240.0f */
extern "C" f32 lbl_8079B7C4; /* -50.0f */
extern "C" f32 lbl_8079B7C8; /* 150.0f */
extern "C" f32 lbl_8079B7CC; /* -500.0f */
extern "C" f32 lbl_8079B7D0; /* 60.0f */
extern "C" f32 lbl_8079B7D4; /* 0.001f */
extern "C" f32 lbl_8079B7D8; /* 40000.0f */
extern "C" f32 lbl_8079B7DC; /* 850.0f */
extern "C" f32 lbl_8079B7E0; /* 1250.0f */
extern "C" f32 lbl_8079B7E4; /* 9973.0f */
extern "C" f32 lbl_8079B7E8; /* 1320.0f */
extern "C" f32 lbl_8079B7EC; /* 0.9f */
extern "C" f32 lbl_8079B7F0; /* 500.0f */
extern "C" f32 lbl_8079B7F4; /* 20.0f */
extern "C" f32 lbl_8079B7F8; /* 176.0f */
extern "C" f32 lbl_8079B7FC; /* 0.0f */
extern "C" f32 lbl_8079B800; /* -0.1f */
extern "C" f32 lbl_8079B804; /* 0.1f */
extern "C" f32 lbl_8079B808; /* 270400.0f */
extern "C" f32 lbl_8079B80C; /* -30.0f */
extern "C" f32 lbl_8079B810; /* 80.0f */
extern "C" f32 lbl_8079B814; /* -60.0f */
extern "C" f32 lbl_8079B818; /* -1500.0f */
extern "C" f32 lbl_8079B81C; /* -400.0f */

/* The `eft053` family's per-slot work record, the object `_EFT::work_0x38` points at.  It is three
 * parallel arrays behind one header: the pooled model handles at +0x04 (stride 4), the per-slot frame
 * counters at +0x40 (stride 4) and the per-slot state bytes at +0x60 (stride 1).  `eft053_set` splits
 * one allocation of this size into the record plus its handle, so size: 0x7C (the record's own `.bss`
 * holds one per effect slot). */
typedef struct Eft053Work {
    /* +0x000 */ s32 count;      /* the slot count `eft053_model_list_get` writes */
    /* +0x004 */ MHchar* models[8]; /* one pooled model handle per slot; `models[0]` is what the
                                     * release path (`eft_res_models_release`) is handed as its list base */
    /* +0x024 */ u8 kind_0x24;   /* the stage model-list kind `eft053_model_list_get` writes */
    /* +0x025 */ u8 sub_0x25[8];  /* per-slot sub-kind (the record's +0x02, the shown joint) */
    /* +0x02D */ u8 rec_kind_0x2D[8]; /* per-slot record kind (0/2 timed by the gate block, 3 a rank gimmick) */
    /* +0x035 */ u8 field_0x35[8]; /* per-slot placement index into the family's `Vec` table
                                    * (`eft053_place_table`, 0xC bytes an entry) */
    /* +0x03D */ u8 pad_0x3D[0x3];
    /* +0x040 */ s32 counters[8]; /* per-slot frame counter */
    /* +0x060 */ u8 states[8];   /* per-slot state byte */
    /* +0x068 */ u8 field_0x68[2]; /* the two rank gimmicks' state byte (`eft053_slot_gimmick_step`'s program step) */
    /* +0x06A */ u16 field_0x6A[2];
    /* +0x06E */ u16 field_0x6E[2]; /* the frame window the step is divided into */
    /* +0x072 */ u8 field_0x72[2][4];
    /* +0x07A */ u8 pad_0x7A[0x2];
} Eft053Work; /* size: 0x7C (approximate past +0x68: no body of this unit reads further) */


/* ---------------------------------------------------------------------------------------------------
 * this range's own functions (the map's stems are `fn_XXXXXXXX`, so they are `extern "C"`; the two
 * runtime-dump names keep C++ linkage, reproduced exactly by their real signature)
 * ------------------------------------------------------------------------------------------------- */
extern "C" void eft053_shell_pos_project(_PLW* plw, VEC3* out);
extern "C" s32 eft053_model_list_get(u8* out_kind, s32* out_count);
s32 eft053_get_shell_data(_PLW* plw, u8 index, VEC3* a, VEC3* b, VEC3* c);
u16 eft053_get_model_ang(u8 a, u8 b);
extern "C" s32 eft053_work_act_ck(_ENEMY_WORK* work);
extern "C" void eft053_set(void);
extern "C" void eft053_release(_EFT* self);
extern "C" void eft053_dispatch(_EFT* self);
extern "C" void eft053_init(_EFT* self);
extern "C" void eft053_slot_swing_step(_EFT* self, s32 index);
extern "C" void eft053_slot_lever_step(_EFT* self, s32 index);
extern "C" void eft053_slot_rot_step(_EFT* self, s32 index);
extern "C" s32 eft053_slot_move_step(_EFT* self, s32 index, u8 table_off, s32 randomize);
extern "C" s32 eft053_slot_steer_step(_EFT* self, s32 index, s32 table_off);
extern "C" void eft053_slot_gimmick_step(_EFT* self, s32 index);
extern "C" void eft053_run(_EFT* self);
extern "C" void eft053_state_advance(_EFT* self);
extern "C" void eft053_destroy(_EFT* self);

/* One entry of the current stage's model list, reached through the `eft053_model_lists` pointer table.
 * size: 0x18 (the loop stride the target multiplies the index by) */
typedef struct EftModelRec {
    /* +0x00 */ u8 kind;      /* the family kind `eft053_slot_lever_step` compares with its slot's +0x2D byte */
    /* +0x01 */ u8 index;     /* the slot index the same unit compares with its +0x35 byte */
    /* +0x02 */ u8 field_0x02; /* the sub-kind the shell projection switches on */
    /* +0x03 */ u8 pad_0x03[0x01];
    /* +0x04 */ Vec pos_0x04; /* the record's placement vector */
    /* +0x10 */ u16 rot_x;
    /* +0x12 */ u16 rot_y;    /* the rotation `eft053_get_model_ang` returns */
    /* +0x14 */ u16 rot_z;
    /* +0x16 */ u8 pad_0x16[0x02];
} EftModelRec; /* size: 0x18 */

/* The range's own unwritten bodies, declared before use. */
extern "C" void eft053_run(_EFT* self);
extern "C" void eft053_state_advance(_EFT* self);
extern "C" void eft053_destroy(_EFT* self);

/* The per-stage model lists `eft053_model_list_get` selects by map and area (kind 0..6): one record per model,
 * with its family kind, slot index, sub-kind, placement and rotation. */
EftModelRec eft053_models_map6_area1[8] = {
    {0, 0, 1, {0}, {0.0f, 63.0f, -1304.0f}, 0x6C1, 0x0, 0x0, {0}},
    {1, 0, 2, {0}, {-280.0f, 248.0f, -124.0f}, 0x0, 0xC001, 0x0, {0}},
    {1, 0, 4, {0}, {-280.0f, 248.0f, -124.0f}, 0x0, 0xC001, 0x0, {0}},
    {1, 1, 3, {0}, {280.0f, 248.0f, -124.0f}, 0x0, 0x4000, 0x0, {0}},
    {1, 1, 5, {0}, {280.0f, 248.0f, -124.0f}, 0x0, 0x4000, 0x0, {0}},
    {2, 0, 6, {0}, {-88.0f, 463.0f, 1409.0f}, 0x0, 0x0, 0x0, {0}},
    {3, 0, 7, {0}, {2000.0f, 0.0f, 6300.0f}, 0x0, 0x0, 0x0, {0}},
    {3, 1, 7, {0}, {2700.0f, 0.0f, 5300.0f}, 0x0, 0x0, 0x0, {0}},
};
EftModelRec eft053_models_map6_area2[8] = {
    {0, 0, 1, {0}, {9686.0f, 41.0f, -913.0f}, 0x6C1, 0x470A, 0x0, {0}},
    {1, 0, 2, {0}, {9675.0f, 248.0f, -124.0f}, 0x0, 0xC001, 0x0, {0}},
    {1, 0, 4, {0}, {9675.0f, 248.0f, -124.0f}, 0x0, 0xC001, 0x0, {0}},
    {1, 1, 3, {0}, {10241.0f, 248.0f, -124.0f}, 0x0, 0x4000, 0x0, {0}},
    {1, 1, 5, {0}, {10241.0f, 248.0f, -124.0f}, 0x0, 0x4000, 0x0, {0}},
    {2, 0, 6, {0}, {9872.0f, 463.0f, 1409.0f}, 0x0, 0x0, 0x0, {0}},
    {3, 0, 7, {0}, {9300.0f, 20.0f, 2500.0f}, 0x0, 0x4000, 0x0, {0}},
    {3, 1, 7, {0}, {9300.0f, 25.0f, -1600.0f}, 0x0, 0x4000, 0x0, {0}},
};
EftModelRec eft053_models_map7_area3[7] = {
    {0, 0, 1, {0}, {-10100.0f, -25528.0f, -460.0f}, 0x0, 0xC001, 0x0, {0}},
    {0, 1, 2, {0}, {-10100.0f, -25750.0f, 623.0f}, 0x0, 0xC001, 0xFDDF, {0}},
    {0, 2, 3, {0}, {-10100.0f, -25790.0f, 1701.0f}, 0x0, 0xC001, 0x0, {0}},
    {1, 0, 4, {0}, {-5597.0f, -25096.0f, 4579.0f}, 0x0, 0x4000, 0x0, {0}},
    {1, 0, 5, {0}, {-5597.0f, -25096.0f, 4579.0f}, 0x0, 0x4000, 0x0, {0}},
    {1, 1, 4, {0}, {-3208.0f, -26182.0f, -4281.0f}, 0x0, 0x2CCD, 0x0, {0}},
    {1, 1, 5, {0}, {-3208.0f, -26182.0f, -4281.0f}, 0x0, 0x2CCD, 0x0, {0}},
};
EftModelRec eft053_models_map8_area1[4] = {
    {0, 0, 1, {0}, {69.0f, 470.0f, -4924.0f}, 0x0, 0x8000, 0x0, {0}},
    {0, 1, 2, {0}, {514.0f, 470.0f, -4924.0f}, 0x0, 0x8000, 0x0, {0}},
    {0, 2, 3, {0}, {69.0f, 151.0f, -4924.0f}, 0x0, 0x8000, 0x0, {0}},
    {0, 3, 4, {0}, {514.0f, 151.0f, -4924.0f}, 0x0, 0x8000, 0x0, {0}},
};
EftModelRec eft053_models_map9_area0[4] = {
    {0, 0, 1, {0}, {69.0f, 470.0f, -4924.0f}, 0x0, 0x8000, 0x0, {0}},
    {0, 1, 2, {0}, {514.0f, 470.0f, -4924.0f}, 0x0, 0x8000, 0x0, {0}},
    {0, 2, 3, {0}, {69.0f, 151.0f, -4924.0f}, 0x0, 0x8000, 0x0, {0}},
    {0, 3, 4, {0}, {514.0f, 151.0f, -4924.0f}, 0x0, 0x8000, 0x0, {0}},
};
EftModelRec eft053_models_map9_area1[4] = {
    {0, 0, 1, {0}, {-252.0f, -634.0f, -3620.0f}, 0x0, 0x8000, 0x0, {0}},
    {0, 1, 2, {0}, {226.0f, -634.0f, -3620.0f}, 0x0, 0x8000, 0x0, {0}},
    {0, 2, 3, {0}, {-252.0f, -1436.0f, -3620.0f}, 0x0, 0x8000, 0x0, {0}},
    {0, 3, 4, {0}, {226.0f, -1436.0f, -3620.0f}, 0x0, 0x8000, 0x0, {0}},
};
EftModelRec eft053_models_map11_area1[4] = {
    {0, 0, 1, {0}, {-252.0f, -634.0f, -3620.0f}, 0x0, 0x8000, 0x0, {0}},
    {0, 1, 2, {0}, {226.0f, -634.0f, -3620.0f}, 0x0, 0x8000, 0x0, {0}},
    {0, 2, 3, {0}, {-252.0f, -1436.0f, -3620.0f}, 0x0, 0x8000, 0x0, {0}},
    {0, 3, 4, {0}, {226.0f, -1436.0f, -3620.0f}, 0x0, 0x8000, 0x0, {0}},
};

/* The model list of each kind, indexed by the kind `eft053_model_list_get` writes. */
EftModelRec* eft053_model_lists[7] = {
    eft053_models_map6_area1,
    eft053_models_map6_area2,
    eft053_models_map7_area3,
    eft053_models_map8_area1,
    eft053_models_map9_area0,
    eft053_models_map9_area1,
    eft053_models_map11_area1,
};

/* The lever head's offset from its base, turned by the lever's Y rotation. */
Vec eft053_lever_offset = {0.0f, 105.0f, 75.0f};

/* The rank gimmicks' placements: per gimmick (0/1) its start and the em025 shore-phase stations, then the hold
 * placements at +10. */
Vec eft053_place_table[12] = {
    {2000.0f, 0.0f, 5300.0f},
    {2700.0f, 0.0f, 4300.0f},
    {0.0f, 0.0f, 5800.0f},
    {0.0f, 0.0f, 4800.0f},
    {-2700.0f, 0.0f, 4800.0f},
    {-2000.0f, 0.0f, 3800.0f},
    {-2700.0f, 0.0f, -2200.0f},
    {-2000.0f, 0.0f, -1200.0f},
    {2000.0f, 0.0f, 7800.0f},
    {2700.0f, 0.0f, 7300.0f},
    {4175.0f, 0.0f, 5234.0f},
    {-2093.0f, 0.0f, 4230.0f},
};


/* ---------------------------------------------------------------------------------------------------
 * bodies, in address order
 * ------------------------------------------------------------------------------------------------- */

#pragma optimization_level 4
#pragma peephole off
#pragma fp_contract off
#pragma pool_data off

/* Projects the player's shell frame into `out` through the Z-X-Y rotation of the signed frame angle and the
 * per-kind scale triple. */
extern "C" void eft053_shell_pos_project(_PLW* plw, VEC3* out)
{
    MTX34 mtx;
    _CP_VECTOR rot;
    f32 scale;
    f32 t;
    s32 a;
    s16 v;

    MTX34_ctor(&mtx);
    setVector3(out, lbl_8079B740, lbl_8079B740, lbl_8079B740);
    eft_rot_vec_copy(&rot, (_CP_VECTOR*)&plw->param_0x54); /* +0x54 is the actor's 3-word rotation */
    a = (s8)plw->shell_ang_0x583;
    if (a >= 0) {
        t = lbl_8079B750 * (f32)a;
        t = t * lbl_8079B74C;
        t = t * lbl_8079B748;
        t = t / lbl_8079B754;
        v = (s16) -(u16)(s32)(lbl_8079B744 + t);
    } else {
        t = lbl_8079B758 * (f32)a;
        t = t * lbl_8079B74C;
        t = t * lbl_8079B748;
        t = t / lbl_8079B754;
        v = (s16) -(u16)(s32)(lbl_8079B744 + t);
    }
    rot.x = v;
    cpSetRotMatrixZXY(&rot, &mtx);
    if (plw->kind_0x09 == 3) {
        out->y = lbl_8079B75C;
        out->z = lbl_8079B760;
        scale = lbl_8079B764;
    } else {
        out->y = lbl_8079B768;
        out->z = lbl_8079B760;
        scale = lbl_8079B76C;
    }
    rotVecY(out, plw->field_0x058);
    addVec3To(out, &plw->vec_0x03C);
    mtx.m[0][3] = out->x;
    mtx.m[1][3] = out->y;
    mtx.m[2][3] = out->z;
    setVector3(out, lbl_8079B740, lbl_8079B740, scale);
    mulVecMatAddTrans(out, &mtx);
}

/* Pick the family's model record for the current map: the map number selects one of six per-stage
 * bodies, each of which tests the current area and writes the record kind and count. */
extern "C" s32 eft053_model_list_get(u8* out_kind, s32* out_count)
{
    *out_kind = 0;
    *out_count = 0;
    switch (get_now_mapno()) {
    case 6:
    case 17:
        switch (get_now_areano()) {
        case 1:
            if (quest_flag_200000_ck(0) != 0) {
                *out_count = 6;
            } else {
                *out_count = 8;
            }
            *out_kind = 0;
            return 1;
        case 2:
            if (quest_flag_200000_ck(0) != 0) {
                *out_count = 6;
            } else {
                *out_count = 8;
            }
            *out_kind = 1;
            return 1;
        }
        break;
    case 7:
    case 18:
        switch (get_now_areano()) {
        case 3:
            *out_count = 7;
            *out_kind = 2;
            return 1;
        }
        break;
    case 8:
    case 19:
        switch (get_now_areano()) {
        case 1:
            *out_count = 4;
            *out_kind = 3;
            return 1;
        }
        break;
    case 9:
        switch (get_now_areano()) {
        case 0:
            *out_count = 4;
            *out_kind = 4;
            return 1;
        case 1:
            *out_count = 4;
            *out_kind = 5;
            return 1;
        }
        break;
    case 11:
    case 20:
        switch (get_now_areano()) {
        case 1:
            *out_count = 4;
            *out_kind = 6;
            return 1;
        }
        break;
    }
    return 0;
}

/* Fills the three vectors with the shell's transformed record placement for one kind; returns 0 when the current
 * map has no record for it. */
s32 eft053_get_shell_data(_PLW* plw, u8 index, VEC3* a, VEC3* b, VEC3* c)
{
    VEC3 tmp_a;
    VEC3 tmp_c;
    VEC3 tmp_b;
    EftModelRec* list;
    u8 kind;
    s32 count;
    f32 ang;
    f32 scale_off;
    f32 off_a;
    f32 c28;
    f32 c27;
    f32 c26;
    f32 c25;
    f32 c24;
    f32 c23;
    s32 i;

    kind = 0;
    count = 0;
    ang = lbl_8079B740;
    scale_off = ang;
    off_a = ang;
    c28 = lbl_8079B778;
    c27 = lbl_8079B77C;
    c26 = lbl_8079B780;
    c25 = lbl_8079B784;
    c24 = lbl_8079B788;
    c23 = lbl_8079B78C;
    setVector3(a, ang, ang, ang);
    setVector3(b, lbl_8079B740, lbl_8079B740, lbl_8079B740);
    setVector3(c, lbl_8079B740, lbl_8079B740, lbl_8079B790);
    if (eft053_model_list_get(&kind, &count) == 0) {
        return 0;
    }
    list = eft053_model_lists[kind];
    for (i = 0; i < count; i++) {
        u32 found = 0;
        switch (plw->kind_0x015) {
        case 6:
        case 17:
            off_a = c28;
            scale_off = c25;
            switch (plw->area_0x16) {
            case 1:
                switch (list[i].field_0x02) {
                case 1:
                    found = 1;
                    ang = lbl_8079B794;
                    break;
                case 6:
                    vec_to_mh_vec3(a, &list[i].pos_0x04);
                    return 1;
                }
                break;
            case 2:
                switch (list[i].field_0x02) {
                case 1:
                    found = 1;
                    ang = lbl_8079B794;
                    break;
                case 6:
                    vec_to_mh_vec3(a, &list[i].pos_0x04);
                    return 1;
                }
                break;
            }
            break;
        case 7:
        case 18:
            off_a = c27;
            scale_off = c24;
            switch (plw->area_0x16) {
            case 3:
                switch (list[i].field_0x02) {
                case 1:
                    found = 1;
                    ang = lbl_8079B798;
                    break;
                case 2:
                    found = 1;
                    ang = lbl_8079B79C;
                    break;
                case 3:
                    found = 1;
                    ang = lbl_8079B798;
                    break;
                }
            }
            break;
        case 8:
        case 19:
            off_a = c26;
            scale_off = c23;
            if ((u32)(plw->area_0x16 - 1) <= 1) {
                switch (list[i].field_0x02) {
                case 1:
                    found = 1;
                    ang = lbl_8079B7A0;
                    break;
                case 2:
                    found = 1;
                    ang = lbl_8079B7A4;
                    break;
                case 3:
                    found = 1;
                    ang = lbl_8079B794;
                    break;
                case 4:
                    found = 1;
                    ang = lbl_8079B7A4;
                    break;
                }
            }
            break;
        case 9:
            off_a = c26;
            scale_off = c23;
            if (plw->area_0x16 <= 1) {
                switch (list[i].field_0x02) {
                case 1:
                    found = 1;
                    ang = lbl_8079B7A0;
                    break;
                case 2:
                    found = 1;
                    ang = lbl_8079B7A4;
                    break;
                case 3:
                    found = 1;
                    ang = lbl_8079B794;
                    break;
                case 4:
                    found = 1;
                    ang = lbl_8079B7A4;
                    break;
                }
            }
            break;
        case 11:
        case 20:
            off_a = c26;
            scale_off = c23;
            switch (plw->area_0x16) {
            case 1:
                switch (list[i].field_0x02) {
                case 1:
                    found = 1;
                    ang = lbl_8079B7A0;
                    break;
                case 2:
                    found = 1;
                    ang = lbl_8079B7A4;
                    break;
                case 3:
                    found = 1;
                    ang = lbl_8079B794;
                    break;
                case 4:
                    found = 1;
                    ang = lbl_8079B7A4;
                    break;
                }
            }
            break;
        }
        if (found == 1 && index == list[i].field_0x02) {
            ang += lbl_8079B7A8;
            rotVecZ(c, list[i].rot_z);
            rotVecX(c, list[i].rot_x);
            rotVecY(c, list[i].rot_y);
            vec3_normalize_into(c, c);
            vec_to_mh_vec3(b, &list[i].pos_0x04);
            vec3_scale(&tmp_a, c, scale_off);
            addVec3To(b, &tmp_a);
            vec3_scale(&tmp_b, c, off_a);
            addVec3(&tmp_c, b, &tmp_b);
            copyVec3(a, &tmp_c);
            vec3_scale_in_place(c, ang * lbl_8079B7AC);
            return 1;
        }
    }
    return 0;
}

/* Look the (kind, index) pair up in the stage's model record list and return its 16-bit id, 0 when
 * the current map has no record list or the pair is absent. */
u16 eft053_get_model_ang(u8 a, u8 b)
{
    u8 kind;
    s32 count;
    EftModelRec* records;
    s32 i;

    kind = 0;
    count = 0;
    if (eft053_model_list_get(&kind, &count) == 0) {
        return 0;
    }
    records = eft053_model_lists[kind];
    for (i = 0; i < count; i++) {
        if (records[i].kind == a && records[i].index == b) {
            return records[i].rot_y;
        }
    }
    return 0;
}

/* True while the enemy work record is in one of the four act states this family reacts to. */
extern "C" s32 eft053_work_act_ck(_ENEMY_WORK* work)
{
    if (em_act_ck(work, 13, 0) || em_act_ck(work, 13, 2) || em_act_ck(work, 13, 3) || em_act_ck(work, 13, 4)) {
        return 1;
    }
    return 0;
}

/* Allocate a pooled effect record for the family, fill its model slots and arm its state machine. */
extern "C" void eft053_set(void)
{
    _EFT* eft;
    Eft053Work* work;
    s32 i;

    eft = (_EFT*)eft_res_slot_get(0x7C);
    if (eft == 0) {
        return;
    }
    eft->release_0x40 = eft053_release;
    work = (Eft053Work*)eft->work_0x38;
    if (eft053_model_list_get(&work->kind_0x24, &work->count) == 0) {
        eft053_destroy(eft);
        return;
    }
    for (i = 0; i < work->count; i++) {
        work->models[i] = (MHchar*)eft_res_model_get();
        if (work->models[i] == 0) {
            eft_res_slot_release(eft);
            return;
        }
    }
    eft->source_0x30 = 0;
    eft->dispatch_0x34 = eft053_dispatch;
    eft->area_0x44 = get_now_areano();
    eft->field_0x03 = 53;
    eft_state_flags_set(eft, 8, 0);
}

/* Hand the effect's work block to the release path. */
extern "C" void eft053_release(_EFT* self)
{
    Eft053Work* work = (Eft053Work*)self->work_0x38;
    eft_res_models_release(&work->models[0], work->count);
}

/* Dispatch on the effect's state byte into the family's per-state entry points. */
extern "C" void eft053_dispatch(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        return eft053_init(self);
    case 1:
        return eft053_run(self);
    case 2:
        return eft053_state_advance(self);
    case 3:
        return eft053_destroy(self);
    }
}

/* Builds the family's models from the stage's model list (the model id from the map and area), places and orients
 * them, arms the timed slots from the gate block, and on maps 6/17 places the two rank gimmicks by the em025 state. */
extern "C" void eft053_init(_EFT* self)
{
    Eft053Work* work = (Eft053Work*)self->work_0x38;
    VEC3 offset;
    PlSlotGate* gate = stg_place_timers;
    EftModelRec* rec;
    u16 id;
    s32 i;
    s32 j;
    s32 found;
    _ENEMY_WORK* enemy;
    u16 max;

    VEC3_ctor(&offset);
    self->state_0x05++;
    self->flag_0x01 = 1;
    self->field_0x10 = 0;
    self->timer_0x0C = 0;
    switch (get_now_mapno()) {
    case 6:
        switch (self->area_0x44) {
        case 1:
            id = 0x7B;
            break;
        case 2:
            id = 0x7D;
            break;
        }
        break;
    case 17:
        switch (self->area_0x44) {
        case 1:
            id = 0x7C;
            break;
        case 2:
            id = 0x7E;
            break;
        }
        break;
    case 7:
        switch (self->area_0x44) {
        case 3:
            id = 0x82;
            break;
        }
        break;
    case 18:
        switch (self->area_0x44) {
        case 3:
            id = 0x83;
            break;
        }
        break;
    case 8:
        switch (self->area_0x44) {
        case 1:
            id = 0x88;
            break;
        }
        break;
    case 19:
        switch (self->area_0x44) {
        case 1:
            id = 0x89;
            break;
        }
        break;
    case 9:
        switch (self->area_0x44) {
        case 0:
            id = 0x8A;
            break;
        case 1:
            id = 0x8B;
            break;
        }
        break;
    case 11:
        switch (self->area_0x44) {
        case 1:
            id = 0x8C;
            break;
        }
        break;
    case 20:
        switch (self->area_0x44) {
        case 1:
            id = 0x8D;
            break;
        }
        break;
    }
    rec = eft053_model_lists[work->kind_0x24];
    for (i = 0; i < work->count; i++, rec++) {
        if (res_eft_model_create_light(work->models[i], id, 0x28, 2) == NULL) {
            eft053_destroy(self);
            return;
        }
        vec_to_mh_vec3(&work->models[i]->pos_0x04, &rec->pos_0x04);
        setVector3(&work->models[i]->scale_0x1C, 1.0f, 1.0f, 1.0f);
        work->models[i]->field_0x28 = rec->rot_x;
        work->models[i]->field_0x2C = rec->rot_y;
        work->models[i]->field_0x30 = rec->rot_z;
        work->models[i]->ready = 0;
        work->sub_0x25[i] = rec->field_0x02;
        work->counters[i] = 0;
        work->states[i] = 0;
        work->rec_kind_0x2D[i] = rec->kind;
        work->field_0x35[i] = rec->index;
        switch (get_now_mapno()) {
        case 6:
        case 17:
            if (work->sub_0x25[i] == 2 || work->sub_0x25[i] == 3) {
                vec_to_mh_vec3(&offset, &eft053_lever_offset);
                rotVecY(&offset, work->models[i]->field_0x2C);
                addVec3To(&work->models[i]->pos_0x04, &offset);
            }
            break;
        case 7:
        case 18:
            if (work->sub_0x25[i] == 4) {
                vec_to_mh_vec3(&offset, &eft053_lever_offset);
                rotVecY(&offset, work->models[i]->field_0x2C);
                addVec3To(&work->models[i]->pos_0x04, &offset);
            }
            break;
        }
        for (j = 0; (u32)j < (u32)work->models[i]->get_joint_num(); j++) {
            if (rec->field_0x02 == j) {
                work->models[i]->setVisibility(j, 1);
            } else {
                work->models[i]->setVisibility(j, 0);
            }
        }
        if (work->rec_kind_0x2D[i] == 0) {
            if (gate[0].flag_0x00 == 0) {
                work->states[i] = 0;
            } else {
                work->counters[i] = gate[0].length_0x04;
                work->states[i] = 8;
            }
        }
        if (work->rec_kind_0x2D[i] == 2) {
            if (gate[1].flag_0x00 == 0) {
                work->states[i] = 0;
            } else {
                work->counters[i] = gate[1].length_0x04;
                work->states[i] = 6;
            }
        }
    }
    if (get_now_mapno() == 6 || get_now_mapno() == 17) {
        found = 0;
        enemy = (_ENEMY_WORK*)get_move_work_adrs(3);
        max = get_move_work_max(3);
        for (i = 0; i < max; i++, enemy++) {
            if (enemy->team == 25 && enemy->area_no == self->area_0x44 && em_work_die_ck(enemy) != 1) {
                found = 1;
                break;
            }
        }
        for (i = 0; i < 2; i++) {
            if (work->count < 8 || work->rec_kind_0x2D[6 + i] != 3) {
                continue;
            }
            if (i == 0) {
                if (quest_grade_rank_get(0x4B) != 0) {
                    work->field_0x68[i] = 11;
                    continue;
                }
            } else if (i == 1 && quest_grade_rank_get(0x32) != 0) {
                work->field_0x68[i] = 11;
                continue;
            }
            if (self->area_0x44 == 1) {
                if (found != 0 && enemy != NULL) {
                    if (eft053_work_act_ck(enemy) != 0) {
                        vec_to_mh_vec3(&work->models[6 + i]->pos_0x04, &eft053_place_table[i + 10]);
                        work->field_0x68[i] = 13;
                    } else {
                        switch ((u8)em025_shore_phase_get(enemy)) {
                        case 0:
                            work->field_0x68[i] = 0;
                            vec_to_mh_vec3(&work->models[6 + i]->pos_0x04, &eft053_place_table[i]);
                            break;
                        case 1:
                        case 3:
                            work->field_0x68[i] = 3;
                            vec_to_mh_vec3(&work->models[6 + i]->pos_0x04, &eft053_place_table[i + 4]);
                            break;
                        case 4:
                            work->field_0x68[i] = 5;
                            vec_to_mh_vec3(&work->models[6 + i]->pos_0x04, &eft053_place_table[i + 6]);
                            break;
                        case 2:
                            work->field_0x68[i] = 9;
                            vec_to_mh_vec3(&work->models[6 + i]->pos_0x04, &eft053_place_table[i + 8]);
                            break;
                        default:
                            work->field_0x68[i] = 0;
                            vec_to_mh_vec3(&work->models[6 + i]->pos_0x04, &eft053_place_table[i]);
                            break;
                        }
                    }
                } else {
                    work->field_0x68[i] = 0;
                    vec_to_mh_vec3(&work->models[6 + i]->pos_0x04, &eft053_place_table[i]);
                }
            } else {
                work->field_0x68[i] = 12;
            }
            work->field_0x6A[i] = 0;
            work->field_0x6E[i] = 0;
            memset(work->field_0x72[i], 0, 4);
        }
    }
    eft053_run(self);
}

/* Runs a gate slot's swing program: armed by the stage's first timing gate, the model winds up, swings (spawning the
 * ramp and debris effects on the bridge kinds), holds, swings back and re-arms, and is then placed at its list
 * position pushed along its facing by the program's reach. */
extern "C" void eft053_slot_swing_step(_EFT* self, s32 index)
{
    VEC3 dir;
    VEC3 jitter;
    VEC3 se_pos;
    _CP_VECTOR rot;
    VEC3 tmp_a;
    VEC3 tmp_b;
    VEC3 tmp_c;
    VEC3 step;
    f32 reach;
    f32 dist;
    s32 speed;
    Eft053Work* work;
    s32 wait;
    u16 d;
    u8 kind;
    PlSlotGate* gate;

    speed = 0;
    setVec3(&dir, lbl_8079B740, lbl_8079B740, lbl_8079B790);
    dist = lbl_8079B740;
    work = (Eft053Work*)self->work_0x38;
    setVec3(&jitter, dist, dist, dist);
    gate = stg_place_timers;
    setVec3(&se_pos, lbl_8079B740, lbl_8079B740, lbl_8079B740);
    switch (work->kind_0x24) {
    default:
        kind = 0;
        reach = lbl_8079B794;
        speed = 0x2000;
        break;
    case 2:
        switch (work->sub_0x25[index]) {
        case 1:
            kind = 1;
            reach = lbl_8079B798;
            break;
        case 2:
            kind = 2;
            reach = lbl_8079B79C;
            break;
        default:
            kind = 3;
            reach = lbl_8079B798;
            break;
        }
        break;
    case 3:
    case 4:
    case 5:
    case 6:
        switch (work->sub_0x25[index]) {
        case 1:
            kind = 4;
            reach = lbl_8079B7A0;
            speed = 0x1500;
            break;
        case 2:
            kind = 5;
            reach = lbl_8079B7A4;
            speed = 0x1000;
            break;
        case 3:
            kind = 6;
            reach = lbl_8079B794;
            speed = 0x1A00;
            break;
        default:
            kind = 7;
            reach = lbl_8079B7A4;
            speed = 0x2000;
            break;
        }
        break;
    }
    work->counters[index] += 1;
    switch (work->states[index]) {
    case 0:
        if (gate[0].flag_0x00 != 0) {
            work->states[index] += 1;
            switch (kind) {
            default:
                work->counters[index] = -30;
                break;
            case 1:
                work->counters[index] = 0;
                break;
            case 2:
                work->counters[index] = -6;
                break;
            case 3:
                work->counters[index] = -2;
                break;
            case 4:
                work->counters[index] = 0;
                break;
            case 5:
                work->counters[index] = -6;
                break;
            case 6:
                work->counters[index] = -2;
                break;
            case 7:
                work->counters[index] = -4;
                break;
            }
        }
        break;
    case 1:
        if (work->counters[index] >= 15 && kind == 0) {
            if (work->counters[index] == 15 && self->area_0x44 == get_now_areano()) {
                setVector3(&se_pos, lbl_8079B740, lbl_8079B740, reach);
                rotVecY(&se_pos, work->models[index]->field_0x2C);
                addVec3To(&se_pos, &work->models[index]->pos_0x04);
                map_se_req(14, &se_pos);
            }
            jitter.y = (f32)(ran_suu(1) % 16 - 8);
            work->models[index]->field_0x30 -=
                (u16)(s32)(lbl_8079B744 +
                           lbl_8079B748 * (lbl_8079B7B0 + (f32)(work->counters[index] - 15)) / lbl_8079B754);
        }
        if (work->counters[index] >= 45) {
            work->states[index] += 1;
            work->counters[index] = 0;
            if (work->kind_0x24 == 2) {
                eft026_ramp_place_set(7, &work->models[index]->pos_0x04, &work->models[index]->rot_0x28,
                                      self->area_0x44, reach);
                copyVec3(&se_pos, setVec3(&tmp_a, lbl_8079B740, lbl_8079B740, lbl_8079B7B4));
                rotVecY(&se_pos, work->models[index]->field_0x2C);
                addVec3To(&se_pos, &work->models[index]->pos_0x04);
                eft_rot_vec_copy(&rot, &work->models[index]->rot_0x28);
                rot.y += 0x8000;
                eft028_set_scaled(8, &se_pos, &rot, lbl_8079B7B8, self->area_0x44, 0);
            } else if (work->kind_0x24 == 5 || work->kind_0x24 == 6) {
                eft026_ramp_place_set(10, &work->models[index]->pos_0x04, &work->models[index]->rot_0x28,
                                      self->area_0x44, reach);
                copyVec3(&se_pos, setVec3(&tmp_b, lbl_8079B740, lbl_8079B740, lbl_8079B7BC));
                rotVecY(&se_pos, work->models[index]->field_0x2C);
                addVec3To(&se_pos, &work->models[index]->pos_0x04);
                eft_rot_vec_copy(&rot, &work->models[index]->rot_0x28);
                rot.y += 0x8000;
                eft028_set_scaled(13, &se_pos, &rot, lbl_8079B7B0, self->area_0x44, 0);
            }
            if (self->area_0x44 == get_now_areano()) {
                setVector3(&se_pos, lbl_8079B740, lbl_8079B740, reach * lbl_8079B744);
                rotVecY(&se_pos, work->models[index]->field_0x2C);
                addVec3To(&se_pos, &work->models[index]->pos_0x04);
                map_se_req(2, &se_pos);
            }
        }
        break;
    case 2:
        if (kind == 0) {
            work->models[index]->field_0x30 -= speed;
        } else if (kind >= 4 && kind <= 7) {
            work->models[index]->field_0x30 += speed;
        }
        dist = (lbl_8079B7A8 + reach) * ((f32)work->counters[index] * lbl_8079B7AC);
        if (work->counters[index] == 1 && (work->kind_0x24 == 5 || work->kind_0x24 == 6)) {
            eft026_ramp_place_set(10, &work->models[index]->pos_0x04, &work->models[index]->rot_0x28,
                                  self->area_0x44, reach);
            copyVec3(&se_pos, setVec3(&tmp_c, lbl_8079B740, lbl_8079B740, lbl_8079B7C0));
            rotVecY(&se_pos, work->models[index]->field_0x2C);
            addVec3To(&se_pos, &work->models[index]->pos_0x04);
            eft_rot_vec_copy(&rot, &work->models[index]->rot_0x28);
            rot.y += 0x8000;
            eft028_set_scaled(13, &se_pos, &rot, lbl_8079B7B0, self->area_0x44, 0);
        }
        if (work->counters[index] >= 4) {
            work->states[index] += 1;
            work->counters[index] = 0;
        }
        break;
    case 3:
        if (kind == 0) {
            work->models[index]->field_0x30 -= speed;
        } else if (kind >= 4 && kind <= 7) {
            d = speed - work->counters[index] * 64;
            if (speed > work->counters[index] * 64) {
                work->models[index]->field_0x30 += d;
            }
        }
        if (work->counters[index] < 4) {
            dist = (lbl_8079B7A8 + reach) - lbl_8079B7C4 * (f32)work->counters[index];
        } else {
            dist = reach;
        }
        if (work->counters[index] >= 90) {
            work->states[index] += 1;
            work->counters[index] = 0;
        }
        break;
    case 4:
        switch (kind) {
        case 0:
            wait = 0;
            break;
        case 1:
            wait = 0;
            break;
        case 2:
            wait = 90;
            break;
        case 3:
            wait = 45;
            break;
        case 4:
            wait = 0;
            break;
        case 5:
            wait = 60;
            break;
        case 6:
            wait = 30;
            break;
        case 7:
            wait = 45;
            break;
        default:
            wait = 0;
            break;
        }
        if (kind >= 4 && kind <= 7) {
            d = speed - (work->counters[index] + 90) * 64;
            if (speed > (work->counters[index] + 90) * 64) {
                work->models[index]->field_0x30 += d;
            }
        }
        dist = reach;
        if (work->counters[index] < wait) {
            break;
        }
        work->states[index] += 1;
        work->counters[index] = 0;
        if (self->area_0x44 == get_now_areano()) {
            setVector3(&se_pos, lbl_8079B740, lbl_8079B740, reach * lbl_8079B744);
            rotVecY(&se_pos, work->models[index]->field_0x2C);
            addVec3To(&se_pos, &work->models[index]->pos_0x04);
            map_se_req(3, &se_pos);
        }
        /* fall through */
    case 5:
        if (kind == 0) {
            d = speed - speed / 150 * work->counters[index];
            if (work->counters[index] >= 100) {
                if (abs((s16)-work->models[index]->field_0x30) < d) {
                    work->models[index]->field_0x30 = 0;
                } else {
                    work->models[index]->field_0x30 -= d;
                }
            } else {
                work->models[index]->field_0x30 -= d;
            }
            dist = reach - reach * ((f32)work->counters[index] / lbl_8079B7C8);
            if (work->counters[index] >= 150) {
                work->states[index] = 8;
                work->counters[index] = gate[0].length_0x04;
            }
        } else {
            dist = reach - (reach - lbl_8079B7CC) * ((f32)work->counters[index] / lbl_8079B7C8);
            if (work->counters[index] >= 150) {
                work->states[index] += 1;
                work->counters[index] = 0;
            }
        }
        break;
    case 6:
        dist = lbl_8079B7CC;
        if (work->counters[index] >= 10) {
            work->states[index] += 1;
            work->counters[index] = 0;
        }
        break;
    case 7:
        dist = lbl_8079B7CC - lbl_8079B7CC * ((f32)work->counters[index] / lbl_8079B7D0);
        jitter.y = (f32)(ran_suu(1) % 16 - 8);
        if (work->counters[index] >= 60) {
            work->states[index] = 8;
            work->counters[index] = gate[0].length_0x04;
        }
        break;
    case 8:
        if (work->counters[index] < gate[0].length_0x04) {
            if (kind == 0) {
                work->counters[index] = 0;
                work->states[index] = 1;
            } else {
                work->counters[index] = 0;
                work->states[index] = 9;
            }
        } else {
            if (gate[0].flag_0x00 == 0) {
                if (kind == 0) {
                    work->states[index] = 0;
                } else {
                    work->states[index] = 9;
                }
            }
            work->counters[index] = gate[0].length_0x04;
        }
        break;
    case 9:
        if (kind == 3) {
            if (index - 2 >= 0 && work->states[index - 2] == 9 && index - 1 >= 0 && work->states[index - 1] == 9) {
                work->states[index - 2] = 0;
                work->states[index - 1] = 0;
                work->states[index] = 0;
            }
        } else if (kind == 7) {
            if (index - 3 >= 0 && work->states[index - 3] == 9 && index - 2 >= 0 && work->states[index - 2] == 9 &&
                index - 1 >= 0 && work->states[index - 1] == 9) {
                work->states[index - 3] = 0;
                work->states[index - 2] = 0;
                work->states[index - 1] = 0;
                work->states[index] = 0;
            }
        }
        break;
    }
    rotVecZXY(&dir, &work->models[index]->rot_0x28);
    if (vec3_length_sq(&dir.x) > lbl_8079B7D4) {
        vec3_normalize_into(&dir, &dir);
    } else {
        setVector3(&dir, lbl_8079B740, lbl_8079B740, lbl_8079B790);
    }
    vec_to_mh_vec3(&work->models[index]->pos_0x04, &eft053_model_lists[work->kind_0x24][index].pos_0x04);
    vec3_scale(&step, &dir, dist);
    addVec3To(&work->models[index]->pos_0x04, &step);
    rotVecY(&jitter, work->models[index]->field_0x2C);
    addVec3To(&work->models[index]->pos_0x04, &jitter);
}

/* Runs a lever slot (models `index` and `index + 1`): a player of the area standing in it with the lever action and
 * the slot's key turns it (the local master first, else any other), the turn and tilt sounds play, and the lever
 * follows the player's facing and shell angle; untouched, it returns to its list placement. */
extern "C" void eft053_slot_lever_step(_EFT* self, s32 index)
{
    Eft053Work* work = (Eft053Work*)self->work_0x38;
    VEC3 pos;
    VEC3_ctor(&pos);
    u32 moved = 0;
    s32 master = 0;
    s32 tilt = 0;
    _PLW* plw = (_PLW*)get_move_work_adrs(2);
    u16 max = get_move_work_max(2);
    u8 kind;
    s32 count;
    u8 on[4];
    s32 i;
    EftModelRec* recs;

    for (i = 0; i < max; i++, plw++) {
        on[i] = 0;
        if (plw->slot_active != 0 && plw->field_0x001 != 0 && plw->area_0x16 == self->area_0x44 &&
            Pl_bari_ck(plw, 2) == 1 && (u8)plw->field_0x0B6 == work->field_0x35[index]) {
            on[i] = 1;
            if (Pl_master_ck(plw) != 0) {
                master = 1;
            }
        }
    }
    if (master != 0) {
        plw = (_PLW*)get_move_work_adrs(2) + (s8)my_player_no();
        pos.x = work->models[index]->pos_0x04.x;
        pos.y = plw->motion_pos_0x40;
        pos.z = work->models[index]->pos_0x04.z;
        if (vec3_dist_sq(&plw->vec_0x03C, &pos) <= 40000.0f) {
            if ((u16)plw->field_0x058 != (u16)work->counters[index] && work->states[index] == 1 &&
                self->area_0x44 == get_now_areano()) {
                shell_se_req(NULL, &work->models[index]->pos_0x04, 0x12, work->field_0x35[index]);
            }
            if (plw->shell_ang_s_0x583 != work->counters[index + 1] && work->states[index] == 1 &&
                self->area_0x44 == get_now_areano()) {
                shell_se_req(NULL, &work->models[index]->pos_0x04, 0x13, work->field_0x35[index]);
            }
            tilt = plw->shell_ang_s_0x583;
            work->counters[index + 1] = tilt;
            work->counters[index] = (u16)plw->field_0x058;
            work->states[index] = 1;
            moved = 1;
        }
    } else {
        plw = (_PLW*)get_move_work_adrs(2);
        for (i = 0; i < max; i++, plw++) {
            if (plw->slot_active == 0 || plw->field_0x001 == 0 || plw->area_0x16 != self->area_0x44 ||
                Pl_master_ck(plw) != 0 || on[i] == 0) {
                continue;
            }
            pos.x = work->models[index]->pos_0x04.x;
            pos.y = plw->motion_pos_0x40;
            pos.z = work->models[index]->pos_0x04.z;
            if (vec3_dist_sq(&plw->vec_0x03C, &pos) > 40000.0f) {
                continue;
            }
            if ((u16)plw->field_0x058 != (u16)work->counters[index] && work->states[index] == 1 &&
                self->area_0x44 == get_now_areano()) {
                shell_se_req(NULL, &work->models[index]->pos_0x04, 0x12, work->field_0x35[index]);
            }
            if (plw->shell_ang_s_0x583 != work->counters[index + 1] && work->states[index] == 1 &&
                self->area_0x44 == get_now_areano()) {
                shell_se_req(NULL, &work->models[index]->pos_0x04, 0x13, work->field_0x35[index]);
            }
            tilt = plw->shell_ang_s_0x583;
            work->counters[index + 1] = tilt;
            work->counters[index] = (u16)plw->field_0x058;
            work->states[index] = 1;
            moved = 1;
            break;
        }
    }
    if (moved == 1) {
        if (eft053_model_list_get(&kind, &count) == 1) {
            recs = eft053_model_lists[kind];
            for (i = 0; i < count; i++) {
                if (recs[i].kind == work->rec_kind_0x2D[index] && recs[i].index == work->field_0x35[index]) {
                    vec_to_mh_vec3(&work->models[index]->pos_0x04, &recs[i].pos_0x04);
                    break;
                }
            }
        }
        if (tilt >= 0) {
            work->models[index]->field_0x28 =
                (s16)-(u16)(s32)(0.5f + (65536.0f * (0.01f * (45.0f * (f32)tilt))) / 360.0f);
        } else {
            work->models[index]->field_0x28 =
                (s16)-(u16)(s32)(0.5f + (65536.0f * (0.01f * (35.0f * (f32)tilt))) / 360.0f);
        }
        work->models[index]->field_0x2C = (u16)work->counters[index];
        vec_to_mh_vec3(&pos, &eft053_lever_offset);
        rotVecY(&pos, work->models[index]->field_0x2C);
        addVec3To(&work->models[index]->pos_0x04, &pos);
        work->models[index + 1]->field_0x2C = (u16)work->counters[index];
        return;
    }
    if (eft053_model_list_get(&kind, &count) == 1) {
        recs = eft053_model_lists[kind];
        for (i = 0; i < count; i++) {
            if (recs[i].kind == work->rec_kind_0x2D[index] && recs[i].index == work->field_0x35[index]) {
                work->models[index]->field_0x28 = recs[i].rot_x;
                work->models[index]->field_0x2C = recs[i].rot_y;
                vec_to_mh_vec3(&work->models[index]->pos_0x04, &recs[i].pos_0x04);
                vec_to_mh_vec3(&pos, &eft053_lever_offset);
                rotVecY(&pos, work->models[index]->field_0x2C);
                addVec3To(&work->models[index]->pos_0x04, &pos);
                work->states[index] = 0;
                work->models[index + 1]->field_0x28 = recs[i + 1].rot_x;
                work->models[index + 1]->field_0x2C = recs[i + 1].rot_y;
                return;
            }
        }
    }
}

/* Steps one slot of the rotation machine: `states[index]` runs one of seven stages and `counters[index]` feeds the
 * model's rotation. */
extern "C" void eft053_slot_rot_step(_EFT* self, s32 index)
{
    VEC3 pos;
    Eft053Work* work;
    PlSlotGate* timer;
    s32 c;

    work = (Eft053Work*)self->work_0x38;
    timer = stg_place_timers;
    VEC3_ctor(&pos);
    work->counters[index] += 1;
    switch (self->area_0x44) {
    case 1:
        setVector3(&pos, lbl_8079B740, lbl_8079B7DC, lbl_8079B7E0);
        break;
    case 2:
        setVector3(&pos, lbl_8079B7E4, lbl_8079B78C, lbl_8079B7E8);
        break;
    default:
        work->states[index] = 6;
        return;
    }
    switch (work->states[index]) {
    case 0:
        if (timer[1].flag_0x00 != 0) {
            work->states[index] += 1;
            work->counters[index] = 0;
        }
        break;
    case 1:
        if (work->counters[index] >= 45) {
            work->states[index] += 1;
            work->counters[index] = 0;
        }
        break;
    case 2:
        work->models[index]->field_0x30 = -(work->counters[index] * 3034);
        if (work->counters[index] >= 3) {
            work->states[index] += 1;
            work->counters[index] = 0;
            eft019_set_core(&pos, self->area_0x44, 111, lbl_8079B7EC, lbl_8079B7EC);
            fn_80056A84(&pos, 10, self->area_0x44);
            if (get_now_areano() == self->area_0x44) {
                map_se_req(9, &pos);
            }
        }
        break;
    case 3:
        c = work->counters[index];
        if (c <= 2) {
            work->models[index]->field_0x30 = -(9102 - c * 1274);
        } else if (c <= 4) {
            work->models[index]->field_0x30 = -((c - 2) * 1274 + 6554);
        } else if (c <= 5) {
            work->models[index]->field_0x30 = -(9102 - (c - 4) * 849);
        } else if (c <= 6) {
            work->models[index]->field_0x30 = -((c - 5) * 849 + 8253);
        }
        if (work->counters[index] >= 6) {
            work->states[index] += 1;
            work->counters[index] = 0;
        }
        if (work->counters[index] == 4) {
            eft019_set_core(&pos, self->area_0x44, 111, lbl_8079B7EC, lbl_8079B7EC);
        }
        break;
    case 4:
        if (work->counters[index] >= 30) {
            work->states[index] += 1;
            work->counters[index] = 0;
        }
        break;
    case 5:
        work->models[index]->field_0x30 = -(9102 - work->counters[index] * 303);
        if (work->counters[index] < 30) {
            break;
        }
        work->states[index] = 6;
        work->counters[index] = timer[1].length_0x04;
        /* fall through */
    case 6:
        if (work->counters[index] < timer[1].length_0x04) {
            work->counters[index] = 0;
            work->states[index] = 1;
        } else {
            if (timer[1].flag_0x00 == 0) {
                work->states[index] = 0;
            }
            work->counters[index] = timer[1].length_0x04;
        }
        break;
    }
}

/* Steps one slot of the translation machine over its frame window and re-places the model from the `Vec` table
 * when a stage completes; returns 1 on the last stage. */
extern "C" s32 eft053_slot_move_step(_EFT* self, s32 index, u8 table_off, s32 randomize)
{
    VEC3 pos;
    Eft053Work* work;
    f32 step;
    s32 ret;
    s32 seed;
    s32 start;

    work = (Eft053Work*)self->work_0x38;
    ret = 0;
    step = (f32)work->field_0x6E[work->field_0x35[index]];
    VEC3_ctor(&pos);
    if (randomize != 0) {
        seed = ran_suu(1) % 5;
        start = ran_suu(1) % (s32)(lbl_8079B7F0 / step);
    } else {
        seed = 0;
        start = (s32)(lbl_8079B7F0 / step);
    }
    switch (work->states[index]) {
    case 0:
        work->states[index] += 1;
        work->field_0x6E[work->field_0x35[index]] = lbl_8079B7F4 + (f32)seed;
        break;
    case 1:
        work->counters[index] += 1;
        work->models[index]->pos_0x04.z -= lbl_8079B744 * (f32)work->counters[index];
        if (lbl_8079B744 * (f32)work->counters[index] >= step) {
            work->states[index] += 1;
            work->counters[index] = start;
            vec_to_mh_vec3(&pos, &eft053_place_table[table_off + work->field_0x35[index]]);
            work->counters[index] =
                (s32)((f32)work->counters[index] + calcVecDistXZ(&work->models[index]->pos_0x04, &pos) / step);
        }
        break;
    case 2:
        work->models[index]->pos_0x04.z -= step;
        work->counters[index] -= 1;
        if (work->counters[index] <= 0) {
            work->states[index] += 1;
            work->counters[index] = 0;
        }
        break;
    case 3:
        work->counters[index] += 1;
        work->models[index]->pos_0x04.z -= step - lbl_8079B744 * (f32)work->counters[index];
        if (step - lbl_8079B744 * (f32)work->counters[index] <= lbl_8079B740) {
            work->states[index] += 1;
            work->counters[index] = 0;
            work->field_0x6E[work->field_0x35[index]] = lbl_8079B7F4 + (f32)seed;
        }
        break;
    case 4:
        work->counters[index] += 1;
        work->models[index]->pos_0x04.z += lbl_8079B744 * (f32)work->counters[index];
        if (lbl_8079B744 * (f32)work->counters[index] >= step) {
            work->states[index] += 1;
            work->counters[index] = start;
            vec_to_mh_vec3(&pos, &eft053_place_table[table_off + work->field_0x35[index]]);
            work->counters[index] =
                (s32)((f32)work->counters[index] + calcVecDistXZ(&work->models[index]->pos_0x04, &pos) / step);
        }
        break;
    case 5:
        work->models[index]->pos_0x04.z += step;
        work->counters[index] -= 1;
        if (work->counters[index] <= 0) {
            work->states[index] += 1;
            work->counters[index] = 0;
        }
        break;
    case 6:
        work->counters[index] += 1;
        work->models[index]->pos_0x04.z += step - lbl_8079B744 * (f32)work->counters[index];
        if (step - lbl_8079B744 * (f32)work->counters[index] <= lbl_8079B740) {
            work->states[index] = 0;
            work->counters[index] = 0;
            ret = 1;
        }
        break;
    }
    return ret;
}

/* Steers one rank-gimmick slot toward its `Vec` table placement in three stages (accelerate, cruise inside 520 units,
 * decelerate), banking its Y/Z rotation by the heading's X; returns 1 when the last stage ends. */
extern "C" s32 eft053_slot_steer_step(_EFT* self, s32 index, s32 table_off)
{
    VEC3 dir;
    VEC3 target;
    VEC3 step_a;
    VEC3 step_b;
    VEC3 step_c;
    Eft053Work* work;
    u32 ang;
    s32 ret;

    work = (Eft053Work*)self->work_0x38;
    VEC3_ctor(&dir);
    VEC3_ctor(&target);
    ret = 0;
    switch (work->states[index]) {
    case 0:
        work->counters[index] += 1;
        vec_to_mh_vec3(&target, &eft053_place_table[(u8)table_off + work->field_0x35[index]]);
        ang = calcVecAng2(&work->models[index]->pos_0x04, &target);
        setVector3(&dir, lbl_8079B740, lbl_8079B740, lbl_8079B790);
        rotVecY(&dir, ang);
        vec3_scale(&step_a, &dir, lbl_8079B744 * (f32)work->counters[index]);
        addVec3To(&work->models[index]->pos_0x04, &step_a);
        if (lbl_8079B744 * (f32)work->counters[index] >= lbl_8079B7F4) {
            work->states[index] += 1;
            work->counters[index] = 0;
        }
        if (dir.x < lbl_8079B800) {
            if ((u32)work->models[index]->field_0x2C < 0x71C) {
                work->field_0x6A[work->field_0x35[index]] += 9;
                work->models[index]->field_0x2C =
                    (u16)(work->models[index]->field_0x2C + work->field_0x6A[work->field_0x35[index]]);
            } else if (work->field_0x6A[work->field_0x35[index]] != 0) {
                work->field_0x6A[work->field_0x35[index]] = (u16)(work->field_0x6A[work->field_0x35[index]] - 9);
                if (work->field_0x6A[work->field_0x35[index]] > 0x8000) {
                    work->field_0x6A[work->field_0x35[index]] = 0;
                }
                work->models[index]->field_0x2C =
                    (u16)(work->models[index]->field_0x2C + work->field_0x6A[work->field_0x35[index]]);
            }
            if ((u32)work->models[index]->field_0x30 < 0x71C) {
                work->field_0x6E[work->field_0x35[index]] += 9;
                work->models[index]->field_0x30 =
                    (u16)(work->models[index]->field_0x30 + work->field_0x6E[work->field_0x35[index]]);
            } else if (work->field_0x6E[work->field_0x35[index]] != 0) {
                work->field_0x6E[work->field_0x35[index]] = (u16)(work->field_0x6E[work->field_0x35[index]] - 9);
                if (work->field_0x6E[work->field_0x35[index]] > 0x8000) {
                    work->field_0x6E[work->field_0x35[index]] = 0;
                }
                work->models[index]->field_0x30 =
                    (u16)(work->models[index]->field_0x30 + work->field_0x6E[work->field_0x35[index]]);
            }
        } else if (dir.x > lbl_8079B804) {
            if (work->models[index]->field_0x2C == 0 || (u32)work->models[index]->field_0x2C > 0xF8E4) {
                work->field_0x6A[work->field_0x35[index]] += 9;
                work->models[index]->field_0x2C =
                    (u16)(work->models[index]->field_0x2C - work->field_0x6A[work->field_0x35[index]]);
            } else if (work->field_0x6A[work->field_0x35[index]] != 0) {
                work->field_0x6A[work->field_0x35[index]] = (u16)(work->field_0x6A[work->field_0x35[index]] - 9);
                if (work->field_0x6A[work->field_0x35[index]] > 0x8000) {
                    work->field_0x6A[work->field_0x35[index]] = 0;
                }
                work->models[index]->field_0x2C =
                    (u16)(work->models[index]->field_0x2C - work->field_0x6A[work->field_0x35[index]]);
            }
            if (work->models[index]->field_0x30 == 0 || (u32)work->models[index]->field_0x30 > 0xF8E4) {
                work->field_0x6E[work->field_0x35[index]] += 9;
                work->models[index]->field_0x30 =
                    (u16)(work->models[index]->field_0x30 - work->field_0x6E[work->field_0x35[index]]);
            } else if (work->field_0x6E[work->field_0x35[index]] != 0) {
                work->field_0x6E[work->field_0x35[index]] = (u16)(work->field_0x6E[work->field_0x35[index]] - 9);
                if (work->field_0x6E[work->field_0x35[index]] > 0x8000) {
                    work->field_0x6E[work->field_0x35[index]] = 0;
                }
                work->models[index]->field_0x30 =
                    (u16)(work->models[index]->field_0x30 - work->field_0x6E[work->field_0x35[index]]);
            }
        } else {
            work->models[index]->field_0x2C = 0;
            work->models[index]->field_0x30 = 0;
        }
        break;
    case 1:
        vec_to_mh_vec3(&target, &eft053_place_table[(u8)table_off + work->field_0x35[index]]);
        ang = calcVecAng2(&work->models[index]->pos_0x04, &target);
        setVector3(&dir, lbl_8079B740, lbl_8079B740, lbl_8079B790);
        rotVecY(&dir, ang);
        vec3_scale(&step_b, &dir, lbl_8079B7F4);
        addVec3To(&work->models[index]->pos_0x04, &step_b);
        if (vec3_dist_sq(&work->models[index]->pos_0x04, &target) <= lbl_8079B808) {
            work->states[index] += 1;
            work->counters[index] = 0;
        }
        if (dir.x < lbl_8079B800) {
            if ((u32)work->models[index]->field_0x2C < 0x71C) {
                work->field_0x6A[work->field_0x35[index]] += 9;
                work->models[index]->field_0x2C =
                    (u16)(work->models[index]->field_0x2C + work->field_0x6A[work->field_0x35[index]]);
            } else if (work->field_0x6A[work->field_0x35[index]] != 0) {
                work->field_0x6A[work->field_0x35[index]] = (u16)(work->field_0x6A[work->field_0x35[index]] - 9);
                if (work->field_0x6A[work->field_0x35[index]] > 0x8000) {
                    work->field_0x6A[work->field_0x35[index]] = 0;
                }
                work->models[index]->field_0x2C =
                    (u16)(work->models[index]->field_0x2C + work->field_0x6A[work->field_0x35[index]]);
            }
            if ((u32)work->models[index]->field_0x30 < 0x71C) {
                work->field_0x6E[work->field_0x35[index]] += 9;
                work->models[index]->field_0x30 =
                    (u16)(work->models[index]->field_0x30 + work->field_0x6E[work->field_0x35[index]]);
            } else if (work->field_0x6E[work->field_0x35[index]] != 0) {
                work->field_0x6E[work->field_0x35[index]] = (u16)(work->field_0x6E[work->field_0x35[index]] - 9);
                if (work->field_0x6E[work->field_0x35[index]] > 0x8000) {
                    work->field_0x6E[work->field_0x35[index]] = 0;
                }
                work->models[index]->field_0x30 =
                    (u16)(work->models[index]->field_0x30 + work->field_0x6E[work->field_0x35[index]]);
            }
        } else if (dir.x > lbl_8079B804) {
            if (work->models[index]->field_0x2C == 0 || (u32)work->models[index]->field_0x2C > 0xF8E4) {
                work->field_0x6A[work->field_0x35[index]] += 9;
                work->models[index]->field_0x2C =
                    (u16)(work->models[index]->field_0x2C - work->field_0x6A[work->field_0x35[index]]);
            } else if (work->field_0x6A[work->field_0x35[index]] != 0) {
                work->field_0x6A[work->field_0x35[index]] = (u16)(work->field_0x6A[work->field_0x35[index]] - 9);
                if (work->field_0x6A[work->field_0x35[index]] > 0x8000) {
                    work->field_0x6A[work->field_0x35[index]] = 0;
                }
                work->models[index]->field_0x2C =
                    (u16)(work->models[index]->field_0x2C - work->field_0x6A[work->field_0x35[index]]);
            }
            if (work->models[index]->field_0x30 == 0 || (u32)work->models[index]->field_0x30 > 0xF8E4) {
                work->field_0x6E[work->field_0x35[index]] += 9;
                work->models[index]->field_0x30 =
                    (u16)(work->models[index]->field_0x30 - work->field_0x6E[work->field_0x35[index]]);
            } else if (work->field_0x6E[work->field_0x35[index]] != 0) {
                work->field_0x6E[work->field_0x35[index]] = (u16)(work->field_0x6E[work->field_0x35[index]] - 9);
                if (work->field_0x6E[work->field_0x35[index]] > 0x8000) {
                    work->field_0x6E[work->field_0x35[index]] = 0;
                }
                work->models[index]->field_0x30 =
                    (u16)(work->models[index]->field_0x30 - work->field_0x6E[work->field_0x35[index]]);
            }
        } else {
            work->models[index]->field_0x2C = 0;
            work->models[index]->field_0x30 = 0;
        }
        break;
    case 2:
        work->counters[index] += 1;
        vec_to_mh_vec3(&target, &eft053_place_table[(u8)table_off + work->field_0x35[index]]);
        ang = calcVecAng2(&work->models[index]->pos_0x04, &target);
        setVector3(&dir, lbl_8079B740, lbl_8079B740, lbl_8079B790);
        rotVecY(&dir, ang);
        vec3_scale(&step_c, &dir, lbl_8079B7F4 - lbl_8079B744 * (f32)work->counters[index]);
        addVec3To(&work->models[index]->pos_0x04, &step_c);
        if (lbl_8079B7F4 - lbl_8079B744 * (f32)work->counters[index] <= lbl_8079B740) {
            work->states[index] = 0;
            work->models[index]->field_0x30 = 0;
            work->counters[index] = 0;
            ret = 1;
        }
        if (dir.x < lbl_8079B800) {
            work->models[index]->field_0x2C = (u16)(work->models[index]->field_0x2C - 91);
            if ((u32)work->models[index]->field_0x2C > 0x8000) {
                work->models[index]->field_0x2C = 0;
            }
            work->models[index]->field_0x30 = (u16)(work->models[index]->field_0x30 - 91);
            if ((u32)work->models[index]->field_0x30 > 0x8000) {
                work->models[index]->field_0x30 = 0;
            }
        } else if (dir.x > lbl_8079B804) {
            work->models[index]->field_0x2C = (u16)(work->models[index]->field_0x2C + 91);
            if ((u32)work->models[index]->field_0x2C < 0x8000) {
                work->models[index]->field_0x2C = 0;
            }
            work->models[index]->field_0x30 = (u16)(work->models[index]->field_0x30 + 91);
            if ((u32)work->models[index]->field_0x30 < 0x8000) {
                work->models[index]->field_0x30 = 0;
            }
        } else {
            work->models[index]->field_0x2C = 0;
            work->models[index]->field_0x30 = 0;
        }
        break;
    }
    return ret;
}

/* Runs a rank-gimmick slot against the area's em025: its program walks the `Vec` table by the monster's shore phase,
 * fires a shot at the monster's joint when a player in the area holds the right act, and on the quest rank drops the
 * gimmick to the ground. */
extern "C" void eft053_slot_gimmick_step(_EFT* self, s32 index)
{
    Eft053Work* work = (Eft053Work*)self->work_0x38;
    MTX34 mtx;
    MTX34_ctor(&mtx);
    u32 ang[3] = {0, 0x8000, 0};
    VEC3 joint;
    VEC3 shot;
    VEC3 dir;
    VEC3 ground;
    VEC3 offset;
    VEC3 sum;
    VEC3 diff;
    _ENEMY_WORK* enemy;
    _ENEMY_WORK* target;
    _PLW* plw;
    MHchar** model;
    u16 max;
    s32 i;
    s32 p;
    s32 e;
    u8 phase;

    VEC3_ctor(&joint);
    VEC3_ctor(&shot);
    VEC3_ctor(&dir);
    VEC3_ctor(&ground);
    VEC3_ctor(&offset);
    enemy = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    for (i = 0; i < max; i++, enemy++) {
        if (enemy->team == 25 && enemy->area_no == self->area_0x44 && em_work_die_ck(enemy) != 1U) {
            break;
        }
    }
    if (i < 0 || i >= max) {
        return;
    }
    if (self->area_0x44 == 1) {
        if (work->field_0x68[work->field_0x35[index]] != 13 && eft053_work_act_ck(enemy) != 0) {
            work->field_0x68[work->field_0x35[index]] = 13;
            work->states[index] = 0;
            vec_to_mh_vec3(&work->models[index]->pos_0x04, &eft053_place_table[work->field_0x35[index] + 10]);
        }
    }
    switch (work->field_0x68[work->field_0x35[index]]) {
    case 0:
        if (eft053_slot_move_step(self, index, 0, 1) != 0) {
            phase = em025_shore_phase_get(enemy);
            if (phase != 0 && phase != 0xFF) {
                work->field_0x68[work->field_0x35[index]] += 1;
                work->states[index] = 0;
            }
        }
        break;
    case 1:
        if (eft053_slot_steer_step(self, index, 2) != 0) {
            work->field_0x68[work->field_0x35[index]] += 1;
            work->states[index] = 0;
        }
        break;
    case 2:
        if (eft053_slot_steer_step(self, index, 4) != 0) {
            work->field_0x68[work->field_0x35[index]] += 1;
            work->states[index] = 0;
        }
        break;
    case 3:
        if (eft053_slot_move_step(self, index, 4, 1) != 0) {
            phase = em025_shore_phase_get(enemy);
            if (phase != 1 && phase != 3 && phase != 0xFF) {
                work->field_0x68[work->field_0x35[index]] += 1;
                work->states[index] = 0;
            }
        }
        break;
    case 4:
        if (eft053_slot_steer_step(self, index, 6) != 0) {
            work->field_0x68[work->field_0x35[index]] += 1;
            work->states[index] = 0;
        }
        break;
    case 5:
        if (eft053_slot_move_step(self, index, 6, 1) != 0) {
            phase = em025_shore_phase_get(enemy);
            if (phase != 4 && phase != 0xFF) {
                work->field_0x68[work->field_0x35[index]] += 1;
                work->states[index] = 0;
            }
        }
        break;
    case 6:
        if (eft053_slot_steer_step(self, index, 4) != 0) {
            work->field_0x68[work->field_0x35[index]] += 1;
            work->states[index] = 0;
        }
        break;
    case 7:
        if (eft053_slot_steer_step(self, index, 2) != 0) {
            work->field_0x68[work->field_0x35[index]] += 1;
            work->states[index] = 0;
        }
        break;
    case 8:
        if (eft053_slot_steer_step(self, index, 8) != 0) {
            work->field_0x68[work->field_0x35[index]] += 1;
            work->states[index] = 0;
        }
        break;
    case 9:
        eft053_slot_move_step(self, index, 8, 1);
        break;
    case 10:
        model = &work->models[index];
        if (self->area_0x44 == 1) {
            (*model)->pos_0x04.y += lbl_8079B80C;
            (*model)->pos_0x04.z += lbl_8079B810;
            (*model)->field_0x28 += 364;
        } else {
            (*model)->pos_0x04.y += lbl_8079B814;
            (*model)->field_0x28 += 728;
        }
        if ((u32)(*model)->field_0x28 > 10923) {
            (*model)->field_0x28 = 10923;
        }
        if ((*model)->pos_0x04.y < lbl_8079B818) {
            work->field_0x68[work->field_0x35[index]] += 1;
            (*model)->field_0x34 = 0;
        }
        break;
    case 11:
        work->models[index]->field_0x34 = 0;
        break;
    case 13:
        eft053_slot_move_step(self, index, 10, 0);
        if (self->area_0x44 == 1 && eft053_work_act_ck(enemy) == 0) {
            work->field_0x68[work->field_0x35[index]] = 0;
            work->states[index] = 0;
            vec_to_mh_vec3(&work->models[index]->pos_0x04, &eft053_place_table[work->field_0x35[index]]);
        }
        break;
    }
    if (self->area_0x44 == 1 && work->models[index]->field_0x34 != 0 && system_w.field_0x0c % 10 == 0) {
        eft028_set_scaled(10, &work->models[index]->pos_0x04, &work->models[index]->rot_0x28, lbl_8079B790,
                          self->area_0x44, 0);
    }
    model = &work->models[index];
    if ((*model)->field_0x34 != 0) {
        plw = (_PLW*)get_move_work_adrs(2);
        max = get_move_work_max(2);
        for (p = 0; p < max; p++, plw++) {
            if (plw->slot_active == 0 || plw->field_0x001 == 0 || plw->area_0x16 != self->area_0x44) {
                work->field_0x72[work->field_0x35[index]][p] = 0;
                continue;
            }
            if (pl_act_held_class_get(plw) == 0) {
                work->field_0x72[work->field_0x35[index]][p] = 0;
                continue;
            }
            if (work->field_0x72[work->field_0x35[index]][p] != 0) {
                continue;
            }
            if (ran_suu(1) % 5 != 0) {
                work->field_0x72[work->field_0x35[index]][p] = 1;
                continue;
            }
            target = (_ENEMY_WORK*)get_move_work_adrs(3);
            max = get_move_work_max(3);
            for (e = 0; e < max; e++, target++) {
                if (target->team == 25 && target->area_no == self->area_0x44 && em_work_die_ck(target) != 1U) {
                    get_joint_wmat_em(target, 5, &mtx);
                    joint.x = mtx.m[0][3];
                    joint.y = mtx.m[1][3];
                    joint.z = mtx.m[2][3];
                    setVector3(&offset, lbl_8079B740, lbl_8079B7C8, lbl_8079B81C);
                    rotVecY(&offset, (*model)->field_0x2C);
                    addVec3(&sum, &(*model)->pos_0x04, &offset);
                    copyVec3(&shot, &sum);
                    subVec3(&diff, &joint, &shot);
                    copyVec3(&dir, &diff);
                    calcVecAngXY(&dir, &ang[0], &ang[1]);
                    shell_set_func_ptr->method_0x7C(&shot, ang, self->area_0x44, shell_set_func_ptr);
                    work->field_0x72[work->field_0x35[index]][e] = 2;
                    break;
                }
            }
            break;
        }
    }
    if (work->field_0x68[work->field_0x35[index]] != 10 && (*model)->field_0x34 == 1) {
        switch (work->field_0x35[index]) {
        case 0:
            if (quest_grade_rank_get(75) != 0) {
                work->field_0x68[work->field_0x35[index]] = 10;
                copyVec3(&ground, &(*model)->pos_0x04);
                ground.y = GetGroundHit(&ground, 0x1000, self->area_0x44);
                eft019_set(&ground, self->area_0x44, 2);
            }
            break;
        case 1:
            if (quest_grade_rank_get(50) != 0) {
                work->field_0x68[work->field_0x35[index]] = 10;
                copyVec3(&ground, &(*model)->pos_0x04);
                ground.y = GetGroundHit(&ground, 0x1000, self->area_0x44);
                eft019_set(&ground, self->area_0x44, 2);
            }
            break;
        }
    }
}

/* Steps every slot by its list kind and sub-kind (the motion, shell, rotation and rank programs), then moves and
 * spawns its model when it is shown. */
extern "C" void eft053_run(_EFT* self)
{
    s32 i;
    Eft053Work* work = (Eft053Work*)self->work_0x38;

    for (i = 0; i < work->count; i++) {
        switch (work->kind_0x24) {
        case 0:
        case 1:
            switch (work->sub_0x25[i]) {
            case 1:
                eft053_slot_swing_step(self, i);
                break;
            case 2:
            case 3:
                eft053_slot_lever_step(self, i);
                break;
            case 6:
                eft053_slot_rot_step(self, i);
                break;
            case 7:
                eft053_slot_gimmick_step(self, i);
                break;
            }
            break;
        case 2:
            switch (work->sub_0x25[i]) {
            case 1:
            case 2:
            case 3:
                eft053_slot_swing_step(self, i);
                break;
            case 4:
                eft053_slot_lever_step(self, i);
                break;
            }
            break;
        case 3:
        case 4:
        case 5:
        case 6:
            switch (work->sub_0x25[i]) {
            case 1:
            case 2:
            case 3:
            case 4:
                eft053_slot_swing_step(self, i);
                break;
            }
            break;
        }
        work->models[i]->move(0);
        if (work->models[i]->field_0x34 != 0) {
            eft_res_models_spawn(self, (void**)&work->models[i], 2, 1, 0);
        }
    }
}

/* Advances the record's state. */
extern "C" void eft053_state_advance(_EFT* self)
{
    self->state_0x05++;
}

/* Retires the record to the pool. */
extern "C" void eft053_destroy(_EFT* self)
{
    eft_res_slot_release(self);
}
