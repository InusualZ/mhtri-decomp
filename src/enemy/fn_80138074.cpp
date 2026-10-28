/* enemy/fn_80138074.cpp - the enemy "user data" driver and its accessors, two clusters of one TU: the `_ENEMY_WORK`
 *   per-frame driver (0x80138074-0x801391E8: `fn_8013823C` the per-tick update, `em_post_update` the sleep/death half
 *   and their helpers) and the `ResUserDataAc` accessor with the per-entity user-data state machine
 *   (0x801394B8-0x8013AC08).
 * RANGE. .text 0x80138074-0x8013ACC4 (73 functions); .data 0x805A1358-0x805A1530, .sdata 0x807919F0-0x80791A20,
 *   .sdata2 0x80796D40-0x80796D90, extab, extabindex.
 * NAMES. MTX33_ctor is a GUESS (the 3x3 matrix constructor, the MTX34_ctor scheme).  user_data_type_names is a GUESS
 *   (the TYPE strings ExecCallbackC compares).  Registry names (foreign rows): res_user_data_ctor (0x8008A220, the
 *   `g3d_resuser_ac.h` handle constructor) and res_node_user_data_get (0x80099378, `ResNode::GetResUserData`).
 *   The map stem; the `ResUserData`/`ResUserDataItem` panics name the NW4R accessor header `g3d_resuser_ac.h`,
 *   not this file.  `em_res_user_data_set` is a GUESS (it installs a callback).
 *   From their bodies: em_userdata_motion_head_load is a GUESS (0x8013817C); em_userdata_state_exit is a GUESS
 *   (0x8013AACC); em_userdata_state_reenter_alt is a GUESS (0x8013AB6C); em_userdata_roll is a GUESS (0x8013AC00);
 *   em_userdata_record_find_back is a GUESS (0x8013AC08); em_res_user_data_ck is a GUESS (0x801391E8);
 *   joint_mtx_store is a GUESS (0x80139A64); joint_mtx_load is a GUESS (0x80139A7C).
 *   From their bodies, GUESS: em_motion_settle (0x801381F4), em_fx_slot_step (0x80138EC8), em_pl_distance_update
 *   (0x80138F3C), em_turn_joint_update (0x801391FC), joint_mtx_set (0x80139A98), joint_local_mtx_get (0x80139AA4),
 *   em_user_data_timing_flags_get (0x8013A6F4), em_user_data_next_node_set (0x8013A770), em_userdata_state_reset
 *   (0x8013A954), em_userdata_state_select (0x8013AA1C).  The `res_user_data_*` / `res_user_data_item_*` accessors
 *   (0x8013A218..0x8013A648) are named from their `ResUserData` / `ResUserDataItem` panics; the `_const` pair of
 *   ref/ptr/ofs_to_ptr and `res_user_data_item_tail_assign` are GUESS spellings.  Registry names (foreign rows):
 *   anm_chr_result_rot_mtx_get (0x8008DF8C, `ChrAnmResult::GetRotMtx`).
 *   `ResUserDataAc` (`enemy/res_user_data_ac.h`) is the owner's spelling; it derives from nw4r's
 *   `ICalcWorldCallback` (the base constructor stores that class's table first) and overrides ExecCallbackA/B/C.
 *   Its six hooks are GUESS names from the dispatch: Attach (+0x18, run by `em_res_user_data_set`), CalcAnmItem /
 *   CalcAnmItemEx (+0x1C/+0x20, ExecCallbackA's items), CalcMtxItem / CalcMtxItemEx (+0x24/+0x28, ExecCallbackB's),
 *   CalcWorldEnd (+0x2C, run at the end of ExecCallbackC).
 * RESIDUALS. Every row is written; 70 of 73 rows match.
 *  - `ExecCallbackC`: the TYPE-name loop reuses the test's `lwz r4,0(name)` in the body where retail reloads it
 *    (`lwz r0` in the test, `lwz r4` in the body; `opt_common_subs`, `opt_propagation`, `opt_loop_invariants` off
 *    and a `while` form do not change it), and `kind`/`type` swap r21/r23;
 *  - `em_turn_joint_update`: `best` takes r28 where retail gives it r26 (every declaration order measured);
 *  - `fn_8013823C`: the `field_0x88C` slot test loads into r0 where retail loads r3.
 *  - flipcheck: `.data` and `.sdata` claimed, not emitted as retail's: the accessor strings (0x805A1398..0x805A1530)
 *    the panics name through `lbl_805A13xx` externs, and the `.sdata` literals merge under `-str reuse` (one `"ref"`
 *    where retail keeps three, so the order and size differ);
 *  - flipcheck: the destructor calls ICalcWorldCallback's destructor out of line (`g3d/g3d_scnmdlsmpl`'s kept
 *    copy), so the object carries that inline destructor's weak copy (`.text` 0x2C8C against 0x2C50 before the
 *    link dead-strips it); extabindex differs in one byte;
 *  - flipcheck: `anm_chr_result_rot_mtx_get`, `res_node_user_data_get` and `res_user_data_ctor` are registry names
 *    the map does not carry yet.
 * SHAPES. `#pragma peephole off` is scoped to the bodies whose retail keeps the unfused `clrlwi`/`rlwinm` + `cmpwi`,
 *   `clrlwi` + `slwi`, narrowing masks and the `li r0,N; psq_lx` FPR epilogue (ExecCallbackB/C and em_post_update
 *   among them).  Early exits: `if (item == NULL) return;` ahead of the item loops (ExecCallbackA/B,
 *   em_user_data_next_node_set); fn_80138074's two guards are one `||` condition (playbook 255).  Type tests read
 *   from the compare chain: a `switch` gives `cmpwi` (ExecCallbackC's 0/3 test, the 0x1E2 and 0x1E5 tests),
 *   em_user_data_timing_flags_get is `(u32)(type - 1) > 1` around a `switch`, em_userdata_state_kind_get puts
 *   `default:` first.  Counters use `++`/`--` (a cast assignment keeps a mask retail does not have);
 *   em_turn_joint_update's up/down steps are `u32` temporaries stored as `(s16)`, its `total` a 32-bit accumulator.
 *   Loops walk a pointer beside the index where retail increments one (em_pl_distance_update's `move++`,
 *   fn_8013823C's `record`).  The float pool comes from literals (first-use order is retail's); compound `-=`/`+=`/
 *   `*=` keep fn_8013823C's operand order.  fn_8013823C's add/scale results are their own temporaries (its two
 *   unused constructed VEC3s stay).  The unit's own symbols are plain C names in one `extern "C"` block; the nw4r
 *   members are called as C++.  ExecCallbackC calls `calcMtxAngX` through a `u32` view (retail converts its
 *   result unmasked).  The destructor is compiled under `#pragma dont_inline on`: retail calls the base destructor
 *   out of line (`li r4,0; bl`), `-inline noauto` inlines it away.
 */

struct _ENEMY_WORK; /* file scope, so the leaf headers' `struct _ENEMY_WORK*` parameters name this type (C scopes a struct first named in a prototype) */
#include "enemy/em_motion_param_set.h" /* em_motion_param_set (rule 2: the owner's header) */
#include "enemy/get_enemy_data__FP11_ENEMY_WORK.h" /* get_enemy_data__FP11_ENEMY_WORK (rule 2: the owner's header) */
#include "enemy/em_owner_slot_update.h" /* em_owner_slot_update (rule 2: the owner's header) */
#include "enemy/fn_80126098.h" /* fn_80126098 (rule 2: the owner's header) */
#include "enemy/em_data_point_get.h" /* em_data_point_get (rule 2: the owner's header) */
#include "enemy/em_hook_get.h" /* em_hook_get (rule 2: the owner's header) */
#include "enemy/em_hit_mask_get.h" /* em_hit_mask_get (rule 2: the owner's header) */
#include "enemy/fn_801281EC.h" /* fn_801281EC (rule 2: the owner's header) */
#include "enemy/fn_801281F8.h" /* fn_801281F8 (rule 2: the owner's header) */
#include "enemy/fn_80128204.h" /* fn_80128204 (rule 2: the owner's header) */
#include "enemy/em_delay_act_tick.h" /* em_delay_act_tick (rule 2: the owner's header) */
#include "enemy/em_target_pos_set.h" /* em_target_pos_set (rule 2: the owner's header) */
#include "enemy/em_timers_update.h" /* em_timers_update (rule 2: the owner's header) */
#include "enemy/em_ground_probe.h" /* em_ground_probe (rule 2: the owner's header) */
#include "enemy/em_cell_span_update.h" /* em_cell_span_update (rule 2: the owner's header) */
#include "enemy/em_area_change.h" /* em_area_change (rule 2: the owner's header) */
#include "enemy/em_hate_update.h" /* em_hate_update (rule 2: the owner's header) */
#include "enemy/em_busy_set.h" /* em_busy_set (rule 2: the owner's header) */
#include "enemy/em_idle_flag_get.h" /* em_idle_flag_get (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "enemy/fn_8012BDF4.h"
#include "Pl/yure_joint_apply.h"
#include "g3d/g3d_calcworld.h"
#include "sys_mem.h"
#include "ef/ef_torus.h" /* fn_800C9DCC (rule 2) */
#include "unsplit/enemy.h"
#include "g3d/g3d_rescommon.h" /* nw4r::g3d::ResDic (rule 2) */
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */
#include "g3d/anm_chr_result_rot_mtx_get.h" /* anm_chr_result_rot_mtx_get (rule 2: the owner's leaf header) */
#include "nw4r/math_types.h" /* nw4r::math::MTX34ToMTX33 (rule 2) */
#include "g3d/res_user_data_ctor.h" /* res_user_data_ctor (rule 2: the owner's leaf header) */
#include "g3d/res_node_user_data_get.h" /* res_node_user_data_get (rule 2: the owner's leaf header) */
#include "rotMatrixX.h" /* rotMatrixX / rotMatrixY (rule 9: the owner's C++ declarations) */
#include "vec3_move_damp.h" /* vec3_move_damp (rule 2: the owner's leaf header) */
#include "calcMtxAngX.h" /* calcMtxAngX (rule 2: the owner's leaf header) */
#include "calcMtxAng3.h" /* calcMtxAng3 (rule 2: the owner's header) */
#include "sound/mhchar.h" /* MHchar::move (rule 2) */
#include "enemy/fn_8013ACC4.h" /* em_userdata_script_run (rule 2) */
#include "g3d/g3d_calcview.h" /* fn_800710BC/fn_8006FDCC/mtx34_copy_ps (rule 2) */
#include "g3d/g3d_scnmdlsmpl.h" /* ScnMdlSimple, FuncObjCalcWorld (rule 2) */
#include "enemy/res_user_data_ac.h" /* ResUserDataAc, UserDataItem (rule 1) */
#include "nw4r/db_assert.h" /* nw4r::db::Panic, owner nw4r/db_assert.cpp (rule 2) */
#include "g3d/g3d_state.h"
#include "unsplit/sound.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "unsplit/enemy_pool.h" /* the enemy band's .data pool labels, declared in the band header */
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define calcMtxAngX_c1 ((u32 (*)(Mtx34*))calcMtxAngX)

