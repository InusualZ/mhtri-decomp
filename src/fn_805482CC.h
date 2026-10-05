/*
 * fn_805482CC.h - the types of the 0x805482CC-0x8054E894 game-UI band (rule 1: the panel record and its
 * two dispatch tables are this unit's, so they live beside its source; the units that share the band
 * include this header rather than copying the layout).
 *
 * Layout evidence: the panel's constructor `fn_80542D8C` (0x80542D8C, one band below) writes +0x000
 * (vtable `lbl_80657678`), +0x05C (`lbl_80651708`, the sub-object's table), +0x118, and then the run
 * +0x18F8..+0x1A00; its sub-object is built by `fn_80503314` at +0x10.  The functions of this unit
 * read +0x193C / +0x1940 / +0x1944 as pointers to heap objects sharing one dispatch table, +0x1964 as
 * an embedded scroll record (table `lbl_8064F338`) and +0x19F0 as a pointer to a record whose +0x04 is
 * a u16 cursor.
 */
#ifndef MHTRI_FN_805482CC_H
#define MHTRI_FN_805482CC_H

#include "types.h"

struct Panel805482CC;
struct Panel805482CC_Sub;
struct Panel805482CC_Item;
struct Panel805482CC_Scroll;

/* One helper object of the panel (the heap records at +0x193C / +0x1940 / +0x1944).  Their tables
 * differ per instance, so the slots are reached through the object's own first word. */
typedef struct Panel805482CC_ItemVtbl Panel805482CC_ItemVtbl;
struct Panel805482CC_ItemVtbl {
    /* +0x00 */ void* slot_0x00;
    /* +0x04 */ void* slot_0x04;
    /* +0x08 */ void (*fn_0x08)(Panel805482CC_Item* self, s32 arg);
    /* +0x0C */ void* slot_0x0C;
    /* +0x10 */ void* slot_0x10;
    /* +0x14 */ void (*fn_0x14)(Panel805482CC_Item* self);
    /* +0x18 */ void* slot_0x18;
    /* +0x1C */ void* slot_0x1C;
    /* +0x20 */ void* slot_0x20;
    /* +0x24 */ void* slot_0x24;
    /* +0x28 */ void* slot_0x28;
    /* +0x2C */ void* slot_0x2C;
    /* +0x30 */ void* slot_0x30;
    /* +0x34 */ void* slot_0x34;
    /* +0x38 */ void* slot_0x38;
    /* +0x3C */ void* slot_0x3C;
    /* +0x40 */ void* slot_0x40;
    /* +0x44 */ void* slot_0x44;
    /* +0x48 */ void* slot_0x48;
    /* +0x4C */ void (*fn_0x4C)(Panel805482CC_Item* self, u32 key);
    /* +0x50 */ void* slot_0x50;
    /* +0x54 */ s32 (*fn_0x54)(Panel805482CC_Item* self);
    /* +0x58 */ void* slot_0x58;
    /* +0x5C */ void (*fn_0x5C)(Panel805482CC_Item* self, u32 key);
    /* +0x60 */ void (*fn_0x60)(Panel805482CC_Item* self, s32 arg);
    /* +0x64 */ void* slot_0x64;
    /* +0x68 */ void (*fn_0x68)(Panel805482CC_Item* self);
    /* +0x6C */ void (*fn_0x6C)(Panel805482CC_Item* self, s32 arg);
    /* +0x70 */ void* slot_0x70;
    /* +0x74 */ void* slot_0x74;
    /* +0x78 */ void* slot_0x78;
    /* +0x7C */ void* slot_0x7C;
    /* +0x80 */ void* slot_0x80;
    /* +0x84 */ void* slot_0x84;
    /* +0x88 */ s32 (*fn_0x88)(Panel805482CC_Item* self);
    /* +0x8C */ void* slot_0x8C;
    /* +0x90 */ void* slot_0x90;
    /* +0x94 */ void* slot_0x94;
    /* +0x98 */ void* slot_0x98;
    /* +0x9C */ void* slot_0x9C;
    /* +0xA0 */ void* slot_0xA0;
    /* +0xA4 */ void* slot_0xA4;
    /* +0xA8 */ void* slot_0xA8;
    /* +0xAC */ void* slot_0xAC;
    /* +0xB0 */ void* slot_0xB0;
    /* +0xB4 */ void* slot_0xB4;
    /* +0xB8 */ void* slot_0xB8;
    /* +0xBC */ void* slot_0xBC;
    /* +0xC0 */ void* slot_0xC0;
    /* +0xC4 */ void* slot_0xC4;
    /* +0xC8 */ void* slot_0xC8;
    /* +0xCC */ s32 (*fn_0xCC)(Panel805482CC_Item* self);
    /* +0xD0 */ void* slot_0xD0;
    /* +0xD4 */ void (*fn_0xD4)(Panel805482CC_Item* self);
    /* +0xD8 */ s32 (*fn_0xD8)(Panel805482CC_Item* self);
    /* +0xDC */ void* slot_0xDC;
    /* +0xE0 */ void* slot_0xE0;
    /* +0xE4 */ s32 (*fn_0xE4)(Panel805482CC_Item* self);
    /* +0xE8 */ s32 (*fn_0xE8)(Panel805482CC_Item* self);
    /* +0xEC */ void (*fn_0xEC)(Panel805482CC_Item* self, s32 arg);
    /* +0xF0 */ void* slot_0xF0;
    /* +0xF4 */ void* slot_0xF4;
    /* +0xF8 */ u16 (*fn_0xF8)(Panel805482CC_Item* self);
    /* +0xFC */ s32 (*fn_0xFC)(Panel805482CC_Item* self);
    /* Slot 0x100 is called with no argument at one site and with one at another (the tables are
     * shared by objects of more than one class in this band), so it carries both views. */
    union {
        /* +0x100 */ u32 (*fn_0x100)(Panel805482CC_Item* self);
        /* +0x100 */ void (*fn_0x100_arg)(Panel805482CC_Item* self, s32 arg);
    };
    /* +0x104 */ void* slot_0x104;
    /* +0x108 */ void (*fn_0x108)(Panel805482CC_Item* self, u32 key);
    /* +0x10C */ void* slot_0x10C;
    /* +0x110 */ void (*fn_0x110)(Panel805482CC_Item* self);
    /* +0x114 */ void (*fn_0x114)(Panel805482CC_Item* self);
    /* +0x118 */ void* slot_0x118;
    /* +0x11C */ void (*fn_0x11C)(Panel805482CC_Item* self, s32 arg);
}; /* size: 0x120 - a lower bound (the tables continue past the slots this band dispatches) */

