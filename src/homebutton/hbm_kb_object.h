/*
 * homebutton/hbm_kb_object.h - the types and views shared by the homebutton units that took over the functions of the
 * former registered unit `homebutton/keyboard.cpp` (0x805632BC..0x80569DAC).  Moved unchanged from that source; its declarations
 * of other symbols stayed inline in the units that use them.  The former unit's evidence and residual notes are in
 * docs/splits/phase4/homebutton-carried-notes.md.
 */
#ifndef HOMEBUTTON_HBM_KB_OBJECT_H
#define HOMEBUTTON_HBM_KB_OBJECT_H

#include "types.h"

/* ---------------------------------------------------------------------------------------------------
 * helpers that live in other splits.  They sit in the unsplit `main` band before this unit, so no
 * registered unit owns them - rule 2 leaves them a counted gap (tools/units/stylelint.py, "address
 * band interleaves modules"), the same way the neighbouring `homebutton/gui.cpp` declares them.
 * ------------------------------------------------------------------------------------------------ */

/* The state word the dispatchers switch on - slot 0xF0 of the wrapped object's vtable returns it. */
typedef s32 (*KbStateFn)(void);

/* The 2D position the band returns by value (two floats, so MWCC hands them back in r3/r4). */
/* size: 0x08 */
struct Vec2f {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
}; /* size: 0x08 */

/* The argument tuple fn_80563740 copies onto the stack and hands to the callee (slot 0x224). */
/* size: 0x18 */
struct KbArgs {
    /* +0x00 */ s32 a;
    /* +0x04 */ f32 x;
    /* +0x08 */ f32 y;
    /* +0x0C */ s32 b;
    /* +0x10 */ s32 c;
    /* +0x14 */ s32 d;
}; /* size: 0x18 */

/* ---------------------------------------------------------------------------------------------------
 * types.  `KbObject` is a *view* of the band's largest object: only the members this unit's functions
 * touch are named, the gaps are padding.  Its size is an approximation - the largest offset reached
 * in the band is 0x1BE8 plus the four bytes read at +0x1B0C, and the 0x1A04 this-adjusting thunks
 * show that the object whose members sit at 0x1A04.. is itself a base sub-object of a larger class.
 * ------------------------------------------------------------------------------------------------ */
/* size: 0x08 (approximation: only +0x04 is touched by this unit) */
struct Adapter {
    /* +0x00 */ u8 unused_0x00[4];
    /* +0x04 */ Adapter* sub;
};

/* The declaration-only view of a polymorphic sub-object: its `.data` vtable belongs to another split,
 * so only the slots this unit loads are declared (rule 10 - declaring the class's `virtual`s would
 * make MWCC emit a table into this object).  A vtable is a flat array of word slots, so the struct is
 * one member per slot from 0x00 to 0x114. */
