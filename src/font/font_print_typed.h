/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `font/flfnt.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_FONT_FONT_PRINT_TYPED_H
#define MHTRI_FONT_FONT_PRINT_TYPED_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8005CA64 - prints the first `count` characters of `text` at (x, y); non-zero once all are out.  GUESS name. */
s8 font_print_typed(s16 x, s16 y, s16 color, s32 count, const char* text);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FONT_FONT_PRINT_TYPED_H */
