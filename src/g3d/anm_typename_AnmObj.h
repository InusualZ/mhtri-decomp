/* g3d/anm_typename_AnmObj.h - leaf header (docs/plan.md 6.5 rule 2) for `anm_typename_AnmObj`, owned by
 *   `g3d/fn_80063888.cpp`, for `g3d/g3d_anmchr.cpp`: that unit must not see the owner header's global placement
 *   `operator delete`, which would give its Construct functions a landing pad retail does not have. */
#ifndef MHTRI_G3D_ANM_TYPENAME_ANMOBJ_H
#define MHTRI_G3D_ANM_TYPENAME_ANMOBJ_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8056F568 - the "AnmObj" type-name record (`.rodata`: a length word, then the NUL-terminated name)
 * `g3d/g3d_anmchr.cpp`'s AnmObj type-info members read. */
extern u8 anm_typename_AnmObj[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_ANM_TYPENAME_ANMOBJ_H */