/* size: 0x2A8 */
struct KbVTable {
    /* +0x000 */ void (*slot_000)(void);
    /* +0x004 */ void (*slot_004)(void);
    /* +0x008 */ void (*slot_008)(void);
    /* +0x00C */ void (*slot_00C)(void);
    /* +0x010 */ void (*slot_010)(void);
    /* +0x014 */ void (*slot_014)(void);
    /* +0x018 */ void (*slot_018)(void);
    /* +0x01C */ void (*slot_01C)(void);
    /* +0x020 */ void (*slot_020)(void);
    /* +0x024 */ void (*slot_024)(void);
    /* +0x028 */ void (*slot_028)(void);
    /* +0x02C */ void (*slot_02C)(void);
    /* +0x030 */ void (*slot_030)(void);
    /* +0x034 */ void (*slot_034)(void);
    /* +0x038 */ void (*slot_038)(void);
    /* +0x03C */ void (*slot_03C)(void);
    /* +0x040 */ void (*slot_040)(void);
    /* +0x044 */ void (*slot_044)(void);
    /* +0x048 */ void (*slot_048)(void);
    /* +0x04C */ void (*slot_04C)(void);
    /* +0x050 */ void (*slot_050)(void);
    /* +0x054 */ void (*slot_054)(void);
    /* +0x058 */ void (*slot_058)(void);
    /* +0x05C */ void (*slot_05C)(void);
    /* +0x060 */ void (*slot_060)(void);
    /* +0x064 */ void (*slot_064)(void);
    /* +0x068 */ void (*slot_068)(void);
    /* +0x06C */ void (*slot_06C)(void);
    /* +0x070 */ void (*slot_070)(void);
    /* +0x074 */ void (*slot_074)(void);
    /* +0x078 */ void (*slot_078)(void);
    /* +0x07C */ void (*slot_07C)(void);
    /* +0x080 */ void (*slot_080)(void);
    /* +0x084 */ void (*slot_084)(void);
    /* +0x088 */ void (*slot_088)(void);
    /* +0x08C */ void (*slot_08C)(void);
    /* +0x090 */ void (*slot_090)(void);
    /* +0x094 */ void (*slot_094)(void);
    /* +0x098 */ void (*slot_098)(void);
    /* +0x09C */ void (*slot_09C)(void);
    /* +0x0A0 */ void (*slot_0A0)(void);
    /* +0x0A4 */ void (*slot_0A4)(void);
    /* +0x0A8 */ void (*slot_0A8)(void);
    /* +0x0AC */ void (*slot_0AC)(void);
    /* +0x0B0 */ void (*slot_0B0)(void);
    /* +0x0B4 */ void (*slot_0B4)(void);
    /* +0x0B8 */ void (*slot_0B8)(void);
    /* +0x0BC */ void (*slot_0BC)(void);
    /* +0x0C0 */ void (*slot_0C0)(void);
    /* +0x0C4 */ void (*slot_0C4)(void);
    /* +0x0C8 */ void (*slot_0C8)(void);
    /* +0x0CC */ void (*slot_0CC)(void);
    /* +0x0D0 */ void (*slot_0D0)(void);
    /* +0x0D4 */ void (*slot_0D4)(void);
    /* +0x0D8 */ void (*slot_0D8)(void);
    /* +0x0DC */ void (*slot_0DC)(void);
    /* +0x0E0 */ void (*slot_0E0)(void);
    /* +0x0E4 */ void (*slot_0E4)(void);
    /* +0x0E8 */ void (*slot_0E8)(void);
    /* +0x0EC */ void (*slot_0EC)(void);
    /* +0x0F0 */ void (*slot_0F0)(void);
    /* +0x0F4 */ void (*slot_0F4)(void);
    /* +0x0F8 */ void (*slot_0F8)(void);
    /* +0x0FC */ void (*slot_0FC)(void);
    /* +0x100 */ void (*slot_100)(void);
    /* +0x104 */ void (*slot_104)(void);
    /* +0x108 */ void (*slot_108)(void);
    /* +0x10C */ void (*slot_10C)(void);
    /* +0x110 */ void (*slot_110)(void);
    /* +0x114 */ void (*slot_114)(void);
    /* +0x118 */ void (*slot_118)(void);
    /* +0x11C */ void (*slot_11C)(void);
    /* +0x120 */ void (*slot_120)(void);
    /* +0x124 */ void (*slot_124)(void);
    /* +0x128 */ void (*slot_128)(void);
    /* +0x12C */ void (*slot_12C)(void);
    /* +0x130 */ void (*slot_130)(void);
    /* +0x134 */ void (*slot_134)(void);
    /* +0x138 */ void (*slot_138)(void);
    /* +0x13C */ void (*slot_13C)(void);
    /* +0x140 */ void (*slot_140)(void);
    /* +0x144 */ void (*slot_144)(void);
    /* +0x148 */ void (*slot_148)(void);
    /* +0x14C */ void (*slot_14C)(void);
    /* +0x150 */ void (*slot_150)(void);
    /* +0x154 */ void (*slot_154)(void);
    /* +0x158 */ void (*slot_158)(void);
    /* +0x15C */ void (*slot_15C)(void);
    /* +0x160 */ void (*slot_160)(void);
    /* +0x164 */ void (*slot_164)(void);
    /* +0x168 */ void (*slot_168)(void);
    /* +0x16C */ void (*slot_16C)(void);
    /* +0x170 */ void (*slot_170)(void);
    /* +0x174 */ void (*slot_174)(void);
    /* +0x178 */ void (*slot_178)(void);
    /* +0x17C */ void (*slot_17C)(void);
    /* +0x180 */ void (*slot_180)(void);
    /* +0x184 */ void (*slot_184)(void);
    /* +0x188 */ void (*slot_188)(void);
    /* +0x18C */ void (*slot_18C)(void);
    /* +0x190 */ void (*slot_190)(void);
    /* +0x194 */ void (*slot_194)(void);
    /* +0x198 */ void (*slot_198)(void);
    /* +0x19C */ void (*slot_19C)(void);
    /* +0x1A0 */ void (*slot_1A0)(void);
    /* +0x1A4 */ void (*slot_1A4)(void);
    /* +0x1A8 */ void (*slot_1A8)(void);
    /* +0x1AC */ void (*slot_1AC)(void);
    /* +0x1B0 */ void (*slot_1B0)(void);
    /* +0x1B4 */ void (*slot_1B4)(void);
    /* +0x1B8 */ void (*slot_1B8)(void);
    /* +0x1BC */ void (*slot_1BC)(void);
    /* +0x1C0 */ void (*slot_1C0)(void);
    /* +0x1C4 */ void (*slot_1C4)(void);
    /* +0x1C8 */ void (*slot_1C8)(void);
    /* +0x1CC */ void (*slot_1CC)(void);
    /* +0x1D0 */ void (*slot_1D0)(void);
    /* +0x1D4 */ void (*slot_1D4)(void);
    /* +0x1D8 */ void (*slot_1D8)(void);
    /* +0x1DC */ void (*slot_1DC)(void);
    /* +0x1E0 */ void (*slot_1E0)(void);
    /* +0x1E4 */ void (*slot_1E4)(void);
    /* +0x1E8 */ void (*slot_1E8)(void);
    /* +0x1EC */ void (*slot_1EC)(void);
    /* +0x1F0 */ void (*slot_1F0)(void);
    /* +0x1F4 */ void (*slot_1F4)(void);
    /* +0x1F8 */ void (*slot_1F8)(void);
    /* +0x1FC */ void (*slot_1FC)(void);
    /* +0x200 */ void (*slot_200)(void);
    /* +0x204 */ void (*slot_204)(void);
    /* +0x208 */ void (*slot_208)(void);
    /* +0x20C */ void (*slot_20C)(void);
    /* +0x210 */ void (*slot_210)(void);
    /* +0x214 */ void (*slot_214)(void);
    /* +0x218 */ void (*slot_218)(void);
    /* +0x21C */ void (*slot_21C)(void);
    /* +0x220 */ void (*slot_220)(void);
    /* +0x224 */ void (*slot_224)(void);
    /* +0x228 */ void (*slot_228)(void);
    /* +0x22C */ void (*slot_22C)(void);
    /* +0x230 */ void (*slot_230)(void);
    /* +0x234 */ void (*slot_234)(void);
    /* +0x238 */ void (*slot_238)(void);
    /* +0x23C */ void (*slot_23C)(void);
    /* +0x240 */ void (*slot_240)(void);
    /* +0x244 */ void (*slot_244)(void);
    /* +0x248 */ void (*slot_248)(void);
    /* +0x24C */ void (*slot_24C)(void);
    /* +0x250 */ void (*slot_250)(void);
    /* +0x254 */ void (*slot_254)(void);
    /* +0x258 */ void (*slot_258)(void);
    /* +0x25C */ void (*slot_25C)(void);
    /* +0x260 */ void (*slot_260)(void);
    /* +0x264 */ void (*slot_264)(void);
    /* +0x268 */ void (*slot_268)(void);
    /* +0x26C */ void (*slot_26C)(void);
    /* +0x270 */ void (*slot_270)(void);
    /* +0x274 */ void (*slot_274)(void);
    /* +0x278 */ void (*slot_278)(void);
    /* +0x27C */ void (*slot_27C)(void);
    /* +0x280 */ void (*slot_280)(void);
    /* +0x284 */ void (*slot_284)(void);
    /* +0x288 */ void (*slot_288)(void);
    /* +0x28C */ void (*slot_28C)(void);
    /* +0x290 */ void (*slot_290)(void);
    /* +0x294 */ void (*slot_294)(void);
    /* +0x298 */ void (*slot_298)(void);
    /* +0x29C */ void (*slot_29C)(void);
    /* +0x2A0 */ void (*slot_2A0)(void);
    /* +0x2A4 */ void (*slot_2A4)(void);
}; /* size: 0x2A8 */

