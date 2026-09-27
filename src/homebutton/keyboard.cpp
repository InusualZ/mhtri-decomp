/*
 * homebutton/keyboard.cpp - the software-keyboard band at the head of the home-button GUI area.
 *
 * `.text` 0x805632BC-0x80569DAC (142 functions, 27376 B).  This is one maximal unclaimed run
 * (docs/plan.md 12).
 *
 * SEAM (unproven, and the brief says so): the left edge is a `--max-bytes` cap, not a translation-unit
 * boundary.  The evidence is that the code just before 0x805632BC references the *same* data pool as
 * this range: the tail of the previous retired split run (`auto_03_8056083C_text`, its functions
 * fn_805617B8/fn_805626DC) loads `lbl_80657698` ("WiiBitmapFontType2.brfnt", "fs_VK_textBox_a.brlyt")
 * and `lbl_80657F10`, exactly as this range's own functions do, so the retail translation unit
 * continues to the left and this registration is the upper slice of it.  The right edge *is* a real
 * boundary: 0x80569DAC is the first address of the registered `homebutton/gui.cpp`.
 * `tudiscover.py at 0x805632BC` (run in MAIN, whose asm dump the tool needs) reports only weak left
 * cuts (0x80561074/0x80561528/0x8056229C, "codegen fingerprint change", share 0.22) and no must-link
 * anchors - no cut it will stand behind.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>` for every fn_8056 address of the range: every one
 * answers a bare `zz_<addr>_`/`FUN_<addr>` placeholder or an unrelated J3D/JSU name - the dump's map
 * resolves by nearest symbol and names no function of this band).
 *
 * Name evidence (docs/plan.md 12): class 3 - what the code does plus the naming scheme of its
 * neighbours.  The unit's own `.data` pool is the keyboard layout vocabulary of the home-button menu
 * (`.data` 0x80657698 "WiiBitmapFontType2.brfnt" / "fs_VK_textBox_a.brlyt" / "fs_VK_bg_a.brlyt",
 * 0x80657714 "T_hiragana" / "B_hiragana" / "T_katakana" / "P_dakuten" / "B_Gkey_handaku" /
 * "T_hankaku" / "T_zenkaku" / "T_Mode_roma_hira" / "B_Mode_roma_kata", 0x80657ba8 "N_Header" /
 * "N_Footer" / "T_Nigaoe" / "B_Nigaoe" / "T_Letter" / "T_TouchLetter", 0x80657d9c
 * "P_txtScrll_UP" / "P_txtScrll_DOWN" / "N_txt_scrl" / "N_TopBtn_00" / "B_txtScrll_UP" /
 * "B_txtScrll_DOWN", 0x80657c90 "T_2l_TextBox" / "B_2l_TextBox" / "N_MemoRoot" / "G_ArwRoop" /
 * "G_ArwL_HDAc") - a Japanese/roman software keyboard with a text box, a memo root, a portrait pane
 * and scroll arrows.  The module is `homebutton`: the immediately following registered unit is
 * `homebutton/gui.cpp` (0x80569DAC, documented as the homebutton::gui band on dump evidence and on
 * config/RMHE08/hbm_data/symbols.txt, the HomeButton RSO export table), and the band after it is
 * `tiHKBManager.cpp`, the home-button keyboard ("HKB") manager.  No `__FILE__` string is emitted
 * anywhere in the range (no `<name>.cpp`/`<name>.c` string is referenced by any function of it), so
 * class 1 does not apply and the name is descriptive; the file name `keyboard.cpp` is this unit's
 * share of the retail keyboard translation unit (see SEAM above).
 *
 * Sections claimed: `.text` 0x805632BC-0x80569DAC only.  The unit's `.data` (the vtables at
 * 0x80657798/0x806578c0/0x80657970) and its `.sdata`/`.sdata2` (the static objects `lbl_80795A90`,
 * the float pool `lbl_8079D738`..) live in other splits, so they are referenced here as the map's
 * `lbl_` symbols and never re-emitted (rule 10 / rule 2).
 *
 * Exceptions: the retired target objects (`auto_03_8056083C_text.o`, `auto_fn_80566340_text.o`,
 * `auto_03_80566440_text.o`) carry no extab/extabindex, so the retail object was built with
 * exceptions off while cflags_main turns them on; the deviation is scoped to this file with a
 * `#pragma exceptions off` (the same call as the neighbouring `tiHKBManager.cpp`).
 *
 * Reconstruction status (measured with `tools/units/recompile.py homebutton/keyboard --measure <sym>`,
 * the official report metric, against MAIN's retired split objects `auto_03_8056083C_text.o`,
 * `auto_fn_80566340_text.o` and `auto_03_80566440_text.o`): 71 of the range's 142 functions have a body,
 * 38 of them byte-identical, and the unit measures 7.140 % (1954.6 of 27376 `.text` bytes).  The
 * per-symbol table is in the outbox .pi/outbox/805632bc-fn-805632bc-74c1.json.  The unit is far short of
 * the 80 % bar: the 71 bodies written this session are the mechanical half of the band (the no-ops, the
 * constant returns, the accessors, the tail-call thunks, the small virtual forwarders, the pane/text
 * lookups and the deleting destructors) and the band's byte mass sits in the large bodies that are not
 * attempted yet (fn_8056802C 0x9D4, fn_80563E50 0x6A8, fn_805655C4 0x604, fn_80565BC8 0x550,
 * fn_80564D54 0x534, fn_805638D4 0x518, fn_805632BC 0x1E8, ...) - the 71 bodies written here cover
 * 2396 B of the range and the 71 functions without a body cover 24980 B.
 *
 * RESIDUALS (what still differs and why):
 *   - **The virtual-call register is the unit's systematic residual.**  A virtual call's vtable pointer
 *     is materialised into a scratch GPR here (`lwz r4, 0(r3)` / `lwz r5, 0(r3)`) where the retail object
 *     uses r12 (`lwz r12, 0(r3)`), because the conformant spelling of a call through a slot outside our
 *     ranges is a struct of function pointers (rule 10) and MWCC only uses the r12 idiom for a *virtual*
 *     call on a declared class.  That is one instruction per call site: the nine 0x14-byte forwarders
 *     (fn_80563614 fn_805636A4 fn_80563DEC fn_80563E00 fn_80563E14 fn_80563E28 fn_80564A2C fn_80564A40
 *     fn_805655B0) measure 98.00 % for that one reason, and every larger body carries the same penalty at
 *     each of its call sites.  The neighbour `homebutton/gui.cpp` records the identical residual.
 *   - fn_80563740 58.59 %, fn_80563834 69.78 % - the two switch dispatchers, sizes exact.  Their bodies
 *     call `self->slot34()` *twice* in the retail object (the first call's result is discarded) and the
 *     stack tuple fn_80563740 hands to slot 0x224 is stored in the retail order; the source here spells
 *     that as two statements, so the difference left is which GPR each load lands in (the residual above)
 *     plus the order the argument tuple's stores are scheduled in.
 *   - fn_80568004 69.50 % - the `frsp`+`fneg` pair: the retail object converts before negating
 *     (`frsp f0,f1; fneg f0,f0`), our spelling (`out = -value;`) negates directly.  Four spellings tried
 *     (`-value`, `-(f32)value`, `0.0f - value`, a local); the size stays 40 B either way.
 *   - fn_80568DD0 69.86 % - the two-float return: sizes exact (56 B); the retail object materialises the
 *     constant pair on the stack and reloads it as two words, ours keeps the pair in the return
 *     registers - a source-shape residual in how the `Vec2f` temporary is initialised.
 *   - fn_8056931C 45.60 % - the pane lookup: the retail object computes the `lis/addi` of the pane name
 *     *before* the prologue's saves and keeps the intermediate in r3/r4, ours loads it into r4/r5 and
 *     schedules it later; the sequence of the four loads/calls is otherwise identical.
 *   - fn_8056945C/fn_80569470 58.00/45.11 % - the two `lbl_80794738` lookups: the `.sdata` string is
 *     reached with `li r4, lbl_80794738@sda21` in retail, which needs the symbol declared with a size
 *     inside the sdata threshold (`extern char lbl_80794738[8]` - measured: 55.0/37.7 % as an unsized
 *     `char[]`, 58.0/45.1 % with the size, i.e. the `@sda21` reloc is reached but the surrounding load
 *     order still differs).
 *   - fn_80563630 83.72 %, fn_805636BC 95.39 %, fn_80568C60 83.89 %, fn_80568DBC 88.00 %,
 *     fn_805694C0 83.85 %, fn_8056985C 87.17 %, fn_80569910 78.83 %, fn_80569970 93.00 %: sizes exact;
 *     the shortfall is the register residual above plus the source shape of the guard (retail compares
 *     the state word with `cmpwi`, i.e. the field is signed - modelled as `s32 field_0x1E4`).
 *   - the five deleting destructors (fn_805661EC fn_8056622C fn_80566280 fn_805662C0 fn_80566300) and the
 *     two stream destructors (fn_80569BC8 fn_80569C28) measure 87.50 / 81.74 %: sizes exact (64/92 B),
 *     and the residual is MWCC scheduling `cmpwi r3,0` above the prologue's stores in retail where our
 *     `if (self != 0 && flag > 0)` spelling schedules it after them.
 *   - the 14 this-adjusting thunks fn_80569D3C/D44/D4C/D54/D5C/D64/D6C/D74/D7C/D84/D8C/D94/D9C/DA4
 *     (`subi r3,r3,0x10|0x1A04; b <victim>`) are not written: they are MWCC's own output for a virtual
 *     inherited from a base sub-object, and the conformant way to get them (declare the class with
 *     `virtual`) would make MWCC emit the vtable into this object (rule 10), while writing them as a
 *     hand pointer adjustment is rule 6.  Same call as the neighbour `homebutton/gui.cpp` records for
 *     its three `addi r3,r3,-4` thunks.
 *   - the remaining 71 functions have no body yet (0 %, unpaired): see the outbox for the list.  The
 *     largest are fn_8056802C (0x9D4), fn_80563E50 (0x6A8), fn_805655C4 (0x604), fn_80565BC8 (0x550),
 *     fn_80564D54 (0x534), fn_805638D4 (0x518), fn_80566C94 (0x370), fn_80567A14 (0x328),
 *     fn_80565288 (0x328), fn_80564A54 (0x27C), fn_8056725C (0x268), fn_80568A00 (0x260),
 *     fn_805647FC (0x230), fn_80566A7C (0x218), fn_8056999C (0x1FC), fn_805632BC (0x1E8).
 */

