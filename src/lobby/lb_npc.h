/* The types of `lobby/lb_npc.cpp`: `_LB_NPC` is the name the map's mangling encodes
 * (`lb_npc_Get_motion_no__FP7_LB_NPC`); `sound/fn_800D7F54.cpp` still defines its own partial view of it (rule 1). */
#ifndef MHTRI_LOBBY_LB_NPC_H
#define MHTRI_LOBBY_LB_NPC_H

#include "types.h"
#include "nw4r/math.h"
#include "Pl/plw.h"   /* _PLW (`lb_npc_act_set`'s player work) */

/* ---------------------------------------------------------------------------------------------------
 * Types
 */
/* The block `lobby_world_block` points at (a 4-byte pointer in .sbss).  Only the fields this unit touches
 * are named; the rest is padding and carries its offset. */
typedef struct LbNpcUserData {
    /* +0x0000 */ u8 pad_0x0000[0x3E00];
    /* +0x3E00 */ u8 field_0x3E00;
    /* +0x3E01 */ u8 field_0x3E01;
    /* +0x3E02 */ u8 pad_0x3E02;
    /* +0x3E03 */ u8 field_0x3E03;
    /* +0x3E04 */ u8 pad_0x3E04[0x264];
    /* +0x4068 */ u8 name_0x4068[0x10];
    /* +0x4078 */ u8 pad_0x4078[0x7C3];
    /* +0x483B */ u8 field_0x483B;
    /* +0x483C */ u8 pad_0x483C[0x960];
    /* +0x519C */ u32 field_0x519C;
    /* +0x51A0 */ u8 parts_0x51A0[0x3C];
    /* +0x51DC */ u8 pad_0x51DC[0x8];
    /* +0x51E4 */ u8 tail_0x51E4[];
} LbNpcUserData; /* size: 0x51E4+ */

/* The g3d model handle an NPC owns through `field_0x050`; only its flag byte is read here. */
typedef struct LbNpcModel {
    /* +0x000 */ u8 pad_0x000[6];
    /* +0x006 */ u8 flag_0x006;
} LbNpcModel; /* size: 0x7+ */

/* The system work block's tail byte `lobby_frame_update` clears (a partial view; the full block belongs to the
 * ef/system units). */
typedef struct LbNpcSystemWork {
    /* +0x000 */ u8 pad_0x000[0x8C0];
    /* +0x8C0 */ u8 field_0x8C0;
} LbNpcSystemWork; /* size: 0x8C1 */

/* One 8-byte resource id pair in the lobby's `lbl_80582A68` table. size: 0x8 */
typedef struct LbResId {
    /* +0x00 */ u32 a_0x00;
    /* +0x04 */ u32 b_0x04;
} LbResId;

/* The loaded-resource header `fn_801FCADC` reads its payload pointer from. */
typedef struct LbResource {
    /* +0x00 */ u8 pad_0x00[0x44];
    /* +0x44 */ void* payload_0x44;
} LbResource; /* size: 0x48 */

/* The 0xC-byte resource record `fn_801FCA80`/`fn_801FCADC` build. size: 0xC */
typedef struct LbResRec {
    /* +0x00 */ u32 a_0x00;
    /* +0x04 */ u32 b_0x04;
    /* +0x08 */ u32 c_0x08;
} LbResRec;

/* One 0x14-byte entry of the NPC's motion table - the array `_LB_NPC::field_0x204` points at and
 * `field_0x208` indexes (the band 0x802029B4..0x802076D4's state machines read an entry's id and hand
 * it to `lb_npc_motion_restart`, which restarts that motion).  Only the id is read here; size from the two
 * `mulli ..., 20` sites. size: 0x14 */
typedef struct LbNpcMotionEntry {
    /* +0x00 */ u8 pad_0x00[0x0C];
    /* +0x0C */ u16 motion_0x0C;
    /* +0x0E */ u8 pad_0x0E[2];
    /* +0x10 */ struct LbNpcMotionEntry* field_0x10;
} LbNpcMotionEntry;