/* The panel's own table, reached through `self->vtbl`. */
typedef struct Panel805482CC_Vtbl Panel805482CC_Vtbl;
struct Panel805482CC_Vtbl {
    /* +0x000 */ void* slot_0x000;
    /* +0x004 */ void* slot_0x004;
    /* +0x008 */ void* slot_0x008;
    /* +0x00C */ void* slot_0x00C;
    /* +0x010 */ void* slot_0x010;
    /* +0x014 */ void* slot_0x014;
    /* +0x18 */ void (*fn_0x18)(Panel805482CC* self, s32 id, void* arg);
    /* +0x01C */ void* slot_0x01C;
    /* +0x020 */ void* slot_0x020;
    /* +0x024 */ void* slot_0x024;
    /* +0x028 */ void* slot_0x028;
    /* +0x02C */ void* slot_0x02C;
    /* +0x030 */ void* slot_0x030;
    /* +0x034 */ void* slot_0x034;
    /* +0x038 */ void* slot_0x038;
    /* +0x03C */ void* slot_0x03C;
    /* +0x040 */ void* slot_0x040;
    /* +0x044 */ void* slot_0x044;
    /* +0x048 */ void* slot_0x048;
    /* +0x04C */ void* slot_0x04C;
    /* +0x050 */ void* slot_0x050;
    /* +0x054 */ void* slot_0x054;
    /* +0x058 */ void* slot_0x058;
    /* +0x05C */ void* slot_0x05C;
    /* +0x060 */ void* slot_0x060;
    /* +0x064 */ void* slot_0x064;
    /* +0x068 */ void* slot_0x068;
    /* +0x06C */ void* slot_0x06C;
    /* +0x070 */ void* slot_0x070;
    /* +0x074 */ void* slot_0x074;
    /* +0x078 */ void* slot_0x078;
    /* +0x07C */ void* slot_0x07C;
    /* +0x080 */ void* slot_0x080;
    /* +0x084 */ void* slot_0x084;
    /* +0x088 */ void* slot_0x088;
    /* +0x08C */ void* slot_0x08C;
    /* +0x090 */ void* slot_0x090;
    /* +0x094 */ void* slot_0x094;
    /* +0x098 */ void* slot_0x098;
    /* +0x09C */ void* slot_0x09C;
    /* +0x0A0 */ void* slot_0x0A0;
    /* +0x0A4 */ void* slot_0x0A4;
    /* +0x0A8 */ void* slot_0x0A8;
    /* +0x0AC */ void* slot_0x0AC;
    /* +0x0B0 */ void* slot_0x0B0;
    /* +0x0B4 */ void* slot_0x0B4;
    /* +0x0B8 */ void* slot_0x0B8;
    /* +0x0BC */ void* slot_0x0BC;
    /* +0x0C0 */ void* slot_0x0C0;
    /* +0x0C4 */ void* slot_0x0C4;
    /* +0x0C8 */ void* slot_0x0C8;
    /* +0x0CC */ void* slot_0x0CC;
    /* +0x0D0 */ void* slot_0x0D0;
    /* +0x0D4 */ void* slot_0x0D4;
    /* +0xD8 */ void (*fn_0xD8)(Panel805482CC* self);
    /* +0x0DC */ void* slot_0x0DC;
    /* +0x0E0 */ void* slot_0x0E0;
    /* +0x0E4 */ void* slot_0x0E4;
    /* +0x0E8 */ void* slot_0x0E8;
    /* +0x0EC */ void* slot_0x0EC;
    /* +0x0F0 */ void* slot_0x0F0;
    /* +0x0F4 */ void* slot_0x0F4;
    /* +0x0F8 */ void* slot_0x0F8;
    /* +0x0FC */ void* slot_0x0FC;
    /* +0x100 */ void* slot_0x100;
    /* +0x104 */ void* slot_0x104;
    /* +0x108 */ void* slot_0x108;
    /* +0x10C */ void* slot_0x10C;
    /* +0x110 */ void* slot_0x110;
    /* +0x114 */ void* slot_0x114;
    /* +0x118 */ void* slot_0x118;
    /* +0x11C */ void* slot_0x11C;
    /* +0x120 */ void* slot_0x120;
    /* +0x124 */ void* slot_0x124;
    /* +0x128 */ void* slot_0x128;
    /* +0x12C */ void* slot_0x12C;
    /* +0x130 */ void* slot_0x130;
    /* +0x134 */ void* slot_0x134;
    /* +0x138 */ void* slot_0x138;
    /* +0x13C */ void* slot_0x13C;
    /* +0x140 */ void* slot_0x140;
    /* +0x144 */ void* slot_0x144;
    /* +0x148 */ void* slot_0x148;
    /* +0x14C */ void* slot_0x14C;
    /* +0x150 */ void* slot_0x150;
    /* +0x154 */ void* slot_0x154;
    /* +0x158 */ void* slot_0x158;
    /* +0x15C */ void* slot_0x15C;
    /* +0x160 */ void* slot_0x160;
    /* +0x164 */ void* slot_0x164;
    /* +0x168 */ void* slot_0x168;
    /* +0x16C */ void* slot_0x16C;
    /* +0x170 */ void* slot_0x170;
    /* +0x174 */ void* slot_0x174;
    /* +0x178 */ void* slot_0x178;
    /* +0x17C */ void (*fn_0x17C)(Panel805482CC* self, s32 id);
    /* +0x180 */ void* slot_0x180;
    /* +0x184 */ void* slot_0x184;
    /* +0x188 */ void* slot_0x188;
    /* +0x18C */ void* slot_0x18C;
    /* +0x190 */ void* slot_0x190;
}; /* size: 0x194 - a lower bound (retail's table is longer) */

