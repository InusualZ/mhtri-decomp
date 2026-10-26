/* The types of `lobby/lb_npc.cpp`: `_LB_NPC` is the name the map's mangling encodes
 * (`lb_npc_Get_motion_no__FP7_LB_NPC`); `sound/fn_800D7F54.cpp` still defines its own partial view of it (rule 1). */
#ifndef MHTRI_LOBBY_LB_NPC_H
#define MHTRI_LOBBY_LB_NPC_H

#include "types.h"
#include "nw4r/math.h"
#include "Pl/plw.h"   /* _PLW (`lb_npc_act_set`'s player work) */
#include "ef/cp_vector.h"   /* _CP_VECTOR, the NPC's rotation view */

/* ---------------------------------------------------------------------------------------------------
 * Types
 */
/* The block `lobby_world_block` points at (a 4-byte pointer in .sbss).  Only the fields this unit touches
 * are named; the rest is padding and carries its offset. */
typedef struct LbNpcUserData {
    /* +0x0000 */ u8 pad_0x0000[0x18];
    /* +0x0018 */ s32 money_0x18;          /* the zenny the shop charges and credits */
    /* +0x001C */ u8 pad_0x001C[0x144];
    /* +0x0160 */ _SLOTENT supplies_0x160[8];   /* the carried supplies the sell list's last page shows */
    /* +0x0180 */ _SLOTENT box_0x180[0xF20];    /* the item box */
    /* +0x3E00 */ u8 field_0x3E00;
    /* +0x3E01 */ u8 field_0x3E01;
    /* +0x3E02 */ u8 pad_0x3E02;
    /* +0x3E03 */ u8 field_0x3E03;
    /* +0x3E04 */ u8 pad_0x3E04[0x106];
    /* +0x3F0A */ u8 room_lock_0x3F0A;      /* the room's visit lock (the room-visit screen's 0 / 1 / 2) */
    /* +0x3F0B */ u8 pad_0x3F0B[0x15D];
    /* +0x4068 */ u8 name_0x4068[0x10];
    /* +0x4078 */ u8 pad_0x4078[0x84];
    /* +0x40FC */ u8 room_pass_0x40FC[0x5]; /* the room's password */
    /* +0x4101 */ u8 pad_0x4101[0x73A];
    /* +0x483B */ u8 field_0x483B;
    /* +0x483C */ u8 place_0x483C[0x11];    /* the place info `place_info_set` shows (one interior id per room spot) */
    /* +0x484D */ u8 room_layout_0x484D;    /* the room layout the screen-0x1E list picked */
    /* +0x484E */ u8 pad_0x484E[0x94E];
    /* +0x519C */ u32 field_0x519C;          /* the rank point bits the quest results OR in */
    /* +0x51A0 */ u16 hunt_rank_0x51A0;      /* the last quest's hunt rank */
    /* +0x51A2 */ u16 time_rank_0x51A2;      /* its time rank */
    /* +0x51A4 */ u8 pad_0x51A4[2];
    /* +0x51A6 */ u16 cat_rank_0x51A6[7];    /* the felyne companion's rank by companion kind */
    /* +0x51B4 */ _SLOTENT rank_items_0x51B4[12];   /* the result's rank reward items */
    /* +0x51E4 */ u8 tail_0x51E4[];
} LbNpcUserData; /* size: 0x51E4+ */

/* One 8-byte NPC placement record of the map's NPC table (`npc_data_town`/`npc_data_village` rows, stride 8, a zero
 * first byte ends the table); an NPC keeps a pointer to its record in `field_0x050`.  size: 0x8 */
typedef struct LbNpcModel {
    /* +0x000 */ u8 npc_id_0x000;    /* copied to `_LB_NPC::field_0x002` */
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ s16 model_0x002;    /* the model kind `lb_npc_id_ck` compares */
    /* +0x004 */ u8 variant_0x004;   /* the model variant (0xFF: none) */
    /* +0x005 */ u8 flags_0x005;     /* 0x08: always shown, 0x10: kept updating off screen, 0x20: dropped to the floor */
    /* +0x006 */ u8 flag_0x006;
    /* +0x007 */ u8 pad_0x007;
} LbNpcModel;

/* The ground probe record `em_land_data_init` constructs and the ground query fills: the ground attribute and the
 * surface normal.  size: 0x14 */
typedef struct LbNpcGround {
    /* +0x00 */ u16 attr_0x00;
    /* +0x02 */ u8 pad_0x02[6];
    /* +0x08 */ VEC3 normal_0x08;
} LbNpcGround;

