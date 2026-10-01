/* The types and declarations `src/homebutton/fn_8054E894.cpp` owns (docs/plan.md 6.5, rules 1-5).
 *
 * The unit is the 0x8054E894-0x80555374 slice of the home-button (Wii HOME overlay) software-keyboard
 * block.  It keeps the map's `fn_8054E894` stem (no `__FILE__` string covers the range and the shared
 * runtime dump answers only `zz_054e894_`), so `fn_XXXXXXXX` names are the map's own placeholders and
 * stay legal under rule 7's per-file deferral - see the source header.
 *
 * The types here are *views*: the band spans several classes and the retail objects' layouts are only
 * partly recoverable, so each view declares the fields the bodies below prove and fills the rest with
 * padding, keeping every offset exact.  `HkbWidgetVtable` is a view of tables another unit owns
 * (`.data` 0x8064E780 and its group), so it is a struct of typed slots here and never a definition we
 * emit (rule 10).
 *
 * The callees whose owners are registered are declared in the *owner's* header (`include/homebutton/<unit>.h`) and
 * included from the source.  A callee whose band holds no registered unit (the
 * 0x805124F4-0x8054E894 and 0x805425B4-0x8054F550 bands - the address band interleaves the `DWCi` and
 * `homebutton` modules, so there is no sound `include/unsplit/<module>.h`) is declared here with the
 * signature its call site shows.
 */
#ifndef MHTRI_HOMEBUTTON_FN_8054E894_H
#define MHTRI_HOMEBUTTON_FN_8054E894_H

#include "types.h"

struct HkbWidget;

/* The keyboard widget the band's state machines run on.  It is a complete object with base
 * subobjects at +0x04, +0x10, +0x14, +0x18, +0x17F4, +0x189C and +0x1A04 (the adjustor thunks
 * below convert one of those pointers back with the named field's offset).  Only the members the
 * bodies below touch are named and every other byte keeps its offset as filler.
 * size: 0x1B89 (approximate - the band never sees the tail). */
