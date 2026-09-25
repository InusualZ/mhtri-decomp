/* The g3d ambient-light animation unit `g3d/g3d_resanmamblight.c`.
 *
 * Declarations moved here from the consumer units' `src/` files (docs/plan.md 6.5 rule 2:
 * an extern lives with the TU that owns the symbol).  `fn_8008F6E8` (`g3d/g3d_resanmfog.cpp`)
 * is the second consumer, so the shared channel evaluators are declared once here.
 *
 * The functions keep C linkage (the map carries plain `fn_XXXXXXXX` stems).
 *
 * Parameter order: the retail fog call site (`g3d/g3d_resanmfog.cpp`) schedules the float `frame`
 * argument before the integer flag (MWCC evaluates arguments left to right), so the evidenced
 * signature is `(self, f32 frame, s32 flag)`.  The owner's reconstructed definition in
 * `g3d_resanmamblight.c` spells it `(self, s32 flag, f32 frame)`; the two are ABI-identical (the
 * PPC EABI assigns integer and float arguments to separate register files), so the declaration is
 * interchangeable and only the scheduling differs.  The evidenced order is kept here.
 */
#ifndef MHTRI_G3D_G3D_RESANMAMBLIGHT_H
#define MHTRI_G3D_G3D_RESANMAMBLIGHT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8008A188 - `flag` set returns the inline word, else evaluates the referenced channel. */
s32 fn_8008A188(u32 *self, f32 frame, s32 flag);

/* 0x8008A1A8 - clamps `frame` into [0, count]. */
f32 fn_8008A1A8(u16 *count, f32 frame);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_RESANMAMBLIGHT_H */
