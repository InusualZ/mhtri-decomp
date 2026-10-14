/*
 * The `mh3_pad.cpp` unit's cross-unit declarations (docs/plan.md 6.5 rule 2).  A symbol a registered
 * unit owns is declared once, in that owner's header, and every consumer includes it; this is that
 * header for the game-root pad/mode file registered from proposal `800408A8`
 * (`.text` 0x800408A8-0x80047398).
 *
 * The three 3-float-record helpers at 0x80041E40/0x80041E8C/0x80043EA8 are this unit's, and this is
 * their single declaration: `ef.h` and `unsplit/ef.h` used to carry copies spelled
 * `VEC3*`/`Vec*`, so a TU including both headers failed with MWCC `(10197) illegal function
 * overloading` (measured on `src/Pl/fn_8028F66C.cpp`).  Every referrer includes this header now.
 *
 * The parameter types are the record's real one, `nw4r::math::VEC3*` (see `mh3_pad/vec3.h` for the
 * evidence); they were `void*` until the type-fix pass - the erased form docs/plan.md 6.5 rule 11
 * bans.  `copyVec3`/`setVec3` return that pointer: the callers keep the first argument as the result
 * (`mr r4,r3` after the `bl`, e.g. `fn_80166330` in `enemy/fn_80165FC8.cpp`) and the bodies leave r3
 * untouched.
 *
 * GUESS (naming - confirm or rename when a body pass reaches the record type):
 *   * `VEC3_ctor` (0x80043EA8) - a 4-byte `blr` no-op called 1,873 times in the DOL, always with the
 *     address of a 3-float record local right after its declaration (`VEC3 v; VEC3_ctor(&v);`).  Read
 *     as the out-of-line default constructor of that record type; being empty it is a no-op either way.
 *   * `copyVec3` (0x80041E40) / `setVec3` (0x80041E8C) - named in the sibling scheme of this band
 *     (`setVector3`, `copyMat33`, `mulVecMat`, `vec_to_mh_vec3`).  `setVec3`'s body is byte-identical
 *     to the map's `setVector3__FPQ34nw4r4math4VEC3fff` (0x8004FFA8) - a second out-of-line copy of the
 *     same helper in another TU - so the name cannot be reused: the map cannot hold it twice.
 */
#ifndef MHTRI_MH3_PAD_H
#define MHTRI_MH3_PAD_H

#include "types.h"
#include "mh3_pad/control.h"   /* get_ControlType / fn_80044B14 (rule 2) */
#include "mh3_pad/vec3.h"      /* the three 3-float-record helpers below (rule 2) */
#include "mh3_pad/task.h"      /* `TaskSlot`, the task table entry (rule 1) */

#ifdef __cplusplus
extern "C" {
#endif

/* `VEC3_ctor` (0x80043EA8), `copyVec3` (0x80041E40) and `setVec3` (0x80041E8C) are declared in
 * `mh3_pad/vec3.h`, included above - `ef/ef_cube.cpp` needs them without `types.h`. */

/* Added when `camera/fn_802B5C58.cpp` registered (rule 2): the camera accessors all start by copying a
 * 4-byte camera handle through this unit's helper (`word_copy` does the word copy). */
/* 0x8004723C - copies the word `*src` into `*out` and returns `out`.  A raw 4-byte word copy, so
 * the parameters carry no pointee type: the two call sites pass different ones for the same slot
 * (`camera/fn_802B5C58.cpp` a `void**`, `stage/fn_802B2AA0.cpp` a `s32*`); the owner's body is
 * `word_copy` + `mr r3,r31`.  docs/plan.md 6.5 rule 11 exemption. */
void* word_copy_return_dst(void *out /* untyped: a raw word the callee copies byte-wise */,
                  const void *src /* untyped: a raw word the callee copies byte-wise */);
/* 0x80047058 - `Screen_w`'s +0x1A byte as a 0/1 flag; added with the `light/light.cpp` registration
 * (rule 2: this range owns the address), whose `fn_802BECD0` gates the second light work on it. */
s32 screen_split_mode_ck(void);

/* The KPAD tunables in this unit's .sdata (0x80790EA0..0x80790ED3) and .sbss (0x80794800): KPAD/kpad.c reads them
 * and KPADInitEx writes kp_obj_interval, kp_err_dist_max and kp_err_dist_min.  SDK names by value pattern; the
 * order of kp_ah_circle_pw and kp_ah_circle_radius is a GUESS. */
extern f32 kp_obj_interval;      /* 0x80790EA0: 0.2 */
extern f32 kp_acc_horizon_pw;    /* 0x80790EA4: 0.05 */
extern f32 kp_ah_circle_pw;      /* 0x80790EA8: 0.06 */
extern u16 kp_ah_circle_ct;      /* 0x80790EAC: 100 */
extern f32 kp_ah_circle_radius;  /* 0x80790EB0: 0.07 */
extern f32 kp_err_outside_frame; /* 0x80790EB4: 0.05 */
extern f32 kp_err_dist_max;      /* 0x80790EB8: 3.0 */
extern f32 kp_err_dist_speed;    /* 0x80790EBC: 0.04 */
extern f32 kp_err_first_inpr;    /* 0x80790EC0: 0.9 */
extern f32 kp_err_next_inpr;     /* 0x80790EC4: 0.9 */
extern f32 kp_err_acc_inpr;      /* 0x80790EC8: 0.9 */
extern f32 kp_err_up_inpr;       /* 0x80790ECC: 0.7 */
extern f32 kp_err_near_pos;      /* 0x80790ED0: 0.1 */
extern f32 kp_err_dist_min;      /* 0x80794800 (.sbss) */

#ifdef __cplusplus
/* 0x80040954 - releases every loaded sub-overlay and resets the loader state (GUESS name). */
void rso_overlay_release_all(void);
/* 0x80040AC4 / 0x80040ED0 - load the two-overlay (state 2) and the fifth-overlay (state 4) sets (GUESS names). */
void rso_overlay_mode2_load(void);
void rso_overlay_mode4_load(void);
/* 0x8004278C - clears pad `pad`'s button and stick words (GUESS name). */
void pad_input_clear(s32 pad);

}

/* 0x8004136C - writes into `out` the localised form of the resource file name `name` (the region's
 * language folder is applied to it); a C++ free function, the map's `cnvt_eur_fname__FPcPc` (rule 9).
 * Added with `quest/arenatask.cpp`, whose `arena_resource_load` builds its `.brres` path with it. */
void cnvt_eur_fname(char* out, char* name);

/* 0x80041978/0x80041A1C - enter the current game mode / the Vs mode; 0x80046EB4 - arms (or clears) the
 * soft-reset flag.  Added with `quest/arenatask.cpp` (rule 2: this unit owns the addresses). */
void GameModeExec(void);
void VsGameModeExec(void);
void setSoftresetFlag(bool flag);
#endif

#ifdef __cplusplus
/* 0x80041A34 - hands the game over to the arena-select task (C++ scope: `ArenaSelExec__Fv`). */
void ArenaSelExec(void);
#endif

#endif /* MHTRI_MH3_PAD_H */