/* The unit's own symbols and its callees' map spellings are plain C names. */
extern "C" {

/* --------------------------------------------------------------------------------------------- */
/* shared pool symbols (another unit owns the bytes)                                              */
/* --------------------------------------------------------------------------------------------- */



/* --------------------------------------------------------------------------------------------- */
/* math types (the mangled callees take nw4r::math spellings)                                     */
/* --------------------------------------------------------------------------------------------- */

/* `Vec3` (three `f32`) and `Mtx34` (a 3x4 `f32` matrix) are the `nw4r/math.h` types, now includable
 * from C; the mangled callees below take those spellings. */

/* `_CP_VECTOR` as `cpSetRotMatrix` takes it: three 16-bit angles widened to words. */
typedef struct CPVector { /* size: 0x0C */
    /* +0x00 */ u32 x;
    /* +0x04 */ u32 y;
    /* +0x08 */ u32 z;
} CPVector;

/* --------------------------------------------------------------------------------------------- */
/* forward declarations of this unit's own symbols                                               */
/* --------------------------------------------------------------------------------------------- */

/* The record's own tag is `_ENEMY_WORK` (the name the map's `em_*__FP11_ENEMY_WORK...` manglings
 * encode); `EnemyWork` is this unit's local spelling of it.  The fields below are this unit's names
 * for the offsets it reads. */
typedef struct _ENEMY_WORK EnemyWork;
typedef struct EnemyData EnemyData;
typedef struct EmDataRecord EmDataRecord;
typedef struct KeyFrameSet KeyFrameSet;
typedef struct ResUserData ResUserData;
typedef struct ResUserDataItemData ResUserDataItemData;
typedef struct MtxHolder MtxHolder;
typedef struct MHchar MHchar;
typedef struct EmCharBase EmCharBase;
#define EM_CHAR(self) ((MHchar*)&(self)->char_0x024)
typedef struct WorkSlot WorkSlot;
typedef struct EffectArea EffectArea;
typedef struct MoveSlot MoveSlot;
typedef struct WorkRecord8 WorkRecord8;
typedef struct MoveWork MoveWork;

void fn_80138074(EnemyWork* self, u8 arg1);
void em_userdata_motion_head_load(EnemyWork* self);
void em_motion_settle(EnemyWork* self);
void fn_8013823C(EnemyWork* self);
void em_post_update(EnemyWork* self);
void fn_80138E18(EnemyWork* self);
void fn_80138E28(EnemyWork* self);
void em_char_pose_sync(EnemyWork* self);
void em_fx_slot_step(EnemyWork* self, u32 arg1);
void em_pl_distance_update(EnemyWork* self);
void fn_80139024(EnemyWork* self);
void em_res_user_data_set(EnemyWork* self, ResUserDataAc* arg1);
s32 em_res_user_data_ck(EnemyWork* self);
void em_turn_joint_update(EnemyWork* self);
void joint_mtx_store(MtxHolder* dst, void* src);
void joint_mtx_load(MtxHolder* holder, void* mtx);
void joint_mtx_set(MtxHolder* holder, void* src);
void joint_local_mtx_get(Mtx34* out, void* arg1, Mtx34* arg2);
ResUserDataItemData* res_user_data_item_s32_get(ResUserData* self);
void* res_user_data_item_ofs_to_ptr(ResUserData* self, s32 arg1);
ResUserDataItemData* res_user_data_item_ref(ResUserData* self);
ResUserDataItemData* res_user_data_item_ptr(ResUserData* self);
const char* res_user_data_item_class_name(void);
s32 res_user_data_item_value_type_get(ResUserData* self);
ResUserDataItemData* res_user_data_item_ref_const(ResUserData* self);
ResUserDataItemData* res_user_data_item_ptr_const(ResUserData* self);
ResUserDataItemData* res_user_data_item_string_get(ResUserData* self);
void* res_user_data_item_ofs_to_ptr_const(ResUserData* self, s32 arg1);
s32 res_user_data_item_valid_ck(ResUserData* self);
ResUserData* res_user_data_item_assign(ResUserData* dst, ResUserData* src);
void res_user_data_item_tail_assign(ResUserData* dst, ResUserData* src);
void res_user_data_item_ptr_assign(ResUserData* dst, ResUserData* src);
ResUserDataItemData* res_user_data_item_find(ResUserData* self, const char* key);
ResUserDataItemData* res_user_data_ref(ResUserData* self);
ResUserDataItemData* res_user_data_ptr(ResUserData* self);
const char* res_user_data_class_name(void);
u32 res_user_data_entry_num_get(ResUserData* self);
s32 res_user_data_valid_ck(ResUserData* self);
ResUserData* res_user_data_assign(ResUserData* self, ResUserData* src);
void res_user_data_ptr_assign(ResUserData* dst, ResUserData* src);
ResUserData* res_user_data_item_ctor(ResUserData* self, ResUserDataItemData* data);
void res_user_data_item_ptr_set(ResUserData* self, ResUserDataItemData* data);
void em_res_user_data_flag_set(ResUserDataAc* self, u32 arg1);
UserDataItem* em_user_data_items_get(EnemyWork* work);
s32 em_user_data_timing_flags_get(EnemyWork* work);
void em_user_data_next_node_set(nw4r::g3d::FuncObjCalcWorld* cursor, EnemyWork* work);
s32 em_break_part_data_get(EnemyWork* work, u8 arg1);
u32 fn_8013A884(EnemyWork* work, u8 arg1);
s32 fn_8013A8B4(EnemyWork* work, u8 arg1, u8 arg2);
s32 em_userdata_state_kind_get(EnemyWork* work);
void em_userdata_state_reset(EnemyWork* work, u8 arg1);
void fn_8013A978(EnemyWork* work);
void fn_8013A9F4(EnemyWork* work);
void fn_8013AA00(EnemyWork* work);
void em_userdata_state_select(EnemyWork* work, u8 arg1);
void em_user_data_reenter(EnemyWork* work);
void em_userdata_state_exit(EnemyWork* work, u8 arg1);
void em_userdata_state_reenter_alt(EnemyWork* work);
s32 fn_8013AB74(EnemyWork* work, u8 arg1, u8 arg2);
void em_userdata_roll(void);
u8 em_userdata_record_find_back(EnemyWork* work, u8 arg1, u8 arg2);

/* --------------------------------------------------------------------------------------------- */
/* callees (map spellings)                                                                       */
/* --------------------------------------------------------------------------------------------- */

extern s32 strcmp(const char* a, const char* b);

extern void mtx34_identity(void* mtx);
extern void subVec3(Vec3* out, const Vec3* a, const Vec3* b);
extern f32 vec3_distance(const Vec3* a, const Vec3* b);
extern f32 calcVecDistXZ(const Vec3* a, const Vec3* b);
extern void addVec3(Vec3* out, const Vec3* a, const Vec3* b);
extern void MTX33_ctor(nw4r::math::MTX33* mtx);
extern void mtx34_blend(Mtx34* out, const Mtx34* a, const Mtx34* b, f32 wa, f32 wb, u8 keep_unit);
extern void vec3_scale(Vec3* out, const Vec3* v, f32 scale);
extern f32 vec3_dot(const Vec3* a, const Vec3* b);
extern void mtx34_copy(Mtx34* out, const Mtx34* src);
extern void res_node_assign(void* out, const void* src);
extern s32 res_node_get_parent(void* arg0);
extern void anm_chr_result_rot_mul(void* arg0, const nw4r::math::MTX33* mtx);
extern void anm_chr_result_trans_set(void* arg0, const Vec3* v);
extern f32 getKeyData__FPff(s32 key, f32 t);
extern void cpSetRotMatrix__FP10_CP_VECTORPQ34nw4r4math5MTX34(CPVector* angles, Mtx34* mtx);
extern void rotMatrixX__FUlPQ34nw4r4math5MTX34(u16 angle, Mtx34* mtx);
extern void rotMatrixY__FUlPQ34nw4r4math5MTX34(u16 angle, Mtx34* mtx);
extern void copyMat33__FPQ34nw4r4math5MTX34PQ34nw4r4math5MTX34(Mtx34* dst, const Mtx34* src);
extern void setVector3__FPQ34nw4r4math4VEC3fff(Vec3* v, f32 x, f32 y, f32 z);
extern s32 calcVecAng2__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3(const Vec3* a, const Vec3* b);
extern void rotVecY__FPQ34nw4r4math4VEC3Ul(Vec3* v, u32 angle);
extern void get_worldworld_pos__FPQ34nw4r4math4VEC3Uc(Vec3* out, const Vec3* v, u8 area);
extern void* get_move_work_adrs__FUc(u8 area);
extern u16 get_move_work_max__FUc(u8 area);
extern u8 get_now_areano__Fv(void);
extern s32 event_demo_ck__Fv(void);
extern s32 ran_suu__Fl(s32 n);

extern void em_damage_reenter_ck(EnemyWork* work);
extern u32 em_damage_eval(EnemyWork* work);
extern void em_coll_shape_set(EnemyWork* work, u8 arg1, u16 arg2);
extern void hit_attack_list_push(void* arg0);
extern void hit_knock_set(void* arg0, s32 arg1);
extern void get_worldlocal_pos(Vec3* out, const Vec3* v, u8 area);
extern f32 stg_area_ceiling_get(u8 area);
extern u8 stage_map_kind_get(u8 arg0);
extern void em_roster_record_release(s32 arg0);
extern s32 em_roster_record_slot_id_get(s32 arg0);
extern s32 em_roster_record_alive_ck(void);

/* --------------------------------------------------------------------------------------------- */
/* reconstructed types                                                                            */
/* --------------------------------------------------------------------------------------------- */

/* The lower-bound view of the `MHchar` base the work block embeds at +0x24 (0x40 bytes of the actor's character object);
 * calls reach it as `MHchar*`, whose definition is `pl.h`'s. */
struct EmCharBase { /* size: 0x40 */
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ Vec3 field_0x04;
    /* +0x10 */ u8 pad_0x10[0x18];
    /* +0x28 */ u32 field_0x28;
    /* +0x2C */ u32 field_0x2C;
    /* +0x30 */ u32 field_0x30;
    /* +0x34 */ u8 field_0x34;
    /* +0x35 */ u8 field_0x35;
    /* +0x36 */ u8 pad_0x36[0x0A];
};

/* One 0x0C-byte motion record of `EnemyData::records`. */
struct EmDataRecord { /* size: 0x0C */
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ f32 field_0x04;
    /* +0x08 */ u8 field_0x08;
    /* +0x09 */ u8 field_0x09;
    /* +0x0A */ u8 field_0x0A;
    /* +0x0B */ u8 field_0x0B;
};

/* The keyframe block a `UserDataItem` may point at (+0x14). */
struct KeyFrameSet { /* size: 0x18 */
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u32 field_0x0C;
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u32 field_0x14;
};

/* `get_enemy_data(work)`'s result. */
struct EnemyData { /* size: 0x9C */
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 pad_0x01[0x03];
    /* +0x04 */ EmDataRecord* records;
    /* +0x08 */ u16 field_0x08;
    /* +0x0A */ u16 field_0x0A;
    /* +0x0C */ u8 pad_0x0C[0x88];
    /* +0x94 */ UserDataItem* items;
    /* +0x98 */ s32* table_0x98;
};

/* One 0x5C-byte effect slot the work block carries two of. */
struct WorkSlot { /* size: 0x5C */
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 field_0x05;
    /* +0x06 */ u8 pad_0x06[0x56];
};

/* One 0x84-byte per-area record at work+0x488 (two of them, then work+0x590). */
struct EffectArea { /* size: 0x84 */
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 pad_0x01[0x06];
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ u8 pad_0x08[0x02];
    /* +0x0A */ s16 field_0x0A;
    /* +0x0C */ u8 pad_0x0C[0x20];
    /* +0x2C */ Mtx34 matrix;
    /* +0x5C */ u8 pad_0x5C[0x28];
};

/* One 0x90-byte record at work+0x590 (the per-area matrix slots). */
struct MoveSlot { /* size: 0x90 */
    /* +0x00 */ u8 pad_0x00[0x90];
};

/* One 8-byte record of the `field_0x9AC` array. */
struct WorkRecord8 { /* size: 0x08 */
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 pad_0x01[0x07];
};

/* One 0xB20-byte move-work record (`get_move_work_adrs`). */
struct MoveWork { /* size: 0xB20 */
    /* +0x000 */ u8 field_0x00;
    /* +0x001 */ u8 pad_0x001[0x07];
    /* +0x008 */ u8 field_0x08;
    /* +0x009 */ u8 pad_0x009[0x33];
    /* +0x03C */ Vec3 field_0x3C;
    /* +0x048 */ u8 pad_0x048[0xAD8];
};

/* A struct holding one matrix pointer (the `joint_mtx_store`/`joint_mtx_load` argument). */
struct MtxHolder { /* size: 0x04 */
    /* +0x00 */ void* mtx;
};

/* The value header a `ResUserData` points at. */
struct ResUserDataItemData { /* size: 0x10 */
    /* +0x00 */ u8 pad_0x00[0x04];
    /* +0x04 */ u32 field_0x04;      /* value offset inside the blob */
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u32 field_0x0C;      /* value type */
};

struct ResUserData { /* size: 0x04 */
    /* +0x00 */ ResUserDataItemData* data;
};

/* The enemy work block.  Only the fields this unit touches are named; the rest are padding that
 * keeps every measured offset in place. */
struct _ENEMY_WORK { /* size: 0xB00 */
    /* +0x000 */ u8 field_0x000;
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ u8 pad_0x002[0x02];
    /* +0x004 */ u8 field_0x004;
    /* +0x005 */ u8 field_0x005;
    /* +0x006 */ u8 pad_0x006[0x02];
    /* +0x008 */ u8 field_0x008;
    /* +0x009 */ u8 pad_0x009[0x03];
    /* +0x00C */ u8 field_0x00C;
    /* +0x00D */ u8 pad_0x00D[0x0A];
    /* +0x017 */ u8 field_0x017;
    /* +0x018 */ s16 field_0x018;
    /* +0x01A */ u8 pad_0x01A[0x0A];
    /* +0x024 */ EmCharBase char_0x024; /* the `MHchar` base, passed on through `EM_CHAR` */
    /* +0x064 */ u32 field_0x064;
    /* +0x068 */ u8 pad_0x068[0xA8];
    /* +0x110 */ u32 field_0x110;
    /* +0x114 */ u8 pad_0x114[0x28];
    /* +0x13C */ u32 field_0x13C;
    /* +0x140 */ u8 pad_0x140[0x48];
    /* +0x188 */ Vec3 field_0x188;
    /* +0x194 */ Vec3 field_0x194;
    /* +0x1A0 */ Vec3 field_0x1A0;
    /* +0x1AC */ f32 field_0x1AC;
    /* +0x1B0 */ u8 pad_0x1B0[0x0C];
    /* +0x1BC */ u32 field_0x1BC;
    /* +0x1C0 */ u32 field_0x1C0;
    /* +0x1C4 */ u32 field_0x1C4;
    /* +0x1C8 */ u32 flags_0x1C8;
    /* +0x1CC */ u8 pad_0x1CC[0x08];
    /* +0x1D4 */ f32 field_0x1D4;
    /* +0x1D8 */ u8 pad_0x1D8[0x06];
    /* +0x1DE */ u8 field_0x1DE;
    /* +0x1DF */ u8 field_0x1DF;
    /* +0x1E0 */ u8 field_0x1E0;
    /* +0x1E1 */ u8 field_0x1E1;
    /* +0x1E2 */ u8 field_0x1E2;
    /* +0x1E3 */ u8 field_0x1E3;
    /* +0x1E4 */ u8 pad_0x1E4[0x01];
    /* +0x1E5 */ u8 field_0x1E5;
    /* +0x1E6 */ u8 pad_0x1E6[0x01];
    /* +0x1E7 */ u8 field_0x1E7;
    /* +0x1E8 */ u8 pad_0x1E8[0x11];
    /* +0x1F9 */ u8 field_0x1F9;
    /* +0x1FA */ u8 field_0x1FA;
    /* +0x1FB */ u8 field_0x1FB;
    /* +0x1FC */ u8 pad_0x1FC[0x01];
    /* +0x1FD */ u8 field_0x1FD;
    /* +0x1FE */ u8 pad_0x1FE[0x0E];
    /* +0x20C */ f32 field_0x20C;
    /* +0x210 */ u8 pad_0x210[0x08];
    /* +0x218 */ u32 field_0x218;
    /* +0x21C */ u8 pad_0x21C[0x14];
    /* +0x230 */ Vec3 field_0x230;
    /* +0x23C */ u8 pad_0x23C[0xD0];
    /* +0x30C */ u8 field_0x30C;
    /* +0x30D */ u8 field_0x30D;
    /* +0x30E */ u8 field_0x30E;
    /* +0x30F */ u8 field_0x30F;
    /* +0x310 */ u8 pad_0x310[0x5C];
    /* +0x36C */ Vec3 field_0x36C;
    /* +0x378 */ u8 pad_0x378[0x08];
    /* +0x380 */ u8 field_0x380;
    /* +0x381 */ u8 pad_0x381[0x01];
    /* +0x382 */ u8 field_0x382;
    /* +0x383 */ u8 pad_0x383[0xB6];
    /* +0x439 */ u8 field_0x439;
    /* +0x43A */ u8 pad_0x43A[0x04];
    /* +0x43E */ u8 field_0x43E;
    /* +0x43F */ u8 field_0x43F;
    /* +0x440 */ u8 pad_0x440[0x14];
    /* +0x454 */ f32 field_0x454[5];
    /* +0x468 */ u8 pad_0x468[0x02];
    /* +0x46A */ u8 field_0x46A;
    /* +0x46B */ u8 pad_0x46B[0x05];
    /* +0x470 */ Vec3 field_0x470;
    /* +0x47C */ u8 pad_0x47C[0x08];
    /* +0x484 */ ResUserDataAc* field_0x484;
    /* +0x488 */ EffectArea field_0x488[2];
    /* +0x590 */ MoveSlot field_0x590[3];
    /* +0x740 */ u8 field_0x740;
    /* +0x741 */ u8 pad_0x741[0x01];
    /* +0x742 */ u16 field_0x742[0x10];
    /* +0x762 */ u8 field_0x762;
    /* +0x763 */ u8 pad_0x763[0x01];
    /* +0x764 */ s16 field_0x764;
    /* +0x766 */ u8 field_0x766;
    /* +0x767 */ u8 field_0x767;
    /* +0x768 */ s16 field_0x768;
    /* +0x76A */ u8 pad_0x76A[0x02];
    /* +0x76C */ Vec3 field_0x76C;
    /* +0x778 */ Vec3 field_0x778;
    /* +0x784 */ u8 field_0x784;
    /* +0x785 */ u8 pad_0x785[0x15];
    /* +0x79A */ u8 field_0x79A;
    /* +0x79B */ u8 pad_0x79B[0x75];
    /* +0x810 */ u16 field_0x810;
    /* +0x812 */ u16 field_0x812;
    /* +0x814 */ u8 pad_0x814[0x78];
    /* +0x88C */ s32 field_0x88C[3];
    /* +0x898 */ s16 field_0x898[3];
    /* +0x89E */ u8 field_0x89E;
    /* +0x89F */ u8 field_0x89F;
    /* +0x8A0 */ u8 pad_0x8A0[0x14];
    /* +0x8B4 */ Vec3 field_0x8B4;
    /* +0x8C0 */ u8 field_0x8C0;
    /* +0x8C1 */ u8 pad_0x8C1[0x89];
    /* +0x94A */ u16 field_0x94A;
    /* +0x94C */ u16 field_0x94C;
    /* +0x94E */ u8 pad_0x94E[0x06];
    /* +0x954 */ u32** field_0x954;
    /* +0x958 */ u32 field_0x958;
    /* +0x95C */ u8 field_0x95C;
    /* +0x95D */ u8 field_0x95D;
    /* +0x95E */ u8 pad_0x95E[0x01];
    /* +0x95F */ u8 field_0x95F;
    /* +0x960 */ u8 field_0x960;
    /* +0x961 */ u8 field_0x961;
    /* +0x962 */ u8 field_0x962;
    /* +0x963 */ u8 pad_0x963[0x36];
    /* +0x999 */ u8 field_0x999;
    /* +0x99A */ u8 field_0x99A;
    /* +0x99B */ u8 field_0x99B;
    /* +0x99C */ u8 field_0x99C;
    /* +0x99D */ u8 pad_0x99D[0x07];
    /* +0x9A4 */ EnemyData* field_0x9A4;
    /* +0x9A8 */ u8 field_0x9A8;
    /* +0x9A9 */ u8 pad_0x9A9[0x03];
    /* +0x9AC */ WorkRecord8 field_0x9AC[7];
    /* +0x9E4 */ u8 pad_0x9E4[0x04];
    /* +0x9E8 */ u8 field_0x9E8;
    /* +0x9E9 */ u8 pad_0x9E9[0x07];
    /* +0x9F0 */ u32 field_0x9F0;
    /* +0x9F4 */ u8 pad_0x9F4[0x03];
    /* +0x9F7 */ u8 field_0x9F7;
    /* +0x9F8 */ u8 pad_0x9F8[0x03];
    /* +0x9FB */ u8 field_0x9FB;
    /* +0x9FC */ u8 field_0x9FC;
    /* +0x9FD */ u8 field_0x9FD;
    /* +0x9FE */ u8 field_0x9FE;
    /* +0x9FF */ u8 field_0x9FF;
    /* +0xA00 */ u32 field_0xA00;
    /* +0xA04 */ u8 pad_0xA04[0x04];
    /* +0xA08 */ WorkSlot field_0xA08[2];
    /* +0xAC0 */ u8 pad_0xAC0[0x2C];
    /* +0xAEC */ u8 field_0xAEC[2];
    /* +0xAEE */ u8 pad_0xAEE[0x12];
};

/* --------------------------------------------------------------------------------------------- */
/* 0x80138074..0x801391E8 - the ENEMY_WORK per-frame driver                                       */
/* --------------------------------------------------------------------------------------------- */

#pragma peephole off
/* Applies the work's current rotation to the EmCharBase base and pushes the animation frame. */
void fn_80138074(EnemyWork* self, u8 arg1) {
    Vec3 saved;
    Vec3 offset;

    VEC3_ctor(&saved);
    VEC3_ctor(&offset);
    if (self->field_0x30C == 1 || (self->flags_0x1C8 & 0x400) != 0) {
        return;
    }
    if (self->field_0x1E2 == 1 && self->field_0x1E3 == 1) {
        copyVec3(&saved, &self->field_0x188);
        setVector3__FPQ34nw4r4math4VEC3fff(&offset, 0.0f, 0.0f, 50.0f);
        rotVecY__FPQ34nw4r4math4VEC3Ul(&offset, self->field_0x1C0);
        addVec3To(&self->field_0x188, &offset);
        em_coll_shape_set(self, arg1, em_hit_mask_get(self));
        if (self->field_0x218 == 0) {
            copyVec3(&self->field_0x188, &saved);
        }
    } else {
        em_coll_shape_set(self, arg1, em_hit_mask_get(self));
    }
}

#pragma peephole on
/* Loads the current motion's first frame into the EmCharBase base. */
void em_userdata_motion_head_load(EnemyWork* self) {
    EnemyData* data = self->field_0x9A4;

    if (data->field_0x00 == 0xFE || data->field_0x00 == 0xFC) {
        self->field_0x1F9 = 0;
    } else {
        em_data_point_get(self, data->field_0x08, &self->field_0x188);
        self->field_0x1C0 = self->field_0x9A4->field_0x0A;
        em_area_change(self, 0);
    }
    fn_801408B4(self);
}

/* Steps the motion sequence until it settles on state 2. */
void em_motion_settle(EnemyWork* self) {
    while (self->field_0x95C != 2) {
        if (fn_801408B4(self) == 0) {
            break;
        }
    }
}

#pragma peephole off
/* Per-frame update: ages the action timers, resolves the user-data target, then runs the state. */
void fn_8013823C(EnemyWork* self) {
    Vec3 vD4;
    Vec3 vC8;
    Vec3 vBC;
    Vec3 vB0;
    Vec3 vA4;
    Vec3 v98;
    Vec3 v8C;
    Vec3 v80;
    Vec3 v74;
    Vec3 v68;
    Vec3 v5C;
    Vec3 v50;
    Vec3 v44;
    Vec3 v38;
    Vec3 v2C;
    Vec3 v20;
    Vec3 v14;
    Vec3 v8;
    u8 i;
    u32 flag;
    u8 was_special;

    VEC3_ctor(&vD4);
    VEC3_ctor(&vC8);
    VEC3_ctor(&vBC);
    VEC3_ctor(&vB0);
    VEC3_ctor(&vA4);
    VEC3_ctor(&v98);
    VEC3_ctor(&v8C);
    VEC3_ctor(&v80);
    VEC3_ctor(&v74);
    flag = 0;
    for (i = 0; i < 3; i++) {
        if (event_demo_ck__Fv() == 0 && self->field_0x46A == 0) {
            if (self->field_0x898[i] > 0) {
                self->field_0x898[i]--;
            }
        }
        {
            s32* record = &self->field_0x88C[i];
            if (*record != -1) {
                if (em_roster_record_alive_ck() == 0) {
                    if (em_roster_record_slot_id_get(*record) > 0) {
                        self->field_0x89E++;
                    }
                    *record = -1;
                } else if (self->field_0x898[i] <= 0) {
                    if (em_roster_record_slot_id_get(*record) > 0) {
                        self->field_0x89E++;
                    }
                    em_roster_record_release(*record);
                    *record = -1;
                }
            }
        }
    }
    em_owner_slot_update(self);
    if (self->field_0x1F9 == 1 && self->field_0x1E1 != self->field_0x9F7) {
        if (self->field_0x380 == 8) {
            if (em_busy_ck(self) == 1 && (self->field_0x99A & 1) == 0 && self->field_0x99B == 1 &&
                self->field_0x382 == 0) {
                if (calcVecDistXZ(&self->field_0x188, &self->field_0x36C) <=
                    self->field_0x9A4->records->field_0x04) {
                    em_motion_settle(self);
                    em_userdata_motion_head_load(self);
                    em_finish_flag_set(self);
                }
            }
        } else if (self->field_0x1FA == 1 && self->field_0x1FB == 1) {
            if (stage_map_kind_get(self->field_0x1E0) == 5 && self->field_0x1E1 == 3) {
                setVector3__FPQ34nw4r4math4VEC3fff(&v8C, 7000.0f, 0.0f, 5000.0f);
            } else {
                setVector3__FPQ34nw4r4math4VEC3fff(&v8C, 0.0f, 0.0f, 0.0f);
            }
            get_worldworld_pos__FPQ34nw4r4math4VEC3Uc(&v68, &v8C, self->field_0x1E1);
            copyVec3(&vC8, &v68);
            if (stage_map_kind_get(self->field_0x1E0) == 5 && self->field_0x9F7 == 3) {
                setVector3__FPQ34nw4r4math4VEC3fff(&v8C, 7000.0f, 0.0f, 5000.0f);
            } else {
                setVector3__FPQ34nw4r4math4VEC3fff(&v8C, 0.0f, 0.0f, 0.0f);
            }
            get_worldworld_pos__FPQ34nw4r4math4VEC3Uc(&v5C, &v8C, self->field_0x9F7);
            copyVec3(&vBC, &v5C);
            get_worldworld_pos__FPQ34nw4r4math4VEC3Uc(&v50, &self->field_0x188, self->field_0x1E1);
            copyVec3(&vD4, &v50);
            addVec3(&v38, &vC8, &vBC);
            vec3_scale(&v44, &v38, 0.5f);
            copyVec3(&vB0, &v44);
            subVec3(&v2C, &vBC, &vB0);
            copyVec3(&vA4, &v2C);
            subVec3(&v20, &vD4, &vB0);
            copyVec3(&v98, &v20);
            if (vec3_dot(&vA4, &v98) >= 0.0f) {
                get_worldlocal_pos(&v14, &vD4, self->field_0x9F7);
                copyVec3(&self->field_0x188, &v14);
                self->field_0x188.y = stg_area_ceiling_get(self->field_0x9F7);
                em_area_change(self, 0);
            }
        }
    }
    if (self->field_0x1E1 != get_now_areano__Fv()) {
        if ((self->flags_0x1C8 & 8) == 0) {
            self->field_0x017 = 5;
            return;
        }
        if ((self->flags_0x1C8 & 1) == 0) {
            self->field_0x810 = 0;
            self->field_0x94A = 0;
            self->field_0x94C = 0;
            self->field_0x812 = 0;
            em_poison_init(self);
        }
    }
    em_delay_act_tick(self);
    setVector3__FPQ34nw4r4math4VEC3fff(&self->field_0x1A0, 0.0f, 0.0f, 0.0f);
    self->field_0x1DF = 0;
    self->field_0x79A = 0;
    self->field_0x784 = 0;
    self->field_0x30C = 0;
    self->field_0x30D = 0;
    self->field_0x30E = 0;
    self->field_0x30F = 0;
    copyVec3(&self->field_0x194, &self->field_0x188);
    was_special = self->field_0x1E2;
    em_approach_3d_clear(self);
    if (self->field_0x767 == 1) {
        copyVec3(&self->field_0x778, &self->field_0x76C);
    }
    self->field_0x767 = 1;
    self->field_0x740 = 0;
    self->field_0x43F = 0;
    em_timers_update(self);
    if (event_demo_ck__Fv() == 0 && self->field_0x46A == 0) {
        em_pl_distance_update(self);
        em_motion_param_step(self);
        em_sight_update(self);
        em_hate_update(self);
        fn_8012BDF4(self);
        fn_8012C600(self);
        em_act_pick(self);
    }
    em_shell_rate_init(self);
    if (event_demo_ck__Fv() == 0 && self->field_0x46A == 0) {
        em_damage_reenter_ck(self);
        flag = em_damage_eval(self);
    }
    {
        void (*cb)(EnemyWork*) = em_hook_get(self, 2);
        if (cb != NULL) {
            cb(self);
        }
    }
    if (flag == 1) {
        em_state_refresh(self);
    } else if (em_idle_flag_get(self) == 1 && self->field_0x46A == 0) {
        em_userdata_script_run((struct EmWork*)self);
    } else if ((self->flags_0x1C8 & 8) == 0) {
        em_target_pos_set(self, 0);
    }
    if ((u32)(self->field_0x1E5 - 8) > 1) {
        switch (self->field_0x1E5) {
        case 0xB:
            if ((self->flags_0x1C8 & 1) == 0) {
                em_motion_timer_arm(self);
            }
            if ((self->flags_0x1C8 & 0x10000) == 0) {
                em_fx_flag_set(self);
            }
            break;
        }
    } else {
        em_fx_flag3_set(self);
    }
    {
        void (*cb)(EnemyWork*) = em_hook_get(self, 3);
        if (cb != NULL) {
            if (fn_80137C9C(self, cb) == 0) {
                return;
            }
            if (em_idle_flag_get(self) == 1 && self->field_0x46A == 0 && em_userdata_script_run((struct EmWork*)self) == 1) {
                fn_801281EC(self);
            }
            if (fn_80128204(self) == 1) {
                fn_801281F8(self);
                if (fn_80137C9C(self, cb) == 0) {
                    return;
                }
            }
        }
        if (self->field_0x784 != 0) {
            em_busy_timer_reset(self);
            em_busy_set(self);
            em_ceiling_latch(self);
        }
        em_char_pose_sync(self);
        if (self->field_0x1E2 == 3) {
            em_fall_step(self);
        }
        if (self->field_0x1E2 != 4 && self->field_0x1DF == 0) {
            if (ef_fabsf(self->field_0x1AC) <= 100.0f) {
                if (self->field_0x1AC > 10.0f) {
                    self->field_0x1AC -= 10.0f;
                } else if (self->field_0x1AC < -10.0f) {
                    self->field_0x1AC += 10.0f;
                } else {
                    self->field_0x1AC = 0.0f;
                }
            } else {
                self->field_0x1AC *= 0.9f;
            }
        }
        {
            f32 step = self->field_0x1AC * get_em_chg_scale__FP11_ENEMY_WORK(self);
            self->field_0x1A0.y = self->field_0x1A0.y + step;
            mhchar_ofs_set(EM_CHAR(self), &self->field_0x1A0, step);
        }
        if (em_act_ck__FP11_ENEMY_WORKUcUc(self, 0xC, 0xFF) == 0 &&
            ((em_area_ck__FP11_ENEMY_WORK(self) == 1 && self->field_0x001 != 0) ||
             em_break_part_active_ck(self, 0) == 1)) {
            self->char_0x024.field_0x34 = 1;
        } else {
            self->char_0x024.field_0x34 = 0;
        }
        em_mat_alpha_apply(self);
        if (0.0f == self->field_0x1D4) {
            self->char_0x024.field_0x34 = 0;
        } else if (1.0f != self->field_0x1D4) {
            self->char_0x024.field_0x35 = 1;
        } else {
            self->char_0x024.field_0x35 = 0;
        }
        {
            u32 bits = self->flags_0x1C8;
            u16 move_arg;
            if ((bits & 1) == 0 && (bits & 0x400) == 0) {
                if ((self->flags_0x1C8 & 0x400) != 0) {
                    move_arg = 0x80;
                } else {
                    move_arg = 0x180;
                }
            } else {
                move_arg = 0;
            }
            EM_CHAR(self)->move(move_arg);
        }
        copyVec3(&self->field_0x188, &self->char_0x024.field_0x04);
        switch (self->field_0x1E2) {
        case 0:
            if (was_special != 1) {
                vec3_move_damp(&self->field_0x194, &self->field_0x188, &self->field_0x230, 0.3f,
                               &self->field_0x188);
            }
            break;
        case 2:
            if (self->field_0x194.y < 10.0f + (self->field_0x20C + em_dive_depth_get(self))) {
                vec3_move_damp(&self->field_0x194, &self->field_0x188, &self->field_0x230, 0.6f,
                               &self->field_0x188);
            }
            break;
        }
        subVec3(&v8, &self->field_0x188, &self->field_0x194);
        copyVec3(&self->field_0x470, &v8);
        fn_80138074(self, 0);
        em_ground_probe(self);
        em_height_limit_update(self);
        fn_80137DD0(self);
        em_char_pose_sync(self);
        mhchar_model_mtx_update(EM_CHAR(self));
        {
            u8 slot;
            for (slot = 0; slot < 2; slot++) {
                em_fx_slot_step(self, slot);
            }
        }
    }
}

/* The sleep/death half of the per-frame update. */
void em_post_update(EnemyWork* self) {
    s32 found;

    if (self->field_0x1DE != 0) {
        if (self->field_0x8C0 != 0) {
            addVec3To(&self->field_0x188, &self->field_0x8B4);
            fn_80138074(self, 1);
            em_ground_probe(self);
            em_height_limit_update(self);
        }
        found = -1;
        if (self->field_0x1E2 == 1 && self->field_0x1E3 == 1) {
            found = em_motion_slot_angle_avg(self, 0);
            if (found != -1) {
                em_turn_away_step(self, 0x400, found);
            }
        }
        if (self->field_0x8C0 != 0 || found != -1) {
            em_char_pose_sync(self);
            mhchar_model_mtx_update(EM_CHAR(self));
        }
        em_cell_span_update(self);
        if (self->field_0x766 == 0) {
            if (self->field_0x767 == 1) {
                self->field_0x766++;
                copyVec3(&self->field_0x778, &self->field_0x76C);
            }
            if (self->field_0x768 > 0) {
                self->field_0x768--;
            }
        }
        em_turn_joint_update(self);
        em_break_parts_update(self);
        if (self->field_0x764 >= 0) {
            if (em_sleep_ck__FP11_ENEMY_WORKUc(self, 0) == 1) {
                self->field_0x762 = 1;
            } else if (self->field_0x762 == 0) {
                if (--self->field_0x764 <= 0) {
                    self->field_0x762 = 1;
                    self->field_0x764 = 2;
                }
            } else {
                if (--self->field_0x764 <= 0) {
                    em_latch_0x762_roll(self);
                }
            }
        }
        {
            void (*cb)(EnemyWork*) = em_hook_get(self, 6);
            if (cb != NULL) {
                cb(self);
            }
        }
        em_ailment_fx_update(self);
        if ((self->flags_0x1C8 & 1) == 0) {
            em_ambient_color_update(self, 0);
        } else if ((self->flags_0x1C8 & 0x40000) != 0) {
            if (em_die_ck__FP11_ENEMY_WORK(self) == 1) {
                u32 i;
                for (i = 0; i < mhchar_mat_num_get(EM_CHAR(self)); i++) {
                    mhchar_mat_buffer_clean(EM_CHAR(self), i, 0x10);
                }
            } else {
                u32 i;
                for (i = 0; i < mhchar_mat_num_get(EM_CHAR(self)); i++) {
                    mhchar_cull_mode_set(EM_CHAR(self), i, 2);
                }
            }
        }
        {
            void (*cb)(EnemyWork*) = em_hook_get(self, 8);
            if (cb != NULL) {
                cb(self);
            }
        }
        em_joint_visibility_update_all(self);
        switch (self->field_0x1E2) {
        case 4:
            em_shake_req_set(self);
            break;
        }
    }
}
#pragma peephole on

/* Ages the work's per-entity tick counter. */
void fn_80138E18(EnemyWork* self) {
    self->field_0x004 = (u8)(self->field_0x004 + 1);
}

/* Refreshes the work's update mode, preserving its owner byte. */
void fn_80138E28(EnemyWork* self) {
    u8 owner = self->field_0x008;

    enemy_work_release();
    self->field_0x008 = owner;
}

/* Copies the work's scale and rotation into its EmCharBase base. */
void em_char_pose_sync(EnemyWork* self) {
    EmCharBase* ch = &self->char_0x024;

    setScaleAll__6MHcharFf((MHchar*)ch, get_em_scale__FP11_ENEMY_WORK(self));
    ch->field_0x28 = self->field_0x1BC;
    ch->field_0x2C = self->field_0x1C0;
    ch->field_0x30 = self->field_0x1C4;
    copyVec3(&ch->field_0x04, &self->field_0x188);
}

#pragma peephole off
/* Advances one of the two per-entity effect slots. */
void em_fx_slot_step(EnemyWork* self, u32 arg1) {
    u8 index = (u8)arg1;

    if (self->field_0xA08[index].field_0x05 != 0 && (self->field_0xAEC[index] & 1) != 0) {
        hit_knock_set(&self->field_0xA08[index], 100);
    }
    hit_attack_list_push(&self->field_0xA08[index]);
}

/* Marks every move the work cannot currently reach with the -1 sentinel. */
void em_pl_distance_update(EnemyWork* self) {
    MoveWork* move;
    s32 i;
    s32 max;
    u32 all_blocked;

    move = (MoveWork*)get_move_work_adrs__FUc(2);
    max = get_move_work_max__FUc(2);
    all_blocked = 1;

    for (i = 0; i < max; i++, move++) {
        if (move->field_0x00 != 0 && em_pl_slot_leaving_ck(move->field_0x08) == 0) {
            if (em_pl_target_ck(self, move) == 0) {
                self->field_0x454[i] = (-1.0f);
            } else {
                self->field_0x454[i] = vec3_distance(&self->field_0x188, &move->field_0x3C);
                all_blocked = 0;
            }
        } else {
            self->field_0x454[i] = (-1.0f);
        }
    }
    if (all_blocked == 1) {
        self->field_0x439 = 1;
    }
}

#pragma peephole on
/* The action-state step: enters the wait state, then picks the next action. */
void fn_80139024(EnemyWork* self) {
    u8 state;

    em_motion_timer_arm(self);
    em_fx_flag_set(self);
    state = self->field_0x005;
    switch (state) {
    case 0:
        self->field_0x005 = (u8)(state + 1);
        em_motion_param_set(self, 0.0f, 0);
        em_mot_set(self, 1, 0, 0);
        self->field_0x001 = 0;
        return;
    case 1:
        if (self->field_0x018 > 0) {
            return;
        }
        if (em_busy_ck(self) != 1) {
            return;
        }
        if (self->field_0x1E7 == 0) {
            if (self->field_0x00C == 3) {
                self->field_0x017 = 4;
            } else {
                self->field_0x017 = 1;
            }
        } else {
            self->field_0x017 = 2;
        }
        return;
    }
}

/* Installs (or clears) the work's user-data accessor callback. */
void em_res_user_data_set(EnemyWork* self, ResUserDataAc* arg1) {
    ResUserDataAc* old = self->field_0x484;

    if (old != NULL) {
        delete old;
    }
    self->field_0x484 = arg1;
    if (arg1 != NULL) {
        arg1->work = self;
        ((nw4r::g3d::ScnMdlSimple*)self->field_0x13C)->SetCalcWorldCallback(self->field_0x484);
        self->field_0x484->Attach();
        self->field_0x064 = 0;
    }
}

#pragma peephole off
} /* extern "C" */

