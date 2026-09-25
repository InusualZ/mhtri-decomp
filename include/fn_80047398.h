/*
 * Declarations for the symbols `src/fn_80047398.cpp` owns (docs/plan.md 6.5, rule 2).  `drawSpr2TF`
 * and `subTransSet` are C++ manglings (`drawSpr2TF__FUcP9fltSpr2TFUc`, `subTransSet__FUllPUl`), so
 * they are declared at global scope with the owner's parameter types - never inside `extern "C"`,
 * which would ask the linker for an unmangled name.
 */
#ifndef MHTRI_FN_80047398_H
#define MHTRI_FN_80047398_H

#include "types.h"

/* The 2D sprite record `drawSpr2TF` consumes. size: 0x18 */
typedef struct fltSpr2TF {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
    /* +0x04 */ s16 w;
    /* +0x06 */ s16 h;
    /* +0x08 */ s16 angle; /* degrees; drawSpr2TF scales it to radians */
    /* +0x0A */ s16 pad_0x0A;
    /* +0x0C */ u32 color;
    /* +0x10 */ s16 r;
    /* +0x12 */ s16 g;
    /* +0x14 */ s16 b;
    /* +0x16 */ s16 a;
} fltSpr2TF; /* size: 0x18 */

#ifdef __cplusplus
void drawSpr2TF(u8 id, fltSpr2TF* spr, u8 flag);
void subTransSet(u32 a, s32 b, u32* c);
#endif

#endif /* MHTRI_FN_80047398_H */
