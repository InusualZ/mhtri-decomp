/*
 * homebutton/keyboard_ui.cpp - the lower slice of the home-button software-keyboard band.
 *
 * `.text` 0x8055C894-0x805632BC (133 functions, 27176 B).  This is one maximal unclaimed run
 * (docs/plan.md 12) between the unclaimed 0x80558E84 band below it and the registered
 * `homebutton/keyboard.cpp` (0x805632BC) above it.
 *
 * SEAM (unproven, and the brief says so).  The left edge is a `--max-bytes` cap, not a translation-unit
 * boundary (the discovery note "capped at --max-bytes, seam is a guess").  `tudiscover.py at
 * 0x8055C894`, run in MAIN whose asm dump the tool needs, reports the match set as the single function
 * fn_8055C894 with no must-link anchors and only weak cuts (left 0x8055B4A8/0x8055B534, share 0.108,
 * codegen-fingerprint change; right 0x8055D478, share 0.200) - no cut it will stand behind, exactly as
 * the neighbouring `homebutton/keyboard.cpp` records for its own left edge.  The right edge *is* a
 * registered boundary: 0x805632BC is the first address of `homebutton/keyboard.cpp`, and that unit's
 * header records the same evidence from the other side ("the code just before 0x805632BC references the
 * same data pool as this range ... the retail translation unit continues to the left").  The range is
 * therefore worked as one unit; its extent settles as its bodies match.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup` over the range's addresses and with
 * `grep -n "fn_8055C\|fn_8055D\|fn_8055E\|fn_80560\|fn_80561\|fn_80562" config/RMHE08/symbols.txt`:
 * every defined name of the range is a bare `.text` entry, and the runtime dump answers a bare
 * `zz_<addr>_` placeholder for every one probed - the shared dump names no function of the band).
 *
 * Name evidence (docs/plan.md 12, evidence class 3).  No `__FILE__` string is emitted anywhere in the
 * range (no `.cpp`/`.c` source-name string is referenced by any function of it), so class 1 does not
 * apply.  The module is `homebutton`: the unit's whole `.data` vocabulary is the home-button software
 * keyboard's layout pool - `0x80651870 "T_title_text"`, `0x80651880 "fs_VK_ascii_keytop_a.brlyt"`,
 * `0x806518a0 "fs_VK_cellPhone_a.brlyt"`, `0x806518b8 "WiiBitmapFontType1.brfnt"`,
 * `0x806518d4 "fs_VK_textBox_a.brlyt"`, `0x806518ec "fs_VK_predictInput_a.brlyt"`,
 * `0x80651908 "fs_VK_toolbar_a.brlyt"`, `0x80651920 "fs_prdicSelWidw_a.brlyt"`,
 * `0x80651938 "fs_signWindow_a.brlyt"`, `0x80652300 "T_2l_TextBox"`, `0x806576b4 "fs_VK_textBox_a.brlyt"`,
 * `0x806576e8 "fs_VK_textBox_b.brlyt"`, `0x80657700 "fs_VK_bg_a.brlyt"`, `0x807946f0 "B_abc"`,
 * `0x807946f8 "T_sign"`, `0x80794700 "B_sign"` - the same pool the registered `homebutton/keyboard.cpp`
 * (immediately above at 0x805632BC) and `homebutton/gui.cpp` (0x80569DAC) are documented to own.  The
 * link neighbour is `homebutton/keyboard.cpp`, so the unit takes the `main` lib and cflags_main.  The
 * file is the lower slice of that keyboard translation unit: `keyboard.cpp` is the upper slice, this is
 * the part below it (the name is descriptive, class 3; the symbols keep the map's `fn_` stem with the
 * rule-7 deferral above).
 *
 * Sections claimed: `.text` 0x8055C894-0x805632BC plus the `.ctors` words 0x8056F410-0x8056F424 (the
 * splitter assigns that run to this unit).  The range's `.data`/`.sdata`/`.sdata2` pools
 * (the keyboard layout strings, the float pools `lbl_8079D720`..`lbl_8079D73C`, the small-data globals
 * `lbl_807946D8`, `lbl_80795A78`/`lbl_80795A90`) live in other splits and are referenced here as the
 * map's `lbl_` symbols, never re-emitted (rule 10 / rule 2).
 *
 * Exceptions: the retired target objects (`auto_03_80558E84_text.o`, `auto_03_8055EAB8_text.o`,
 * `auto_03_8056083C_text.o`) carry no extab/extabindex (and the discovery record says
 * `exceptions_on: false`), so the retail object was built with exceptions off while cflags_main turns
 * them on; the deviation is scoped to this file with a `#pragma exceptions off` (the same call as the
 * neighbouring `homebutton/keyboard.cpp` and `tiHKBManager.cpp`).
 *
 * Reconstruction status (measured with `tools/units/recompile.py homebutton/keyboard_ui.cpp --measure <sym>`,
 * the official report metric, against MAIN's retired split objects `auto_03_80558E84_text.o`,
 * `auto_03_8055EAB8_text.o` and `auto_03_8056083C_text.o`): 70 of the range's 133 functions have a
 * body and 63 of them measure >= 80 %.  The unit is far short of the 80 % bar: the 70 bodies written
 * here are the mechanical half of the band (the no-ops, the constant returns, the accessors, the
 * tail-call thunks, the virtual forwarders, the guarded forwarders, the factory/deleting destructors,
 * the dispatch helpers and the string predicates) and the band's byte mass sits in the large bodies
 * that are not attempted yet (fn_8055EC3C 0x464, fn_8055D594 0x378, fn_8055CFC8 0x338, fn_8055D90C
 * 0x26C, fn_805608F8 0x270, fn_80560BC4 0x280, fn_8055CB34 0x1EC, fn_8055CD60 0x1F4, fn_8055D478
 * 0x11C, fn_8055D36C 0x10C, fn_8055E288 0x1A4, fn_805617B8 0x520, fn_80561CD8 0x420, ...).  The
 * 70 bodies cover 2892 B of the range (the unit measures 9.0428 %); the 63 functions without a body
 * cover 24284 B.  The per-symbol table is in the outbox .pi/outbox/8055c894-fn-8055c894-0851.json.
 *
 * RESIDUALS (what still differs and why):
 *   - **The virtual-call register is the unit's systematic residual.**  A virtual call's vtable pointer
 *     is materialised into a scratch GPR here (`lwz r4, 0(r3)`) where the retail object uses r12
 *     (`lwz r12, 0(r3)`), because the conformant spelling of a call through a slot outside our ranges
 *     is a struct of function pointers (rule 10) and MWCC only uses the r12 idiom for a *virtual* call
 *     on a declared class.  That is one instruction per call site: the ten 0x14-byte forwarders
 *     (fn_8055DB78/DB8C/DBB8/DBCC/DBE0, fn_80560FB4/FC8/FDC/FF0/1004) measure 98.00 % for that one
 *     reason, and every larger body carries the same penalty at each of its call sites.  The
 *     neighbour `homebutton/keyboard.cpp` records the identical residual.
 *   - fn_8055F4D8 / fn_8055F510 / fn_8055F548 71.07 % - the three small-data predicates.  Sizes exact
 *     (56 B).  The target materialises the `.sdata` base into r5 (`li r5, lbl_80795A78@sda21; lwz r3,
 *     0x4(r5)`) before moving self into r4; our spelling materialises it into r3 after the move.  A
 *     local-pointer form was tried (the current source); it does not move the base to r5.
 *   - fn_8056329C 73.75 %, sizes 32 vs 24 B - the nibble dispatcher.  The target is MWCC's `switch`
 *     range shape (`cmpwi; beq case1; bge default; b default`); our `switch` with a single `case 1`
 *     is narrowed to `cmpwi; bne default; b fn_805632BC; b fn_805634A4`, losing the two range
 *     branches.  A `switch` and an `if/else` were both tried.
 *   - fn_8056085C 70.53 %, sizes exact (144 B) - the list walk.  The loop body and the prologue save
 *     order differ; the call's vtable load lands in a fresh GPR (`lwz r6, 0(r31)`) where the target
 *     reuses r12, and MWCC schedules the `mr r28,r3` / `stw r28,0x10(r1)` pair differently.
 *   - fn_8055DF3C 72.63 %, sizes exact (76 B) - the small factory.  The logic is exact; MWCC schedules
 *     `li r4, 0x14` and `li r0, 0` on opposite sides of the prologue store / the `addi r4` here.
 *   - fn_80561018 63.78 %, sizes exact (92 B) - the state-1 forwarder.  The target hoists `cmpwi r4,1`
 *     above the prologue's register saves (the same scheduling residual `keyboard.cpp` records for its
 *     two stream destructors) and reuses r12 for the vtable load.
 *   - The seven this-adjusting thunks fn_8055EAB8/EAC0/EAC8/EAD0/EAD8/EAE0/EAE8 (`subi r3,r3,0xNN; b
 *     <victim>`, 0x4C/0x34/0x17F4/0x1A04/0x14/0x1C/0x24) are not written: they are MWCC's own output
 *     for a virtual inherited from a base sub-object, and the conformant way to get them (declare the
 *     class with `virtual`) would make MWCC emit the vtable into this object (rule 10), while writing
 *     them as a hand pointer adjustment is rule 6.  Same call as the neighbour `homebutton/gui.cpp`
 *     records for its three `addi r3,r3,-4` thunks.
 *   - The remaining 63 functions have no body yet (0 %, unpaired): see the outbox for the list.  The
 *     largest are fn_8055EC3C (0x464), fn_8055D594 (0x378), fn_8055CFC8 (0x338), fn_80560BC4 (0x280),
 *     fn_805608F8 (0x270), fn_8055D90C (0x26C), fn_8055CD60 (0x1F4), fn_8055CB34 (0x1EC), fn_8055E288
 *     (0x1A4), fn_8055D478 (0x11C), fn_8055D36C (0x10C), fn_805617B8 (0x520), fn_80561CD8 (0x420).
 *
 * The `.ctors` words at 0x8056F410-0x8056F424 are claimed with this unit (the splitter assigns them
 * here); no static objects are reconstructed yet, so this object emits no `.ctors` section - a residual
 * for the next pass.
 */