#include "types.h"

/* The retail object was built with exceptions off (no extab/extabindex in any of its sections). */
#pragma exceptions off

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

extern "C" {
void* fn_8055BBAC(void* p);
void* fn_8054E05C(void* p);
void* fn_8055C638(void* p);
void* fn_80547BE8(void* p);
void* fn_8055B174(void* a, void* b);
void  fn_8055C494(void* self, s32 flag);
void  fn_80546BCC(void* self, void* arg);
void  fn_8055A888(void* p, void* arg);
}

/* The compiler's own operator delete: the retail object jumps to `__dl__FPv`. */
void operator delete(void* ptr) throw();

/* Character/keystroke-name strings the keyboard searches its layout with (the two `.sdata` tables at
 * 0x80794728/0x80794738) and the pane name `T_Header` the memo button is looked up by. */
extern "C" {
extern char lbl_80794728[8];
extern char lbl_80794738[8];
extern char lbl_80657E14[];
}

/* The unit's own `.sdata2` pool: another split owns it, so it is declared, never defined
 * (playbook 29). */
extern "C" {
extern f32 lbl_8079D750;
extern f32 lbl_8079D754;
extern f32 lbl_8079D774;
extern f32 lbl_8079D784;
}

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

extern "C" {

/* ---------------------------------------------------------------------------------------------------
 * no-op virtuals: the retail body is a bare `blr`
 * ------------------------------------------------------------------------------------------------ */
void fn_80563610(KbObject* self) { (void)self; }
void fn_805636B8(KbObject* self) { (void)self; }
void fn_80563E48(KbObject* self) { (void)self; }
void fn_80563E4C(KbObject* self) { (void)self; }
void fn_805647F4(KbObject* self) { (void)self; }
void fn_805647F8(KbObject* self) { (void)self; }
void fn_805661C8(KbObject* self) { (void)self; }
void fn_805661CC(KbObject* self) { (void)self; }
void fn_805661D0(KbObject* self) { (void)self; }
void fn_805661D4(KbObject* self) { (void)self; }
void fn_805661E0(KbObject* self) { (void)self; }
void fn_80566274(KbObject* self) { (void)self; }
void fn_80569998(KbObject* self) { (void)self; }
void fn_80569B9C(KbObject* self) { (void)self; }
void fn_80569BA0(KbObject* self) { (void)self; }
void fn_80569C24(KbObject* self) { (void)self; }

/* ---------------------------------------------------------------------------------------------------
 * constant returns
 * ------------------------------------------------------------------------------------------------ */
s32 fn_80564770(KbObject* self) { (void)self; return 0; }
s32 fn_80564778(KbObject* self) { (void)self; return 0; }
s32 fn_805661D8(KbObject* self) { (void)self; return 3; }
s32 fn_805661E4(KbObject* self) { (void)self; return 2; }
s32 fn_8056626C(KbObject* self) { (void)self; return 1; }
s32 fn_80566278(KbObject* self) { (void)self; return 0; }

/* ---------------------------------------------------------------------------------------------------
 * accessors
 * ------------------------------------------------------------------------------------------------ */
void* fn_80563628(KbObject* self) { return self->field_0x44; }

void* fn_80563E3C(KbObject* self) { return self->sub_0x04->field_0x78; }

void* fn_80569BA4(KbObject* self) { return self->field_0x2C; }
void* fn_80569BB4(KbObject* self) { return self->field_0x30; }
void  fn_80569BBC(KbObject* self) { self->field_0x2C = 0; }
void  fn_80569C94(KbObject* self, void* value) { self->tail_0x1A04.field_0x1C8 = value; }

f32 fn_805644F8(KbObject* self) { return self->tail_0x1A04.float_0x104; }
f32 fn_80564500(KbObject* self) { return self->tail_0x1A04.float_0x108; }
f32 fn_80569C9C(KbObject* self) { return self->tail_0x1A04.float_0x100; }
f32 fn_805694B8(KbObject* self) { (void)self; return lbl_8079D754; }

void* fn_80567004(KbObject* self, void* value)
{
    void* old = self->sub_0x04;
    self->sub_0x04 = (Wrapped*)value;
    return old;
}

/* ---------------------------------------------------------------------------------------------------
 * tail-call thunks: the retail body is `b <victim>` (the arguments pass through untouched)
 * ------------------------------------------------------------------------------------------------ */
void fn_805646E4(KbObject* self) { fn_8055BBAC(&self->sub_0x04); }
void fn_80567D3C(void* p) { fn_8054E05C(p); }
void fn_80569B98(void* p) { fn_8055C638(p); }
void fn_80569BAC(KbObject* self, void* value)
{
    self->field_0x2C = value;
    fn_8055C638(self);
}

/* ---------------------------------------------------------------------------------------------------
 * small virtual forwarders: `return self->sub->vt->slot(...)`
 * ------------------------------------------------------------------------------------------------ */
void fn_80563614(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_0F0)(self->sub_0x04); }
void fn_805636A4(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_068)(self->sub_0x04); }
void fn_80563DEC(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_078)(self->sub_0x04); }
void fn_80563E00(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_080)(self->sub_0x04); }
void fn_80563E14(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_090)(self->sub_0x04); }
void fn_80563E28(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_098)(self->sub_0x04); }
void fn_80564A2C(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_0A0)(self->sub_0x04); }
void fn_80564A40(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_088)(self->sub_0x04); }
void fn_805655B0(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_070)(self->sub_0x04); }