/* Destroys the accessor through the world-callback base. */
#pragma dont_inline on
ResUserDataAc::~ResUserDataAc() {
}
#pragma dont_inline reset

extern "C" {

#pragma peephole on
/* Reports whether the work has a user-data accessor installed. */
s32 em_res_user_data_ck(EnemyWork* self) {
    return self->field_0x484 != NULL;
}

#pragma peephole off
/* Re-aims the work toward the nearest user-data joint, clamped to the joint's key span. */
void em_turn_joint_update(EnemyWork* self) {
    f32 v;
    Vec3 pos;
    u32 best;
    u32 total;
    UserDataItem* item;

    VEC3_ctor(&pos);
    best = (u32)-1;
    if (self->field_0x740 == 0) {
        v = 0.0f;
    } else {
        total = 0;
        item = em_user_data_items_get(self);
        if (item != NULL) {
            while (item->field_0x00 != (u32)-1) {
                if (item->field_0x04 == 1) {
                    if (best == (u32)-1 || best > item->field_0x00) {
                        best = item->field_0x00;
                    }
                    total = total + (u16)item->field_0x0C;
                }
                item++;
            }
        }
        if (best != (u32)-1) {
            get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3(self, best, &pos);
        } else {
            copyVec3(&pos, &self->field_0x188);
        }
        v = (f32)(s16)(calcVecAng2__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3(&pos, &self->field_0x36C) -
                       (u16)self->field_0x1C0);
        if (v > (f32)(u16)total) {
            v = (f32)(u16)total;
        } else if (v < -(f32)(u16)total) {
            v = -(f32)(u16)total;
        }
    }
    item = em_user_data_items_get(self);
    if (item != NULL) {
        while (item->field_0x00 != (u32)-1) {
            if (item->field_0x04 == 1) {
                u16 current;
                u16 target = (s32)(v * ((f32)item->field_0x0C / (f32)(u16)total));
                u16* slot = &self->field_0x742[item->field_0x08];
                current = *slot;
                if ((s16)(target - current) < 0) {
                    u32 down = current - (u32)(0.03f * (f32)item->field_0x0C);
                    if ((s16)(target - down) < 0) {
                        *slot = (s16)down;
                    } else {
                        *slot = target;
                    }
                } else {
                    u32 up = (u32)(0.03f * (f32)item->field_0x0C) + current;
                    if ((s16)(target - up) > 0) {
                        *slot = (s16)up;
                    } else {
                        *slot = target;
                    }
                }
            }
            item++;
        }
    }
}

/* --------------------------------------------------------------------------------------------- */
/* 0x801394B8..0x8013AC08 - the g3d_resuser_ac accessor                                           */
/* --------------------------------------------------------------------------------------------- */

#pragma peephole on
} /* extern "C" */

