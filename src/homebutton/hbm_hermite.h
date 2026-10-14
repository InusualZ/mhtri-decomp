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
