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
/* 0x80272E30 - the item/skill value setter `Pl/fn_802489D4.cpp` calls and the item/motion lookup
 * `Pl/pl_act.cpp` and `Pl/pl_act_step.cpp` call; the owner defines it `extern "C" s16`
 * (`Pl/pl_skill.cpp:1233`), so the declaration keeps that return. */
s16 fn_80272E30(struct _PLW* plw, u16 item, s16 value);
/* 0x802739F0 - the equipment-slot record resolver `Pl/fn_80273B14.cpp`'s act-kind switch calls;
 * the owner defines it `extern "C" s32` at `Pl/pl_skill.cpp:1682`. */
s32 fn_802739F0(struct _PLW* plw, s16 value, s32 mode, s8* out);
/* 0x802715A0 - the skill/slot classifier the equipment screen steps its slot selection through
 * (its own row height `fn_802A8F14` is resolved from this).  Owned by this unit
 * (`Pl/pl_skill.cpp:738`); added with `menu/menu_infomation.cpp` (docs/plan.md 6.5 rule 2). */
u8 Pl_Skill_slot_item_get(struct _PLW* plw, u32 slot);
/* 0x802731B4 - the item/skill timer lookup the same unit's consumers and `Pl/fn_8027D684.cpp`'s
 * `fn_8027D76C` call; the owner defines it `int` (`Pl/pl_skill.cpp:1265`), so the declaration keeps
 * that return (docs/plan.md 6.5 rule 2: this is the owner's header). */
int fn_802731B4(struct _PLW* plw, u16 item);
/* 0x80273044 - the item-id -> slot lookup `Pl/fn_8027D684.cpp`'s `fn_8027DE88` walks its id tables
 * with; the owner declares it `u16` (`Pl/pl_skill.cpp:279`). */
u16 fn_80273044(struct _PLW* plw, u16 item);
#ifdef __cplusplus
}
#endif

/* One 0x14C-byte player-object record (`lbl_80794B28->table`, one per player).  The first 11 bytes are
 * the record's 11 state slots (`fn_80269394`/`fn_802693C4`/`player_move_start_ck` walk them); the rest
 * is 7 groups of 11 resource words each (`parts_mdl_release_all`/`fn_802688B0` step `group[k][part]`).
 * size: 0x14C */
struct _PLOBJ {
    /* +0x000 */ u8 state[11];
    /* +0x00B */ union {   /* the pre-merge `unk00B` run (0x00B-0x017) kept whole, with this branch's
                            * split of the same bytes inside it (M4: same byte total) */
        /* +0x00B */ u8 unk00B[0xD];
        struct {
            /* +0x00B */ u8 part_flag_0x0B[11];  /* the per-part armed/consumed flag `fn_802693C4`
                                                  * clears and `fn_80269474` sets */
            /* +0x016 */ u8 pad_0x16[0x2];
        };
    };
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
    /* +0x200 */ union {   /* the pre-merge view is `loaded[7]` plus the four gate words; this branch
                            * named the whole 0x2C-byte run as one 11-word array (M4) */
        struct {
            /* +0x200 */ s32 loaded[7];   /* per-player "load requested" flags */
            /* +0x21C */ s32 gate21C;
            /* +0x220 */ s32 gate220;
            /* +0x224 */ s32 gate224;
            /* +0x228 */ s32 gate228;
        };
        /* +0x200 */ s32 loaded_0x200[11];   /* the per-part "load requested" words `fn_802699C8`
                                             * clears for parts 0..10 (the model parts then the
                                             * control parts) */
    };
    /* +0x22C */ s8 count22C;
    /* +0x22D */ u8 unk22D[0x3];
    /* +0x230 */ u8 slot_state[0x21C];    /* indexed by the player index (a `_PLW` +0x008) */
    /* +0x44C */ u32 players_0x44C[8];    /* `fn_80267A78` indexes it (one word per player) */
    /* +0x46C */ u8 unk46C[0xC];
    /* +0x478 */ s8 limit478;
    /* +0x479 */ s8 limit479;
    /* +0x47A */ s8 limit47A;
    /* +0x47B */ union {   /* one u8, two spellings: the pre-merge `unk47B` and this branch's name */
        /* +0x47B */ u8 unk47B;
        /* +0x47B */ u8 com_motion_type;   /* the common motion type `set_com_motion_type` stores and
                                            * `fn_802699AC` returns */
    };
};
extern struct _PLGLOBAL* lbl_80794B28;

extern u8 lbl_80794B2C[4];

#endif /* MHTRI_PL_PL_SKILL_H */
