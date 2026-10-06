/* ef/fn_801173AC_types.h - the types and macros `ef/fn_801173AC.cpp` and `ef/eft026_fx.cpp` share. */
#ifndef MHTRI_EF_FN_801173AC_TYPES_H
#define MHTRI_EF_FN_801173AC_TYPES_H

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "pl.h"
#include "gx.h"
#include "unsplit/unknown.h"
#include "unsplit/enemy.h"
#include "unsplit/sound.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8012BDF4.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "ef/eft001.h"
#include "ef/eft004.h"
#include "fn_8004CAD8.h"
#include "g3d/g3d_calcworld.h"
#include "nw4r/g3d/scnmdl.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* The player body sub-object `_PLW.physics_0x13C` points at: its `MHchar` sits at +4 (the view
 * `ef/fn_80114E34.cpp` states for the same offset). size: 0x144 (lower bound). */
typedef struct _EFT25_PHYSICS {
    /* +0x00 */ u8 pad_0x00[4];
    /* +0x04 */ MHchar chr_0x04;
} _EFT25_PHYSICS;

/* The player record as the eft025/eft026 family reads it.  It is `pl.h`'s `_PLW` at the same offsets;
 * this view exists because the shared record still spells +0x0C and +0x58 `unkNN` and rule 7 forbids
 * those identifiers in a `src/` file. size: 0x140 (lower bound). */
typedef struct _EFT25_ACTOR {
    /* +0x000 */ u8 pad_0x000[0x0a];
    /* +0x00A */ u8 field_0x00A;         /* the mode the state-1 gate tests against 10 */
    /* +0x00B */ u8 pad_0x00B[0x0c - 0x0b];
    /* +0x00C */ u16 motion_0x00C;       /* the motion index the gate excludes above 2 */
    /* +0x00E */ u8 pad_0x00E[0x54 - 0x0e];
    /* +0x054 */ u32 angle_0x54;         /* the base angle eft026_set adds its first word to */
    /* +0x058 */ u32 angle_0x58;         /* the y rotation angle (0x4000 is a quarter turn) */
    /* +0x05C */ u8 pad_0x05C[0x13c - 0x5c];
    /* +0x13C */ _EFT25_PHYSICS* physics_0x13C;
} _EFT25_ACTOR;
#endif /* MHTRI_EF_FN_801173AC_TYPES_H */
