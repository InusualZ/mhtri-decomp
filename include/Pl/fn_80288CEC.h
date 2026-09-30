/*
 * `Pl/fn_80288CEC.cpp`'s shared record (docs/plan.md 6.5 rule 1: a type more than one unit uses is
 * defined once and included where needed).  `Pl/fn_8028F66C.cpp`'s point-vs-box distance and the
 * hit tests beside it take the same record, so the definition moved here from that unit's source;
 * the box builders themselves (`fn_8028F44C`/`fn_8028F4B4`) stay in the owner.
 */
#ifndef MHTRI_PL_FN_80288CEC_H
#define MHTRI_PL_FN_80288CEC_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus

/* The 0x24-byte record the box builders fill: three 0xC-byte vectors.  `copyVec3` copies one
 * 0xC-byte record and `fn_80050CA0` writes `vec_0x0C - vec_0x00` into the third, so +0x00/+0x0C are
 * the two endpoints and +0x18 their difference.  size: 0x24 */
struct PlBox {
    /* +0x00 */ nw4r::math::VEC3 vec_0x00;
    /* +0x0C */ nw4r::math::VEC3 vec_0x0C;
    /* +0x18 */ nw4r::math::VEC3 vec_0x18;
};


/* ------------------------------------------------------------------------------------------------ *
 * The player-mode root work (`get_move_work_adrs(0)`).
 *
 * Only the fields these functions read are named (rule 5); untouched bytes keep their offset as an
 * `unused_0xNN` run (rules 4/5).  `recs_0x154` is the four 0x800-byte area records `fn_8028EF7C` /
 * `fn_8028F0B4` walk when the key is >= 0x20, each an array of 0x10-byte `{key, ...}` entries whose
 * count lives in `rec_counts_0x2154`; `sparse_0x2274` is the six-entry table they walk for a
 * key < 0x20.
 * size: 0x2300 (lower bound - the highest field read is 0x22DB)
 * ------------------------------------------------------------------------------------------------ */
/* size: 0x10 */
struct PlRootEntry {
    /* +0x0 */ u32 key_0x00; /* matched against the caller's key */
    /* +0x4 */ u8 unused_0x04[0x10 - 0x4];
};

/* size: 0x800 */
struct PlRootArea {
    /* +0x000 */ PlRootEntry entries_0x00[0x800 / 0x10];
};

/* size: 0x2300 (lower bound) */
struct _PL_ROOT {
    /* +0x0000 */ u8 mode_0x00;         /* `fn_8028BD54` tests `== 2` */
    /* +0x0001 */ u8 seq_0x01;
    /* +0x0002 */ u8 kind_0x02;
    /* +0x0003 */ u8 unused_0x0003[0x0004 - 0x0003];
    /* +0x0004 */ s16 timer_0x04;
    /* +0x0006 */ u8 unused_0x0006[0x00DC - 0x0006];
    /* +0x00DC */ u8* mode_work_0xDC;    /* `fn_8028F368` sets its +0x6A3E byte */
    /* +0x00E0 */ u8 unused_0x00E0[0x00E9 - 0x00E0];
    /* +0x00E9 */ u8 area_no_0xE9;       /* `fn_8028F24C` hands it to `stage_map_kind_get` */
    /* +0x00EA */ u8 unused_0x00EA;
    /* +0x00EB */ u8 area_no_0xEB;       /* the area `quest_marker_draw_at` places the master's marker in */
    /* +0x00EC */ u8 unused_0x00EC;
    /* +0x00ED */ u8 area_no_0xED;       /* the two `fn_8028C570` scene selectors */
    /* +0x00EE */ u8 unused_0x00EE;
    /* +0x00EF */ u8 npc_present_0xEF;   /* the AI NPC's marker is drawn while this is set (`quest_npc_marker_draw`) */
    /* +0x00F0 */ u8 unused_0x00F0[0x00FB - 0x00F0];
    /* +0x00FB */ u8 seq_0xFB;           /* `fn_8028F2F8` stores 4 or 6 */
    /* +0x00FC */ u8 seq_0xFC;           /* `fn_8028F368` stores 4 */
    /* +0x00FD */ u8 unused_0x00FD[0x0114 - 0x00FD];
    /* +0x0114 */ s32 flag_0x114;       /* `fn_8028D574` tests `<= 0`; `fn_8028D5B8` stores */
    /* +0x0118 */ u8 unused_0x0118[0x0128 - 0x0118];
    /* +0x0128 */ VEC3 pos_0x128;        /* the master's world position (`hud/cockpit_quest.cpp`'s marker) */
    /* +0x0134 */ u16 rot_y_0x134;       /* its facing, 0x18000 minus it picks the marker's icon rotation */
    /* +0x0136 */ u8 bytes_0x136[0x0154 - 0x0136]; /* `Pl_area_flag_get` indexes it by a `u8` */
    /* +0x0154 */ PlRootArea recs_0x154[4];
    /* +0x2154 */ u32 rec_counts_0x2154[4];
    /* +0x2164 */ u8 unused_0x2164[0x2258 - 0x2164];
    /* +0x2258 */ PlRootEntry* extra_0x2258;
    /* +0x225C */ u8 unused_0x225C[0x2274 - 0x225C];
    /* +0x2274 */ PlRootEntry sparse_0x2274[6];
    /* +0x22D4 */ u8 unused_0x22D4[0x22D7 - 0x22D4];
    /* +0x22D7 */ s8 field_0x22D7;
    /* +0x22D8 */ u8 field_0x22D8;       /* the day/night stage byte `get_gm_daynight` returns */
    /* +0x22D9 */ u8 field_0x22D9;
    /* +0x22DA */ u8 field_0x22DA;
    /* +0x22DB */ u8 field_0x22DB;
};

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8028EF30 - the per-player flag byte at +0x136 of the player move work (0 when the work is missing). */
u8 Pl_area_flag_get(u8 index);

#ifdef __cplusplus
}
#endif


/* Added by `enemy/em_action.cpp` (rule 2: the declaration belongs with the owner TU, which
 * had not declared it yet). */
void fn_8028F558(void* a, void* b);

#endif /* MHTRI_PL_FN_80288CEC_H */
