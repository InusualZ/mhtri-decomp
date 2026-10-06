/* Declarations of `Pl/pl_act.cpp`'s skill and item section (0x80270018-0x80273B14) and the player records.
 * `Pl_Skill_ck` is a C++ free function (the map's `Pl_Skill_ck__FP4_PLWUs` is its mangling): a C++ consumer calls
 * the real declaration, which the front-end mangles back; a C consumer gets the map's spelling (rule 9's C limitation).
 */
#ifndef MHTRI_PL_PL_SKILL_H
#define MHTRI_PL_PL_SKILL_H

#include "types.h"
#include "nw4r/math.h"
#include "Pl/pl_item_add.h"   /* pl_item_add (leaf header) */

struct _PLW;

#ifdef __cplusplus
u32 Pl_Skill_ck(struct _PLW* work, u16 skill); /* -> Pl_Skill_ck__FP4_PLWUs */
#else
u32 Pl_Skill_ck__FP4_PLWUs(struct _PLW* work, u16 skill);
#endif

/* 0x80272AB0 - the cat-skill predicate the main/control cluster gates several states on; defined at C++ scope, so
 * this is the callable spelling of the map name `Pl_cat_skill_ck__FP4_PLWUs` (rule 9). */
#ifdef __cplusplus
u32 Pl_cat_skill_ck(struct _PLW* work, u16 skill);
#else
u32 Pl_cat_skill_ck__FP4_PLWUs(struct _PLW* work, u16 skill);
#endif

/* 0x8027350C - the per-part motion dispatcher the player start-up calls; defined unmangled, so `extern "C"`. */
#ifdef __cplusplus
extern "C" {
#endif
void fn_8027350C(struct _PLW* self, s32 part);
/* 0x80272E30 - `pl_item_add`, the item/skill value setter `Pl/pl_act_step.cpp` calls; declared in the leaf
 * header `Pl/pl_item_add.h`, included below. */
/* 0x802739F0 - the equipment-slot record resolver the act-entry section's act-kind switch calls. */
s32 fn_802739F0(struct _PLW* plw, s16 value, s32 mode, s8* out);
/* 0x802715A0 - the skill/slot classifier the equipment screen steps its slot selection through
 * (its own row height `menu_page_count` is resolved from this). */
u8 Pl_Skill_slot_item_get(struct _PLW* plw, u32 slot);
/* 0x802731B4 - the item/skill timer lookup the equipment section's `fn_8027D76C` and other consumers call;
 * `int`, like the definition. */
int Pl_item_timer_get(struct _PLW* plw, u16 item);
/* 0x80273044 - the item-id -> slot lookup `fn_8027DE88` walks its id tables with; `u16`, like the definition. */
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
    /* +0x00B */ union {   /* the `unk00B` run (0x00B-0x017) kept whole, with a
                            * split of the same bytes inside it (same byte total) */
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

/* The 0x47C-byte player-global record `lbl_80794B28` (`Pl/player_control.cpp`'s `.sbss`) points at - the allocation
 * size `fn_80267548` requests.  The middle fields are named from the player-control code's uses.
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
    /* +0x200 */ union {   /* two views: `loaded[7]` plus the four gate words, and
                            * the whole 0x2C-byte run as one 11-word array */
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
    /* +0x47B */ union {   /* one u8, two spellings: `unk47B` and a name */
        /* +0x47B */ u8 unk47B;
        /* +0x47B */ u8 com_motion_type;   /* the common motion type `set_com_motion_type` stores and
                                            * `fn_802699AC` returns */
    };
};
extern struct _PLGLOBAL* lbl_80794B28;

extern u8 lbl_80794B2C[4];

#endif /* MHTRI_PL_PL_SKILL_H */
