/*
 * homebutton/gui_manager.h - the types and views shared by the homebutton units that took over the functions of the
 * former registered unit `homebutton/gui.cpp` (0x80569DAC..0x8056BBF0).  Moved unchanged from that source; its declarations
 * of other symbols stayed inline in the units that use them.  The former unit's evidence and residual notes are in
 * docs/splits/phase4/homebutton-carried-notes.md.
 */
#ifndef HOMEBUTTON_GUI_MANAGER_H
#define HOMEBUTTON_GUI_MANAGER_H

#include "types.h"
#include "sys_mem.h"

/* An nw4hbm::ut::List embedded in an object: a sentinel node (`next`, `prev`) followed by the
 * element count and the node offset.  size: 0xC */
struct UList {
    /* +0x00 */ void* mNext;
    /* +0x04 */ void* mPrev;
    /* +0x08 */ u16   mSize;
    /* +0x0A */ u16   mOffset;
}; /* size: 0xC */

/* The 16-byte list node this unit allocates: a key (the pane / the component's virtual result) and
 * the component.  size: 0x10 */
struct Node {
    /* +0x00 */ void* mKey;
    /* +0x04 */ void* mValue;
    /* +0x08 */ u8    pad_0x08[8];
}; /* size: 0x10 */

/* A 4-byte function-pointer slot.  The vtable layout is per class (lbl_80658290 has 27 slots,
 * lbl_80658358 has 18); every member is declared `void (*)(void)` so a call site casts to the exact
 * shape it needs without inventing a fake type. */
struct VTable {
    /* +0x00 */ void (*slot_00)(void); /* +0x04 */ void (*slot_04)(void);
    /* +0x08 */ void (*slot_08)(void); /* +0x0C */ void (*slot_0C)(void);
    /* +0x10 */ void (*slot_10)(void); /* +0x14 */ void (*slot_14)(void);
    /* +0x18 */ void (*slot_18)(void); /* +0x1C */ void (*slot_1C)(void);
    /* +0x20 */ void (*slot_20)(void); /* +0x24 */ void (*slot_24)(void);
    /* +0x28 */ void (*slot_28)(void); /* +0x2C */ void (*slot_2C)(void);
    /* +0x30 */ void (*slot_30)(void); /* +0x34 */ void (*slot_34)(void);
    /* +0x38 */ void (*slot_38)(void); /* +0x3C */ void (*slot_3C)(void);
    /* +0x40 */ void (*slot_40)(void); /* +0x44 */ void (*slot_44)(void);
    /* +0x48 */ void (*slot_48)(void); /* +0x4C */ void (*slot_4C)(void);
    /* +0x50 */ void (*slot_50)(void); /* +0x54 */ void (*slot_54)(void);
    /* +0x58 */ void (*slot_58)(void); /* +0x5C */ void (*slot_5C)(void);
    /* +0x60 */ void (*slot_60)(void); /* +0x64 */ void (*slot_64)(void);
    /* +0x68 */ void (*slot_68)(void);
}; /* size: 0x6C */

/* homebutton::gui::Component - the 0xA0-byte element/trigger record.  +0x05 is one colour byte per
 * index, +0x0D one flag byte, +0x18 a 12-byte float position, +0x80 a u16 counter; +0x94/+0x98/+0x9C
 * are the owner manager, an event target and the pane.  size: 0xA0 */
struct StageRec {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
}; /* size: 0xC */

struct Component {
    /* +0x00 */ VTable* vtable;
    /* +0x04 */ void*   field_0x04;
    /* +0x08 */ u8      pad_0x08[0x0D - 0x08];
    /* +0x0D */ u8      flags_0x0D[0x18 - 0x0D];
    /* +0x18 */ StageRec recs_0x18[4];
    /* +0x48 */ u8      pad_0x48[0x78 - 0x48];
    /* +0x78 */ u32     field_0x78;      /* trigger mask */
    /* +0x7C */ u32     field_0x7C;      /* component id */
    /* +0x80 */ u16     counts_0x80[4];
    /* +0x88 */ u8      pad_0x88[0x90 - 0x88];
    /* +0x90 */ u8      field_0x90;
    /* +0x91 */ u8      pad_0x91[0x94 - 0x91];
    /* +0x94 */ struct Component* field_0x94;  /* owner manager */
    /* +0x98 */ struct Component* field_0x98;
    /* +0x9C */ void*   field_0x9C;      /* PaneComponent's pane */
}; /* size: 0xA0 */

/* The per-stage colour/counter block fn_8056A82C..fn_8056A870 view (colour byte at +5, u16 counter
 * at +0x80).  size: 0x88 (approximate). */
struct ColorBlock {
    /* +0x00 */ u8  pad_0x00[0x05];
    /* +0x05 */ u8  colors_0x05[0x80 - 0x05];
    /* +0x80 */ u16 counts_0x80[4];
}; /* size: 0x88 */

/* homebutton::gui::Manager - owns the component list at +0xC and the allocator at +8.
 * size: 0x20 (approximate - evidenced to +0x1C) */
struct Manager {
    /* +0x00 */ VTable* vtable;
    /* +0x04 */ u32     field_0x04;
    /* +0x08 */ void*   allocator;      /* MEMAllocator*; null -> operator new/delete */
    /* +0x0C */ UList   list_0x0C;
    /* +0x18 */ void*   field_0x18;
    /* +0x1C */ u32     field_0x1C;
}; /* size: 0x20 */

/* homebutton::gui::PaneManager - Manager plus the pane-keyed list at +0x24 and its id counter at +0x20.
 * size: 0x30 (approximate) */
struct PaneManager : Manager {
    /* +0x20 */ u32   field_0x20;
    /* +0x24 */ UList list_0x24;
}; /* size: 0x30 */

/* The linked node fn_8056BB7C walks: a `next` at +0xC and a visibility bit at +0xBB. */
struct VisNode {
    /* +0x00 */ u8       pad_0x00[0x0C];
    /* +0x0C */ VisNode* next_0x0C;
    /* +0x10 */ u8       pad_0x10[0xBB - 0x10];
    /* +0xBB */ u8       flags_0xBB;
}; /* size: 0xBC */

/* The intrusive node fn_8056BBD0 links (`prev` at +4, `next` at +8). */
struct CNode {
    /* +0x00 */ u32    field_0x00;
    /* +0x04 */ CNode* prev_0x04;
    /* +0x08 */ CNode* next_0x08;
}; /* size: 0xC */

/* The nw4hbm::lyt::Pane view fn_8056B6D0 reads: only its inline name at +0xBC. */
struct Pane {
    /* +0x00 */ u8   pad_0x00[0xBC];
    /* +0xBC */ char name_0xBC[0x20];
}; /* size: 0xDC */

/* The per-update event record fn_8056A638 reads: the stage index at +0, the point at +4/+8 and the
 * event mask at +0x10.  size: 0x14 */
struct StageEvent {
    /* +0x00 */ u32 idx;
    /* +0x04 */ f32 x;
    /* +0x08 */ f32 y;
    /* +0x0C */ u32 field_0x0C;
    /* +0x10 */ u32 field_0x10;
}; /* size: 0x14 */

#endif