/* Re-enters the accessor's default flag state. */
void ResUserDataAc::Attach() {
    em_res_user_data_flag_set(this, 1);
}

#pragma peephole off
/* 0x801394C0 (0xC): Stores the node the next callback runs at (nw4r's inline accessor, kept here). */
void nw4r::g3d::FuncObjCalcWorld::SetCallbackNodeID(u32 nodeID) {
    mNodeID = (u16)(nodeID & 0xFFFF);
}

#pragma peephole on
/* 0x801394CC (0x8): Returns the node the next callback runs at. */
u32 nw4r::g3d::FuncObjCalcWorld::GetCallbackNodeID() const {
    return mNodeID;
}

#pragma peephole off

/* Dispatches every user-data item of the cursor's index to the accessor's virtuals. */
void ResUserDataAc::ExecCallbackA(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj) {
    u32 index = pFuncObj->GetCallbackNodeID();
    UserDataItem* item;

    if (index == work->field_0x110) {
        Vec3 pos;
        VEC3_ctor(&pos);
        pos.x = 0.0f;
        pos.y = 0.0f;
        pos.z = 0.0f;
        anm_chr_result_trans_set(pResult, &pos);
    }
    item = em_user_data_items_get(work);
    if (item == NULL) {
        return;
    }
    while (item->field_0x00 != (u32)-1) {
        if (item->field_0x00 == index) {
            if ((u32)(item->field_0x04 - 3) <= 1 || item->field_0x04 == 0) {
                CalcAnmItem(pResult, mdl, pFuncObj, index, item);
            } else {
                CalcAnmItemEx(pResult, mdl, pFuncObj, index, item);
            }
        }
        item++;
    }
    if ((flags & 2) == 0) {
        em_user_data_next_node_set(pFuncObj, work);
    }
}

