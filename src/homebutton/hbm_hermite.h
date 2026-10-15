/*
 * homebutton/hbm_hermite.h - declarations of the symbols owned by `homebutton/hbm_hermite.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_HBM_HERMITE_H
#define HOMEBUTTON_HBM_HERMITE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

s32 hbmNameMatches(const char* a, const char* b);

/* Cubic Hermite interpolation of `t` between (t0, v0, slope0) and (t1, v1, slope1). */
f32 hbmHermite(f32 t, f32 t0, f32 v0, f32 slope0, f32 t1, f32 v1, f32 slope1);

/* Returns whether a line may break before `c` (always). */
s32 hbmCanBreakBefore(u16 c);

/* Returns the full-width form of `c`. */
u16 hbmToFullWidth(u16 c);

/* Returns the wide character of the code `c`. */
u16 hbmToWideChar(u16 c);

void hbmNameCopyWithChar(char* dst, u32 n, const char* src, u32 idx, char ch);

/* A pair of kana tables indexed alike: the plain forms and one modified form (voiced, semi-voiced or small).  The
 * toggles search either table and answer the same index of the other.  size: 0x8 */
typedef struct HbmKanaPair {
    /* +0x0 */ const u16* table[2];
} HbmKanaPair; /* size: 0x8 */

extern HbmKanaPair hbmKanaDakuten;

extern HbmKanaPair hbmKanaHandakuten;

extern HbmKanaPair hbmKanaSmall;

u16 hbmKanaToggleDakuten(u16 c);

/* Whether `c` is a voiced, semi-voiced or small kana. */
BOOL hbmKanaIsDakuten(u16 c);

BOOL hbmKanaIsHandakuten(u16 c);

BOOL hbmKanaIsSmall(u16 c);

struct VuVec4;

/* Stores the 4:3 and the 16:9 screen rectangle (left, top, right, bottom). */
void hbmGetScreenRect4x3(struct VuVec4* rect);

void hbmGetScreenRect16x9(struct VuVec4* rect);

u16 hbmKanaToggleHandakuten(u16 c);

u16 hbmKanaToggleSmall(u16 c);

u16 hbmKanaCycle(u16 c);

extern f32 lbl_8079D720;

extern f32 lbl_8079D724;

extern f32 lbl_8079D728;

extern f32 lbl_8079D72C;

extern f32 lbl_8079D730;

extern f32 lbl_8079D734;

#ifdef __cplusplus
}
#endif

#endif