#include "types.h"
#include "sys_mem.h"

/* The retail object was built with exceptions off (no extab/extabindex in any of its sections). */
#pragma exceptions off

/* ---------------------------------------------------------------------------------------------------
 * helpers that live in other splits.  They sit in the unsplit `main` band (fn_80501xxx / fn_8054xxx /
 * fn_8053xxx), so no registered unit owns them - rule 2 leaves them a counted gap (tools/units/stylelint.py,
 * "address band interleaves modules"), the same way the neighbouring `homebutton/gui.cpp` declares them.
 * ------------------------------------------------------------------------------------------------ */
extern "C" {
void* fn_80501C60(void* list, void* prev);   /* ut::List GetNext (head when prev is 0) */
void  fn_80501A64(void* list, void* node);   /* ut::List append */
void  MEMInitList(void* list, u16 offset);   /* ut::List initialiser */
void* fn_80546660(void* p);
void* MEMAllocFromAllocator(void* alloc, u32 size);
void  fn_8056D814(void* obj, void* owner);
void  fn_8055B7D4(void* sub, s32 flag);
s32   fn_80463C1C(void* base, void* key);
s32   strlen(const char* s);
s32   strncmp(const char* a, const char* b, u32 n);
char* strncpy(char* dst, const char* src, u32 n);
void* memset(void* dst, s32 c, u32 n);

/* The 7 base destructors the range's deleting destructors dispatch into (each in the unsplit band). */
void fn_805385D0(void* self, s32 flag);
void fn_8053F65C(void* self, s32 flag);
void fn_8054F90C(void* self, s32 flag);
void fn_8054CF38(void* self, s32 flag);
void fn_80554714(void* self, s32 flag);
void fn_80555B0C(void* self, s32 flag);
void fn_80557228(void* self, s32 flag);

/* The two dispatchers of the registered neighbour `homebutton/keyboard.cpp` that fn_8056329C tail-calls. */
void fn_805632BC(void* self);
void fn_805634A4(void* self);
}

