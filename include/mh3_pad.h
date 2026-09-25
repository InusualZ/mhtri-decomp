/*
 * The `mh3_pad.cpp` unit's cross-unit declarations (docs/plan.md 6.5 rule 2).  A symbol a registered
 * unit owns is declared once, in that owner's header, and every consumer includes it; this is that
 * header for the game-root pad/mode file registered from proposal `800408A8`
 * (`.text` 0x800408A8-0x80047398).
 *
 * `fn_80043EA8` is the 3-float record writer the effect and g3d teardown code calls; `ef/effect.cpp`
 * and `g3d/fn_80063888.cpp` both reach it.
 */
#ifndef MHTRI_MH3_PAD_H
#define MHTRI_MH3_PAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80043EA8 - writes a 0xC-byte float vector record through `sub`. */
void fn_80043EA8(void *sub);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_H */