struct HkbWidget {
    /* slot +0x008 */ virtual void v_0x008(void* arg1, u32 arg2, u32 arg3);
    /* slot +0x00C */ virtual void v_0x00C(u32 value);
    /* slot +0x010 */ virtual void* v_0x010();
    /* slot +0x014 */ virtual void v_0x014(u32 value);
    /* slot +0x018 */ virtual void v_0x018(u32 id, void* arg);
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020(void* arg);
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void v_0x030(void* arg);
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C(u32 value);
    /* slot +0x040 */ virtual void v_0x040(u8 value);
    /* slot +0x044 */ virtual u8 v_0x044();
    /* slot +0x048 */ virtual void v_0x048();
    /* slot +0x04C */ virtual void v_0x04C();
    /* slot +0x050 */ virtual void v_0x050();
    /* slot +0x054 */ virtual void v_0x054();
    /* slot +0x058 */ virtual void v_0x058();
    /* slot +0x05C */ virtual void v_0x05C(u32 value);
    /* slot +0x060 */ virtual void v_0x060();
    /* slot +0x064 */ virtual void v_0x064();
    /* slot +0x068 */ virtual void* v_0x068();
    /* slot +0x06C */ virtual void v_0x06C();
    /* slot +0x070 */ virtual void v_0x070();
    /* slot +0x074 */ virtual void v_0x074();
    /* slot +0x078 */ virtual void v_0x078();
    /* slot +0x07C */ virtual void v_0x07C();
    /* slot +0x080 */ virtual void v_0x080();
    /* slot +0x084 */ virtual void v_0x084();
    /* slot +0x088 */ virtual void v_0x088();
    /* slot +0x08C */ virtual void v_0x08C();
    /* slot +0x090 */ virtual void v_0x090();
    /* slot +0x094 */ virtual void v_0x094();
    /* slot +0x098 */ virtual void v_0x098();
    /* slot +0x09C */ virtual void v_0x09C();
    /* slot +0x0A0 */ virtual void v_0x0A0();
    /* slot +0x0A4 */ virtual void v_0x0A4();
    /* slot +0x0A8 */ virtual void v_0x0A8();
    /* slot +0x0AC */ virtual void v_0x0AC();
    /* slot +0x0B0 */ virtual void v_0x0B0();
    /* slot +0x0B4 */ virtual void v_0x0B4();
    /* slot +0x0B8 */ virtual void v_0x0B8();
    /* slot +0x0BC */ virtual void v_0x0BC();
    /* slot +0x0C0 */ virtual void v_0x0C0();
    /* slot +0x0C4 */ virtual void v_0x0C4(u32 value);
    /* slot +0x0C8 */ virtual void v_0x0C8();
    /* slot +0x0CC */ virtual void v_0x0CC();
    /* slot +0x0D0 */ virtual void v_0x0D0();
    /* slot +0x0D4 */ virtual void v_0x0D4();
    /* slot +0x0D8 */ virtual void v_0x0D8();
    /* slot +0x0DC */ virtual void v_0x0DC();
    /* slot +0x0E0 */ virtual void v_0x0E0(u32 value);
    /* slot +0x0E4 */ virtual void v_0x0E4();
    /* slot +0x0E8 */ virtual void v_0x0E8();
    /* slot +0x0EC */ virtual void v_0x0EC();
    /* slot +0x0F0 */ virtual void v_0x0F0();
    /* slot +0x0F4 */ virtual void v_0x0F4();
    /* slot +0x0F8 */ virtual void v_0x0F8();
    /* slot +0x0FC */ virtual void v_0x0FC();
    /* slot +0x100 */ virtual void v_0x100();
    /* slot +0x104 */ virtual void v_0x104();
    /* slot +0x108 */ virtual void v_0x108(u32 value);
    /* slot +0x10C */ virtual void v_0x10C();
    /* slot +0x110 */ virtual void v_0x110();
    /* slot +0x114 */ virtual void v_0x114();
    /* slot +0x118 */ virtual void v_0x118();
    /* slot +0x11C */ virtual void v_0x11C();
    /* slot +0x120 */ virtual void v_0x120();
    /* slot +0x124 */ virtual void v_0x124();
    /* slot +0x128 */ virtual void v_0x128();
    /* slot +0x12C */ virtual void v_0x12C();
    /* slot +0x130 */ virtual void v_0x130();
    /* slot +0x134 */ virtual void v_0x134();
    /* slot +0x138 */ virtual void v_0x138();
    /* slot +0x13C */ virtual void v_0x13C();
    /* slot +0x140 */ virtual void v_0x140();
    /* slot +0x144 */ virtual void v_0x144();
    /* slot +0x148 */ virtual void v_0x148();
    /* slot +0x14C */ virtual void v_0x14C();
    /* slot +0x150 */ virtual void v_0x150();
    /* slot +0x154 */ virtual void v_0x154();
    /* slot +0x158 */ virtual void v_0x158();
    /* slot +0x15C */ virtual void v_0x15C();
    /* slot +0x160 */ virtual void v_0x160();
    /* slot +0x164 */ virtual void v_0x164();
    /* slot +0x168 */ virtual void v_0x168();
    /* slot +0x16C */ virtual void v_0x16C();
    /* slot +0x170 */ virtual void v_0x170();
    /* slot +0x174 */ virtual void v_0x174();
    /* slot +0x178 */ virtual void v_0x178();
    /* slot +0x17C */ virtual void v_0x17C();
    /* slot +0x180 */ virtual void v_0x180();
    /* slot +0x184 */ virtual void v_0x184();
    /* slot +0x188 */ virtual void v_0x188();
    /* slot +0x18C */ virtual void v_0x18C();
    /* slot +0x190 */ virtual void v_0x190();
    /* slot +0x194 */ virtual void v_0x194();
    /* slot +0x198 */ virtual void v_0x198();
    /* slot +0x19C */ virtual void v_0x19C();
    /* slot +0x1A0 */ virtual void v_0x1A0();
    /* slot +0x1A4 */ virtual void v_0x1A4();
    /* slot +0x1A8 */ virtual void v_0x1A8();
    /* slot +0x1AC */ virtual void v_0x1AC();
    /* slot +0x1B0 */ virtual void v_0x1B0();
    /* slot +0x1B4 */ virtual void v_0x1B4();
    /* slot +0x1B8 */ virtual void v_0x1B8();
    /* slot +0x1BC */ virtual void v_0x1BC();
    /* slot +0x1C0 */ virtual void v_0x1C0();
    /* slot +0x1C4 */ virtual void v_0x1C4();
    /* slot +0x1C8 */ virtual void v_0x1C8();
    /* slot +0x1CC */ virtual void v_0x1CC();
    /* slot +0x1D0 */ virtual void v_0x1D0();
    /* slot +0x1D4 */ virtual void v_0x1D4();
    /* slot +0x1D8 */ virtual void v_0x1D8();
    /* slot +0x1DC */ virtual void v_0x1DC();
    /* slot +0x1E0 */ virtual void v_0x1E0();
    /* slot +0x1E4 */ virtual void v_0x1E4();
    /* slot +0x1E8 */ virtual void v_0x1E8();
    /* slot +0x1EC */ virtual void v_0x1EC();
    /* slot +0x1F0 */ virtual void v_0x1F0();
    /* slot +0x1F4 */ virtual void v_0x1F4();
    /* slot +0x1F8 */ virtual void v_0x1F8();
    /* slot +0x1FC */ virtual void v_0x1FC();
    /* slot +0x200 */ virtual void v_0x200();
    /* slot +0x204 */ virtual void v_0x204();
    /* slot +0x208 */ virtual void v_0x208();
    /* slot +0x20C */ virtual void v_0x20C();
    /* slot +0x210 */ virtual void v_0x210();
    /* slot +0x214 */ virtual void v_0x214();
    /* slot +0x218 */ virtual void v_0x218();
    /* slot +0x21C */ virtual void v_0x21C();
    /* slot +0x220 */ virtual void v_0x220();
    /* slot +0x224 */ virtual void v_0x224();
    /* slot +0x228 */ virtual void v_0x228();
    /* slot +0x22C */ virtual void v_0x22C();
    /* slot +0x230 */ virtual void v_0x230();
    /* the class is polymorphic on purpose: MWCC emits the canonical
     * `lwz r12, 0(r3); lwz r12, <slot>(r12); mtctr r12; bctr` virtual call for `p->v_0xNNN()`,
     * which a `struct` of function pointers does not (it stages the table pointer through a
     * temporary).  No method is defined and the class is never instantiated, so MWCC emits no table
     * into this object (rule 10) - the tables live in the units above.
     * The slots are named by their offset only: the retail names are not recoverable. */