/* The scroll record embedded at +0x1964 (table `lbl_8064F338`). */
typedef struct Panel805482CC_ScrollVtbl Panel805482CC_ScrollVtbl;
struct Panel805482CC_ScrollVtbl {
    /* +0x00 */ void* slot_0x00;
    /* +0x04 */ void* slot_0x04;
    /* +0x08 */ void (*fn_0x08)(Panel805482CC_Scroll* self, s32 a, s32 b, f32 c, f32 d, f32 e);
    /* +0x0C */ void* slot_0x0C;
    /* +0x10 */ void* slot_0x10;
    /* +0x14 */ s32 (*fn_0x14)(Panel805482CC_Scroll* self);
}; /* size: 0x18 */

typedef struct Panel805482CC_Scroll {
    /* +0x00 */ Panel805482CC_ScrollVtbl* vtbl;
    /* +0x04 */ u8 pad_0x04[0x0C];
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u8 field_0x14;
    /* +0x15 */ u8 field_0x15;
    /* +0x16 */ u8 pad_0x16[0x2];
    /* +0x18 */ u32 field_0x18;
    /* +0x1C */ u8 pad_0x1C[0x4];
    /* +0x20 */ f32 field_0x20;
} Panel805482CC_Scroll; /* size: 0x24 */

/* The sub-object at +0x10; its dispatch table pointer sits at its own +0x4C (i.e. +0x5C of the panel),
 * which is the offset `fn_80542D8C` stores `lbl_80651708` into.  Its tail is not touched by this
 * band's functions, so the size is a lower bound. */
