/* nw4r::ef draw-strategy records shared by the draw-strategy units.
 *
 * These three records were first reconstructed privately in `ef/ef_drawstripestrategy.cpp`; this unit
 * (`ef/ef_drawpointstrategy.cpp`) is the second consumer, so per AGENTS.md -> Conventions rule 1 the
 * declarations move to a header and both units include it.  The sibling's private copies are left for the
 * next pass that touches it (they are single-file and harmless until then).
 *
 * Every offset below is the address the target object reads it at; a field name is what the code stores
 * there or compares it against.
 */
#ifndef MHTRI_EF_DRAWSTRATEGY_H
#define MHTRI_EF_DRAWSTRATEGY_H

#include "ef.h"

/* The per-draw request block, the asserts' `pm`.  `emitter` at +0x24 is the effect's emitter record; the
 * two Panic format strings call it `pm->mResource`. size: 0x28 (lower bound, the record continues) */
typedef struct EfDrawArgs {
    /* +0x00 */ u8 pad_0x00[0x24];
    /* +0x24 */ void* emitter;
} EfDrawArgs; /* size: 0x28 */

/* The emitter-shape record the request's emitter resolves to, the asserts' `ed`.  +0x00 is the draw-flag
 * word the point walker tests (bit 0x800 selects the particle order), +0xAD/+0xAE the shape selectors the
 * sibling stripe/tube dispatches read. size: 0xAF (lower bound, the record continues) */
typedef struct EfEmitterShape {
    /* +0x00 */ u16 flags_0x00;
    /* +0x02 */ u8 pad_0x02[0xAB];
    /* +0xAD */ u8 shape_0xAD;
    /* +0xAE */ u8 shape_0xAE;
} EfEmitterShape; /* size: 0xAF */

/* The walker function pointer the two Get*DrawParticleFunc slots return. */
typedef void (*EfPointWalkerFn)(void);

/* The draw-strategy vtable prefix.  +0x10/+0x14 are the two walker getters this unit calls; slots
 * +0x00..+0x0C are the shared DrawStrategy virtuals, not named here. size: 0x18 (lower bound) */
typedef struct EfDrawStrategyVtbl {
    /* +0x00 */ void* slot_0x00;
    /* +0x04 */ void* slot_0x04;
    /* +0x08 */ void* slot_0x08;
    /* +0x0C */ void* slot_0x0C;
    /* +0x10 */ EfPointWalkerFn (*get_first)(void*, u32);
    /* +0x14 */ EfPointWalkerFn (*get_next)(void*, u32);
} EfDrawStrategyVtbl; /* size: 0x18 */

/* The draw-strategy object itself: the vtable pointer at +0x00 and the draw-order flag at +0xD0.  The
 * vtable slots the point walker invokes are +0x10 (GetFirstDrawParticleFunc) and +0x14
 * (GetNextDrawParticleFunc), both shared with the base DrawStrategy. size: 0xD1 (lower bound) */
typedef struct EfDrawStrategyObj {
    /* +0x00 */ EfDrawStrategyVtbl* vtable;
    /* +0x04 */ u8 pad_0x04[0xCC];
    /* +0xD0 */ u8 flag_0xD0;
} EfDrawStrategyObj; /* size: 0xD1 */

/* The particle record the walker yields: its world position is at +0xAC. size: 0xB8 (lower bound) */
typedef struct EfParticleRecord {
    /* +0x00 */ u8 pad_0x00[0xAC];
    /* +0xAC */ Vec world_pos;
} EfParticleRecord; /* size: 0xB8 */

#endif /* MHTRI_EF_DRAWSTRATEGY_H */