extern "C" {

#pragma peephole on
} /* extern "C" */

/* Builds the effect transform for one user-data item and applies it to the EmCharBase. */
void ResUserDataAc::CalcAnmItem(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 node, UserDataItem* item) {
    Vec3 angles;
    Mtx34 mtx;
    nw4r::math::MTX33 dst;
    KeyFrameSet* frames;

    VEC3_ctor(&angles);
    MTX34_ctor(&mtx);
    MTX33_ctor(&dst);
    switch (item->field_0x04) {
    case 0:
    case 3: {
        u8 area = item->field_0x10;
        if (area != -1) {
            EffectArea* rec = &work->field_0x488[area];
            if (rec->field_0x00 >= 2) {
                frames = item->frames;
                if (frames != NULL) {
                    s32 x;
                    s32 y;
                    s32 z;
                    if (rec->field_0x07 == 1) {
                        x = (s32)frames->field_0x0C;
                        y = (s32)frames->field_0x10;
                        z = (s32)frames->field_0x14;
                    } else {
                        x = 0;
                        y = 0;
                        z = 0;
                    }
                    if (x == 0) {
                        x = (s32)frames->field_0x00;
                    }
                    if (y == 0) {
                        y = (s32)frames->field_0x04;
                    }
                    if (z == 0) {
                        z = (s32)frames->field_0x08;
                    }
                    angles.x = getKeyData__FPff(x, (f32)rec->field_0x0A);
                    angles.y = getKeyData__FPff(y, (f32)rec->field_0x0A);
                    angles.z = getKeyData__FPff(z, (f32)rec->field_0x0A);
                    {
                        CPVector cp;
                        cp.x = (u32)(u16)(s32)(0.5f + ((65536.0f * angles.x) / 360.0f));
                        cp.y = (u32)(u16)(s32)(0.5f + ((65536.0f * angles.y) / 360.0f));
                        cp.z = (u32)(u16)(s32)(0.5f + ((65536.0f * angles.z) / 360.0f));
                        cpSetRotMatrix__FP10_CP_VECTORPQ34nw4r4math5MTX34(&cp, &mtx);
                    }
                    nw4r::math::MTX34ToMTX33(&dst, &mtx);
                    anm_chr_result_rot_mul(pResult, &dst);
                }
            }
        }
        return;
    }
    case 4:
        anm_chr_result_rot_mtx_get((struct ResAnmChrObj*)pResult, &dst.m[0][0]);
        work->field_0x76C.x = dst.m[0][2];
        work->field_0x76C.y = dst.m[1][2];
        work->field_0x76C.z = dst.m[2][2];
        break;
    }
}

