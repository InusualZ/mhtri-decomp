/*
 * ef/eft046.h - `ef/eft046.cpp`'s views: `MenuFxWorkB`, the menu target marker's `+0x38` work record, and
 *   `MenuPlJoints`, the joint holder the marker follows.
 */
#ifndef MHTRI_EF_EFT046_H
#define MHTRI_EF_EFT046_H

#include "types.h"
#include "ef.h"
#include "pl.h"
#include "ef/eft045.h" /* MenuFxWork */

/* The second effect family's `+0x38` view: a single model and the s16 byte `menu_target_marker_set` seats beside
 * it (the same bytes `MenuFxWork` reads as `models[0]`/`models[1]`; the family spawns one model). */
struct MenuFxWorkB {
    /* +0x00 */ s32 count;
    /* +0x04 */ MHchar* model_0x04;
    /* +0x08 */ u16 bob_phase_0x08;   /* the bob angle, +0x200 a frame (`menu_target_marker_follow`) */
    /* +0x0A */ u8  pad_0x0A[0x06];
};
/* size: 0x10 (approximate) */

/* The joint holder `_PLW::physics_0x13C` points at, in the view the menu effect reads: the player's `MHchar` sits
 * at +0x04.  size: 0x144 (approximate: the model record's end) */
struct MenuPlJoints {
    /* +0x000 */ u8 pad_0x000[0x4];
    /* +0x004 */ MHchar joint_0x004;
};

#endif /* MHTRI_EF_EFT046_H */
