/*
 * The `mh3_pad.cpp` unit's cross-unit declarations (docs/plan.md 6.5 rule 2).  A symbol a registered
 * unit owns is declared once, in that owner's header, and every consumer includes it; this is that
 * header for the game-root pad/mode file registered from proposal `800408A8`
 * (`.text` 0x800408A8-0x80047398).
 *
 * The three 3-float-record helpers at 0x80041E40/0x80041E8C/0x80043EA8 are this unit's, and this is
 * their single declaration: `include/ef.h` and `include/unsplit/ef.h` used to carry copies spelled
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
s32 fn_80047058(void);

#ifdef __cplusplus
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

#endif /* MHTRI_MH3_PAD_H */
