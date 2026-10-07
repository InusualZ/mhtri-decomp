/* enemy/fn_80138074.cpp - the enemy "user data" driver and its accessors, two clusters of one TU: the `_ENEMY_WORK`
 *   per-frame driver (0x80138074-0x801391E8: `fn_8013823C` the per-tick update, `fn_80138B60` the sleep/death half
 *   and their helpers) and the `ResUserDataAc` accessor with the per-entity user-data state machine
 *   (0x801394B8-0x8013AC08).
 * RANGE. .text 0x80138074-0x8013ACC4 (73 functions); .data 0x805A1358-0x805A1530, .sdata 0x807919F0-0x80791A20,
 *   .sdata2 0x80796D40-0x80796D90, extab, extabindex.
 * NAMES. The map stem; the `ResUserData`/`ResUserDataItem` panics name the NW4R accessor header `g3d_resuser_ac.h`,
 *   not this file.  `em_res_user_data_set` is a GUESS (it installs a callback).
 *   From their bodies: em_userdata_motion_head_load is a GUESS (0x8013817C); em_userdata_state_exit is a GUESS
 *   (0x8013AACC); em_userdata_state_reenter_alt is a GUESS (0x8013AB6C); em_userdata_roll is a GUESS (0x8013AC00);
 *   em_userdata_record_find_back is a GUESS (0x8013AC08); em_res_user_data_ck is a GUESS (0x801391E8);
 *   joint_mtx_store is a GUESS (0x80139A64); joint_mtx_load is a GUESS (0x80139A7C).
 * RESIDUALS. 1 row unwritten: 0x80139B6C-0x8013A218 (`fn_80139B6C`, the per-item matrix builder).
 *  - `fn_80138074`, `fn_8013823C`, `fn_80138B60`, `fn_80138EC8`, `fn_801391FC`, `fn_801394D4`, `fn_8013A5E4`,
 *    `em_userdata_record_find_back`: retail keeps `rlwinm`/`clrlwi`/`extsh` + `cmpwi`, ours emits the record form;
 *  - `fn_8013823C`, `fn_8013A830`, `fn_8013A884`, `fn_8013A8B4`, `fn_8013A900`, `fn_8013AB74`: retail keeps `clrlwi`
 *    + `slwi`, ours fuses them into `clrlslwi`;
 *  - `fn_801394C0` (the `clrlwi r0,r4,16` before the `sth`), `fn_8013918C`, `fn_80139858`, `fn_8013A954`,
 *    `fn_8013AA1C`, `em_userdata_state_exit`: retail narrows the value, ours elides the mask;
 *  - `fn_801391FC`, `fn_80139620`: ours converts int to float through the object's own `.sdata2` magic where retail
 *    references `lbl_80796D78`/`lbl_80796D80`, and `fn_801391FC` lacks retail's `__cvt_fp2unsigned` call; `fn_801391FC` and `fn_80138F3C` also differ in the `psq_st` f31 spill;
 *  - `fn_8013A29C`, `fn_8013A338`, `fn_8013A4E8`: ours forms the `lbl_807919F0`/`F4`/`F8` addresses with `lis` +
 *    `addi`, retail with one instruction;
 *  - `fn_8013A494`, `fn_8013A560`: ours passes a stack temporary (`addi r3,r1,0x8`); `fn_8013A43C`: ours passes the
 *    record where retail passes its +0x4;
 *  - `fn_80139024`, `fn_80139954`, `fn_8013A6F4`, `fn_8013A770`, `em_res_user_data_set`: the branch layout differs
 *    (an extra or a missing `b`/`beq`);
 *  - `fn_8013823C`, `fn_801394D4`, `fn_80139620`: the frame differs (0x100/0x40/0xC0 against our 0xE0/0x30/0xD0);
 *    `em_userdata_motion_head_load`: ours saves r30.
 *   flipcheck: `.data`/`.sdata` claimed, not emitted; `.sdata2`/`.text`/extab/extabindex short of the claim.
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `__cvt_fp2unsigned`,
 *     `fn_805012E8`, `MTX34ToMTX33__Q24nw4r4mathFPQ34nw4r4math5MTX33PCQ34nw4r4math5MTX34`, `lbl_807919F4`,
 *     `lbl_807919F0`, `lbl_807919F8`.
 *   flipcheck: referenced but defined by nothing a flip can use: `fn_805012E8`.
 * SHAPES. `fn_8013A770` and `fn_8013A6F4` hoist the `item != NULL` test out of the loop; `fn_8013A770`'s index and
 *   `fn_801391FC`'s `total` are 32-bit accumulators narrowed at the use (`u16` locals re-mask every iteration);
 *   `fn_8013A900` is a `switch` inside `for (;;)`, not three `if`s.  The unit's own symbols are plain C names, so its
 *   definitions and its callees' map spellings sit in one `extern "C"` block; the nw4r members are called as C++.
 */

struct _ENEMY_WORK; /* file scope, so the leaf headers' `struct _ENEMY_WORK*` parameters name this type (C scopes a struct first named in a prototype) */
#include "enemy/em_motion_param_set.h" /* em_motion_param_set (rule 2: the owner's header) */
#include "enemy/get_enemy_data__FP11_ENEMY_WORK.h" /* get_enemy_data__FP11_ENEMY_WORK (rule 2: the owner's header) */
#include "enemy/fn_801252DC.h" /* fn_801252DC (rule 2: the owner's header) */
#include "enemy/fn_80126098.h" /* fn_80126098 (rule 2: the owner's header) */
#include "enemy/fn_80126278.h" /* fn_80126278 (rule 2: the owner's header) */
#include "enemy/fn_801264BC.h" /* fn_801264BC (rule 2: the owner's header) */
#include "enemy/em_hit_mask_get.h" /* em_hit_mask_get (rule 2: the owner's header) */
#include "enemy/fn_801281EC.h" /* fn_801281EC (rule 2: the owner's header) */
#include "enemy/fn_801281F8.h" /* fn_801281F8 (rule 2: the owner's header) */
#include "enemy/fn_80128204.h" /* fn_80128204 (rule 2: the owner's header) */
#include "enemy/fn_80128308.h" /* fn_80128308 (rule 2: the owner's header) */
#include "enemy/em_target_pos_set.h" /* em_target_pos_set (rule 2: the owner's header) */
#include "enemy/fn_8012987C.h" /* fn_8012987C (rule 2: the owner's header) */
#include "enemy/fn_8012A3B4.h" /* fn_8012A3B4 (rule 2: the owner's header) */
#include "enemy/fn_8012A414.h" /* fn_8012A414 (rule 2: the owner's header) */
#include "enemy/em_area_change.h" /* em_area_change (rule 2: the owner's header) */
#include "enemy/fn_8012B64C.h" /* fn_8012B64C (rule 2: the owner's header) */
#include "enemy/em_busy_set.h" /* em_busy_set (rule 2: the owner's header) */
#include "enemy/fn_80133BCC.h" /* fn_80133BCC (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "enemy/fn_8012BDF4.h"
#include "Pl/yure_joint_apply.h"
#include "g3d/g3d_calcworld.h"
#include "g3d/g3d_resanmcamera.h"
#include "sys_mem.h"
#include "ef/ef_torus.h" /* fn_800C9DCC (rule 2) */
#include "unsplit/enemy.h"
#include "g3d/g3d_rescommon.h" /* nw4r::g3d::ResDic (rule 2) */
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */
#include "sound/mhchar.h" /* MHchar::move (rule 2) */
#include "enemy/fn_8013ACC4.h" /* em_userdata_script_run (rule 2) */
#include "g3d/g3d_calcview.h" /* fn_800710BC/fn_8006FDCC/mtx34_copy_ps (rule 2) */
#include "g3d/g3d_scnmdlsmpl.h" /* fn_80080B10/fn_800810DC (rule 2) */
#include "nw4r/db_assert.h" /* nw4r::db::Panic, owner nw4r/db_assert.cpp (rule 2) */
#include "g3d/g3d_state.h"
#include "unsplit/sound.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "unsplit/enemy_pool.h" /* the enemy band's .data pool labels, declared in the band header */
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_80126098_c1 ((s32 (*)(void))fn_80126098)
#define fn_801281EC_c1 ((void (*)(EnemyWork*))fn_801281EC)
#define fn_801281F8_c1 ((void (*)(EnemyWork*))fn_801281F8)
#define fn_80128204_c1 ((u32 (*)(EnemyWork*))fn_80128204)
#define em_busy_set_c1 ((void (*)(EnemyWork*))em_busy_set)