/* The motion record an NPC points at through `field_0x214`; only its trailing VEC3 is used here. */
typedef struct LbNpcMotion {
    /* +0x00 */ u8 pad_0x00[0x3C];
    /* +0x3C */ VEC3 vec_0x3C;
} LbNpcMotion; /* size: 0x48+ */

/* One `_LB_NPC` record - the type `lb_npc_Get_motion_no__FP7_LB_NPC` / `lb_npc_area_ck__FP7_LB_NPCUc`
 * take and the 0x12-element `lb_npc` array's element (stride 0x268).  The class name is the map's own
 * (`7_LB_NPC`); `sound/fn_800D7F54.cpp` still carries a partial view of the same name that reads only +0x002. */
typedef struct _LB_NPC {
    /* +0x000 */ u8 field_0x000;
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ u8 field_0x002;
    /* +0x003 */ u8 field_0x003;
    /* +0x004 */ u8 field_0x004;
    /* +0x005 */ u8 field_0x005;
    /* +0x006 */ u8 field_0x006;
    /* +0x007 */ u8 field_0x007;
    /* +0x008 */ u8 field_0x008;
    /* +0x009 */ u8 pad_0x009[3];
    /* +0x00C */ s32 field_0x00C;
    /* +0x010 */ VEC3 pos_0x10;
    /* +0x01C */ VEC3 field_0x01C;
    /* +0x028 */ s32 field_0x028;
    /* +0x02C */ s32 field_0x02C;
    /* +0x030 */ s32 field_0x030;
    /* +0x034 */ f32 field_0x034;
    /* +0x038 */ VEC3 field_0x038;
    /* +0x044 */ VEC3 field_0x044;
    /* +0x050 */ LbNpcModel* model_0x050;
    /* +0x054 */ u8 field_0x054[4];
    /* +0x058 */ u8 body_0x058[0x50];
    /* +0x0A8 */ u16 motion_0x0A8;
    /* +0x0AA */ u8 pad_0x0AA[0x9E];
    /* +0x148 */ u8 field_0x148;
    /* +0x149 */ u8 field_0x149;
    /* +0x14A */ u8 field_0x14A;
    /* +0x14B */ u8 pad_0x14B[0x19];
    /* +0x164 */ void* field_0x164;
    /* +0x168 */ u8 pad_0x168[8];
    /* +0x170 */ s32 field_0x170;
    /* +0x174 */ u8 pad_0x174[0x0E];
    /* +0x182 */ u16 field_0x182;
    /* +0x184 */ u8 pad_0x184[0x48];
    /* +0x1CC */ u16 field_0x1CC;
    /* +0x1CE */ u8 field_0x1CE;
    /* +0x1CF */ u8 pad_0x1CF;
    /* +0x1D0 */ u16 field_0x1D0;
    /* +0x1D2 */ u8 pad_0x1D2[2];
    /* +0x1D4 */ s32 field_0x1D4;
    /* +0x1D8 */ s16 field_0x1D8;
    /* +0x1DA */ u8 pad_0x1DA[2];
    /* +0x1DC */ f32 field_0x1DC;
    /* +0x1E0 */ u8 field_0x1E0;
    /* +0x1E1 */ u8 pad_0x1E1;
    /* +0x1E2 */ s16 field_0x1E2;
    /* +0x1E4 */ s16 field_0x1E4;
    /* +0x1E8 */ s32 field_0x1E8;
    /* +0x1EC */ VEC3 target_0x1EC;
    /* +0x1F8 */ VEC3 vel_0x1F8;
    /* +0x204 */ LbNpcMotionEntry* field_0x204;
    /* +0x208 */ s16 field_0x208;
    /* +0x20A */ s16 field_0x20A;
    /* +0x20C */ f32 field_0x20C;
    /* +0x210 */ u8 field_0x210;
    /* +0x211 */ u8 pad_0x211[3];
    /* +0x214 */ LbNpcMotion* field_0x214;
    /* +0x218 */ s32 field_0x218;
    /* +0x21C */ s16 field_0x21C;
    /* +0x21E */ s16 field_0x21E;
    /* +0x220 */ s16 field_0x220;
    /* +0x222 */ u8 field_0x222;
    /* +0x223 */ u8 field_0x223;
    /* +0x224 */ u8 field_0x224;
    /* +0x225 */ u8 field_0x225;
    /* +0x226 */ u8 field_0x226;
    /* +0x227 */ u8 field_0x227;
    /* +0x228 */ u8 field_0x228;
    /* +0x229 */ u8 field_0x229;
    /* +0x22A */ u8 field_0x22A;
    /* +0x22B */ u8 field_0x22B;
    /* +0x22C */ u8 field_0x22C;
    /* +0x22D */ u8 field_0x22D;
    /* +0x22E */ u8 field_0x22E;
    /* +0x22F */ u8 pad_0x22F;
    /* +0x230 */ s16 field_0x230;
    /* +0x232 */ u8 pad_0x232[2];
    /* +0x234 */ u8 field_0x234[0x20];
    /* +0x254 */ u8 field_0x254;
    /* +0x255 */ u8 field_0x255;
    /* +0x256 */ u8 field_0x256;
    /* +0x257 */ u8 pad_0x257;
    /* +0x258 */ VEC3 field_0x258;
    /* +0x264 */ u8 field_0x264;
    /* +0x265 */ u8 pad_0x265[3];
} _LB_NPC; /* size: 0x268 */