    /* the vptr the virtuals above require sits at +0x0000 and is compiler-emitted; the data
     * members below therefore start at +0x0004. */
    /* +0x0004 */ void* vtable_04;           /* base #1 (adjustor thunk `addi r3, r3, 4`) */
    /* +0x0008 */ u32 value_08;              /* the word fn_8054F788 clears, fn_80555110 stores */
    /* +0x000C */ u32 value_0C;              /* the word fn_80554AD8 stores */
    /* +0x0010 */ void* vtable_10;           /* base #2 (adjustor thunk `subi 0x10`) */
    /* +0x0014 */ void* vtable_14;           /* base #3 (adjustor thunk `subi 0x14`) */
    /* +0x0018 */ HkbWidget* table_18;     /* base #4: `lwzu r12, 24(r3)`, slot +0x14 */
    /* +0x001C */ u8 pad_001C[0x8];
    /* +0x0024 */ HkbWidget* pane_24;       /* the pane fn_80554F18/fn_80555014 dispatch to */
    /* +0x0028 */ HkbWidget* pane_28;       /* the pane fn_80550D5C/fn_8055512C dispatch to */
    /* +0x002C */ u32 state;                 /* the selector fn_8054F120 re-sorts after */
    /* +0x0030 */ void* current;             /* the child the accessors hand back */
    /* +0x0034 */ u8 pad_0034[0x64];
    /* +0x0098 */ f32 value_98;              /* the timer fn_8054F154 stores */
    /* +0x009C */ u8 pad_009C[0x20];
    /* +0x00BC */ u32 field_BC;              /* the subobject fn_8054F14C hands out */
    /* +0x00C0 */ u8 pad_00C0[0x0C];
    /* +0x00CC */ u32 field_CC;              /* the word fn_8054F13C reads */
    /* +0x00D0 */ u8 pad_00D0[0x34];
    /* +0x0104 */ u8 flag_104;               /* the byte fn_8054F144 reads */
    /* +0x0105 */ u8 pad_0105[0x16E3];
    /* +0x17E8 */ u8 flag_17E8;              /* the byte fn_8054F810 stores */
    /* +0x17E9 */ u8 flag_17E9;              /* the byte fn_8054F8FC stores, fn_80551698 compares */
    /* +0x17EA */ u8 pad_17EA[0x2];
    /* +0x17EC */ u32 value_17EC;            /* the word fn_805508F4 stores */
    /* +0x17F0 */ u8 pad_17F0[0x4];
    /* +0x17F4 */ void* vtable_17F4;         /* base #5 (adjustor thunk `subi 0x17F4`) */
    /* +0x17F8 */ u8 pad_17F8[0x0C];
    /* +0x1804 */ HkbWidget* pane_1804;       /* the pane fn_8054F90C clears */
    /* +0x1808 */ HkbWidget* child_1808;      /* the pane fn_805512B4/fn_80551314 re-targets */
    /* +0x180C */ u8 pad_180C[0x60];
    /* +0x186C */ f32 value_186C;            /* the subtrahend of fn_80551EF4 */
    /* +0x1870 */ u8 pad_1870[0x4];
    /* +0x1874 */ f32 value_1874;            /* the minuend of fn_80551EF4 */
    /* +0x1878 */ u8 pad_1878[0x24];
    /* +0x189C */ void* vtable_189C;         /* base #6 (adjustor thunk `subi 0x189C`) */
    /* +0x18A0 */ u8 pad_18A0[0x10];
    /* +0x18B0 */ u32 value_18B0;            /* the word fn_8054F90C copies */
    /* +0x18B4 */ u32 value_18B4;            /* the word fn_8054F90C compares */
    /* +0x18B8 */ u8 sub_18B8[0x84];         /* the sub-object fn_805512B4 hands to fn_80553074 */
    /* +0x193C */ HkbWidget* child_193C;     /* the sub-widget fn_8054F410/4F424 dispatch to */
    /* +0x1940 */ HkbWidget* child_1940;     /* the sub-widget fn_8054F164 dispatches to */
    /* +0x1944 */ u8 pad_1944[0x4C];
    /* +0x1990 */ u32 count_1990;            /* the counter fn_8054F408 stores */
    /* +0x1994 */ u32 value_1994;            /* the word fn_8054F400 stores */
    /* +0x1998 */ u32 value_1998;            /* the word fn_8054F3DC stores */
    /* +0x199C */ u8 pad_199C[0x5E];
    /* +0x19FA */ u8 flag_19FA;              /* the byte fn_8054F178 stores */
    /* +0x19FB */ u8 pad_19FB[0x1];
    /* +0x19FC */ u32 value_19FC;            /* the word fn_8054F178 stores */
    /* +0x1A00 */ u8 pad_1A00[0x4];
    /* +0x1A04 */ void* vtable_1A04;         /* base #7 (adjustor thunk `subi 0x1A04`) */
    /* +0x1A08 */ u8 pad_1A08[0x80];
    /* +0x1A88 */ u32 value_1A88;            /* the word fn_80550CBC stores */
    /* +0x1A8C */ u32 value_1A8C;            /* the word fn_80550CC4/fn_80551964 use */
    /* +0x1A90 */ u32 value_1A90;            /* the word fn_805544C0 masks, fn_80551314 counts */
    /* +0x1A94 */ u8 pad_1A94[0x4];
    /* +0x1A98 */ s32 count_1A98;            /* the limit fn_805512B4 bounds-checks */
    /* +0x1A9C */ u8 pad_1A9C[0x8];
    /* +0x1AA4 */ s32 index_1AA4;            /* the cursor fn_805512B4 bounds-checks */
    /* +0x1AA8 */ f32 value_1AA8;            /* the float fn_80553848 stores */
    /* +0x1AAC */ f32 value_1AAC;            /* the float fn_80551EF4 stores */
    /* +0x1AB0 */ u8 pad_1AB0[0x4];
    /* +0x1AB4 */ u8 flag_1AB4;              /* the byte fn_8054EEBC reads */
    /* +0x1AB5 */ u8 flag_1AB5;              /* the byte fn_8054EEC4 reads */
    /* +0x1AB6 */ u8 pad_1AB6[0xCE];
    /* +0x1B84 */ HkbWidget* pane_1B84;      /* the pane fn_80551698 dispatches to */
    /* +0x1B88 */ u8 flag_1B88;              /* the byte fn_80550768 stores */
};