/* ---------------------------------------------------------------------------------------------------
 * state dispatchers: `switch (((KbStateFn)self->sub_0x04->vt->slot_0F0)()) { case 1: case 2: <body> }`
 * ------------------------------------------------------------------------------------------------ */
void fn_80563630(KbObject* self)
{
    switch (((KbStateFn)self->sub_0x04->vt->slot_0F0)()) {
    case 1:
    case 2:
        ((Wrapped* (*)(void))self->vt->slot_034)()->vt->slot_14C();
        break;
    }
}

void fn_805636BC(KbObject* self)
{
    if (((KbStateFn)self->sub_0x04->vt->slot_0F0)() == 1 || ((KbStateFn)self->sub_0x04->vt->slot_0F0)() == 2)
        ((Wrapped* (*)(void))self->vt->slot_034)()->vt->slot_148();
}

void fn_80563834(KbObject* self, void* value)
{
    switch (((KbStateFn)self->sub_0x04->vt->slot_0F0)()) {
    case 1:
    case 2:
        Wrapped* receiver;
        self->vt->slot_034();
        receiver = ((Wrapped* (*)(void))self->vt->slot_034)();
        ((void (*)(Wrapped*, void*))receiver->vt->slot_228)(receiver, value);
        break;
    }
}

s32 fn_80563740(KbObject* self, s32 a, f32 x, f32 y, s32 b, s32 c, s32 d)
{
    s32 ret;

    switch (((KbStateFn)self->sub_0x04->vt->slot_0F0)()) {
    case 1:
    case 2: {
        /* the retail object hands the callee a copy of the argument tuple as the 8th argument */
        KbArgs args;
        self->vt->slot_034();
        args.a = a;
        args.x = x;
        args.y = y;
        args.b = b;
        args.c = c;
        args.d = d;
        ret = (s32)((s32 (*)(f32, f32, s32, s32, s32, s32, KbArgs*))self->vt->slot_224)(x, y, a, b, c, d, &args);
        break;
    }
    default:
        ret = 0;
        break;
    }
    return ret;
}