/* ---------------------------------------------------------------------------------------------------
 * This unit's callable surface; the map spells the names unmangled, so the declarations are `extern "C"`.
 */
/* One of the 0x12 NPC work records in the `lb_npc` array (stride 0x268). */
typedef struct LbNpcWork {
    /* +0x000 */ u8 alive_0x000;
    /* +0x001 */ u8 pad_0x001[0x267];
} LbNpcWork; /* size: 0x268 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801FF984 - the block constructor `fn_802C2698` hands a record's +0x08 to; unwritten, so the declaration is that
 * call site's view. */
void mhchar_construct(void* block);

#ifdef __cplusplus
}
#endif

/* Declarations moved here from `unsplit/lobby.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

extern u8* lb_item_get_data;

/* 0x802087D4 - resets the lobby party state when the party is dissolved (GUESS name, from the body). */
void lb_party_state_reset(void);

/* 0x801FCA00 / 0x801FC8F0 - release every NPC's model and refill the NPC slots from the map's table (GUESS names). */
void lb_npc_model_release_all(void);
void lb_npc_map_setup(void);
/* 0x801FC6EC - the lobby's per-frame world update (GUESS name). */
void lobby_frame_update(void);
/* 0x80209070 - places the local player at spawn record `spawn` (entry `id`) of map `map`'s area `area` (GUESS name). */
void lb_player_spawn_set(u8* spawn, u16 id, u8 map, u8 area);

/* 0x801FE5B0 - the live NPC of kind `kind` (GUESS name). */
struct _LB_NPC* lb_npc_find(u8 kind);
/* 0x80207E9C - raises the NPC's +0x234 event flag with `state` beside it (GUESS name). */
void lb_npc_event_set(struct _LB_NPC* self, u8 state);
/* 0x801FE13C - restarts the NPC's motion state machine on motion `motion_id` (GUESS name). */
void lb_npc_motion_restart(struct _LB_NPC* self, u16 motion_id);
/* 0x8020A3E4 - starts act `act`/`sub` on the player work, with the request `flags` (bit 0x20 re-rolls the random
 * pick).  GUESS name; the spelling is the owner's own (`lobby/lb_npc.cpp`). */
void lb_npc_act_set(_PLW* self, u32 act, s32 sub, s32 flags);
/* 0x80208A00 - latches the act step when the master act runs (GUESS name). */
void lb_player_act_latch(struct _PLW* self);
/* 0x8020E778 - the heading from the player towards `target` (GUESS name). */
u32 lb_player_angle_to(struct _PLW* self, VEC3* target);

#ifdef __cplusplus
}

/* 0x801FE1C4 - the NPC's current motion number (`lb_npc_Get_motion_no__FP7_LB_NPC`, C++ linkage - rule 9). */
u16 lb_npc_Get_motion_no(_LB_NPC* self);
#endif

#endif /* MHTRI_LOBBY_LB_NPC_H */
