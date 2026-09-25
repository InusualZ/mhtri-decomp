/* The `lobby` NPC group's reconstructed types (`src/lobby/lb_npc.cpp`).
 *
 * A header rather than the unit's own source because `_LB_NPC` is the name the map's mangling encodes
 * (`lb_npc_Get_motion_no__FP7_LB_NPC`) and `src/sound/fn_800D7F54.cpp` already defines a partial view of
 * that name - rule 1's "a shared type is defined once" keeps the reconstruction here, and the sound
 * unit's view is part of the pre-existing backlog (its own header move is a config_request).
 * docs/plan.md 6.5 rules 3/4/5: every type states its size, every field its offset and a name.
 */
#ifndef MHTRI_LOBBY_LB_NPC_H
#define MHTRI_LOBBY_LB_NPC_H

#include "types.h"
#include "nw4r/math.h"

/* ---------------------------------------------------------------------------------------------------
 * Types
 */
/* The block `lbl_80794880` points at (a 4-byte pointer in .sbss).  Only the fields this unit touches
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

/* The system work block's tail byte `fn_801FC6EC` clears (a partial view; the full block belongs to the
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

/* The motion record an NPC points at through `field_0x214`; only its trailing VEC3 is used here. */
typedef struct LbNpcMotion {
    /* +0x00 */ u8 pad_0x00[0x3C];
    /* +0x3C */ VEC3 vec_0x3C;
} LbNpcMotion; /* size: 0x48+ */

/* One `_LB_NPC` record - the type `lb_npc_Get_motion_no__FP7_LB_NPC` / `lb_npc_area_ck__FP7_LB_NPCUc`
 * take and the 0x12-element `lb_npc` array's element (stride 0x268).  The class name is the map's own
 * (`7_LB_NPC`); the sound unit `src/sound/fn_800D7F54.cpp` carries a partial view of the same name that
 * only reads +0x002 - the shared home is a config_request. */
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
    /* +0x174 */ u8 pad_0x174[0x58];
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
    /* +0x204 */ s32 field_0x204;
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

/* The `.bss` lobby work block (`lobby_w`, 0x17C B).  Its view here is this unit's own (see the note
 * above): `include/unsplit/lobby.h` carries the menu-layer unit's partial view, whose `lbl_80794880`
 * is declared as an array rather than the pointer the target loads, so the two views cannot be one. */
typedef struct LbNpcLobbyWork {
    /* +0x000 */ u8 field_0x000;
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ s8 field_0x002;
    /* +0x003 */ u8 field_0x003;
    /* +0x004 */ u8 pad_0x004[0x23];
    /* +0x027 */ u8 field_0x027;
    /* +0x028 */ u8 pad_0x028[0x0E];
    /* +0x036 */ s16 field_0x036[13];
    /* +0x050 */ u8 pad_0x050[0x29];
    /* +0x079 */ s8 field_0x079;
    /* +0x07A */ u8 pad_0x07A[6];
    /* +0x080 */ u8 field_0x080;
    /* +0x081 */ u8 pad_0x081[0x3F];
    /* +0x0C0 */ u8 talk_0x0C0[0x6C];
    /* +0x12C */ u8 field_0x12C;
    /* +0x12D */ u8 pad_0x12D[0x36];
    /* +0x163 */ u8 field_0x163;
    /* +0x164 */ u8 pad_0x164[0x0B];
    /* +0x16F */ s8 field_0x16F;
    /* +0x170 */ u8 pad_0x170[2];
    /* +0x172 */ s16 field_0x172;
    /* +0x174 */ u8 pad_0x174[8];
} LbNpcLobbyWork; /* size: 0x17C */

/* One of the 0x12 NPC work records in the `lb_npc` array (stride 0x268). */
typedef struct LbNpcWork {
    /* +0x000 */ u8 alive_0x000;
    /* +0x001 */ u8 pad_0x001[0x267];
} LbNpcWork; /* size: 0x268 */



#endif /* MHTRI_LOBBY_LB_NPC_H */
