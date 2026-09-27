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
 * The parameter types are deliberately type-erased (`void*`): two of the three bodies only store
 * floats and the third is empty, so there is no pointee evidence, while the ~1,400 call sites pass the
 * engine's `Vec*`, nw4r's `VEC3*`, `f32*` and `void*` for the same argument.  `copyVec3`/`setVec3`
 * returning `void*` IS evidenced: the callers keep the first argument as the result (`mr r4,r3` after
 * the `bl`, e.g. `fn_80166330` in `enemy/fn_80165FC8.cpp`) and the bodies leave r3 untouched.
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

#ifdef __cplusplus
extern "C" {
#endif

/* `VEC3_ctor` (0x80043EA8), `copyVec3` (0x80041E40) and `setVec3` (0x80041E8C) are declared in
 * `mh3_pad/vec3.h`, included above - `ef/ef_cube.cpp` needs them without `types.h`. */

/* Added when `camera/fn_802B5C58.cpp` registered (rule 2): the camera accessors all start by copying a
 * 4-byte camera handle through this unit's helper (`fn_8004726C` does the word copy). */
/* 0x8004723C - copies the word `*src` into `*out` and returns `out`.  The types are erased because
 * the two call sites pass different ones for the same slot (`camera/fn_802B5C58.cpp` a `void**`,
 * `stage/fn_802B2AA0.cpp` a `s32*`); the owner's body is `fn_8004726C` + `mr r3,r31`. */
void* fn_8004723C(void *out, const void *src);
/* 0x80047058 - `Screen_w`'s +0x1A byte as a 0/1 flag; added with the `light/light.cpp` registration
 * (rule 2: this range owns the address), whose `fn_802BECD0` gates the second light work on it. */
s32 fn_80047058(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_H */