/* Placeholder the retail object keeps as an empty body. */
void ResUserDataAc::CalcAnmItemEx(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 node, UserDataItem* item) {
}

#pragma peephole off
/* Dispatches every user-data item of the cursor's index to the accessor's second virtual pair. */
void ResUserDataAc::ExecCallbackB(nw4r::g3d::WorldMtxManip* pManip, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj) {
    u32 index = pFuncObj->GetCallbackNodeID();
    UserDataItem* item = em_user_data_items_get(work);

    if (item == NULL) {
        return;
    }
    while (item->field_0x00 != (u32)-1) {
        if (item->field_0x00 == index) {
            if (item->field_0x04 <= 3) {
                CalcMtxItem(pManip, mdl, pFuncObj, index, item);
            } else {
                CalcMtxItemEx(pManip, mdl, pFuncObj, index, item);
            }
        }
        item++;
    }
    em_user_data_next_node_set(pFuncObj, work);
}
#pragma peephole on

/* Applies one user-data item's transform to the caller's matrix. */
void ResUserDataAc::CalcMtxItem(nw4r::g3d::WorldMtxManip* pManip, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 node, UserDataItem* item) {
    Mtx34 mtx;

    MTX34_ctor(&mtx);
    switch (item->field_0x04) {
    case 0:
    case 3: {
        u8 area = item->field_0x10;
        if (area < 2) {
            EffectArea* rec = &work->field_0x488[area];
            if (item->field_0x08 == 0 && rec->field_0x00 >= 2) {
                joint_mtx_set((MtxHolder*)pManip, &rec->matrix);
            }
        }
        break;
    }
    case 1:
        if (item->field_0x08 < 6) {
            joint_mtx_load((MtxHolder*)pManip, &mtx);
            rotMatrixY(work->field_0x742[item->field_0x08], &mtx);
            joint_mtx_store((MtxHolder*)pManip, &mtx);
        }
        break;
    case 2:
        yure_joint_apply((struct YureRec*)&work->field_0x590[item->field_0x08], (struct MtxHolder*)pManip);
        break;
    }
}

extern "C" {

/* Copies a matrix into the holder, or resets it when no source is given. */
void joint_mtx_store(MtxHolder* dst, void* src) {
    if (src != NULL) {
        mtx34_copy_ps((Mtx34*)dst->mtx, (const Mtx34*)src);
        return;
    }
    mtx34_identity(dst->mtx);
}

/* Copies the holder's matrix into the destination. */
void joint_mtx_load(MtxHolder* holder, void* mtx) {
    if (mtx == NULL) {
        return;
    }
    mtx34_copy_ps((Mtx34*)mtx, (const Mtx34*)holder->mtx);
}

/* Copies the holder's matrix into the destination, unconditionally. */
void joint_mtx_set(MtxHolder* holder, void* src) {
    mtx34_copy_ps((Mtx34*)holder->mtx, (const Mtx34*)src);
}

} /* extern "C" */

/* Placeholder the retail object keeps as an empty body. */
void ResUserDataAc::CalcMtxItemEx(nw4r::g3d::WorldMtxManip* pManip, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 node, UserDataItem* item) {
}

extern "C" {

/* Writes the node's matrix relative to its parent's (the parent's inverse times the node's) into out. */
void joint_local_mtx_get(Mtx34* out, void* arg1, Mtx34* arg2) {
    Mtx34 m1;
    Mtx34 m2;
    Mtx34 m3;
    nw4r::g3d::ResNode key((void*)0);
    s32 idx;

    MTX34_ctor(&m1);
    MTX34_ctor(&m2);
    MTX34_ctor(&m3);
    mtx34_copy(&m1, &arg2[res_node_mtx_id_get(arg1)]);
    idx = res_node_get_parent(arg1);
    res_node_assign(&key, &idx);
    mtx34_copy(&m2, &arg2[res_node_mtx_id_get(&key)]);
    mtx34_inverse(&m3, &m2);
    mtx34_concat(out, &m3, &m1);
}

} /* extern "C" */

/* The node user data's TYPE values: a twist about X, a twist about Y, a blend of two joints. */
const char* user_data_type_names[] = {"T-X", "T-Y", "DBL", NULL};