/* ---------------------------------------------------------------------------------------------------
 * the unit's own `.sdata2` float pool and `.sdata` globals: other splits own them, so they are
 * declared, never defined (playbook 29).
 * ------------------------------------------------------------------------------------------------ */
extern "C" {
extern f32 lbl_8079D720;
extern f32 lbl_8079D724;
extern f32 lbl_8079D728;
extern f32 lbl_8079D72C;
extern f32 lbl_8079D730;
extern f32 lbl_8079D734;
extern f32 lbl_807946D8[2];  /* a small-data record whose float at +4 is written by fn_8056167C */
extern u32 lbl_80795A78[2];  /* small-data records whose word at +4 is the search base */
extern u32 lbl_80795A80[2];
extern u32 lbl_80795A88[2];
extern u8  lbl_8064F0B0[];   /* the vtable/type record a factory stores into its fresh object */
}

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

extern "C" {

/* ---------------------------------------------------------------------------------------------------
 * no-op virtuals: the retail body is a bare `blr`
 * ------------------------------------------------------------------------------------------------ */
void fn_8055CAB8(VuObject* self) { (void)self; }
void fn_8055CABC(VuObject* self) { (void)self; }
void fn_8055CAC0(VuObject* self) { (void)self; }
void fn_8055CAC4(VuObject* self) { (void)self; }
void fn_8055CAC8(VuObject* self) { (void)self; }
void fn_8055CACC(VuObject* self) { (void)self; }
void fn_8055E7B8(VuObject* self) { (void)self; }
void fn_8055E7BC(VuObject* self) { (void)self; }
void fn_8055E834(VuObject* self) { (void)self; }

/* ---------------------------------------------------------------------------------------------------
 * constant returns
 * ------------------------------------------------------------------------------------------------ */
s32 fn_8055F648(VuObject* self) { (void)self; return 1; }

/* ---------------------------------------------------------------------------------------------------
 * accessors
 * ------------------------------------------------------------------------------------------------ */
void* fn_8055E694(VuObject* self) { return self->field_0x20; }
void* fn_8055E778(VuObject* self) { return self->sub_0x1C; }
void* fn_8055E780(VuObject* self) { return self->field_0x24; }
void* fn_8055E788(VuObject* self) { return self->field_0x18; }
void* fn_8055E790(VuObject* self) { return self->field_0x28; }
void* fn_8055E798(VuObject* self) { return self->field_0x28; }
void* fn_8055E7A0(VuObject* self) { return self->field_0x2C; }
void* fn_8055E7A8(VuObject* self) { return self->field_0x14; }
void* fn_8055E7B0(VuObject* self) { return self->field_0x10; }
void* fn_8055E82C(VuObject* self) { return self->field_0x38; }

void fn_8055DBA8(VuObject* self, void* value) { self->field_0x38 = value; }
void fn_8055DBB0(VuObject* self, void* value) { self->field_0x34 = value; }
void fn_8055E838(VuObject* self, void* value) { self->field_0x14 = value; }

/* ---------------------------------------------------------------------------------------------------
 * tail-call thunks: the retail body is `...; b <victim>` (the arguments pass through untouched)
 * ------------------------------------------------------------------------------------------------ */
void fn_8055CAB0(VuObject* self, void* node) { fn_80501A64(&self->list_0x08, node); }
void fn_8055DBA0(VuObject* self) { fn_80546660(self->sub_0x1C); }
void fn_805608EC(VuObject* self) { MEMInitList(&self->list_0x04, 4); }

/* ---------------------------------------------------------------------------------------------------
 * small virtual forwarders: `return self->sub->vt->slot(...)`
 * ------------------------------------------------------------------------------------------------ */
void fn_8055DB78(VuObject* self) { ((void (*)(VuSub*))self->sub_0x1C->vt->slot_0DC)(self->sub_0x1C); }
void fn_8055DB8C(VuObject* self) { ((void (*)(VuSub*))self->sub_0x1C->vt->slot_240)(self->sub_0x1C); }
void fn_8055DBB8(VuObject* self) { ((void (*)(VuSub*))self->sub_0x1C->vt->slot_0FC)(self->sub_0x1C); }
void fn_8055DBCC(VuObject* self) { ((void (*)(VuSub*))self->sub_0x1C->vt->slot_100)(self->sub_0x1C); }
void fn_8055DBE0(VuObject* self) { ((void (*)(VuSub*))self->sub_0x1C->vt->slot_104)(self->sub_0x1C); }

void fn_80560FB4(VuObject* self) { ((void (*)(VuSub*))self->sub_0x40->vt->slot_020)(self->sub_0x40); }
void fn_80560FC8(VuObject* self) { ((void (*)(VuSub*))self->sub_0x40->vt->slot_018)(self->sub_0x40); }
void fn_80560FDC(VuObject* self) { ((void (*)(VuSub*))self->sub_0x40->vt->slot_01C)(self->sub_0x40); }
void fn_80560FF0(VuObject* self) { ((void (*)(VuSub*))self->sub_0x40->vt->slot_024)(self->sub_0x40); }
void fn_80561004(VuObject* self) { ((void (*)(VuSub*))self->sub_0x40->vt->slot_028)(self->sub_0x40); }

/* ---------------------------------------------------------------------------------------------------
 * guarded forwarders: `if (state < 1 || state >= 3) return; <wrapped>->vt->slot(...)`
 * ------------------------------------------------------------------------------------------------ */
void fn_80563054(VuObject* self)
{
    if (self->field_0x44 >= 3)
        return;
    if (self->field_0x44 < 1)
        return;
    ((void (*)(VuSub*))self->sub_0x1C->vt->slot_258)(self->sub_0x1C);
}

void fn_80563080(VuObject* self)
{
    if (self->field_0x44 >= 3)
        return;
    if (self->field_0x44 < 1)
        return;
    ((void (*)(VuSub*))self->sub_0x1C->vt->slot_25C)(self->sub_0x1C);
}

/* The nibble dispatcher: tail-calls the registered neighbour according to bits 4..7 of +0x48. */
void fn_8056329C(VuObject* self)
{
    switch ((self->field_0x48 >> 4) & 0xF) {
    case 1:
        fn_805632BC(self);
        break;
    default:
        fn_805634A4(self);
        break;
    }
}

/* A byte-flag setter that dispatches on the new value. */
void fn_8055DCD0(VuObject* self, u8 flag)
{
    self->field_0x3C = flag;
    if (flag == 1)
        ((void (*)(VuObject*))self->vt->slot_0D0)(self);
    else
        ((void (*)(VuObject*))self->vt->slot_0CC)(self);
}

/* Three chained virtual calls: slot 0x68 -> slot 0xE0 -> slot 0x13C(a, b). */
void fn_8055E7C0(VuObject* self, void* a, void* b)
{
    VuObject* first = (VuObject*)((void* (*)(VuObject*))self->vt->slot_068)(self);
    VuObject* second = (VuObject*)((void* (*)(VuObject*))first->vt->slot_0E0)(first);

    ((void (*)(VuObject*, void*, void*))second->vt->slot_13C)(second, a, b);
}

/* The same three-call chain through slot 0x138 instead of 0x13C. */
void fn_8055D300(VuObject* self, void* a, void* b)
{
    VuObject* first = (VuObject*)((void* (*)(VuObject*))self->vt->slot_068)(self);
    VuObject* second = (VuObject*)((void* (*)(VuObject*))first->vt->slot_0E0)(first);

    ((void (*)(VuObject*, void*, void*))second->vt->slot_138)(second, a, b);
}

/* A plain deleting destructor with no base-destructor call. */
void* fn_8055E59C(void* self, s32 flag)
{
    if (self != 0) {
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void* fn_8055CD20(void* self, s32 flag)
{
    if (self != 0) {
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

/* A deleting destructor whose base sub-object sits at +4 of the object. */
void* fn_80560B68(VuObject* self, s32 flag)
{
    if (self != 0) {
        fn_8055B7D4((void*)&self->list_0x04, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

/* Event forwarders: a state change pushes the wrapped object's value through a virtual slot. */
void fn_80561018(VuObject* self, s32 state)
{
    if (state != 1)
        return;
    void* value = ((void* (*)(VuSub*))self->sub_0x40->vt->slot_00C)(self->sub_0x40);
    ((void (*)(VuObject*, void*))self->vt->slot_0EC)(self, value);
}

void fn_80562244(VuObject* self)
{
    ((void (*)(VuObject*))self->vt->slot_114)(self);
    self->field_0x44 = 0xB;
    ((void (*)(VuSub*, s32))self->sub_0x1C->vt->slot_23C)(self->sub_0x1C, 1);
}

void fn_80562BFC(VuObject* self)
{
    ((void (*)(VuObject*))self->vt->slot_114)(self);
    self->field_0x44 = 8;
    ((void (*)(VuSub*, s32))((VuSub*)self->field_0x18)->vt->slot_104)((VuSub*)self->field_0x18, 1);
}

void fn_80562C54(VuObject* self)
{
    ((void (*)(VuObject*))self->vt->slot_128)(self);
    self->field_0x44 = 9;
}

void fn_80562FE4(VuObject* self)
{
    ((void (*)(VuObject*))self->vt->slot_100)(self);
    self->field_0x44 = 0xD;
    ((void (*)(VuSub*, s32))((VuSub*)self->field_0x18)->vt->slot_108)((VuSub*)self->field_0x18, 0);
    ((void (*)(VuSub*, s32))((VuSub*)self->field_0x14)->vt->slot_120)((VuSub*)self->field_0x14, 0);
}

/* Two factories: allocate a sub-object from the owner's allocator, then build or construct it. */
VuHeader* fn_8055DF3C(VuOwner* self)
{
    VuHeader* obj = (VuHeader*)MEMAllocFromAllocator(self->allocator, 0x14);

    if (obj != 0) {
        obj->vt = (void*)lbl_8064F0B0;
        obj->field_0x04 = 0;
        obj->field_0x08 = 0;
        obj->field_0x0C = 0;
    }
    return obj;
}

void fn_8055DF88(VuOwner* self)
{
    void* obj = MEMAllocFromAllocator(self->allocator, 0x28);

    if (obj != 0)
        fn_8056D814(obj, self);
}

/* Cubic Hermite interpolation of the band's shaped-value helpers. */
f32 fn_8055F650(f32 a, f32 b, f32 c, f32 d, f32 e, f32 f, f32 g)
{
    f32 t = (a - b) / (e - b);
    f32 t2 = t * t;
    f32 two_t = t + t;
    f32 base = t * d;
    f32 u = t2 - t;
    f32 v = d * u;
    f32 w = two_t * u;
    f32 x = g * u;

    v = d + v;
    w = w - t2;
    v = v + x;
    x = w * f;
    base = base - v;
    x = c + x;
    base = (a - b) * base;
    x = x - base;
    return x;
}

/* ---------------------------------------------------------------------------------------------------
 * chained-sub-object calls
 * ------------------------------------------------------------------------------------------------ */
void fn_8056083C(VuObject* self, VuSub* arg)
{
    self->field_0x0C = arg;
    ((void (*)(VuSub*, VuObject*))arg->vt->slot_01C)(arg, self);
}

void fn_8056085C(VuObject* self, void* a, void* b)
{
    VuSub* node = (VuSub*)fn_80501C60(&self->list_0x04, 0);

    while (node != 0) {
        ((void (*)(VuSub*, void*, void*))node->vt->slot_01C)(node, a, b);
        node = (VuSub*)fn_80501C60(&self->list_0x04, node);
    }
}

/* ---------------------------------------------------------------------------------------------------
 * stores into the unit's small-data globals and the float pools
 * ------------------------------------------------------------------------------------------------ */
void fn_8056167C(f32 value) { lbl_807946D8[1] = value; }

void fn_8055F6A0(VuVec4* self)
{
    f32 a = lbl_8079D720;
    f32 b = lbl_8079D724;
    f32 c = lbl_8079D728;
    f32 d = lbl_8079D72C;

    self->x = a;
    self->z = b;
    self->w = c;
    self->y = d;
}

void fn_8055F6C4(VuVec4* self)
{
    f32 a = lbl_8079D730;
    f32 b = lbl_8079D734;
    f32 c = lbl_8079D728;
    f32 d = lbl_8079D72C;

    self->x = a;
    self->z = b;
    self->w = c;
    self->y = d;
}

/* ---------------------------------------------------------------------------------------------------
 * deleting destructors: `if (self && flag > 0) delete self;` returns the object (the compiler's own
 * deleting-destructor shape, dispatching into the class's non-deleting destructor first)
 * ------------------------------------------------------------------------------------------------ */
void* fn_8055E840(void* self, s32 flag)
{
    if (self != 0) {
        fn_805385D0(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void* fn_8055E898(void* self, s32 flag)
{
    if (self != 0) {
        fn_8053F65C(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void* fn_8055E8F0(void* self, s32 flag)
{
    if (self != 0) {
        fn_8054F90C(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void* fn_8055E948(void* self, s32 flag)
{
    if (self != 0) {
        fn_8054CF38(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void* fn_8055E9A0(void* self, s32 flag)
{
    if (self != 0) {
        fn_80554714(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void* fn_8055E9F8(void* self, s32 flag)
{
    if (self != 0) {
        fn_80555B0C(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void* fn_8055EA50(void* self, s32 flag)
{
    if (self != 0) {
        fn_80557228(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

/* ---------------------------------------------------------------------------------------------------
 * string/predicate helpers of the band's tail (the pool +0x8055F4D8..0x8055F5E8)
 * ------------------------------------------------------------------------------------------------ */
s32 fn_8055F4D8(void* self)
{
    u32* base = lbl_80795A78;
    return fn_80463C1C((void*)base[1], self) != 0;
}

s32 fn_8055F510(void* self)
{
    u32* base = lbl_80795A80;
    return fn_80463C1C((void*)base[1], self) != 0;
}

s32 fn_8055F548(void* self)
{
    u32* base = lbl_80795A88;
    return fn_80463C1C((void*)base[1], self) != 0;
}

s32 fn_8055F580(const char* a, const char* b)
{
    u32 n = (u32)strlen(a);

    if (n >= 0x10)
        n = 0x10;
    if (strncmp(a, b, n) == 0)
        return 1;
    return 0;
}

void fn_8055F5E8(char* dst, u32 n, const char* src, u32 idx, char ch)
{
    memset(dst, 0, n);
    strncpy(dst, src, n);
    dst[idx] = ch;
}

} /* extern "C" */