/* One 8-byte model-dressing op of an `npc_sub_data` list (an op of 0xFF ends the list): 0 hides node `node_0x1`,
 * 2 shows it, 1 sets its material colour to the RGB in `color_0x4`.  size: 0x8 */
typedef struct LbNpcSubOp {
    /* +0x0 */ u8 op_0x0;
    /* +0x1 */ u8 node_0x1;
    /* +0x2 */ u8 pad_0x2[2];
    /* +0x4 */ u32 color_0x4;
} LbNpcSubOp;

/* One NPC motion-table override of an event set (an NPC id of 0xFF ends the set).  size: 0x8 */
typedef struct LbNpcEventSet {
    /* +0x0 */ u8 npc_0x0;
    /* +0x1 */ u8 pad_0x1[3];
    /* +0x4 */ struct LbNpcMotionEntry* table_0x4;
} LbNpcEventSet;

/* `npc_event_set_data` (.sbss): the override sets by world mode and by running event.  size: 0x8 */
typedef struct LbNpcEventSets {
    /* +0x0 */ LbNpcEventSet** by_mode_0x0;
    /* +0x4 */ LbNpcEventSet** by_event_0x4;
} LbNpcEventSets;

/* `move_npc_tbl_ptr` (.bss): the NPC motion tables `lb_npc_motion_table_select` picks from.  Names are GUESSes from the NPC
 * each list serves.  size: 0x38 */
typedef struct LbNpcMoveTables {
    /* +0x00 */ struct LbNpcMotionEntry** by_npc_0x00;      /* by NPC id */
    /* +0x04 */ struct LbNpcMotionEntry** by_area_0x04;     /* NPC 1, by area */
    /* +0x08 */ struct LbNpcMotionEntry** npc2_0x08;        /* by lobby layout */
    /* +0x0C */ struct LbNpcMotionEntry** npc9_0x0C;
    /* +0x10 */ struct LbNpcMotionEntry** npc14_0x10;
    /* +0x14 */ struct LbNpcMotionEntry** npc15_0x14;
    /* +0x18 */ struct LbNpcMotionEntry** npc23_0x18;       /* by talk area */
    /* +0x1C */ struct LbNpcMotionEntry** npc48_0x1C;       /* [the kujira event is on] */
    /* +0x20 */ struct LbNpcMotionEntry** npc71_0x20;       /* by area 5..7 */
    /* +0x24 */ struct LbNpcMotionEntry** npc73_0x24;       /* by area 5..7 */
    /* +0x28 */ struct LbNpcMotionEntry** model12_0x28;
    /* +0x2C */ struct LbNpcMotionEntry** npc4_kujira_0x2C;
    /* +0x30 */ struct LbNpcMotionEntry** npc2_talk_0x30;
    /* +0x34 */ struct LbNpcMotionEntry** npc14_talk_0x34;
} LbNpcMoveTables;

/* One 0x18-byte lobby spot (a zero kind ends an area's list): the spot kind and id, its position and the facing
 * the player takes there.  size: 0x18 */
typedef struct LbSpot {
    /* +0x00 */ s16 kind_0x00;
    /* +0x02 */ s16 id_0x02;
    /* +0x04 */ VEC3 pos_0x04;
    /* +0x10 */ u32 angle_0x10;
    /* +0x14 */ u8 pad_0x14[4];
} LbSpot;

/* An area entry the player is placed at (`lb_player_area_enter`): the area, the position and the facing.  size: 0x34 */
typedef struct LbPlaceRec {
    /* +0x00 */ u8 area_0x00;
    /* +0x01 */ u8 pad_0x01[0x23];
    /* +0x24 */ f32 x_0x24;
    /* +0x28 */ f32 y_0x28;
    /* +0x2C */ f32 z_0x2C;
    /* +0x30 */ u32 angle_0x30;
} LbPlaceRec;

/* The three `Pl_chr_set_attr_default` arguments of an item act: the motion, the blend frames and the start
 * frame.  size: 0x6 */
typedef struct LbMotionArgs {
    /* +0x0 */ u16 motion_0x0;
    /* +0x2 */ s16 blend_0x2;
    /* +0x4 */ s16 start_0x4;
} LbMotionArgs;

/* The arm-wrestle screen's work block (`LbLobbyWork::arm_wrestle_0x0AC`, cleared 0x2000 bytes at a time): the
 * step, its timers and the two players.  Names are GUESSes from the screen's state machine.  size: 0x2000 */
