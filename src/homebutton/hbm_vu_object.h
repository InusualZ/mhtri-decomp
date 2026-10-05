/*
 * homebutton/hbm_vu_object.h - the types and views shared by the homebutton units that took over the functions of the
 * former registered unit `homebutton/keyboard_ui.cpp` (0x8055C894..0x805632BC).  Moved unchanged from that source; its declarations
 * of other symbols stayed inline in the units that use them.  The former unit's evidence and residual notes are in
 * docs/splits/phase4/homebutton-carried-notes.md.
 */
#ifndef HOMEBUTTON_HBM_VU_OBJECT_H
#define HOMEBUTTON_HBM_VU_OBJECT_H

#include "types.h"
#include "sys_mem.h"

/* ---------------------------------------------------------------------------------------------------
 * types.  `VuVTable` is a declaration-only view of a polymorphic sub-object's vtable: the table itself
 * belongs to another split, so only the slots this unit loads are named (rule 10 - declaring the
 * class's `virtual`s would make MWCC emit a table into this object).  `VuSub`/`VuObject` are views of
 * the band's objects; only the members this unit touches are named, every gap is filler, and the sizes
 * are approximations (the largest offset reached is +0x48, and the `subi r3,r3,0x1A04` this-adjusting
 * thunks of the band show the objects are base sub-objects of a much larger class).
 * ------------------------------------------------------------------------------------------------ */
/* size: 0x260 (approximation: a slice of a vtable, only the slots this unit loads are named) */
struct VuVTable {
    /* +0x000 */ u8      unused_0x000[0xC];
    /* +0x00C */ void  (*slot_00C)(void);
    /* +0x010 */ u8      unused_0x010[8];
    /* +0x018 */ void  (*slot_018)(void);
    /* +0x01C */ void  (*slot_01C)(void);
    /* +0x020 */ void  (*slot_020)(void);
    /* +0x024 */ void  (*slot_024)(void);
    /* +0x028 */ void  (*slot_028)(void);
    /* +0x02C */ u8      unused_0x02C[0x3C];
    /* +0x068 */ void  (*slot_068)(void);
    /* +0x06C */ u8      unused_0x06C[0x60];
    /* +0x0CC */ void  (*slot_0CC)(void);
    /* +0x0D0 */ void  (*slot_0D0)(void);
    /* +0x0D4 */ u8      unused_0x0D4[8];
    /* +0x0DC */ void  (*slot_0DC)(void);
    /* +0x0E0 */ void  (*slot_0E0)(void);
    /* +0x0E4 */ u8      unused_0x0E4[8];
    /* +0x0EC */ void  (*slot_0EC)(void);
    /* +0x0F0 */ u8      unused_0x0F0[0x0C];
    /* +0x0FC */ void  (*slot_0FC)(void);
    /* +0x100 */ void  (*slot_100)(void);
    /* +0x104 */ void  (*slot_104)(void);
    /* +0x108 */ void  (*slot_108)(void);
    /* +0x10C */ u8      unused_0x10C[8];
    /* +0x114 */ void  (*slot_114)(void);
    /* +0x118 */ u8      unused_0x118[8];
    /* +0x120 */ void  (*slot_120)(void);
    /* +0x124 */ u8      unused_0x124[4];
    /* +0x128 */ void  (*slot_128)(void);
    /* +0x12C */ u8      unused_0x12C[0x0C];
    /* +0x138 */ void  (*slot_138)(void);
    /* +0x13C */ void  (*slot_13C)(void);
    /* +0x140 */ u8      unused_0x140[0xFC];
    /* +0x23C */ void  (*slot_23C)(void);
    /* +0x240 */ void  (*slot_240)(void);
    /* +0x244 */ u8      unused_0x244[0x14];
    /* +0x258 */ void  (*slot_258)(void);
    /* +0x25C */ void  (*slot_25C)(void);
}; /* size: 0x260 */

/* size: 0x04 */
struct VuSub {
    /* +0x00 */ VuVTable* vt;
}; /* size: 0x04 */

/* size: 0x10 */
struct VuVec4 {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
    /* +0x0C */ f32 w;
}; /* size: 0x10 */

/* A freshly-allocated sub-object header: a vtable pointer followed by three zeroed words. */
/* size: 0x10 */
struct VuHeader {
    /* +0x00 */ void* vt;
    /* +0x04 */ u32   field_0x04;
    /* +0x08 */ u32   field_0x08;
    /* +0x0C */ u32   field_0x0C;
}; /* size: 0x10 */

/* The factory's owner object: only its allocator pointer at +4 is touched. */
/* size: 0x08 (approximation) */
struct VuOwner {
    /* +0x00 */ u8    unused_0x00[4];
    /* +0x04 */ void* allocator;
}; /* size: 0x08 */

/* size: 0x4C (approximation - see the comment above the type) */
struct VuObject {
    /* +0x00 */ VuVTable* vt;
    /* +0x04 */ void*   list_0x04;
    /* +0x08 */ void*   list_0x08;
    /* +0x0C */ void*   field_0x0C;
    /* +0x10 */ void*   field_0x10;
    /* +0x14 */ void*   field_0x14;
    /* +0x18 */ void*   field_0x18;
    /* +0x1C */ VuSub*  sub_0x1C;
    /* +0x20 */ void*   field_0x20;
    /* +0x24 */ void*   field_0x24;
    /* +0x28 */ void*   field_0x28;
    /* +0x2C */ void*   field_0x2C;
    /* +0x30 */ u8      unused_0x30[4];
    /* +0x34 */ void*   field_0x34;
    /* +0x38 */ void*   field_0x38;
    /* +0x3C */ u8      field_0x3C;
    /* +0x3D */ u8      unused_0x3D[3];
    /* +0x40 */ VuSub*  sub_0x40;
    /* +0x44 */ s32     field_0x44;
    /* +0x48 */ u8      field_0x48;
    /* +0x49 */ u8      unused_0x49[3];
}; /* size: 0x4C */

#endif