/* The unit's own symbols and its callees' map spellings are plain C names. */
extern "C" {

/* --------------------------------------------------------------------------------------------- */
/* shared pool symbols (another unit owns the bytes)                                              */
/* --------------------------------------------------------------------------------------------- */

extern const char lbl_807919F0[];
extern const char lbl_807919F4[];
extern const char lbl_807919F8[];
extern const char lbl_80791A08[];
extern const char lbl_80791A10[];
extern const char lbl_80791A14[];
extern const char lbl_80791A18[];

extern const f32 lbl_80796D40;
extern const f32 lbl_80796D44;
extern const f32 lbl_80796D48;
extern const f32 lbl_80796D4C;
extern const f32 lbl_80796D50;
extern const f32 lbl_80796D54;
extern const f32 lbl_80796D58;
extern const f32 lbl_80796D5C;
extern const f32 lbl_80796D60;
extern const f32 lbl_80796D64;
extern const f32 lbl_80796D68;
extern const f32 lbl_80796D6C;
extern const f32 lbl_80796D70;
extern const f32 lbl_80796D74;
extern const f64 lbl_80796D78;
extern const f64 lbl_80796D80;
extern const f32 lbl_80796D88;
extern const f32 lbl_80796D8C;

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
typedef struct UserDataItem UserDataItem;
typedef struct KeyFrameSet KeyFrameSet;
typedef struct ResUserDataAc ResUserDataAc;
typedef struct ResUserDataAcVtbl ResUserDataAcVtbl;
typedef struct ResUserData ResUserData;
typedef struct ResUserDataItemData ResUserDataItemData;
typedef struct UserDataCursor UserDataCursor;
typedef struct MtxHolder MtxHolder;
typedef struct MHchar MHchar;
typedef struct EmCharBase EmCharBase;
#define fn_80126278_c1 ((void (*)(u16, Vec3*))fn_80126278)
#define EM_CHAR(self) ((MHchar*)&(self)->char_0x024)
typedef struct WorkSlot WorkSlot;
typedef struct EffectArea EffectArea;
typedef struct MoveSlot MoveSlot;
typedef struct WorkRecord8 WorkRecord8;
typedef struct MoveWork MoveWork;

void fn_80138074(EnemyWork* self, u8 arg1);
void em_userdata_motion_head_load(EnemyWork* self);
void fn_801381F4(EnemyWork* self);
void fn_8013823C(EnemyWork* self);
void fn_80138B60(EnemyWork* self);
void fn_80138E18(EnemyWork* self);
void fn_80138E28(EnemyWork* self);
void fn_80138E64(EnemyWork* self);
void fn_80138EC8(EnemyWork* self, u32 arg1);
void fn_80138F3C(EnemyWork* self);
void fn_80139024(EnemyWork* self);
void em_res_user_data_set(EnemyWork* self, ResUserDataAc* arg1);
void fn_8013918C(void* arg0, s16 arg1);
s32 em_res_user_data_ck(EnemyWork* self);
void fn_801391FC(EnemyWork* self);
void fn_801394B8(ResUserDataAc* self);
void fn_801394C0(UserDataCursor* self, u32 arg1);
u32 fn_801394CC(UserDataCursor* self);
void fn_801394D4(ResUserDataAc* self, s32 arg1, s32* arg2, UserDataCursor* arg3);
void fn_80139620(ResUserDataAc* self, void* arg1, void* arg2, s32 arg3, s32 arg4, UserDataItem* arg5);
void fn_80139854(void);
void fn_80139858(ResUserDataAc* self, s32 arg1, s32* arg2, UserDataCursor* arg3);
void fn_80139954(ResUserDataAc* self, MtxHolder* arg1, s32 arg2, s32 arg3, s32 arg4, UserDataItem* arg5);
void joint_mtx_store(MtxHolder* dst, void* src);
void joint_mtx_load(MtxHolder* holder, void* mtx);
void fn_80139A98(MtxHolder* holder, void* src);
void fn_80139AA0(void);
void fn_80139AA4(Mtx34* out, void* arg1, Mtx34* arg2);
void fn_80139B6C(ResUserDataAc* self, Mtx34* arg1, s32* arg2, s32 arg3);
ResUserDataItemData* fn_8013A218(ResUserData* self);
void* fn_8013A280(ResUserData* self, s32 arg1);
ResUserDataItemData* fn_8013A29C(ResUserData* self);
ResUserDataItemData* fn_8013A300(ResUserData* self);
const char* fn_8013A308(void);
s32 fn_8013A314(ResUserData* self);
ResUserDataItemData* fn_8013A338(ResUserData* self);
ResUserDataItemData* fn_8013A39C(ResUserData* self);
ResUserDataItemData* fn_8013A3A4(ResUserData* self);
void* fn_8013A40C(ResUserData* self, s32 arg1);
s32 fn_8013A428(ResUserData* self);
ResUserData* fn_8013A43C(ResUserData* dst, ResUserData* src);
void fn_8013A484(ResUserData* dst, ResUserData* src);
void fn_8013A488(ResUserData* dst, ResUserData* src);
ResUserDataItemData* fn_8013A494(ResUserData* self, const char* key);
ResUserDataItemData* fn_8013A4E8(ResUserData* self);
ResUserDataItemData* fn_8013A54C(ResUserData* self);
const char* fn_8013A554(void);
void fn_8013A560(ResUserData* self);
s32 fn_8013A594(ResUserData* self);
ResUserData* fn_8013A5A8(ResUserData* self, ResUserData* src);
void fn_8013A5D8(ResUserData* dst, ResUserData* src);
ResUserData* fn_8013A5E4(ResUserData* self, ResUserDataItemData* data);
void fn_8013A648(ResUserData* self, ResUserDataItemData* data);
void fn_8013A650(void);
void fn_8013A654(ResUserDataAc* self, u32 arg1);
UserDataItem* fn_8013A6C0(EnemyWork* work);
s32 fn_8013A6F4(EnemyWork* work);
void fn_8013A770(UserDataCursor* cursor, EnemyWork* work);
s32 fn_8013A830(EnemyWork* work, u8 arg1);
u32 fn_8013A884(EnemyWork* work, u8 arg1);
s32 fn_8013A8B4(EnemyWork* work, u8 arg1, u8 arg2);
s32 fn_8013A900(EnemyWork* work);
void fn_8013A954(EnemyWork* work, u8 arg1);
void fn_8013A978(EnemyWork* work);
void fn_8013A9F4(EnemyWork* work);
void fn_8013AA00(EnemyWork* work);
void fn_8013AA1C(EnemyWork* work, u8 arg1);
void fn_8013AAC4(EnemyWork* work);
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
extern f32 fn_80050EF4(const Vec3* a, const Vec3* b);
extern f32 calcVecDistXZ(const Vec3* a, const Vec3* b);
extern void addVec3(Vec3* out, const Vec3* a, const Vec3* b);
extern void fn_800516F0(Mtx34* mtx);
extern void fn_80051894(Mtx34* out, const Mtx34* a, const Mtx34* b, s32 arg3, f32 t, f32 u);
extern void vec3_scale(Vec3* out, const Vec3* v, f32 scale);
extern f32 vec3_dot(const Vec3* a, const Vec3* b);
extern void fn_800524C0(Vec3* out, const Vec3* a, const Vec3* b, const Vec3* c, f32 t);
extern void mtx34_copy(Mtx34* out, const Mtx34* src);
extern void fn_8005D0CC(void* out, const void* src);
extern s32 fn_8005D124(void* arg0);
extern void fn_8008DF8C(void* arg0, Mtx34* out);
extern void fn_8008EE68(void* arg0, Mtx34* mtx);
extern void fn_8008F148(void* arg0, const Vec3* v);
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

extern void fn_8011F5C0(EnemyWork* work);
extern u32 fn_8011F654(EnemyWork* work);
extern void fn_80295578(EnemyWork* work, u8 arg1, u16 arg2);
extern void hit_attack_list_push(void* arg0);
extern void hit_knock_set(void* arg0, s32 arg1);
extern void fn_802B01AC(Vec3* out, const Vec3* v, u8 area);
extern f32 fn_802B0430(u8 area);
extern u8 stage_map_kind_get(u8 arg0);
extern void em_roster_record_release(s32 arg0);
extern s32 em_roster_record_slot_id_get(s32 arg0);
extern s32 em_roster_record_alive_ck(void);
extern void fn_805012E8(Mtx34* out, const Mtx34* src);

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

/* One 0x18-byte entry of `EnemyData::items` (the user-data list). */
struct UserDataItem { /* size: 0x18 */
    /* +0x00 */ u32 field_0x00;      /* -1 terminates the list */
    /* +0x04 */ u8 field_0x04;       /* item type */
    /* +0x05 */ u8 pad_0x05[0x03];
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u32 field_0x0C;
    /* +0x10 */ u8 field_0x10;
    /* +0x11 */ u8 pad_0x11[0x03];
    /* +0x14 */ KeyFrameSet* frames;
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

/* The `ResUserDataAc` virtual table.  Slots 0x1C/0x20/0x24/0x28/0x2C are the per-item dispatch
 * entries the accessor calls; everything before them is out of this unit. */
struct ResUserDataAcVtbl { /* size: 0x30 */
    /* +0x00 */ void (*fn_0x00)(void);
    /* +0x04 */ void (*fn_0x04)(void);
    /* +0x08 */ void (*fn_0x08)(ResUserDataAc*, s32);
    /* +0x0C */ void (*fn_0x0C)(void);
    /* +0x10 */ void (*fn_0x10)(void);
    /* +0x14 */ void (*fn_0x14)(void);
    /* +0x18 */ void (*fn_0x18)(ResUserDataAc*);
    /* +0x1C */ void (*fn_0x1C)(ResUserDataAc*, s32, s32*, UserDataCursor*, u16, UserDataItem*);
    /* +0x20 */ void (*fn_0x20)(ResUserDataAc*, s32, s32*, UserDataCursor*, u16, UserDataItem*);
    /* +0x24 */ void (*fn_0x24)(ResUserDataAc*, s32, s32*, UserDataCursor*, u16, UserDataItem*);
    /* +0x28 */ void (*fn_0x28)(ResUserDataAc*, s32, s32*, UserDataCursor*, u16, UserDataItem*);
    /* +0x2C */ void (*fn_0x2C)(ResUserDataAc*, Mtx34*, s32*, s32);
};

struct ResUserDataAc { /* size: 0x0C */
    /* +0x00 */ ResUserDataAcVtbl* vtable;
    /* +0x04 */ EnemyWork* work;
    /* +0x08 */ u32 flags;
};

/* The little index object `fn_8013A770` maintains (its `u16` at +6). */
struct UserDataCursor { /* size: 0x08 */
    /* +0x00 */ u8 pad_0x00[0x06];
    /* +0x06 */ u16 field_0x06;
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

struct ResUserData { /* size: 0x08 */
    /* +0x00 */ ResUserDataItemData* data;
    /* +0x04 */ u32 field_0x04;
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
    /* +0x784 */ u32 field_0x784;
    /* +0x788 */ u8 pad_0x788[0x12];
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

/* Applies the work's current rotation to the EmCharBase base and pushes the animation frame. */
void fn_80138074(EnemyWork* self, u8 arg1) {
    Vec3 saved;
    Vec3 offset;

    VEC3_ctor(&saved);
    VEC3_ctor(&offset);
    if (self->field_0x30C != 1) {
        if ((self->flags_0x1C8 & 0x400) != 0) {
            return;
        }
        if (self->field_0x1E2 == 1 && self->field_0x1E3 == 1) {
            copyVec3(&saved, &self->field_0x188);
            setVector3__FPQ34nw4r4math4VEC3fff(&offset, lbl_80796D40, lbl_80796D40, lbl_80796D44);
            rotVecY__FPQ34nw4r4math4VEC3Ul(&offset, self->field_0x1C0);
            addVec3To(&self->field_0x188, &offset);
            fn_80295578(self, arg1, em_hit_mask_get(self));
            if (self->field_0x218 == 0) {
                copyVec3(&self->field_0x188, &saved);
            }
        } else {
            fn_80295578(self, arg1, em_hit_mask_get(self));
        }
    }
}

/* Loads the current motion's first frame into the EmCharBase base. */
void em_userdata_motion_head_load(EnemyWork* self) {
    EnemyData* data = self->field_0x9A4;

    if (data->field_0x00 == 0xFE || data->field_0x00 == 0xFC) {
        self->field_0x1F9 = 0;
    } else {
        fn_80126278_c1(data->field_0x08, &self->field_0x188);
        self->field_0x1C0 = data->field_0x0A;
        em_area_change(self, 0);
    }
    fn_801408B4(self);
}

/* Steps the motion sequence until it settles on state 2. */
void fn_801381F4(EnemyWork* self) {
    while (self->field_0x95C != 2) {
        if (fn_801408B4(self) == 0) {
            break;
        }
    }
}

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
    Vec3 v2C;
    Vec3 v20;
    Vec3 v14;
    Vec3 v8;
    u32 flag;
    u8 i;
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
                self->field_0x898[i] = (s16)(self->field_0x898[i] - 1);
            }
        }
        if (self->field_0x88C[i] != -1) {
            if (em_roster_record_alive_ck() == 0) {
                if (em_roster_record_slot_id_get(self->field_0x88C[i]) > 0) {
                    self->field_0x89E = (u8)(self->field_0x89E + 1);
                }
                self->field_0x88C[i] = -1;
            } else if (self->field_0x898[i] <= 0) {
                if (em_roster_record_slot_id_get(self->field_0x88C[i]) > 0) {
                    self->field_0x89E = (u8)(self->field_0x89E + 1);
                }
                em_roster_record_release(self->field_0x88C[i]);
                self->field_0x88C[i] = -1;
            }
        }
    }
    fn_801252DC(self);
    if (self->field_0x1F9 == 1 && self->field_0x1E1 != self->field_0x9F7) {
        if (self->field_0x380 == 8) {
            if (em_busy_ck(self) == 1 && (self->field_0x99A & 1) == 0 && self->field_0x99B == 1 &&
                self->field_0x382 == 0) {
                if (calcVecDistXZ(&self->field_0x188, &self->field_0x36C) <=
                    self->field_0x9A4->records->field_0x04) {
                    fn_801381F4(self);
                    em_userdata_motion_head_load(self);
                    fn_80133BB4(self);
                }
            }
        } else if (self->field_0x1FA == 1 && self->field_0x1FB == 1) {
            if (stage_map_kind_get(self->field_0x1E0) == 5 && self->field_0x1E1 == 3) {
                setVector3__FPQ34nw4r4math4VEC3fff(&v8C, lbl_80796D48, lbl_80796D40, lbl_80796D4C);
            } else {
                setVector3__FPQ34nw4r4math4VEC3fff(&v8C, lbl_80796D40, lbl_80796D40, lbl_80796D40);
            }
            get_worldworld_pos__FPQ34nw4r4math4VEC3Uc(&v68, &v8C, self->field_0x1E1);
            copyVec3(&vC8, &v68);
            if (stage_map_kind_get(self->field_0x1E0) == 5 && self->field_0x9F7 == 3) {
                setVector3__FPQ34nw4r4math4VEC3fff(&v8C, lbl_80796D48, lbl_80796D40, lbl_80796D4C);
            } else {
                setVector3__FPQ34nw4r4math4VEC3fff(&v8C, lbl_80796D40, lbl_80796D40, lbl_80796D40);
            }
            get_worldworld_pos__FPQ34nw4r4math4VEC3Uc(&v5C, &v8C, self->field_0x9F7);
            copyVec3(&vBC, &v5C);
            get_worldworld_pos__FPQ34nw4r4math4VEC3Uc(&v50, &self->field_0x188, self->field_0x1E1);
            copyVec3(&vD4, &v50);
            addVec3(&v74, &vC8, &vBC);
            vec3_scale(&v80, &v74, lbl_80796D50);
            copyVec3(&vB0, &v80);
            subVec3(&v2C, &vBC, &vB0);
            copyVec3(&vA4, &v2C);
            subVec3(&v20, &vD4, &vB0);
            copyVec3(&v98, &v20);
            if (vec3_dot(&vA4, &v98) >= lbl_80796D40) {
                fn_802B01AC(&v14, &vD4, self->field_0x9F7);
                copyVec3(&self->field_0x188, &v14);
                self->field_0x188.y = fn_802B0430(self->field_0x9F7);
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
            fn_801324E0(self);
        }
    }
    fn_80128308(self);
    setVector3__FPQ34nw4r4math4VEC3fff(&self->field_0x1A0, lbl_80796D40, lbl_80796D40, lbl_80796D40);
    self->field_0x1DF = 0;
    self->field_0x79A = 0;
    self->field_0x784 = 0;
    self->field_0x30C = 0;
    self->field_0x30D = 0;
    self->field_0x30E = 0;
    self->field_0x30F = 0;
    copyVec3(&self->field_0x194, &self->field_0x188);
    was_special = self->field_0x1E2;
    fn_80133C30(self);
    if (self->field_0x767 == 1) {
        copyVec3(&self->field_0x778, &self->field_0x76C);
    }
    self->field_0x767 = 1;
    self->field_0x740 = 0;
    self->field_0x43F = 0;
    fn_8012987C(self);
    if (event_demo_ck__Fv() == 0 && self->field_0x46A == 0) {
        fn_80138F3C(self);
        fn_8012FCE4(self);
        fn_80131150(self);
        fn_8012B64C(self);
        fn_8012BDF4(self);
        fn_8012C600(self);
        fn_8012C9AC(self);
    }
    fn_801320A4(self);
    if (event_demo_ck__Fv() == 0 && self->field_0x46A == 0) {
        fn_8011F5C0(self);
        flag = fn_8011F654(self);
    }
    {
        void (*cb)(EnemyWork*) = fn_801264BC(self, 2);
        if (cb != NULL) {
            cb(self);
        }
    }
    if (flag == 1) {
        em_state_refresh(self);
    } else if (fn_80133BCC(self) == 1 && self->field_0x46A == 0) {
        em_userdata_script_run((struct EmWork*)self);
    } else if ((self->flags_0x1C8 & 8) == 0) {
        em_target_pos_set(self, 0);
    }
    if ((u32)(self->field_0x1E5 - 8) > 1) {
        if (self->field_0x1E5 == 0xB) {
            if ((self->flags_0x1C8 & 1) == 0) {
                em_motion_timer_arm(self);
            }
            if ((self->flags_0x1C8 & 0x10000) == 0) {
                em_fx_flag_set(self);
            }
        }
    } else {
        fn_80131E0C(self);
    }
    {
        void (*cb)(EnemyWork*) = fn_801264BC(self, 3);
        if (cb != NULL) {
            if (fn_80137C9C(self, cb) != 0) {
                if (fn_80133BCC(self) == 1 && self->field_0x46A == 0 && em_userdata_script_run((struct EmWork*)self) == 1) {
                    fn_801281EC_c1(self);
                }
                if (fn_80128204_c1(self) == 1) {
                    fn_801281F8_c1(self);
                    if (fn_80137C9C(self, cb) == 0) {
                        return;
                    }
                }
            } else {
                return;
            }
        }
        if (self->field_0x784 != 0) {
            em_busy_timer_reset(self);
            em_busy_set_c1(self);
            fn_80131D9C(self);
        }
        fn_80138E64(self);
        if (self->field_0x1E2 == 3) {
            fn_80130438(self);
        }
        if (self->field_0x1E2 != 4 && self->field_0x1DF == 0) {
            if (ef_fabsf(self->field_0x1AC) <= lbl_80796D54) {
                if (self->field_0x1AC > lbl_80796D58) {
                    self->field_0x1AC = self->field_0x1AC - lbl_80796D58;
                } else if (self->field_0x1AC < lbl_80796D5C) {
                    self->field_0x1AC = self->field_0x1AC + lbl_80796D58;
                } else {
                    self->field_0x1AC = lbl_80796D40;
                }
            } else {
                self->field_0x1AC = self->field_0x1AC * lbl_80796D60;
            }
        }
        {
            f32 step = self->field_0x1AC * get_em_chg_scale__FP11_ENEMY_WORK(self);
            self->field_0x1A0.y = self->field_0x1A0.y + step;
            fn_800E09D0(EM_CHAR(self), &self->field_0x1A0, step);
        }
        if (em_act_ck__FP11_ENEMY_WORKUcUc(self, 0xC, 0xFF) == 0 &&
            ((em_area_ck__FP11_ENEMY_WORK(self) == 1 && self->field_0x001 != 0) ||
             fn_80135BC4(self, 0) == 1)) {
            self->char_0x024.field_0x34 = 1;
        } else {
            self->char_0x024.field_0x34 = 0;
        }
        fn_801373D0(self);
        if (lbl_80796D40 == self->field_0x1D4) {
            self->char_0x024.field_0x34 = 0;
        } else if (lbl_80796D64 != self->field_0x1D4) {
            self->char_0x024.field_0x35 = 1;
        } else {
            self->char_0x024.field_0x35 = 0;
        }
        {
            u32 bits = self->flags_0x1C8;
            u16 move_arg;
            if ((bits & 1) == 0 && (bits & 0x400) == 0) {
                move_arg = 0x180;
            } else {
                move_arg = 0;
            }
            EM_CHAR(self)->move(move_arg);
        }
        copyVec3(&self->field_0x188, &self->char_0x024.field_0x04);
        switch (self->field_0x1E2) {
        case 0:
            if (was_special != 1) {
                fn_800524C0(&self->field_0x194, &self->field_0x188, &self->field_0x230,
                            &self->field_0x188, lbl_80796D68);
            }
            break;
        case 2:
            if (self->field_0x194.z < lbl_80796D58 + (self->field_0x20C + fn_801302E4(self))) {
                fn_800524C0(&self->field_0x194, &self->field_0x188, &self->field_0x230,
                            &self->field_0x188, lbl_80796D6C);
            }
            break;
        }
        subVec3(&v8, &self->field_0x188, &self->field_0x194);
        copyVec3(&self->field_0x470, &v8);
        fn_80138074(self, 0);
        fn_8012A3B4(self);
        fn_8012FF38(self);
        fn_80137DD0(self);
        fn_80138E64(self);
        fn_800E0914(EM_CHAR(self));
        {
            u8 slot;
            for (slot = 0; slot < 2; slot++) {
                fn_80138EC8(self, slot);
            }
        }
    }
}

/* The sleep/death half of the per-frame update. */
void fn_80138B60(EnemyWork* self) {
    s32 found;

    if (self->field_0x1DE != 0) {
        if (self->field_0x8C0 != 0) {
            addVec3To(&self->field_0x188, &self->field_0x8B4);
            fn_80138074(self, 1);
            fn_8012A3B4(self);
            fn_8012FF38(self);
        }
        found = -1;
        if (self->field_0x1E2 == 1 && self->field_0x1E3 == 1) {
            found = fn_80137EE0(self, 0);
            if (found != -1) {
                fn_80138024(self, 0x400, found);
            }
        }
        if (self->field_0x8C0 != 0 || found != -1) {
            fn_80138E64(self);
            fn_800E0914(EM_CHAR(self));
        }
        fn_8012A414(self);
        if (self->field_0x766 == 0) {
            if (self->field_0x767 == 1) {
                self->field_0x766 = (u8)(self->field_0x766 + 1);
                copyVec3(&self->field_0x778, &self->field_0x76C);
            }
            if (self->field_0x768 > 0) {
                self->field_0x768 = (s16)(self->field_0x768 - 1);
            }
        }
        fn_801391FC(self);
        fn_801363F8(self);
        if (self->field_0x764 >= 0) {
            if (em_sleep_ck__FP11_ENEMY_WORKUc(self, 0) == 1) {
                self->field_0x762 = 1;
            } else if (self->field_0x762 == 0) {
                self->field_0x764 = (s16)(self->field_0x764 - 1);
                if (self->field_0x764 <= 0) {
                    self->field_0x762 = 1;
                    self->field_0x764 = 2;
                }
            } else {
                self->field_0x764 = (s16)(self->field_0x764 - 1);
                if (self->field_0x764 <= 0) {
                    fn_80133B5C(self);
                }
            }
        }
        {
            void (*cb)(EnemyWork*) = fn_801264BC(self, 6);
            if (cb != NULL) {
                cb(self);
            }
        }
        fn_801333E0(self);
        if ((self->flags_0x1C8 & 1) == 0) {
            fn_80136E38(self, 0);
        } else if ((self->flags_0x1C8 & 0x40000) != 0) {
            if (em_die_ck__FP11_ENEMY_WORK(self) == 1) {
                u32 i;
                for (i = 0; i < fn_800E28E4(EM_CHAR(self)); i++) {
                    fn_800E30DC(EM_CHAR(self), i, 0x10);
                }
            } else {
                u32 i;
                for (i = 0; i < fn_800E28E4(EM_CHAR(self)); i++) {
                    fn_800E2EBC(EM_CHAR(self), i, 2);
                }
            }
        }
        {
            void (*cb)(EnemyWork*) = fn_801264BC(self, 8);
            if (cb != NULL) {
                cb(self);
            }
        }
        fn_8012FC60(self);
        if (self->field_0x1E2 == 4) {
            em_shake_req_set(self);
        }
    }
}

/* Ages the work's per-entity tick counter. */
void fn_80138E18(EnemyWork* self) {
    self->field_0x004 = (u8)(self->field_0x004 + 1);
}

/* Refreshes the work's update mode, preserving its owner byte. */
void fn_80138E28(EnemyWork* self) {
    u8 owner = self->field_0x008;

    fn_80143190();
    self->field_0x008 = owner;
}

/* Copies the work's scale and rotation into its EmCharBase base. */
void fn_80138E64(EnemyWork* self) {
    EmCharBase* ch = &self->char_0x024;

    setScaleAll__6MHcharFf((MHchar*)ch, get_em_scale__FP11_ENEMY_WORK(self));
    ch->field_0x28 = self->field_0x1BC;
    ch->field_0x2C = self->field_0x1C0;
    ch->field_0x30 = self->field_0x1C4;
    copyVec3(&ch->field_0x04, &self->field_0x188);
}

/* Advances one of the two per-entity effect slots. */
void fn_80138EC8(EnemyWork* self, u32 arg1) {
    u8 index = (u8)arg1;

    if (self->field_0xA08[index].field_0x05 != 0 && (self->field_0xAEC[index] & 1) != 0) {
        hit_knock_set(&self->field_0xA08[index], 100);
    }
    hit_attack_list_push(&self->field_0xA08[index]);
}

/* Marks every move the work cannot currently reach with the -1 sentinel. */
void fn_80138F3C(EnemyWork* self) {
    MoveWork* move = (MoveWork*)get_move_work_adrs__FUc(2);
    u16 max = get_move_work_max__FUc(2);
    u32 all_blocked = 1;
    s32 i;
    f32 sentinel = lbl_80796D70;

    for (i = 0; i < (s32)max; i++) {
        if (move[i].field_0x00 != 0 && fn_8012D1A8(move[i].field_0x08) == 0) {
            if (fn_8012D0B4(self, &move[i]) == 0) {
                self->field_0x454[i] = sentinel;
            } else {
                self->field_0x454[i] = fn_80050EF4(&self->field_0x188, &move[i].field_0x3C);
                all_blocked = 0;
            }
        } else {
            self->field_0x454[i] = sentinel;
        }
    }
    if (all_blocked == 1) {
        self->field_0x439 = 1;
    }
}

/* The action-state step: enters the wait state, then picks the next action. */
void fn_80139024(EnemyWork* self) {
    u8 state;

    em_motion_timer_arm(self);
    em_fx_flag_set(self);
    state = self->field_0x005;
    switch (state) {
    case 0:
        self->field_0x005 = (u8)(state + 1);
        em_motion_param_set(self, lbl_80796D40, 0);
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
        if (self->field_0x1E7 != 0) {
            self->field_0x017 = 2;
            return;
        }
        if (self->field_0x00C == 3) {
            self->field_0x017 = 4;
            return;
        }
        self->field_0x017 = 1;
        return;
    }
}

/* Installs (or clears) the work's user-data accessor callback. */
void em_res_user_data_set(EnemyWork* self, ResUserDataAc* arg1) {
    ResUserDataAc* old = self->field_0x484;

    if (old != NULL) {
        old->vtable->fn_0x08(old, 1);
    }
    self->field_0x484 = arg1;
    if (arg1 != NULL) {
        arg1->work = self;
        fn_800E25B0((void*)self->field_0x13C, self->field_0x484);
        self->field_0x484->vtable->fn_0x18(self->field_0x484);
        self->field_0x064 = 0;
    }
}

/* Releases a heap block the work allocated, then hands the pointer back. */
void fn_8013918C(void* arg0, s16 arg1) {
    if (arg0 != 0) {
        fn_800810DC(arg0, 0);
        if (arg1 > 0) {
            __dl__FPv(arg0);
        }
    }
}

/* Reports whether the work has a user-data accessor installed. */
s32 em_res_user_data_ck(EnemyWork* self) {
    return self->field_0x484 != NULL;
}

/* Re-aims the work toward the nearest user-data joint, clamped to the joint's key span. */
void fn_801391FC(EnemyWork* self) {
    f32 v;
    Vec3 pos;
    u32 best;
    u32 total;
    u16 i;
    UserDataItem* item;

    VEC3_ctor(&pos);
    best = (u32)-1;
    total = 0;
    if (self->field_0x740 == 0) {
        v = lbl_80796D40;
    } else {
        item = fn_8013A6C0(self);
        while (item != NULL && item->field_0x00 != (u32)-1) {
            if (item->field_0x04 == 1) {
                if (best == (u32)-1 || best > item->field_0x00) {
                    best = item->field_0x00;
                }
                total = total + (u16)item->field_0x0C;
            }
            item++;
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
    item = fn_8013A6C0(self);
    if (item != NULL) {
        while (item->field_0x00 != (u32)-1) {
            if (item->field_0x04 == 1) {
                s16 target = (s16)(s32)(v * ((f32)item->field_0x0C / (f32)(u16)total));
                u16* slot = &self->field_0x742[item->field_0x08];
                s16 current = (s16)*slot;
                s16 delta = (s16)(u32)(lbl_80796D74 * (f32)item->field_0x0C);
                if ((s16)(target - current) < 0) {
                    s16 down = (s16)(current - delta);
                    if ((s16)(target - down) < 0) {
                        *slot = (u16)down;
                    } else {
                        *slot = (u16)target;
                    }
                } else {
                    s16 up = (s16)(current + delta);
                    if ((s16)(up - target) > 0) {
                        *slot = (u16)target;
                    } else {
                        *slot = (u16)up;
                    }
                }
            }
            item++;
        }
    }
    (void)i;
}

/* --------------------------------------------------------------------------------------------- */
/* 0x801394B8..0x8013AC08 - the g3d_resuser_ac accessor                                           */
/* --------------------------------------------------------------------------------------------- */

/* Re-enters the accessor's default flag state. */
void fn_801394B8(ResUserDataAc* self) {
    fn_8013A654(self, 1);
}

/* Stores the cursor's item index. */
void fn_801394C0(UserDataCursor* self, u32 arg1) {
    self->field_0x06 = (u16)(arg1 & 0xFFFF);
}

/* Reads the cursor's item index. */
u32 fn_801394CC(UserDataCursor* self) {
    return self->field_0x06;
}

/* Dispatches every user-data item of the cursor's index to the accessor's virtuals. */
void fn_801394D4(ResUserDataAc* self, s32 arg1, s32* arg2, UserDataCursor* arg3) {
    u32 index = fn_801394CC(arg3);
    UserDataItem* item;

    if (index == self->work->field_0x110) {
        Vec3 pos;
        VEC3_ctor(&pos);
        pos.x = lbl_80796D40;
        pos.y = lbl_80796D40;
        pos.z = lbl_80796D40;
        fn_8008F148((void*)arg1, &pos);
    }
    item = fn_8013A6C0(self->work);
    while (item != NULL && item->field_0x00 != (u32)-1) {
        if (item->field_0x00 == index) {
            s32 value;
            if (item->field_0x04 == 0 || (u32)(item->field_0x04 - 3) <= 1) {
                value = *arg2;
                self->vtable->fn_0x1C(self, arg1, &value, arg3, index, item);
            } else {
                value = *arg2;
                self->vtable->fn_0x20(self, arg1, &value, arg3, index, item);
            }
        }
        item++;
    }
    if ((self->flags & 2) == 0) {
        fn_8013A770(arg3, self->work);
    }
}

/* Builds the effect transform for one user-data item and applies it to the EmCharBase. */
void fn_80139620(ResUserDataAc* self, void* arg1, void* arg2, s32 arg3, s32 arg4, UserDataItem* arg5) {
    Vec3 angles;
    Mtx34 mtx;
    Mtx34 dst;
    KeyFrameSet* frames;

    VEC3_ctor(&angles);
    MTX34_ctor(&mtx);
    fn_800516F0(&dst);
    switch (arg5->field_0x04) {
    case 0:
    case 3: {
        u8 area = arg5->field_0x10;
        if (area != (u8)-1) {
            EffectArea* rec = &self->work->field_0x488[area];
            if (rec->field_0x00 >= 2) {
                frames = arg5->frames;
                if (frames != NULL) {
                    s32 x = 0;
                    s32 y = 0;
                    s32 z = 0;
                    if (rec->field_0x07 == 1) {
                        x = (s32)frames->field_0x0C;
                        y = (s32)frames->field_0x10;
                        z = (s32)frames->field_0x14;
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
                        cp.x = (u32)(u16)(s32)(lbl_80796D50 + ((lbl_80796D88 * angles.x) / lbl_80796D8C));
                        cp.y = (u32)(u16)(s32)(lbl_80796D50 + ((lbl_80796D88 * angles.y) / lbl_80796D8C));
                        cp.z = (u32)(u16)(s32)(lbl_80796D50 + ((lbl_80796D88 * angles.z) / lbl_80796D8C));
                        cpSetRotMatrix__FP10_CP_VECTORPQ34nw4r4math5MTX34(&cp, &mtx);
                    }
                    fn_805012E8(&dst, &mtx);
                    fn_8008EE68(arg1, &dst);
                }
            }
        }
        return;
    }
    case 4:
        fn_8008DF8C(arg1, &dst);
        self->work->field_0x76C.x = dst.m[1][0];
        self->work->field_0x76C.y = dst.m[1][1];
        self->work->field_0x76C.z = dst.m[1][2];
        break;
    }
}

/* Placeholder the retail object keeps as an empty body. */
void fn_80139854(void) {
}

/* Dispatches every user-data item of the cursor's index to the accessor's second virtual pair. */
void fn_80139858(ResUserDataAc* self, s32 arg1, s32* arg2, UserDataCursor* arg3) {
    u32 index = fn_801394CC(arg3);
    UserDataItem* item = fn_8013A6C0(self->work);

    while (item != NULL && item->field_0x00 != (u32)-1) {
        if (item->field_0x00 == index) {
            s32 value;
            if (item->field_0x04 <= 3) {
                value = *arg2;
                self->vtable->fn_0x24(self, arg1, &value, arg3, index, item);
            } else {
                value = *arg2;
                self->vtable->fn_0x28(self, arg1, &value, arg3, index, item);
            }
        }
        item++;
    }
    fn_8013A770(arg3, self->work);
}

/* Applies one user-data item's transform to the caller's matrix. */
void fn_80139954(ResUserDataAc* self, MtxHolder* arg1, s32 arg2, s32 arg3, s32 arg4, UserDataItem* arg5) {
    Mtx34 mtx;

    MTX34_ctor(&mtx);
    switch (arg5->field_0x04) {
    case 0:
    case 3: {
        u8 area = arg5->field_0x10;
        if (area < 2) {
            EffectArea* rec = &self->work->field_0x488[area];
            if (arg5->field_0x08 == 0 && rec->field_0x00 >= 2) {
                fn_80139A98(arg1, &rec->matrix);
                return;
            }
        }
        return;
    }
    case 1:
        if (arg5->field_0x08 < 6) {
            joint_mtx_load(arg1, &mtx);
            rotMatrixY__FUlPQ34nw4r4math5MTX34(self->work->field_0x742[arg5->field_0x08], &mtx);
            joint_mtx_store(arg1, &mtx);
            return;
        }
        break;
    case 2:
        yure_joint_apply((struct YureRec*)&self->work->field_0x590[arg5->field_0x08], (struct MtxHolder*)arg1);
        break;
    }
}

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
void fn_80139A98(MtxHolder* holder, void* src) {
    mtx34_copy_ps((Mtx34*)holder->mtx, (const Mtx34*)src);
}

/* Placeholder the retail object keeps as an empty body. */
void fn_80139AA0(void) {
}

/* Builds one bone matrix from a keyframed scale/rotation/translation triple. */
void fn_80139AA4(Mtx34* out, void* arg1, Mtx34* arg2) {
    Mtx34 m1;
    Mtx34 m2;
    Mtx34 m3;
    nw4r::g3d::ResNode key((void*)0);
    s32 idx;

    MTX34_ctor(&m1);
    MTX34_ctor(&m2);
    MTX34_ctor(&m3);
    mtx34_copy(&m1, &arg2[fn_8006FDCC(arg1)]);
    idx = fn_8005D124(arg1);
    fn_8005D0CC(&key, &idx);
    mtx34_copy(&m2, &arg2[fn_8006FDCC(&key)]);
    mtx34_inverse(&m3, &m2);
    mtx34_concat(out, &m3, &m1);
}

/* --------------------------------------------------------------------------------------------- */
/* 0x8013A218..0x8013AC08 - the ResUserData value accessors                                       */
/* --------------------------------------------------------------------------------------------- */

/* Reads the item's S32 value. */
ResUserDataItemData* fn_8013A218(ResUserData* self) {
    if (fn_8013A314(self) != 0) {
        nw4r::db::Panic(lbl_805A14C4, 54, lbl_805A1480);
    }
    return (ResUserDataItemData*)fn_8013A280(self, (s32)fn_8013A29C(self)->field_0x04);
}

/* Returns the blob-relative pointer for a value offset, or NULL when the offset is zero. */
void* fn_8013A280(ResUserData* self, s32 arg1) {
    ResUserDataItemData* data = self->data;
    if (arg1 != 0) {
        return (u8*)data + arg1;
    }
    return NULL;
}

/* Returns the item's value header, asserting the item is valid. */
ResUserDataItemData* fn_8013A29C(ResUserData* self) {
    if (fn_8013A428(self) == 0) {
        nw4r::db::Panic(lbl_805A143C, 38, lbl_805A1420, fn_8013A308(), lbl_807919F4);
    }
    return fn_8013A300(self);
}

/* Returns the item's value header. */
ResUserDataItemData* fn_8013A300(ResUserData* self) {
    return self->data;
}

/* The class name the "Object not valid." panic reports for an item. */
const char* fn_8013A308(void) {
    return lbl_805A1410;
}

/* Returns the item's value type. */
s32 fn_8013A314(ResUserData* self) {
    return (s32)fn_8013A338(self)->field_0x0C;
}

/* Returns the item's value header, asserting the item is valid. */
ResUserDataItemData* fn_8013A338(ResUserData* self) {
    if (fn_8013A428(self) == 0) {
        nw4r::db::Panic(lbl_805A146C, 38, lbl_805A1450, fn_8013A308(), lbl_807919F0);
    }
    return fn_8013A39C(self);
}

/* Returns the item's value header. */
ResUserDataItemData* fn_8013A39C(ResUserData* self) {
    return self->data;
}

/* Reads the item's string value. */
ResUserDataItemData* fn_8013A3A4(ResUserData* self) {
    if (fn_8013A314(self) != 2) {
        nw4r::db::Panic(lbl_805A151C, 68, lbl_805A14D8);
    }
    return (ResUserDataItemData*)fn_8013A40C(self, (s32)fn_8013A338(self)->field_0x04);
}

/* Returns the blob-relative pointer for a value offset, or NULL when the offset is zero. */
void* fn_8013A40C(ResUserData* self, s32 arg1) {
    ResUserDataItemData* data = self->data;
    if (arg1 != 0) {
        return (u8*)data + arg1;
    }
    return NULL;
}

/* Reports whether the item holds a value. */
s32 fn_8013A428(ResUserData* self) {
    return self->data != NULL;
}

/* Copy-assigns one item wrapper onto another. */
ResUserData* fn_8013A43C(ResUserData* dst, ResUserData* src) {
    fn_8013A488(dst, src);
    fn_8013A484(dst, src);
    return dst;
}

/* Trivial member copy the retail object keeps as an empty body. */
void fn_8013A484(ResUserData* dst, ResUserData* src) {
    (void)dst;
    (void)src;
}

/* Copies the item's value header pointer. */
void fn_8013A488(ResUserData* dst, ResUserData* src) {
    dst->data = src->data;
}

/* Looks a key up in the item's name table and returns the matching value header. */
ResUserDataItemData* fn_8013A494(ResUserData* self, const char* key) {
    ResUserDataItemData* data = fn_8013A4E8(self);
    ResUserData found;

    return fn_8013A5E4(&found, (ResUserDataItemData*)nw4r::g3d::ResDic(&data->field_0x04)[key])->data;
}

/* Returns the item's value header, asserting the item is valid. */
ResUserDataItemData* fn_8013A4E8(ResUserData* self) {
    if (fn_8013A594(self) == 0) {
        nw4r::db::Panic(lbl_805A13C0, 87, lbl_805A13A4, fn_8013A554(), lbl_807919F8);
    }
    return fn_8013A54C(self);
}

/* Returns the item's value header. */
ResUserDataItemData* fn_8013A54C(ResUserData* self) {
    return self->data;
}

/* The class name the "Object not valid." panic reports for the collection. */
const char* fn_8013A554(void) {
    return lbl_805A1398;
}

/* Reports whether the item's name table holds any entries. */
void fn_8013A560(ResUserData* self) {
    nw4r::g3d::ResDic(&fn_8013A4E8(self)->field_0x04).GetNumData();
}

/* Reports whether the item holds a value. */
s32 fn_8013A594(ResUserData* self) {
    return self->data != NULL;
}

/* Copy-constructs one item wrapper from another. */
ResUserData* fn_8013A5A8(ResUserData* self, ResUserData* src) {
    fn_8013A5D8(self, src);
    return self;
}

/* Copies the item's value header pointer. */
void fn_8013A5D8(ResUserData* dst, ResUserData* src) {
    dst->data = src->data;
}

/* Stores the value header, asserting it is 4-byte aligned. */
ResUserData* fn_8013A5E4(ResUserData* self, ResUserDataItemData* data) {
    fn_8013A648(self, data);
    if (((u32)data & 3) != 0) {
        nw4r::db::Panic(lbl_805A13FC, 38, lbl_805A13D4);
    }
    return self;
}

/* Stores the value header pointer. */
void fn_8013A648(ResUserData* self, ResUserDataItemData* data) {
    self->data = data;
}

/* Placeholder the retail object keeps as an empty body. */
void fn_8013A650(void) {
}

/* Refreshes the accessor's flag word from the work's user-data list. */
void fn_8013A654(ResUserDataAc* self, u32 arg1) {
    EnemyWork* work = self->work;
    u32 handle = work->field_0x13C;

    self->flags = arg1;
    self->flags = self->flags | (u32)fn_8013A6F4(work);
    fn_80080B10((void*)handle, self->flags);
    fn_800E3264((void*)handle, work->field_0x110);
}

/* Returns the work's user-data item list. */
UserDataItem* fn_8013A6C0(EnemyWork* work) {
    EnemyData* data = get_enemy_data__FP11_ENEMY_WORK(work);

    if (data != NULL) {
        return data->items;
    }
    return NULL;
}

/* Builds the accessor's per-type flag word from the work's user-data list. */
s32 fn_8013A6F4(EnemyWork* work) {
    s32 flags = 1;
    UserDataItem* item = fn_8013A6C0(work);

    if (item != NULL) {
        while (item->field_0x00 != (u32)-1) {
            u8 type = item->field_0x04;
            if ((u32)(type - 1) <= 1) {
                flags |= 2;
            } else if (type == 0 || type == 3) {
                flags |= 6;
            }
            item++;
        }
    }
    return flags;
}

/* Advances the cursor to the next user-data index the work can still reach. */
void fn_8013A770(UserDataCursor* cursor, EnemyWork* work) {
    u32 current = fn_801394CC(cursor);
    u32 best = current;
    UserDataItem* item = fn_8013A6C0(work);

    if (item != NULL) {
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
    }
    if (best > current && best != (u32)-1) {
        fn_801394C0(cursor, best);
    }
}

/* Reads one entry of the enemy data's secondary table. */
s32 fn_8013A830(EnemyWork* work, u8 arg1) {
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
s32 fn_8013A900(EnemyWork* work) {
    s32 state = work->field_0x95C;
    u8 index = work->field_0x9A8;

    for (;;) {
        switch (state) {
        case 2:
            state = work->field_0x99C;
            break;
        case 1:
            index = (u8)(index - 1);
            state = work->field_0x9AC[index].field_0x00;
            break;
        case 0xA:
            state = work->field_0x9E8;
            break;
        default:
            return state;
        }
    }
}

/* Clears the work's user-data state and re-enters the given mode. */
void fn_8013A954(EnemyWork* work, u8 arg1) {
    work->field_0x962 = 0;
    work->field_0x9A8 = 0;
    work->field_0x43E = 0;
    work->field_0x999 = 0;
    work->field_0x99A = 0;
    work->field_0x99B = 0;
    em_userdata_state_exit(work, arg1);
}

/* Installs the work's user-data state table and re-enters the default state. */
void fn_8013A978(EnemyWork* work) {
    work->field_0x954 = (u32**)fn_80126098_c1();
    if (fn_8013A884(work, 0) == 1) {
        fn_8013AB74(work, 0, 0);
    } else {
        work->field_0x95C = 0;
        work->field_0x95D = 0;
        work->field_0x958 = 0;
    }
    fn_8013A9F4(work);
    fn_8013A954(work, 1);
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

/* Re-enters the work's user-data state, preferring the special index when the work is in state 3. */
void fn_8013AA1C(EnemyWork* work, u8 arg1) {
    if (work->field_0x89F == 3 && fn_8013A884(work, 9) == 1) {
        fn_8013AB74(work, 9, 0);
    } else if (fn_8013A884(work, 0) == 1) {
        fn_8013AB74(work, 0, 0);
    } else {
        work->field_0x95C = 0;
        work->field_0x95D = 0;
        work->field_0x958 = 0;
    }
    fn_8013A954(work, arg1);
}

/* Re-enters the work's user-data state in the default mode. */
void fn_8013AAC4(EnemyWork* work) {
    fn_8013AA1C(work, 1);
}

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

/* Re-enters the work's user-data state in the non-default mode. */
void em_userdata_state_reenter_alt(EnemyWork* work) {
    fn_8013AA1C(work, 0);
}

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

/* Draws one random number for the work's user-data roll. */
void em_userdata_roll(void) {
    ran_suu__Fl(1);
}

/* Walks the work's per-motion records backwards to the first one matching the current state. */
u8 em_userdata_record_find_back(EnemyWork* work, u8 arg1, u8 arg2) {
    u8 index = arg1;
    EnemyData* data = work->field_0x9A4;

    if (data != NULL) {
        EmDataRecord* rec = &data->records[(s16)index];
        while ((s16)index >= 0) {
            if (rec->field_0x0B == 0 || arg2 == rec->field_0x0B) {
                if (rec->field_0x09 != fn_8013023C(work)) {
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