#pragma peephole off
/* Rebuilds the matrix of every node whose user data names a twist or a blend of two joints, then copies the
 * matrices the work's linked user-data items name. */
void ResUserDataAc::ExecCallbackC(nw4r::math::MTX34* pMtxArray, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj) {
    Mtx34 parentMtx;
    Mtx34 parentInv;
    Mtx34 localMtx;
    Mtx34 nodeMtx;
    Mtx34 joint0Mtx;
    Mtx34 joint1Mtx;
    Mtx34 resultMtx;

    MTX34_ctor(&parentMtx);
    MTX34_ctor(&parentInv);
    MTX34_ctor(&localMtx);
    MTX34_ctor(&nodeMtx);
    MTX34_ctor(&joint0Mtx);
    MTX34_ctor(&joint1Mtx);
    MTX34_ctor(&resultMtx);
    nw4r::g3d::ResNode node((void*)0);
    nw4r::g3d::ResNode link((void*)0);
    ResUserData userData;
    ResUserData value;
    u32 count;
    u32 i;
    UserDataItem* item;

    res_user_data_ctor(&userData, 0);
    res_user_data_item_ctor(&value, NULL);
    item = em_user_data_items_get(work);
    count = mdl.GetResNodeNumEntries();
    for (i = 0; i < count; i++) {
        const char* joint0;
        const char* joint1;
        f32 weight;
        u8 kind;
        u32 k;
        {
            nw4r::g3d::ResNode found = mdl.GetResNode(i);
            res_node_assign(&node, &found);
        }
        {
            ResUserData found;
            found.data = (ResUserDataItemData*)res_node_user_data_get((struct ResHandle*)&node);
            res_user_data_assign(&userData, &found);
        }
        if (res_user_data_valid_ck(&userData) == 0) {
            continue;
        }
        if (res_user_data_entry_num_get(&userData) == 0) {
            continue;
        }
        {
            ResUserData found;
            found.data = res_user_data_item_find(&userData, "TYPE");
            res_user_data_item_assign(&value, &found);
        }
        if (res_user_data_item_valid_ck(&value) == 0) {
            continue;
        }
        {
            const char* type = (const char*)res_user_data_item_string_get(&value);
            const char** name = user_data_type_names;
            kind = 255;
            for (k = 0; *name != NULL; name++, k++) {
                if (strcmp(type, *name) == 0) {
                    kind = k;
                }
            }
        }
        if (kind == 255) {
            break;
        }
        {
            ResUserData found;
            found.data = res_user_data_item_find(&userData, "J0");
            res_user_data_item_assign(&value, &found);
        }
        if (res_user_data_item_valid_ck(&value) == 0) {
            continue;
        }
        joint0 = (const char*)res_user_data_item_string_get(&value);
        {
            ResUserData found;
            found.data = res_user_data_item_find(&userData, "J1");
            res_user_data_item_assign(&value, &found);
        }
        if (res_user_data_item_valid_ck(&value) == 0) {
            continue;
        }
        joint1 = (const char*)res_user_data_item_string_get(&value);
        {
            ResUserData found;
            found.data = res_user_data_item_find(&userData, "WEIGHT");
            res_user_data_item_assign(&value, &found);
        }
        if (res_user_data_item_valid_ck(&value) == 0) {
            continue;
        }
        weight = (f32)*(s32*)res_user_data_item_s32_get(&value) / 100.0f;
        mtx34_copy(&nodeMtx, &pMtxArray[res_node_mtx_id_get(&node)]);
        switch (kind) {
        case 0:
        case 1: {
            f32 angle0;
            {
                s32 parent = res_node_get_parent(&node);
                res_node_assign(&link, &parent);
            }
            mtx34_copy(&parentMtx, &pMtxArray[res_node_mtx_id_get(&link)]);
            mtx34_inverse(&parentInv, &parentMtx);
            mtx34_concat(&localMtx, &parentInv, &nodeMtx);
            {
                nw4r::g3d::ResNode found = mdl.GetResNode(joint0);
                res_node_assign(&link, &found);
            }
            joint_local_mtx_get(&joint0Mtx, &link, pMtxArray);
            {
                nw4r::g3d::ResNode found = mdl.GetResNode(joint1);
                res_node_assign(&link, &found);
            }
            joint_local_mtx_get(&joint1Mtx, &link, pMtxArray);
            if (kind == 0) {
                angle0 = (360.0f * (f32)calcMtxAngX_c1(&joint0Mtx)) / 65536.0f;
                rotMatrixX(
                    (u16)(0.5f + (65536.0f * (weight * ((360.0f * (f32)calcMtxAngX_c1(&joint1Mtx)) / 65536.0f - angle0))) / 360.0f),
                    &localMtx);
            } else {
                angle0 = (360.0f * (f32)calcMtxAng3(&joint0Mtx)) / 65536.0f;
                rotMatrixY(
                    (u16)(0.5f + (65536.0f * (weight * ((360.0f * (f32)calcMtxAng3(&joint1Mtx)) / 65536.0f - angle0))) / 360.0f),
                    &localMtx);
            }
            mtx34_concat(&resultMtx, &parentMtx, &localMtx);
            copyMat33(&pMtxArray[res_node_mtx_id_get(&node)], &resultMtx);
            break;
        }
        case 2:
            {
                nw4r::g3d::ResNode found = mdl.GetResNode(joint0);
                res_node_assign(&link, &found);
            }
            mtx34_copy(&joint0Mtx, &pMtxArray[res_node_mtx_id_get(&link)]);
            {
                nw4r::g3d::ResNode found = mdl.GetResNode(joint1);
                res_node_assign(&link, &found);
            }
            mtx34_copy(&joint1Mtx, &pMtxArray[res_node_mtx_id_get(&link)]);
            mtx34_blend(&resultMtx, &joint0Mtx, &joint1Mtx, weight, 1.0f - weight, 0);
            copyMat33(&pMtxArray[res_node_mtx_id_get(&node)], &resultMtx);
            break;
        }
    }
    if (item != NULL) {
        while (item->field_0x00 != (u32)-1) {
            switch (item->field_0x04) {
            case 0:
            case 3:
                if ((u32)(item->field_0x08 - 1) <= 1) {
                u8 area = item->field_0x10;
                if (area < 2 && work->field_0x488[area].field_0x00 <= 1 && item->field_0x0C != (u32)-1) {
                    u32 dst;
                    u32 src;
                    {
                        nw4r::g3d::ResNode found = mdl.GetResNode(item->field_0x00);
                        res_node_assign(&node, &found);
                    }
                    dst = res_node_mtx_id_get(&node);
                    {
                        nw4r::g3d::ResNode found = mdl.GetResNode(item->field_0x0C);
                        res_node_assign(&node, &found);
                    }
                    src = res_node_mtx_id_get(&node);
                    mtx34_copy(&pMtxArray[dst], &pMtxArray[src]);
                }
                }
                break;
            }
            item++;
        }
    }
    CalcWorldEnd(pMtxArray, mdl, pFuncObj);
}
#pragma peephole on

extern "C" {

/* --------------------------------------------------------------------------------------------- */
/* 0x8013A218..0x8013AC08 - the ResUserData value accessors                                       */
/* --------------------------------------------------------------------------------------------- */

/* Reads the item's S32 value. */
ResUserDataItemData* res_user_data_item_s32_get(ResUserData* self) {
    if (res_user_data_item_value_type_get(self) != 0) {
        nw4r::db::Panic(lbl_805A14C4, 54, lbl_805A1480);
    }
    return (ResUserDataItemData*)res_user_data_item_ofs_to_ptr(self, (s32)res_user_data_item_ref(self)->field_0x04);
}

/* Returns the blob-relative pointer for a value offset, or NULL when the offset is zero. */
void* res_user_data_item_ofs_to_ptr(ResUserData* self, s32 arg1) {
    ResUserDataItemData* data = self->data;
    if (arg1 != 0) {
        return (u8*)data + arg1;
    }
    return NULL;
}

/* Returns the item's value header, asserting the item is valid. */
ResUserDataItemData* res_user_data_item_ref(ResUserData* self) {
    if (res_user_data_item_valid_ck(self) == 0) {
        nw4r::db::Panic(lbl_805A143C, 38, lbl_805A1420, res_user_data_item_class_name(), "ref");
    }
    return res_user_data_item_ptr(self);
}

/* Returns the item's value header. */
ResUserDataItemData* res_user_data_item_ptr(ResUserData* self) {
    return self->data;
}

/* The class name the "Object not valid." panic reports for an item. */
const char* res_user_data_item_class_name(void) {
    return lbl_805A1410;
}

/* Returns the item's value type. */
s32 res_user_data_item_value_type_get(ResUserData* self) {
    return (s32)res_user_data_item_ref_const(self)->field_0x0C;
}

/* Returns the item's value header, asserting the item is valid. */
ResUserDataItemData* res_user_data_item_ref_const(ResUserData* self) {
    if (res_user_data_item_valid_ck(self) == 0) {
        nw4r::db::Panic(lbl_805A146C, 38, lbl_805A1450, res_user_data_item_class_name(), "ref");
    }
    return res_user_data_item_ptr_const(self);
}

/* Returns the item's value header. */
ResUserDataItemData* res_user_data_item_ptr_const(ResUserData* self) {
    return self->data;
}

/* Reads the item's string value. */
ResUserDataItemData* res_user_data_item_string_get(ResUserData* self) {
    if (res_user_data_item_value_type_get(self) != 2) {
        nw4r::db::Panic(lbl_805A151C, 68, lbl_805A14D8);
    }
    return (ResUserDataItemData*)res_user_data_item_ofs_to_ptr_const(self, (s32)res_user_data_item_ref_const(self)->field_0x04);
}

/* Returns the blob-relative pointer for a value offset, or NULL when the offset is zero. */
void* res_user_data_item_ofs_to_ptr_const(ResUserData* self, s32 arg1) {
    ResUserDataItemData* data = self->data;
    if (arg1 != 0) {
        return (u8*)data + arg1;
    }
    return NULL;
}

/* Reports whether the item holds a value. */
s32 res_user_data_item_valid_ck(ResUserData* self) {
    return self->data != NULL;
}

/* Copy-assigns one item wrapper onto another; the empty trailing member's copy is handed the words after the
 * two handles. */
ResUserData* res_user_data_item_assign(ResUserData* dst, ResUserData* src) {
    res_user_data_item_ptr_assign(dst, src);
    res_user_data_item_tail_assign(dst + 1, src + 1);
    return dst;
}

/* Trivial member copy the retail object keeps as an empty body. */
void res_user_data_item_tail_assign(ResUserData* dst, ResUserData* src) {
    (void)dst;
    (void)src;
}

/* Copies the item's value header pointer. */
void res_user_data_item_ptr_assign(ResUserData* dst, ResUserData* src) {
    dst->data = src->data;
}

/* Looks a key up in the item's name table and returns the matching value header. */
ResUserDataItemData* res_user_data_item_find(ResUserData* self, const char* key) {
    ResUserDataItemData* data = res_user_data_ref(self);
    ResUserData found;

    return res_user_data_item_ctor(&found, (ResUserDataItemData*)nw4r::g3d::ResDic(&data->field_0x04)[key])->data;
}

/* Returns the item's value header, asserting the item is valid. */
ResUserDataItemData* res_user_data_ref(ResUserData* self) {
    if (res_user_data_valid_ck(self) == 0) {
        nw4r::db::Panic(lbl_805A13C0, 87, lbl_805A13A4, res_user_data_class_name(), "ref");
    }
    return res_user_data_ptr(self);
}

/* Returns the item's value header. */
ResUserDataItemData* res_user_data_ptr(ResUserData* self) {
    return self->data;
}

/* The class name the "Object not valid." panic reports for the collection. */
const char* res_user_data_class_name(void) {
    return lbl_805A1398;
}

/* Reports whether the item's name table holds any entries. */
u32 res_user_data_entry_num_get(ResUserData* self) {
    return nw4r::g3d::ResDic(&res_user_data_ref(self)->field_0x04).GetNumData();
}

/* Reports whether the item holds a value. */
s32 res_user_data_valid_ck(ResUserData* self) {
    return self->data != NULL;
}

/* Copy-constructs one item wrapper from another. */
ResUserData* res_user_data_assign(ResUserData* self, ResUserData* src) {
    res_user_data_ptr_assign(self, src);
    return self;
}

/* Copies the item's value header pointer. */
void res_user_data_ptr_assign(ResUserData* dst, ResUserData* src) {
    dst->data = src->data;
}

#pragma peephole off
/* Stores the value header, asserting it is 4-byte aligned. */
ResUserData* res_user_data_item_ctor(ResUserData* self, ResUserDataItemData* data) {
    res_user_data_item_ptr_set(self, data);
    if (((u32)data & 3) != 0) {
        nw4r::db::Panic(lbl_805A13FC, 38, lbl_805A13D4);
    }
    return self;
}

#pragma peephole on
/* Stores the value header pointer. */
void res_user_data_item_ptr_set(ResUserData* self, ResUserDataItemData* data) {
    self->data = data;
}

} /* extern "C" */

