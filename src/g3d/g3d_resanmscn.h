/*
 * The `g3d/g3d_resanmscn.cpp` unit's cross-unit declarations (docs/plan.md 6.5 rule 2).
 *
 * `g3d/g3d_resanmscn.cpp` (`.text` 0x800908FC-0x800916FC) owns the `ResAnmScn` channel getters and the
 * `ResAnmTexPat` accessor/bind cluster.  The right-hand `g3d/g3d_resanmtexsrt.cpp` unit
 * (0x800916FC-0x80093990) consumes the five `ResAnmTexPat` helpers below through its
 * `ResFile`/`ResDic` chain and used to declare them locally, because the range was registered without a header; registering `g3d_resanmscn.cpp` makes the
 * addresses owned, so the declarations move here (rule 2) and the consumer includes this header.
 *
 * All five keep C linkage (the map carries plain `fn_XXXXXXXX` stems).  The signatures are the owner's
 * own forward declarations in `g3d_resanmscn.cpp`, inside its `extern "C"` block; the owner does not
 * include this header, so no translation unit ever sees two declarations of one C-linkage symbol (the
 * "illegal function overloading" trap).
 */
#ifndef MHTRI_G3D_G3D_RESANMSCN_H
#define MHTRI_G3D_G3D_RESANMSCN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8009125C - the byte array that follows the resolved data head (the next unit's comparator walks
 * it as a `{s32 length; char name[];}` record). */
char* fn_8009125C(void* self);
/* 0x800912D4 - the word at +0x0 of the resolved data pointer. */
u32 fn_800912D4(void* self);
/* 0x800913A0 - `ResAnmTexPat::Bind(ResFile)`: resolve every texture/palette name against `file` and
 * store the handles; returns whether every entry resolved. */
u32 fn_800913A0(void* self, void* file);
/* 0x80091614 - `*(u32*)self != 0`: whether the handle holds a data pointer. */
u32 fn_80091614(void* self);
/* 0x80091628 - `ResAnmTexPat::Release()`: reset every texture/palette handle in the two arrays. */
void fn_80091628(void* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_RESANMSCN_H */
