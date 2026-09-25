#ifndef MHTRI_UNSPLIT_PL_H
#define MHTRI_UNSPLIT_PL_H

#include "types.h"

struct _se_w;

/* Declarations for symbols with no registered owner whose address band names the `Pl` module
 * (docs/plan.md 6.5, rule 2).  Plain C-linkage names, so C-visible.
 */
#ifdef __cplusplus
extern "C" {
#endif

s8 fn_802748C8(void* a);

/* The two per-part SE frame arming helpers `Pl/fn_80241558.cpp` dispatches into (0x80229E10,
 * 0x80229EA8 - no registered owner yet). */
void fn_80229E10(struct _se_w* work, s32 a, s32 b);
void fn_80229EA8(struct _se_w* work, s32 a, s32 b, s32 c);

#ifdef __cplusplus
}

struct _PLW;

/* `Get_motion_no` is defined at 0x8026A308; the map spells it `Get_motion_no__FP4_PLW`, so the real
 * C++ declaration is the callable spelling and the front-end mangles it back (rule 9). */
u16 Get_motion_no(struct _PLW* plw);
#endif

#endif /* MHTRI_UNSPLIT_PL_H */