typedef struct LbArmWrestleWork {
    /* +0x00 */ s8 step_0x00;
    /* +0x01 */ u8 pad_0x01[3];
    /* +0x04 */ s16 wait_0x04;
    /* +0x06 */ s16 count_0x06;
    /* +0x08 */ s16 tick_0x08;
    /* +0x0A */ s16 cooldown_0x0A;
    /* +0x0C */ s16 power_0x0C;
    /* +0x0E */ u8 pad_0x0E[2];
    /* +0x10 */ struct _PLW* self_0x10;
    /* +0x14 */ struct _PLW* opponent_0x14;
    /* +0x18 */ u8 pad_0x18[0x1FE8];
} LbArmWrestleWork;

/* One row of the shop list: the item, its discount mark, the price and the row's state bits.  size: 0xC */
typedef struct LbShopRow {
    /* +0x0 */ u16 item_0x0;
    /* +0x2 */ u8 discount_0x2;    /* set when the price is halved (`lb_shop_price_get`) */
    /* +0x3 */ u8 pad_0x3;
    /* +0x4 */ s32 price_0x4;
    /* +0x8 */ u16 flags_0x8;      /* 0x80: not for sale (0x02: box full), 0x04: sold out */
    /* +0xA */ u8 pad_0xA[2];
} LbShopRow;

/* One cell of the item box preview the sell list shows.  size: 0x18 */
typedef struct LbBoxCell {
    /* +0x00 */ u8 pad_0x00[2];
    /* +0x02 */ u8 selected_0x02;
    /* +0x03 */ u8 pad_0x03[3];
    /* +0x06 */ s16 index_0x06;
    /* +0x08 */ u8 pad_0x08[0x10];
} LbBoxCell;

/* One row the shop list draws: the item and its price, handed to `menu_item_row_draw_by_lsp` by address.  size: 0x4 */
typedef struct LbShopCell {
    /* +0x0 */ u16 item_0x0;
    /* +0x2 */ s16 price_0x2;
} LbShopCell;

/* The NPC shop screen's work block (`LbLobbyWork::shop_0x0AC`, screen 2): the step, the list cursor and pages,
 * the buy/sell count dialog, the shop's player and NPC, the stocked items and the eight shown rows.  Names are
 * GUESSes from the shop screens.  size: 0x2000 */
typedef struct LbShopWork {
    /* +0x000 */ u8 step_0x000;
    /* +0x001 */ u8 sub_0x001;
    /* +0x002 */ u8 pad_0x002[2];
    /* +0x004 */ s16 blink_0x004;
    /* +0x006 */ s16 cursor_0x006;
    /* +0x008 */ s16 page_0x008;
    /* +0x00A */ s16 pages_0x00A;
    /* +0x00C */ u16 count_0x00C;
    /* +0x00E */ u16 item_0x00E;
    /* +0x010 */ s16 count_max_0x010;
    /* +0x012 */ s16 rows_0x012;
    /* +0x014 */ s32 yes_no_0x014;
    /* +0x018 */ u16 slot_0x018;
    /* +0x01A */ u8 pad_0x01A[2];
    /* +0x01C */ union {
        s16 kind_0x01C;            /* the chosen shop list: 0 buy, 1 sell, 2 buy into the box */
        u8 choice_0x01C[0x20];     /* the buy/sell choice window `lb_list_init` builds */
    };
    /* +0x03C */ struct _PLW* plw_0x03C;
    /* +0x040 */ struct _LB_NPC* npc_0x040;
    /* +0x044 */ u16 items_0x044[0x80];
    /* +0x144 */ LbShopRow rows_0x144[8];
    /* +0x1A4 */ LbBoxCell box_0x1A4[8];
    /* +0x264 */ s16 box_page_0x264;
    /* +0x266 */ s16 box_mode_0x266;
    /* +0x268 */ u8 pad_0x268[0x1D98];
} LbShopWork;

/* `lb_npc_scene_work` (.bss): the lobby sub-scene latch block the scripted NPC scenes run on: the scene step, the
 * armed and finished flags, a delay and the scene's frame timer, the scene id and the once-a-frame tick latch.
 * size: 0x20 */
typedef struct LbNpcSceneWork {
    /* +0x00 */ u8 step_0x00;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u8 active_0x02;
    /* +0x03 */ u8 done_0x03;
    /* +0x04 */ s16 delay_0x04;
    /* +0x06 */ s16 timer_0x06;
    /* +0x08 */ u8 pad_0x08[8];
    /* +0x10 */ u8 scene_0x10;
    /* +0x11 */ u8 ticked_0x11;
    /* +0x12 */ u8 pad_0x12[0xE];
} LbNpcSceneWork;

/* The system work block's tail byte `lobby_frame_update` clears (a partial view; the full block belongs to the
 * ef/system units). */
