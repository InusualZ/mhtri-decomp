/* The write-gather-pipe window, the small pipe writers and the SDK colour record.
 *
 * The window at 0xCC008000 is the Wii's GX FIFO; one union member per store width is exactly what the
 * SDK's `GXWGFifo` is, so a writer body reads as `GXWGFifo.u16 = value;`. The union is defined once
 * because more than one unit of the `auto` library owns a copy of the writer family (CLAUDE.md ->
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

/* The TEV/blend selectors the `MHchar` material setters encode (`setTevKColor__6MHcharFUl14_GXTevKColorIDP8_GXColor`,
 * `setMatAlphaBlendMode__6MHcharFUl12_GXBlendMode14_GXBlendFactor14_GXBlendFactor10_GXLogicOp`).  The
 * mangling names the type, so these are the SDK's own tag names; only the names matter for the map, the
 * values are the SDK's enumeration (the call sites use GX_BM_BLEND/GX_BL_SRCALPHA/GX_BL_INVSRCALPHA/GX_LO_CLEAR
 * and GX_KCOLOR3). */
typedef enum _GXBlendMode {
    GX_BM_NONE, GX_BM_BLEND, GX_BM_LOGIC, GX_BM_SUBTRACT, GX_MAX_BLENDMODE
} _GXBlendMode;

typedef enum _GXBlendFactor {
    GX_BL_ZERO, GX_BL_ONE, GX_BL_SRCCLR, GX_BL_INVSRCCLR,
    GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_BL_DSTALPHA, GX_BL_INVDSTALPHA
} _GXBlendFactor;

typedef enum _GXLogicOp {
    GX_LO_CLEAR, GX_LO_AND, GX_LO_REVAND, GX_LO_COPY, GX_LO_INVAND, GX_LO_NOOP, GX_LO_XOR, GX_LO_OR,
    GX_LO_NOR, GX_LO_EQUIV, GX_LO_INV, GX_LO_REVOR, GX_LO_INVCOPY, GX_LO_INVOR, GX_LO_NAND, GX_LO_SET
} _GXLogicOp;

typedef enum _GXTevKColorID {
    GX_KCOLOR0, GX_KCOLOR1, GX_KCOLOR2, GX_KCOLOR3
} _GXTevKColorID;

#endif /* MHTRI_GX_H */