/* ---------------------------------------------------------------------------------------------------
 * guarded forwarders: `if (self->tail_0x1A04.field_0x1E4 != 1) return; <wrapped>->vt->slot(...)`
 * ------------------------------------------------------------------------------------------------ */
void fn_8056772C(KbObject* self)
{
    if (self->tail_0x1A04.field_0x1E4 != 1)
        return;
    ((void (*)(Wrapped*, s32, s32))self->tail_0x1A04.field_0x1DC->vt->slot_014)(self->tail_0x1A04.field_0x1DC, 0, 7);
}

void fn_805677C0(KbObject* self)
{
    if (self->tail_0x1A04.field_0x1E4 != 1)
        return;
    ((void (*)(Wrapped*, s32, s32))self->tail_0x1A04.field_0x1DC->vt->slot_014)(self->tail_0x1A04.field_0x1DC, 1, 7);
}

void fn_80569970(KbObject* self)
{
    if (self->tail_0x1A04.field_0x1E4 != 1)
        return;
    ((void (*)(Wrapped*, s32))self->tail_0x1A04.field_0x1D0->vt->slot_010)(self->tail_0x1A04.field_0x1D0, 0);
}

void fn_80568DBC(KbObject* self)
{
    if (self->tail_0x1A04.field_0x1E4 != 1)
        fn_80547BE8(self);
}

