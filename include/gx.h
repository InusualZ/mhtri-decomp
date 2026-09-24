/* The write-gather-pipe window, the small pipe writers and the SDK colour record.
 *
 * The window at 0xCC008000 is the Wii's GX FIFO; one union member per store width is exactly what the
 * SDK's `GXWGFifo` is, so a writer body reads as `GXWGFifo.u16 = value;`. The union is defined once
 * because more than one unit of the `auto` library owns a copy of the writer family (AGENTS.md ->
 * Conventions, rule 1: a shared type lives in one header).
 *
 * The colour is the SDK's `_GXColor` (`typedef struct _GXColor { ... } GXColor;`).  The tag is
 * `_GXColor` because that is the type the mangled setters encode (`...8_GXColor`); the typedef `GXColor`
 * is the SDK's C spelling and what the draw units use.  Both are the same four bytes.  Consumers:
 * `ef/eft002.cpp`, `ef/eft007.cpp`, `ef/eft009.cpp` and `ef/fn_80114E34.cpp` carry private `_GXColor`
 * copies that move here in wave 2; `ef/ef_drawsmoothstripestrategy.cpp` and `main.cpp` use `GXColor`.
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

/* The SDK colour record.  `_GXColor` is the struct tag (and the name the mangled setters encode); the
 * `GXColor` typedef is the spelling the SDK's own prototypes use. size: 0x4 */
typedef struct _GXColor {
    u8 r; /* +0x0 */
    u8 g; /* +0x1 */
    u8 b; /* +0x2 */
    u8 a; /* +0x3 */
} GXColor;

#endif /* MHTRI_GX_H */