/* The wrapped object every adapter points at: its first word is the vtable of the concrete class and
 * this unit only ever reads the two words at +0x44 / +0x48 through it (fn_80568DD0's pair). */
/* size: 0x7C */
struct Wrapped {
    /* +0x00 */ KbVTable* vt;
    /* +0x04 */ u8      unused_0x04[0x40];
    /* +0x44 */ f32     float_0x44;
    /* +0x48 */ f32     float_0x48;
    /* +0x4C */ u8      unused_0x4C[0x2C];
    /* +0x78 */ void*   field_0x78;
}; /* size: 0x7C */

/* What KbTail::field_0x004 points at: its +0x10 word is the layout pane the lookups go through. */
/* size: 0x14 */
struct KbPane {
    /* +0x00 */ u8      unused_0x00[0x10];
    /* +0x10 */ Wrapped* field_0x10;
}; /* size: 0x14 (approximation) */

/* The base sub-object embedded at +0x1A04.  It is the target of the band's `subi r3,r3,0x1A04; b ...`
 * this-adjusting thunks, so its own offsets are the derived object's minus 0x1A04; the members this
 * unit touches keep their measured offsets and the rest is filler.  size: 0x1E8 (approximation) */
struct KbTail {
    /* +0x000 */ KbVTable* vt;
    /* +0x004 */ KbPane* field_0x004;
    /* +0x008 */ u8    unused_0x008[0xEC];
    /* +0x0F4 */ Wrapped* field_0x0F4;
    /* +0x0F8 */ u8    unused_0x0F8[8];
    /* +0x100 */ f32   float_0x100;
    /* +0x104 */ f32   float_0x104;
    /* +0x108 */ f32   float_0x108;
    /* +0x10C */ u8    unused_0x10C[4];
    /* +0x110 */ f32   float_0x110;
    /* +0x114 */ f32   float_0x114;
    /* +0x118 */ f32   float_0x118;
    /* +0x11C */ f32   float_0x11C;
    /* +0x120 */ u8    unused_0x120[0xA8];
    /* +0x1C8 */ void* field_0x1C8;
    /* +0x1CC */ u8    unused_0x1CC[4];
    /* +0x1D0 */ Wrapped* field_0x1D0;
    /* +0x1D4 */ u8    unused_0x1D4[8];
    /* +0x1DC */ Wrapped* field_0x1DC;
    /* +0x1E0 */ u8    unused_0x1E0[4];
    /* +0x1E4 */ s32   field_0x1E4;
}; /* size: 0x1E8 (approximation) */