/* The (target, argument) node fn_8054F75C and fn_8054F788 drive.  size: 0x0C (approximate - the
 * band never reads past +0x08). */
struct HkbNode {
    /* +0x00 */ u32 pad_00;
    /* +0x04 */ HkbWidget* target;
    /* +0x08 */ u32 argument;
};

/* The (unused, flag) record fn_80553F84 tests.  size: 0x08 */
struct HkbFlagRecord {
    /* +0x00 */ u32 pad_00;
    /* +0x04 */ u32 flag_04;
};

/* The child pointer at +0x10 (fn_80553F84).  size: 0x14 */
struct HkbChild10 {
    /* +0x00 */ u8 pad_00[0x10];
    /* +0x10 */ HkbWidget* child_10;
};

/* The object fn_805546B4 configures: a byte flag at +0x04 and the child it dispatches to.
 * size: 0x14 (approximate) */
struct HkbFlagChild {
    /* +0x00 */ void* table;
    /* +0x04 */ u8 flag_04;
    /* +0x05 */ u8 pad_05[0xB];
    /* +0x10 */ HkbWidget* child_10;
};

/* The u16 array at +0x80 (fn_80554268).  size: 0x180 */
struct HkbU16Array {
    /* +0x00 */ u8 pad_00[0x80];
    /* +0x80 */ u16 values_80[0x80];
};

/* The object's +0x15 byte (fn_8054F540/fn_8054F548).  It is a view of its own because +0x14 is a
 * base-subobject pointer on the widget the rest of the band works on.  size: 0x18 (approximate). */
struct HkbFlagView {
    /* +0x00 */ u8 pad_00[0x15];
    /* +0x15 */ u8 flag_15;
};

/* The record fn_8054F804 copies the caller's 0x17D8-byte keyboard-layout block into.
 * size: 0x17E8 */
struct HkbLayoutRecord {
    /* +0x0000 */ void* vtable;
    /* +0x0004 */ u32 pad_04[3];
    /* +0x0010 */ u8 buffer_10[0x17D8];
};

/* `offsetof` for this band's types (MWCC's `stddef.h` is off the include path).  The adjustor thunks
 * convert a base-subobject pointer back to the complete object with it. */
#define HKB_OFFSET_OF(type, field) ((u32)(&((type*)0)->field))

#endif