/* ---------------------------------------------------------------------------------------------------
 * the 0x1B04.. key-frame block
 * ------------------------------------------------------------------------------------------------ */
void fn_80568004(KbObject* self, f32 value)
{
    f32 out;

    self->tail_0x1A04.float_0x100 = value;
    if (self->tail_0x1A04.field_0x1E4 == 2)
        out = -value;
    else
        out = lbl_8079D754;
    self->float_0x100 = out;
}

void fn_80568C60(KbObject* self)
{
    if (self->tail_0x1A04.float_0x110 == self->tail_0x1A04.float_0x118)
        self->tail_0x1A04.float_0x118 = self->tail_0x1A04.float_0x118 + lbl_8079D774;
    if (self->tail_0x1A04.float_0x114 == self->tail_0x1A04.float_0x11C)
        self->tail_0x1A04.float_0x114 = self->tail_0x1A04.float_0x114 + lbl_8079D774;
    (void)fn_8055B174(&self->field_0x010, &self->tail_0x1A04.float_0x110);
}

/* The 2D anchored position of the keyboard's cursor: the object's own pair when one is attached, the
 * shared constant pair otherwise. */
Vec2f fn_80568DD0(KbObject* self)
{
    Vec2f out;

    if (self->tail_0x1A04.field_0x0F4 == 0) {
        out.x = lbl_8079D750;
        out.y = lbl_8079D750;
        return out;
    }
    out.x = self->tail_0x1A04.field_0x0F4->float_0x44;
    out.y = self->tail_0x1A04.field_0x0F4->float_0x48;
    return out;
}