/* Placeholder the retail object keeps as an empty body. */
void ResUserDataAc::CalcWorldEnd(nw4r::math::MTX34* pMtxArray, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj) {
}

extern "C" {

/* Refreshes the accessor's flag word from the work's user-data list. */
void em_res_user_data_flag_set(ResUserDataAc* self, u32 arg1) {
    EnemyWork* work = self->work;
    u32 handle = work->field_0x13C;

    self->flags = arg1;
    self->flags = self->flags | (u32)em_user_data_timing_flags_get(work);
    ((nw4r::g3d::ScnMdlSimple*)handle)->EnableScnMdlCallbackTiming((nw4r::g3d::ScnObj::Timing)self->flags);
    ((nw4r::g3d::ScnMdlSimple*)handle)->SetCalcWorldNodeID(work->field_0x110);
}

/* Returns the work's user-data item list. */
UserDataItem* em_user_data_items_get(EnemyWork* work) {
    EnemyData* data = get_enemy_data__FP11_ENEMY_WORK(work);

    if (data != NULL) {
        return data->items;
    }
    return NULL;
}

/* Builds the accessor's per-type flag word from the work's user-data list. */
s32 em_user_data_timing_flags_get(EnemyWork* work) {
    s32 flags = 1;
    UserDataItem* item = em_user_data_items_get(work);

    if (item != NULL) {
        while (item->field_0x00 != (u32)-1) {
            u8 type = item->field_0x04;
            if ((u32)(type - 1) > 1) {
                switch (type) {
                case 0:
                case 3:
                    flags |= 6;
                    break;
                }
            } else {
                flags |= 2;
            }
            item++;
        }
    }
    return flags;
}

/* Advances the cursor to the next user-data index the work can still reach. */
void em_user_data_next_node_set(nw4r::g3d::FuncObjCalcWorld* cursor, EnemyWork* work) {
    u32 current = cursor->GetCallbackNodeID();
    u32 best = current;
    UserDataItem* item = em_user_data_items_get(work);

    if (item == NULL) {
        return;
    }
    while (item->field_0x00 != (u32)-1) {
        if (item->field_0x00 > current) {
            if (item->field_0x00 < best) {
                best = item->field_0x00;
            } else if (best == current) {
                best = item->field_0x00;
            }
        }
        item++;
    }
    if (best > current && best != (u32)-1) {
        cursor->SetCallbackNodeID(best);
    }
}

#pragma peephole off
/* Reads one entry of the enemy data's secondary table. */
s32 em_break_part_data_get(EnemyWork* work, u8 arg1) {
    EnemyData* data = get_enemy_data__FP11_ENEMY_WORK(work);

    if (data != NULL && data->table_0x98 != NULL) {
        return data->table_0x98[arg1];
    }
    return 0;
}

/* Reports whether the work's user-data state table holds an entry at the given index. */
u32 fn_8013A884(EnemyWork* work, u8 arg1) {
    if (work->field_0x954 == NULL) {
        return 0;
    }
    return (work->field_0x954[arg1] != NULL);
}

/* Reports whether the work's user-data state table holds a sub-entry. */
s32 fn_8013A8B4(EnemyWork* work, u8 arg1, u8 arg2) {
    u32* row;

    if (work->field_0x954 == NULL) {
        return 0;
    }
    row = work->field_0x954[arg1];
    if (row == NULL) {
        return 0;
    }
    return (row[arg2] != 0);
}

/* Resolves the work's current user-data state through the table's redirect entries. */
s32 em_userdata_state_kind_get(EnemyWork* work) {
    s32 state = work->field_0x95C;
    u8 index = work->field_0x9A8;

    for (;;) {
        switch (state) {
        default:
            return state;
        case 2:
            state = work->field_0x99C;
            break;
        case 1:
            index -= 1;
            state = work->field_0x9AC[index].field_0x00;
            break;
        case 0xA:
            state = work->field_0x9E8;
            break;
        }
    }
}

/* Clears the work's user-data state and re-enters the given mode. */
void em_userdata_state_reset(EnemyWork* work, u8 arg1) {
    work->field_0x962 = 0;
    work->field_0x9A8 = 0;
    work->field_0x43E = 0;
    work->field_0x999 = 0;
    work->field_0x99A = 0;
    work->field_0x99B = 0;
    em_userdata_state_exit(work, arg1);
}

#pragma peephole on
/* Installs the work's user-data state table and re-enters the default state. */
void fn_8013A978(EnemyWork* work) {
    work->field_0x954 = (u32**)fn_80126098(work);
    if (fn_8013A884(work, 0) == 1) {
        fn_8013AB74(work, 0, 0);
    } else {
        work->field_0x95C = 0;
        work->field_0x95D = 0;
        work->field_0x958 = 0;
    }
    fn_8013A9F4(work);
    em_userdata_state_reset(work, 1);
}

/* Clears the work's user-data pending flag. */
void fn_8013A9F4(EnemyWork* work) {
    work->field_0x961 = 0;
}

/* Clears the work's user-data key table. */
void fn_8013AA00(EnemyWork* work) {
    work->field_0x9FB = 0;
    work->field_0x9FC = 0;
    work->field_0x9FE = 0;
    work->field_0x9FD = 0;
    work->field_0xA00 = 0;
}

#pragma peephole off
/* Re-enters the work's user-data state, preferring the special index when the work is in state 3. */
void em_userdata_state_select(EnemyWork* work, u8 arg1) {
    if (work->field_0x89F == 3 && fn_8013A884(work, 9) == 1) {
        fn_8013AB74(work, 9, 0);
    } else if (fn_8013A884(work, 0) == 1) {
        fn_8013AB74(work, 0, 0);
    } else {
        work->field_0x95C = 0;
        work->field_0x95D = 0;
        work->field_0x958 = 0;
    }
    em_userdata_state_reset(work, arg1);
}

#pragma peephole on
/* Re-enters the work's user-data state in the default mode. */
void em_user_data_reenter(EnemyWork* work) {
    em_userdata_state_select(work, 1);
}

#pragma peephole off
/* Tears down the work's user-data state, keeping the key table when it is still in use. */
void em_userdata_state_exit(EnemyWork* work, u8 arg1) {
    if (em_busy_ck(work) == 1) {
        work->field_0x1F9 = 0;
    }
    work->field_0x95F = 0;
    work->field_0x9F0 = 0;
    if (arg1 == 1) {
        fn_8013AA00(work);
        work->field_0x1FD = 0;
    } else if (work->field_0xA00 != 0 && work->field_0x9FF == 0) {
        fn_8013AA00(work);
    }
    work->field_0x9FF = 0;
}

#pragma peephole on
/* Re-enters the work's user-data state in the non-default mode. */
void em_userdata_state_reenter_alt(EnemyWork* work) {
    em_userdata_state_select(work, 0);
}

#pragma peephole off
/* Selects the work's user-data state and caches the selected value. */
s32 fn_8013AB74(EnemyWork* work, u8 arg1, u8 arg2) {
    if (fn_8013A884(work, arg1) == 1) {
        work->field_0x95C = arg1;
        work->field_0x95D = arg2;
        work->field_0x958 = work->field_0x954[arg1][arg2];
        work->field_0x960 = 0;
        return 1;
    }
    return 0;
}

#pragma peephole on
/* Draws one random number for the work's user-data roll. */
void em_userdata_roll(void) {
    ran_suu__Fl(1);
}

#pragma peephole off
/* Walks the work's per-motion records backwards to the first one matching the current state. */
u8 em_userdata_record_find_back(EnemyWork* work, u8 arg1, u8 arg2) {
    EmDataRecord* rec;
    s16 index = arg1;
    EnemyData* data = work->field_0x9A4;

    if (data != NULL) {
        rec = &data->records[index];
        while (index >= 0) {
            if (rec->field_0x0B == 0 || arg2 == rec->field_0x0B) {
                if (rec->field_0x09 != em_phase_get(work)) {
                    return index;
                }
                if (rec->field_0x0A != 1) {
                    return index;
                }
            }
            rec--;
            index--;
        }
    }
    return arg1;
}

}  /* extern "C" */

#pragma peephole on