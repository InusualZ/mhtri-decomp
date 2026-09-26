/* The player skill unit `Pl/pl_skill.cpp`.
 *
 * `Pl_Skill_ck` is a C++ free function (the map's name `Pl_Skill_ck__FP4_PLWUs` is its mangling),
 * moved here from `enemy/fn_8012BDF4.cpp` (docs/plan.md 6.5 rule 2).  A C++ consumer calls it through
 * the owner's real declaration, which the front-end mangles back to the map's name; a C consumer
 * cannot name the mangling, so it gets the map's spelling under `extern "C"` (rule 9's C limitation).
 */
#ifndef MHTRI_PL_PL_SKILL_H
#define MHTRI_PL_PL_SKILL_H

#include "types.h"
#include "nw4r/math.h"

struct _PLW;

#ifdef __cplusplus
u32 Pl_Skill_ck(struct _PLW* work, u16 skill); /* -> Pl_Skill_ck__FP4_PLWUs */
#else
u32 Pl_Skill_ck__FP4_PLWUs(struct _PLW* work, u16 skill);
#endif

/* 0x80272AB0 - the cat-skill predicate `Pl/fn_80262940.cpp` gates several states on; the owner defines
 * it at C++ scope (`Pl/pl_skill.cpp:1060`), so this is the callable spelling of the map name
 * `Pl_cat_skill_ck__FP4_PLWUs` (docs/plan.md 6.5 rule 9). */
#ifdef __cplusplus
u32 Pl_cat_skill_ck(struct _PLW* work, u16 skill);
#else
u32 Pl_cat_skill_ck__FP4_PLWUs(struct _PLW* work, u16 skill);
#endif

/* 0x8027350C - this unit's per-part motion dispatcher (`Pl/fn_80262940.cpp`'s player start-up calls
 * it); the owner defines it unmangled, so the declaration is `extern "C"`. */
#ifdef __cplusplus
extern "C" {
#endif
void fn_8027350C(struct _PLW* self, s32 part);
/* 0x80272E30 - the item/motion lookup `Pl/pl_act.cpp` and `Pl/fn_8024F200.cpp` call; the owner
 * defines it `extern "C" s16` (`Pl/pl_skill.cpp:1233`), so the declaration keeps that return. */
s16 fn_80272E30(struct _PLW* plw, u16 item, s16 value);
#ifdef __cplusplus
}
#endif

/* One 0x14C-byte player-object record (`lbl_80794B28->table`, one per player).  The first 11 bytes are
 * the record's 11 state slots (`fn_80269394`/`fn_802693C4`/`player_move_start_ck` walk them); the rest
 * is 7 groups of 11 resource words each (`parts_mdl_release_all`/`fn_802688B0` step `group[k][part]`).
 * size: 0x14C */
struct _PLOBJ {
    /* +0x000 */ u8 state[11];
    /* +0x00B */ u8 unk00B[0xD];
    /* +0x018 */ s32 group[7][11];
};

/* One 0xB20-byte player work record (`lbl_80794B28->objects`, `get_move_work_adrs(2)`).
 * size: 0xB20 */
struct _PLWORK {
    /* +0x000 */ u8 active;
    /* +0x001 */ u8 unk001a[0x13C - 0x1];
    /* +0x13C */ u8* model_0x13C;   /* the MHchar/actor model `fn_80223E54` moves */
    /* +0x140 */ u8 unk140[0x267 - 0x140];
    /* +0x267 */ u8 busy;   /* `fn_802673E8` counts the records whose byte is set */
    /* +0x268 */ u8 unk268[0xB20 - 0x268];
};

/* The 0x47C-byte player-global record `lbl_80794B28` points at (the allocation size `fn_80267548`
 * requests).  `.sbss:0x80794B28` has no registered owner (no `.sbss` range is in splits.txt), so - like
 * the band headers - the declaration sits with the type's owner.  `Pl/pl_skill.cpp` still carries a
 * private 0x14-byte prefix of this layout (it does not include this header); reconciling it is recorded
 * in the branch's outbox.  The middle fields are named from the uses in `Pl/fn_80262940.cpp`; the record
 * has no owner unit that could name them.
 * size: 0x47C */
struct _PLGLOBAL {
    /* +0x000 */ u8 state;              /* the player-control state machine, 0..5 */
    /* +0x001 */ u8 unk001[0x3];
    /* +0x004 */ s32 count;              /* number of player records (`player_move_start__Fl`) */
    /* +0x008 */ s32 ready;              /* how many players finished `fn_80267AA0`'s start-up */
    /* +0x00C */ struct _PLWORK* objects;  /* `count` records */
    /* +0x010 */ struct _PLOBJ* table;    /* one record per player, `fn_80269394`/`pl_skill` walk it */
    /* +0x014 */ u8 unk014[0x8];
    /* +0x01C */ s32 res_0x1C[0x79];      /* the per-player resource word table */
    /* +0x200 */ s32 loaded[7];           /* per-player "load requested" flags */
    /* +0x21C */ s32 gate21C;
    /* +0x220 */ s32 gate220;
    /* +0x224 */ s32 gate224;
    /* +0x228 */ s32 gate228;
    /* +0x22C */ s8 count22C;
    /* +0x22D */ u8 unk22D[0x3];
    /* +0x230 */ u8 slot_state[0x21C];    /* indexed by the player index (a `_PLW` +0x008) */
    /* +0x44C */ u32 players_0x44C[8];    /* `fn_80267A78` indexes it (one word per player) */
    /* +0x46C */ u8 unk46C[0xC];
    /* +0x478 */ s8 limit478;
    /* +0x479 */ s8 limit479;
    /* +0x47A */ s8 limit47A;
    /* +0x47B */ u8 unk47B;
};
extern struct _PLGLOBAL* lbl_80794B28;

extern u8 lbl_80794B2C[4];

#endif /* MHTRI_PL_PL_SKILL_H */