/* ---------------------------------------------------------------------------------------------------
 * text-search wrappers: the character name (lbl_80794738) is looked up in the keyboard's own list
 * ------------------------------------------------------------------------------------------------ */
void fn_8056945C(KbObject* self)
{
    KbTail* base = &self->tail_0x1A04;

    ((void (*)(KbTail*, char*))base->vt->slot_02C)(base, lbl_80794738);
}

void fn_80569470(KbObject* self)
{
    KbTail* base = &self->tail_0x1A04;

    ((void (*)(KbTail*, char*))base->vt->slot_02C)(base, lbl_80794738);
    ((void (*)(KbTail*, char*, s32))base->vt->slot_040)(base, lbl_80794738, 1);
}

f32 fn_805694C0(KbObject* self)
{
    f32 frame = ((f32 (*)(KbObject*))self->vt->slot_2A4)(self);
    return frame - lbl_8079D784;
}

/* ---------------------------------------------------------------------------------------------------
 * the five deleting destructors of the animation-widget classes: `if (self && flag > 0) delete self;`
 * returns the object (the retail body is the compiler's own deleting-destructor shape)
 * ------------------------------------------------------------------------------------------------ */
void* fn_805661EC(void* self, s32 flag)
{
    if (self != 0 && flag > 0)
        operator delete(self);
    return self;
}

void* fn_8056622C(void* self, s32 flag)
{
    if (self != 0 && flag > 0)
        operator delete(self);
    return self;
}

void* fn_80566280(void* self, s32 flag)
{
    if (self != 0 && flag > 0)
        operator delete(self);
    return self;
}

void* fn_805662C0(void* self, s32 flag)
{
    if (self != 0 && flag > 0)
        operator delete(self);
    return self;
}

void* fn_80566300(void* self, s32 flag)
{
    if (self != 0 && flag > 0)
        operator delete(self);
    return self;
}

/* The two file-input-stream destructors: the inner check is the class's own non-deleting destructor,
 * which the retail object keeps as a second `if (this != 0)`. */
void* fn_80569BC8(void* self, s32 flag)
{
    if (self != 0) {
        if (self != 0)
            fn_8055C494(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void* fn_80569C28(void* self, s32 flag)
{
    if (self != 0) {
        if (self != 0)
            fn_8055C494(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

/* ---------------------------------------------------------------------------------------------------
 * the keyboard's pane/text lookups (the layout vocabulary of the `.data` pool)
 * ------------------------------------------------------------------------------------------------ */
void fn_805692DC(KbObject* self)
{
    KbTail* found = (KbTail*)((void* (*)(KbTail*, char*))self->tail_0x1A04.vt->slot_05C)(&self->tail_0x1A04, lbl_80794728);

    ((void (*)(KbTail*))found->vt->slot_018)(found);
}

void fn_8056931C(KbObject* self, void* arg)
{
    Wrapped* pane = (Wrapped*)((void* (*)(Wrapped*, char*, s32))self->tail_0x1A04.field_0x004->field_0x10->vt->slot_03C)(self->tail_0x1A04.field_0x004->field_0x10, lbl_80657E14, 1);

    ((void (*)(void*, void*, s32))pane->vt->slot_07C)(pane, arg, 0);
}

void fn_8056985C(KbObject* self, void* arg)
{
    if (self->tail_0x1A04.field_0x1E4 == 1)
        ((void (*)(Wrapped*))self->tail_0x1A04.field_0x1D0->vt->slot_018)(self->tail_0x1A04.field_0x1D0);
    fn_80546BCC(self, arg);
}

void fn_80569910(KbObject* self, void* arg)
{
    if (self->tail_0x1A04.field_0x1E4 == 1)
        ((void (*)(Wrapped*))self->tail_0x1A04.field_0x1D0->vt->slot_010)(self->tail_0x1A04.field_0x1D0);
    fn_8055A888(&self->field_0x010, arg);
}

} /* extern "C" */
