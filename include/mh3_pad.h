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
#include "mh3_pad/control.h"   /* get_ControlType / fn_80044B14 (rule 2) */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80043EA8 - writes a 0xC-byte float vector record through `sub`. */
void fn_80043EA8(void *sub);

/* Added when `g3d/g3d_resanmchr.cpp` registered (rule 2): the `ResAnmChr` walkers copy 3-float
 * records and normalise them through this unit's helpers. */
void* fn_80041E40(void *dst, const void *src); /* 0x80041E40 - copies a 0xC-byte record;
    * its body keeps the destination in r31 and ends `mr r3,r31`, so it returns `dst` -- the
    * target's own bytes, not the call sites' `void` guess (normalised 2026-09-27). */
void fn_80041E8C(f32 *out, f32 x, f32 y, f32 z);     /* 0x80041E8C - builds a record from three floats */

/* Added when `camera/fn_802B5C58.cpp` registered (rule 2): the camera accessors all start by copying a
 * 4-byte camera handle through this unit's helper (`fn_8004726C` does the word copy). */
void fn_8004723C(void *out, void **src); /* 0x8004723C - copies the word `*src` into `out` */
/* 0x80047058 - `Screen_w`'s +0x1A byte as a 0/1 flag; added with the `light/light.cpp` registration
 * (rule 2: this range owns the address), whose `fn_802BECD0` gates the second light work on it. */
s32 fn_80047058(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_H */