typedef struct Panel805482CC_SubVtbl Panel805482CC_SubVtbl;
struct Panel805482CC_SubVtbl {
    /* +0x00 */ void* slot_0x00;
    /* +0x04 */ void* slot_0x04;
    /* +0x08 */ void* slot_0x08;
    /* +0x0C */ void* slot_0x0C;
    /* +0x10 */ void* slot_0x10;
    /* +0x14 */ void* slot_0x14;
    /* +0x18 */ void* slot_0x18;
    /* +0x1C */ void* slot_0x1C;
    /* +0x20 */ void* slot_0x20;
    /* +0x24 */ void* slot_0x24;
    /* +0x28 */ void (*fn_0x28)(Panel805482CC_Sub* self);
    /* +0x2C */ void* slot_0x2C;
    /* +0x30 */ void* slot_0x30;
    /* +0x34 */ void* slot_0x34;
    /* +0x38 */ void* slot_0x38;
    /* +0x3C */ void* slot_0x3C;
    /* +0x40 */ void* slot_0x40;
    /* +0x44 */ void* slot_0x44;
    /* +0x48 */ void* slot_0x48;
    /* +0x4C */ void* slot_0x4C;
    /* +0x50 */ void* slot_0x50;
    /* +0x54 */ void* slot_0x54;
    /* +0x58 */ void* slot_0x58;
    /* +0x5C */ void* slot_0x5C;
    /* +0x60 */ void* slot_0x60;
    /* +0x64 */ void* slot_0x64;
    /* +0x68 */ void* slot_0x68;
    /* +0x6C */ void* slot_0x6C;
    /* +0x70 */ void* slot_0x70;
    /* +0x74 */ void* slot_0x74;
    /* +0x78 */ void* slot_0x78;
    /* +0x7C */ void* slot_0x7C;
    /* +0x80 */ void* slot_0x80;
    /* +0x84 */ void* slot_0x84;
    /* +0x88 */ void* slot_0x88;
    /* +0x8C */ void* slot_0x8C;
    /* +0x90 */ void (*fn_0x90)(Panel805482CC_Sub* self);
    /* +0x94 */ void (*fn_0x94)(Panel805482CC_Sub* self);
}; /* size: 0x98 - a lower bound (the table continues past 0x94) */

typedef struct Panel805482CC_Sub {
    /* +0x000 */ u8 pad_0x000[0x4];
    /* +0x004 */ u16 field_0x04;
    /* +0x006 */ u8 pad_0x006[0x46];
    /* +0x04C */ Panel805482CC_SubVtbl* mpVtbl;
    /* +0x050 */ u8 pad_0x050[0xA0];
    /* +0x0F0 */ f32 field_0xF0;
    /* +0x0F4 */ u8 pad_0x0F4[0x14];
} Panel805482CC_Sub; /* size: 0x108 - a lower bound (the constructor writes through +0x105) */

/* One text-position change record, the argument of fn_805482CC. */
typedef struct Panel805482CC_Change {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ f32 field_0x04;
    /* +0x08 */ f32 field_0x08;
} Panel805482CC_Change; /* size: 0x0C */

/* One text-position record, the argument of fn_80548394 (fn_80548B08 stores the cursor at +0x04). */
typedef struct Panel805482CC_Set {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ f32 field_0x04;
    /* +0x08 */ f32 field_0x08;
    /* +0x0C */ f32 field_0x0C;
    /* +0x10 */ f32 field_0x10;
    /* +0x14 */ u8 field_0x14;
    /* +0x15 */ u8 field_0x15;
    /* +0x16 */ u8 pad_0x16[0x2];
    /* +0x18 */ Panel805482CC_Item* field_0x18;
    /* +0x1C */ u32 field_0x1C;
} Panel805482CC_Set; /* size: 0x20 */

/* The cell record the panel's +0x19F0 pointer holds; only its u16 cursor is touched here. */
typedef struct Panel805482CC_Cell {
    /* +0x00 */ u16 field_0x00;
    /* +0x02 */ u16 field_0x02;
    /* +0x04 */ u16 field_0x04;
    /* +0x06 */ u16 pad_0x06;
} Panel805482CC_Cell; /* size: 0x08 */

