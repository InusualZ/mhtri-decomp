/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `font/flfnt.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_FONT_FONT_STR_CHAR_COUNT_H
#define MHTRI_FONT_FONT_STR_CHAR_COUNT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8005C9B8 - the printable characters of `text` (colour codes and newlines skipped).  GUESS name. */
s32 font_str_char_count(const char* text);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FONT_FONT_STR_CHAR_COUNT_H */
