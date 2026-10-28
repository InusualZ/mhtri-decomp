/* rotMatrixX.h - leaf header (docs/plan.md 6.5 rule 2) for `rotMatrixX` and `rotMatrixY`, which `fn_8004CAD8.cpp`
 * owns, with the owner's signatures (`fn_8004CAD8.h` declares the same; this header is for consumers whose own
 * declarations clash with that header's).  C++ linkage at global scope: the map rows are the manglings
 * `rotMatrixX__FUlPQ34nw4r4math5MTX34` / `rotMatrixY__FUlPQ34nw4r4math5MTX34`. */
#ifndef MHTRI_ROTMATRIXX_H
#define MHTRI_ROTMATRIXX_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
/* 0x800500CC - writes the rotation about X by the 16-bit `angle` into `m`. */
void rotMatrixX(u32 angle, MTX34* m);
/* 0x8005027C - the same about Y. */
void rotMatrixY(u32 angle, MTX34* m);
#endif

#endif /* MHTRI_ROTMATRIXX_H */
