/*
 * homebutton/hbm_widget.h - the types and views shared by the homebutton units that took over the functions of the
 * former registered unit `homebutton/fn_80555374.cpp` (0x80555374..0x8055C894).  Moved unchanged from that source; its declarations
 * of other symbols stayed inline in the units that use them.  The former unit's evidence and residual notes are in
 * docs/splits/phase4/homebutton-carried-notes.md.
 */
#ifndef HOMEBUTTON_HBM_WIDGET_H
#define HOMEBUTTON_HBM_WIDGET_H

#include "types.h"

/* --------------------------------------------------------------------------------------------- */
/* Types                                                                                          */
/* --------------------------------------------------------------------------------------------- */

struct HbmWidget;

#define HBM_OFFSET_OF(type, field) ((u32)(&((type*)0)->field))

/* The allocator the band's buffers come from (`.0x08` slot + MEMFreeToAllocator).
 * size: 0x4 */
struct HbmAllocator {
    /* +0x00 */ void** vtable;
};

/* The widget's tail flag bytes (fn_80558CD8 reads +0xD0, fn_80558638 +0xD1).  size: 0x4 */
struct HbmTailFlags {
    /* +0x00 */ u8 flag_D0;
    /* +0x01 */ u8 flag_D1;
    /* +0x02 */ u8 pad_0x02[2];
};

/* The widget vtable group the band dispatches through - `lbl_80650A48` in .data, owned by another
 * unit.  size: 0xDC (the declared view; the real group is 0x104 B - three tables at +0x00,
 * +0x24 and +0x98 - and stops where the band stops reading it).  Slots the band never calls keep their offset as `unused_0xNN`. */
struct HbmWidgetVtable {
    /* +0x00 */ void* unused_0x00;
    /* +0x04 */ void* unused_0x04;
    /* +0x08 */ void (*release)(void* self);
    /* +0x0C */ void* unused_0x0C;
    /* +0x10 */ void (*update)(void* self);
    /* +0x14 */ void (*hide)(void* self);
    /* +0x18 */ void* unused_0x18;
    /* +0x1C */ void* unused_0x1C;
    /* +0x20 */ void* unused_0x20;
    /* +0x24 */ void* unused_0x24;
    /* +0x28 */ void* unused_0x28;
    /* +0x2C */ void* unused_0x2C;
    /* +0x30 */ s32 (*isDone)(void* self);
    /* +0x34 */ void* unused_0x34;
    /* +0x38 */ void* unused_0x38;
    /* +0x3C */ void (*setText)(void* self, const char* text, int on);
    /* +0x40 */ void* unused_0x40;
    /* +0x44 */ void* unused_0x44;
    /* +0x48 */ void* unused_0x48;
    /* +0x4C */ void* unused_0x4C;
    /* +0x50 */ void (*setAnm)(void* self, const char* name, int mode);
    /* +0x54 */ void* unused_0x54;
    /* +0x58 */ void* unused_0x58;
    /* +0x5C */ void (*setId)(void* self, int id);
    /* +0x60 */ void* unused_0x60;
    /* +0x64 */ void* unused_0x64;
    /* +0x68 */ void* unused_0x68;
    /* +0x6C */ void* unused_0x6C;
    /* +0x70 */ void* unused_0x70;
    /* +0x74 */ void* unused_0x74;
    /* +0x78 */ void* unused_0x78;
    /* +0x7C */ void* unused_0x7C;
    /* +0x80 */ void* unused_0x80;
    /* +0x84 */ void* unused_0x84;
    /* +0x88 */ void* unused_0x88;
    /* +0x8C */ void* unused_0x8C;
    /* +0x90 */ void* unused_0x90;
    /* +0x94 */ void* unused_0x94;
    /* +0x98 */ void* unused_0x98;
    /* +0x9C */ void* unused_0x9C;
    /* +0xA0 */ void* unused_0xA0;
    /* +0xA4 */ void* unused_0xA4;
    /* +0xA8 */ void* unused_0xA8;
    /* +0xAC */ void* unused_0xAC;
    /* +0xB0 */ void* unused_0xB0;
    /* +0xB4 */ void* unused_0xB4;
    /* +0xB8 */ void* unused_0xB8;
    /* +0xBC */ void* unused_0xBC;
    /* +0xC0 */ void* unused_0xC0;
    /* +0xC4 */ void* unused_0xC4;
    /* +0xC8 */ void* unused_0xC8;
    /* +0xCC */ void* unused_0xCC;
    /* +0xD0 */ void* unused_0xD0;
    /* +0xD4 */ void (*onTick)(void* self);
    /* +0xD8 */ void* unused_0xD8;
};

/* The widget class the band's state machines run on: one complete object whose base subobjects sit
 * at +0x14, +0x1C, +0x24, +0xC4 and +0xCC (each owns a vtable set by the constructor of the unit
 * that owns that base).  size: 0xD4 (approximate - the largest offset the band touches is +0xD0). */
