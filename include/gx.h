/* The write-gather-pipe window and the small pipe writers the engine's draw code emits out of line.
 *
 * The window at 0xCC008000 is the Wii's GX FIFO; one union member per store width is exactly what the
 * SDK's `GXWGFifo` is, so a writer body reads as `GXWGFifo.u16 = value;`. The union is defined once here
 * because more than one unit of the `auto` library owns a copy of the writer family (AGENTS.md ->
 * Conventions, rule 1: a shared type lives in one header).
 */
#ifndef MHTRI_GX_H
#define MHTRI_GX_H

#include "types.h"

/* size: 0x8 (the f64 member sets the alignment). */
typedef union GXWGFifo_t {
    u8 u8;   /* +0x0 */
    u16 u16; /* +0x0 */
    u32 u32; /* +0x0 */
    s8 s8;   /* +0x0 */
    s16 s16; /* +0x0 */
    s32 s32; /* +0x0 */
    f32 f32; /* +0x0 */
    f64 f64; /* +0x0 */
} GXWGFifo_t;

#define GXWGFifo (*(volatile GXWGFifo_t*)0xCC008000)

/* size: 0x4.  The SDK spells the four colour components `r`/`g`/`b`/`a`; the shape unit
 * (`auto/800FCED4`) and `main.cpp` still carry private copies that move here the next time those units are
 * touched (AGENTS.md -> Conventions, rule 1). */
typedef struct GXColor {
    u8 r; /* +0x0 */
    u8 g; /* +0x1 */
    u8 b; /* +0x2 */
    u8 a; /* +0x3 */
} GXColor;

#endif /* MHTRI_GX_H */
