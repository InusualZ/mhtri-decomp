/*
 * The `gx/fn_8009AA78.c` write-gather-pipe command-writer cluster's cross-unit declarations
 * (docs/plan.md 6.5 rule 2).  A symbol a registered unit owns is declared once, in that owner's
 * header, and every consumer includes it.  The consumer is `gx/fn_8009ACE4.c`, the next unit in the
 * same GX band, whose `fn_8009B0D8` emits the fixed CP/XF setup through `fn_8009AC98`/`AC44`/`AB1C`.
 *
 * All of them carry the map's own `fn_XXXXXXXX` stem, so they keep C linkage.
 */
#ifndef MHTRI_GX_FN_8009AA78_H
#define MHTRI_GX_FN_8009AA78_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8009AB1C - writes a 32-bit command word to the write-gather pipe (caller: gx/fn_8009ACE4.c). */
void fn_8009AB1C(u32 value);
/* 0x8009AC44 - emits the 0x10 opcode with a decremented register and a 16-bit operand. */
void fn_8009AC44(u16 a, u8 b);
/* 0x8009AC98 - emits the 0x08 opcode with a byte register and a 32-bit operand. */
void fn_8009AC98(u8 a, u32 b);
/* 0x8009AB8C - writes texture coordinate `coord`'s scale, bias and wrap pair (caller: g3d/g3d_state.cpp). */
void GDSetTexCoordScale2(u32 coord, u16 sScale, u8 sBias, u8 sWrap, u16 tScale, u8 tBias, u8 tWrap);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_GX_FN_8009AA78_H */