/* The band's own work object.  This is a *view*: only the members this unit's functions touch are
 * named, every gap is a filler, and the size is an approximation - the largest offset any function of
 * the range reaches is +0x1BE8 (KbTail::field_0x1E4). */
/* size: 0x1BEC (approximation - see the comment above) */
struct KbObject {
    /* +0x000 */ KbVTable* vt;
    /* +0x004 */ Wrapped* sub_0x04;
    /* +0x008 */ u8      unused_0x008[8];
    /* +0x010 */ u8      field_0x010[0xC];     /* a sub-object handed by address to fn_8055B174 */
    /* +0x01C */ Wrapped* sub_0x1C;
    /* +0x020 */ Wrapped* sub_0x20;
    /* +0x024 */ Wrapped* sub_0x24;
    /* +0x028 */ u8      unused_0x028[4];
    /* +0x02C */ void*   field_0x2C;
    /* +0x030 */ void*   field_0x30;
    /* +0x034 */ u8      unused_0x034[0x10];
    /* +0x044 */ void*   field_0x44;
    /* +0x048 */ u8      field_0x48;
    /* +0x049 */ u8      field_0x49;
    /* +0x04A */ u8      field_0x4A;
    /* +0x04B */ u8      field_0x4B;
    /* +0x04C */ u8      field_0x4C;
    /* +0x04D */ u8      field_0x4D;
    /* +0x04E */ u8      unused_0x04E[0xB2];
    /* +0x100 */ f32     float_0x100;
    /* +0x104 */ u8      unused_0x104[0x1900];
    /* +0x1A04 */ KbTail  tail_0x1A04;
}; /* size: 0x1BEC */

#endif