/* The embedded record at +0x118: `fn_80542D8C` writes three words there and `fn_8054F788` takes
 * its address.  Its tail is not touched by this band, so the size is a lower bound. */
typedef struct Panel805482CC_Sub118 {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u8 pad_0x0C[0x18];
} Panel805482CC_Sub118; /* size: 0x24 - a lower bound */

/* The embedded record at +0x19C4 (`fn_80526CA0` constructs it, `fn_80526D20` configures it, and
 * `fn_8054CDC4`/`fn_8054CDCC` dispatch through it). */
typedef struct Panel805482CC_Sub19C4 {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8 pad_0x04[0x18];
} Panel805482CC_Sub19C4; /* size: 0x1C - a lower bound */

/* The embedded record at +0x1A04 that the home-button band's helpers at 0x8055BEF0 / 0x8055C1D4 /
 * 0x8055C2CC take the address of. */
typedef struct Panel805482CC_Sub1A04 {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8 pad_0x04[0x10];
} Panel805482CC_Sub1A04; /* size: 0x14 - a lower bound (the panel's +0x1A18 pointer follows) */

/* The embedded record at +0x18B8 (`fn_8054A8DC` hands its address to 0x80553670). */
typedef struct Panel805482CC_Sub18B8 {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8 pad_0x04[0x3C];
} Panel805482CC_Sub18B8; /* size: 0x40 - a lower bound */

typedef struct Panel805482CC {
    /* +0x000 */ Panel805482CC_Vtbl* vtbl;
    /* +0x004 */ u32 field_0x004;
    /* +0x008 */ u16 field_0x008;
    /* +0x00A */ u8 pad_0x00A[0x6];
    /* +0x010 */ Panel805482CC_Sub sub;                      /* size: 0x108 */
    /* +0x118 */ Panel805482CC_Sub118 field_0x118;           /* size: 0x24 */
    /* +0x13C */ u8 pad_0x13C[0x177C];
    /* +0x18B8 */ Panel805482CC_Sub18B8 field_0x18B8;        /* size: 0x40 */
    /* +0x18F8 */ u8 pad_0x18F8[0x40];
    /* +0x193C */ Panel805482CC_Item* field_0x193C;
    /* +0x1940 */ Panel805482CC_Item* field_0x1940;
    /* +0x1944 */ Panel805482CC_Item* field_0x1944;
    /* +0x1948 */ u8 pad_0x1948[0x4];
    /* +0x194C */ s32 field_0x194C;
    /* +0x1950 */ u8 field_0x1950;
    /* +0x1951 */ u8 field_0x1951;
    /* +0x1952 */ u8 field_0x1952;
    /* +0x1953 */ u8 pad_0x1953[0x5];
    /* +0x1958 */ f32 field_0x1958;
    /* +0x195C */ f32 field_0x195C;
    /* +0x1960 */ f32 field_0x1960;
    /* +0x1964 */ Panel805482CC_Scroll scroll;                /* size: 0x24 */
    /* +0x1988 */ s32 field_0x1988;
    /* +0x198C */ u8 pad_0x198C[0x28];
    /* +0x19B4 */ f32 field_0x19B4;
    /* +0x19B8 */ u8 pad_0x19B8[0xC];
    /* +0x19C4 */ Panel805482CC_Sub19C4 field_0x19C4;         /* size: 0x1C */
    /* +0x19E0 */ u8 field_0x19E0;
    /* +0x19E1 */ u8 pad_0x19E1[0x3];
    /* +0x19E4 */ Panel805482CC_Cell* field_0x19E4;
    /* +0x19E8 */ u16 field_0x19E8;
    /* +0x19EA */ u8 pad_0x19EA[0x6];
    /* +0x19F0 */ Panel805482CC_Cell* field_0x19F0;
    /* +0x19F4 */ u8 pad_0x19F4[0x4];
    /* +0x19F8 */ u16 field_0x19F8;
    /* +0x19FA */ u8 field_0x19FA;
    /* +0x19FB */ u8 pad_0x19FB[0x9];
    /* +0x1A04 */ Panel805482CC_Sub1A04 field_0x1A04;         /* size: 0x14 */
    /* +0x1A18 */ Panel805482CC_Item* field_0x1A18;
    /* +0x1A1C */ u8 pad_0x1A1C[0x9A];
    /* +0x1AB6 */ u8 field_0x1AB6;
    /* +0x1AB7 */ u8 pad_0x1AB7[0x3D];
} Panel805482CC; /* size: 0x1AF4 - a lower bound (the band reads the object through +0x1AF0) */

#endif /* MHTRI_FN_805482CC_H */