struct HbmWidget {
    /* +0x00 */ HbmWidgetVtable* vtable;
    /* +0x04 */ void* items[1];         /* indexed as `items[i]` (fn_8055BE48) */
    /* +0x08 */ u32 pad_0x08;
    /* +0x0C */ u32 pad_0x0C;
    /* +0x10 */ HbmWidget* child;       /* the widget's active child */
    /* +0x14 */ void* vtable_14;        /* base #1 (adjustor thunk `subi 0x14`) */
    /* +0x18 */ HbmWidget* owner;       /* the widget that owns this one's base #1 */
    /* +0x1C */ void* vtable_1C;        /* base #2 (adjustor thunk `subi 0x1C`) */
    /* +0x20 */ u32 pad_0x20;
    /* +0x24 */ void* vtable_24;        /* base #3 (adjustor thunk `subi 0x24`) */
    /* +0x28 */ u32 pad_0x28;
    /* +0x2C */ union {
                    u32 state;          /* the state-machine selector */
                    HbmWidget* current; /* the child the accessors dispatch to */
                } mode;
    /* +0x30 */ void* resource;         /* released by the destructor */
    /* +0x34 */ u32 pad_0x34[6];
    /* +0x4C */ HbmWidget* node_4C;     /* fn_80555AD8's static initialiser target */
    /* +0x50 */ u32 arg_50;
    /* +0x54 */ u32 pad_0x54[0x0D];
    /* +0x88 */ void* list;             /* intrusive list head (fn_80501C60/fn_80501BF4) */
    /* +0x8C */ u32 pad_0x8C[4];
    /* +0x9C */ u8 flag_9C;
    /* +0x9D */ u8 pad_0x9D[3];
    /* +0xA0 */ u32 count_A0;
    /* +0xA4 */ u32 pad_A4[6];
    /* +0xBC */ void* ptr_BC;
    /* +0xC0 */ void* ptr_C0;
    /* +0xC4 */ void* vtable_C4;        /* base #4 (adjustor thunk `subi 0xC4`) */
    /* +0xC8 */ u8 flag_C8;
    /* +0xC9 */ u8 flag_C9;
    /* +0xCA */ u8 pad_0xCA[2];
    /* +0xCC */ HbmAllocator* allocator;/* freed through MEMFreeToAllocator */
    /* +0xD0 */ union {                  /* one word in fn_8055A43C, single bytes for the flags */
                    u32 word_D0;
                    HbmTailFlags bytes;
                } tail;
};

/* The value / shown-value / dirty cluster fn_80559C50 and its siblings work on.
 * size: 0x28 (approximate) */
struct HbmTextValue {
    /* +0x00 */ u8 pad_0x00[0x1C];
    /* +0x1C */ u32 value;
    /* +0x20 */ u32 shown;
    /* +0x24 */ u8 dirty;
};

/* The u16-counted array fn_805594C0 writes into.  size: 0x10 */
struct HbmU16Array {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ u16 capacity;
    /* +0x06 */ u16 length;
    /* +0x08 */ u16 offset;              /* a byte offset into `data` */
    /* +0x0A */ u16 pad_0x0A;
    /* +0x0C */ u16* data;
};

/* A second view of the widget where +0x14 holds the band's list node (fn_8055C85C).
 * size: 0x18 */
struct HbmWidgetNodeView {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ struct HbmListNode* node;
};

/* A node of the band's sorted list (fn_8055C85C).  size: 0xC */
struct HbmListNode {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ s32 id;
    /* +0x08 */ f32 value;
};

/* The (id, value) pair fn_8055C85C writes out.  size: 0x8 */
struct HbmIdValue {
    /* +0x00 */ s32 id;
    /* +0x04 */ f32 value;
};

/* The timer cluster fn_8055A40C loads (offsets +0xE8..+0x104).  size: 0x108 (approximate) */
struct HbmTimer {
    /* +0x00 */ u8 pad_0x00[0xE8];
    /* +0xE8 */ f32 start;
    /* +0xEC */ f32 end;
    /* +0xF0 */ f32 base;
    /* +0xF4 */ u32 id;
    /* +0xF8 */ u32 kind;
    /* +0xFC */ f32 a;
    /* +0x100 */ f32 b;
    /* +0x104 */ u8 active;
};

/* The animation-record cluster fn_80556FC4 initialises.  size: 0x1C (approximate) */
struct HbmAnimRecord {
    /* +0x00 */ u8 pad_0x00[0x15];
    /* +0x15 */ u8 flag_15;
    /* +0x16 */ u8 flag_16;
    /* +0x17 */ u8 flag_17;
    /* +0x18 */ const u8* table;   /* table[0] is the record count */
};

#endif