typedef struct LbNpcSystemWork {
    /* +0x000 */ u8 pad_0x000[0xC];
    /* +0x00C */ u32 frame_0x00C;   /* the frame counter the NPC effects gate on (`& 0x3F`, `& 7`) */
    /* +0x010 */ u8 pad_0x010[0x1D];
    /* +0x02D */ u8 field_0x02D;
    /* +0x02E */ u8 pad_0x02E[0x7A0];
    /* +0x7CE */ u8 field_0x7CE;    /* the online lobby kind NPC 1's return spot depends on */
    /* +0x7CF */ u8 pad_0x7CF[0x4];
    /* +0x7D3 */ u8 field_0x7D3;    /* with +0x86A, the arena return that spawns the player at the arena gate */
    /* +0x7D4 */ u8 pad_0x7D4[0x96];
    /* +0x86A */ u8 field_0x86A;
    /* +0x86B */ u8 pad_0x86B[0x55];
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
    /* +0x00 */ VEC3 pos_0x00;       /* the point the NPC walks to (y <= -10000: none); a turn entry keeps its angle in x */
    /* +0x0C */ u16 motion_0x0C;     /* 0xFFFF ends the table */
    /* +0x0E */ u8 pad_0x0E[2];
    /* +0x10 */ union {
        s32 count_0x10;                       /* the entry's frame count; -1 holds, -2 runs until the motion ends */
        struct LbNpcMotionEntry* branch_0x10; /* a branch entry's next table */
    };
} LbNpcMotionEntry;


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
    /* +0x028 */ union {
        struct {
            /* +0x028 */ s32 field_0x028;
            /* +0x02C */ s32 field_0x02C;   /* the heading */
            /* +0x030 */ s32 field_0x030;
        }; /* size: 0xC */
        _CP_VECTOR rot_0x028;   /* the same three words as the rotation triple (`lobby/lb_quest_board.cpp`) */
    };
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
    /* +0x214 */ struct _PLW* field_0x214;   /* the player the NPC talks to (`lb_npc_talk_partner_set`) */
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
    /* +0x230 */ u16 field_0x230;   /* the ground attribute bits under the NPC */
    /* +0x232 */ u8 pad_0x232[2];
    /* +0x234 */ union {   /* size: 0x20 */
        /* +0x234 */ u8 field_0x234[0x20];   /* the NPC's event scratch: byte 0 the event state, byte 1 its argument */
        struct {   /* size: 0x10 */
            /* +0x234 */ VEC3 event_pos_0x234;     /* the scripted spin's centre */
            /* +0x240 */ u16 event_angle_0x240;    /* its angle */
            /* +0x242 */ u16 event_spin_0x242;     /* the spin added to the heading */
        };
    };
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
extern LbNpcSceneWork lb_npc_scene_work;

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
void lb_npc_act_set(_PLW* self, u32 act, u16 sub, u16 flags);
/* 0x801FF5FC - stores the NPC's talk mode byte (+0x255); a null NPC is skipped (GUESS name). */
void lb_npc_talk_mode_set(struct _LB_NPC* self, u8 mode);
/* 0x8020EB00 - stores the player's lobby act latch byte (+0xB00) (GUESS name). */
void lb_act_latch_set(struct _PLW* self, u8 value);
/* 0x80208A00 - latches the act step when the master act runs (GUESS name). */
void lb_player_act_latch(struct _PLW* self);
/* 0x8020E778 - the heading from the player towards `target` (GUESS name). */
u32 lb_player_angle_to(struct _PLW* self, VEC3* target);
/* 0x801FE32C - true when the NPC's placement record has model kind `kind`. */
u32 lb_npc_id_ck(struct _LB_NPC* self, u32 kind);

#ifdef __cplusplus
}

/* 0x801FE1C4 - the NPC's current motion number (`lb_npc_Get_motion_no__FP7_LB_NPC`, C++ linkage - rule 9). */
u16 lb_npc_Get_motion_no(_LB_NPC* self);
#endif

/* Declarations `enemy/em020_prog.cpp` calls (its lobby/quest bands). */
#ifdef __cplusplus
struct PlayerPad;
extern "C" {
void lb_npc_walk_blend_set(_PLW* plw, s32 frames);
void lb_npc_act_req(_PLW* self, u32 kind, s32 act, s32 flags);
s32 lb_npc_act_busy_ck(u8 kind, u16 act);
void lb_npc_net_flag_on(void);
void lb_npc_net_flag_off(void);
void lb_pad_input_flush(struct PlayerPad* pad);
extern u8 lobby_hidden_flag;
/* .bss 0x806A7BA0 - the lobby's eighteen NPC records. */
extern _LB_NPC lb_npc[0x12];
}
#endif

#endif /* MHTRI_LOBBY_LB_NPC_H */
