# Homebutton: notes carried from the former registered units

Phase 4 window fg split the registered units `fn_805482CC` (kept whole in `hbm_text_panel.cpp`), `homebutton/fn_8054E894`, `homebutton/fn_80555374`,
`homebutton/keyboard_ui`, `homebutton/keyboard`, `homebutton/gui` and `tiHKBManager` over the units of `src/homebutton/`. Their file-header comments (evidence, seam
reasoning, residuals, per-function notes) are kept verbatim below; the new unit headers point here. Addresses and function names are the old ones.

## `homebutton/fn_8054E894.cpp` (0x8054E894..0x80555374), now in homebutton/hbm_text_panel.cpp, hbm_kb_widget.cpp, hbm_kb_list.cpp

```text
homebutton software-keyboard band, slice 0x8054E894-0x80555374 (199 functions / 27360 B) plus the
two `.ctors` words at 0x8056F3FC-0x8056F404 (the static initialisers fn_8054F550 / fn_80554600).

Evidence for module and name (brief section 2 classes): no `__FILE__` string covers the range and
the shared runtime dump answers only `zz_054e894_`/`FUN_` placeholders (class 4), so the file keeps
the symbol map's `fn_8054E894` stem.  The module is evidence class 3: the range's `.data`
vocabulary is the home-button software-keyboard layout pool the registered siblings
`homebutton/keyboard.cpp` / `homebutton/keyboard_ui.cpp` / `homebutton/fn_80555374.cpp` document
(`fs_VK_*.brlyt`, `T_2l_TextBox`, `P_txtScrll_UP/DOWN`, `T_prdc_Text_00..19`, `B_CPkey_00..11`,
`P_SGNkey_00..19`, `P_key_00..49`), its right edge is the registered `homebutton/fn_80555374.cpp`,
and its vtables point into the sibling bands.  Language C++ from the object's own structure
(adjustor thunks `addi r3, r3, -0x04/-0x10/-0x14/-0x17F4/-0x189C/-0x1A04`, the deleting
destructors' `__dl__FPv`, the `.ctors` words).

Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
`python tools/symbols/dumpmap.py lookup 0x8054E894` -> `zz_054e894_` and a `__FILE__` scan of the
range's `.data` pool 0x8064E760-0x8064F600, which holds only layout/pane names).

Seam: unproven, and the range is *not* one object.  Both edges are the brief's `--max-bytes` cut
(the left neighbour fn_8054E858 ends exactly at 0x8054E894, the right edge is the registered
`homebutton/fn_80555374.cpp`), and dtk's own auto-object fragmentation places further object starts
inside it: `auto_fn_8054F550` (0x8054F550), `auto_03_8054F6AC`, `auto_fn_80554600` (0x80554600)
and `auto_03_80554664`.  The two `auto_fn_*` boundaries coincide with the `.ctors` words at
0x8056F3FC / 0x8056F400, so the run carries at least five objects.  It is registered as one unit
because that is what the campaign's proposal is; the boundaries are recorded here as a hint for
whoever splits it, and they are why the views in the header are declared per function group.

Residuals:
  - partial reconstruction: the bodies below are the ones proved from the disassembly; the rest of
    the band is still the target object's bytes (see the outbox for the inventory of what is left).
  - the classes are *views* (header): the band spans more than one class and one offset is reused
    with two meanings, so a body that needs the other meaning gets its own view struct.
  - the widget is declared as a polymorphic class in the header purely for its *dispatch* shape: a
    struct of function pointers stages the table through a temporary, a `virtual` declaration gives
    retail's `lwz r12, 0(r3); lwz r12, <slot>(r12)`.  No method is defined and nothing is
    instantiated, so MWCC emits no table into this object (rule 10); the tables belong to the units
    above.  A call through a *base* subobject (`lwzu r12, 24(r3)`) still needs real C++ bases and
    is the recorded blocker for fn_80553670 / fn_80554F18 / fn_80555014.
/* The band was built with C++ exceptions off: every registered target object of the home-button
keyboard block (`homebutton/fn_80555374.o`, `keyboard_ui.o`, `keyboard.o`, `tiHKBManager.o`, and
the `auto_*` objects this slice's symbols are split into) carries `.text` + `.comment` only, while
the `main` lib's `cflags` pass `-Cpp_exceptions on` for `main.cpp`'s benefit.  With the pass on our
object gains extab 0x80 + extabindex 0xC0 the target does not have; the pragma removes both and
leaves `.text` byte-identical (measured at the state it was probed in: 109/199 functions
exact, unit 10.0398 % with and without it).
Playbook row 30's per-file lever; the band-wide fix is a `cflags_homebutton` group (outbox).
```

## `homebutton/fn_80555374.cpp` (0x80555374..0x8055C894), now in homebutton/hbm_kb_list.cpp, hbm_kb_child.cpp, hbm_anim_record.cpp, hbm_value.cpp

```text
homebutton/fn_80555374.cpp - the 0x80555374-0x8055C894 band (213 functions, 29984 B) of the
home-button (Wii HOME menu overlay) software-keyboard block, plus its 3 `.ctors` words at
0x8056F404-0x8056F410.

Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
tools/symbols/dumpmap.py lookup for every row of the range - all `zz_`/`FUN_` placeholders - and
with a scan of the whole DOL's string pool: no bare source-file name is referenced by any
function of the range; the pools it does reference hold the software-keyboard layout vocabulary
`P_SGNkey_01`..`12`, `B_SGNkey_close` (0x8057BD18), `T_SGN_pageNumber`, `P_BT_cancel`,
`N_UP`/`N_DOWN` and `%d/%d`).

Language, seam and module (brief section 2):
  - C++: the band is free `fn_XXXXXXXX` functions operating on a class hierarchy only - the
    `.ctors` initialisers, the deleting-destructor forms (`__dl__FPv`) and the `subi r3, r3,
    0x14/0x1C/0x24/0xC4/0xCC` adjustor thunks in the band settle it; the lib's cflags carry
    exceptions, and the target object has `extab`/`extabindex`.
  - module `homebutton`: the right link neighbour is the registered `homebutton/keyboard_ui.cpp`
    (0x8055C894), the band calls into that range (fn_8055C968), and the data it references is the
    same software-keyboard layout pool that neighbour documents.  Evidence class 3.
  - the seam is unproven: discovery cut the run at `--max-bytes`, and
    `tools/splits/tudiscover.py at 0x80555374` finds no must-link anchor at either edge (its
    closure is the one function fn_80555374; its left candidates are weak codegen fingerprints at
    0x80554DC8/0x805547F4, its right ones at 0x8055664C/0x80556840).  The right edge is the
    registered `homebutton/keyboard_ui.cpp` boundary, so the run is worked as one unit.

Residuals:
  - partial reconstruction: the bodies below are the ones proved from the disassembly; the rest of
    the band is still the target object's bytes.
  - the layouts are *views* of the objects the band operates on (the band spans more than one
    class: a few bodies use one offset with two meanings, and each such body gets its own view).
    Sizes are marked approximate where the band never touches the object's tail.
  - the vtables the band dispatches through belong to another unit (`lbl_80650A48`, `.data`
    0x80650A48-0x80650B4C), so they are typed slot structs here - declaring the classes would make
    MWCC emit their tables into this object.
/
/* `offsetof` for this band's types (MWCC's `stddef.h` is off the include path): the adjustor thunks
below convert a base-subobject pointer back to the complete object with it.
```

## `homebutton/keyboard_ui.cpp` (0x8055C894..0x805632BC), now in homebutton/hbm_value.cpp, hbm_hermite.cpp, hbm_kb_event.cpp

```text
homebutton/keyboard_ui.cpp - the lower slice of the home-button software-keyboard band.

`.text` 0x8055C894-0x805632BC (133 functions, 27176 B).  This is one maximal unclaimed run
(docs/plan.md 12) between the unclaimed 0x80558E84 band below it and the registered
`homebutton/keyboard.cpp` (0x805632BC) above it.

SEAM (unproven, and the brief says so).  The left edge is a `--max-bytes` cap, not a translation-unit
boundary (the discovery note "capped at --max-bytes, seam is a guess").  `tudiscover.py at
0x8055C894`, run in MAIN whose asm dump the tool needs, reports the match set as the single function
fn_8055C894 with no must-link anchors and only weak cuts (left 0x8055B4A8/0x8055B534, share 0.108,
codegen-fingerprint change; right 0x8055D478, share 0.200) - no cut it will stand behind, exactly as
the neighbouring `homebutton/keyboard.cpp` records for its own left edge.  The right edge *is* a
registered boundary: 0x805632BC is the first address of `homebutton/keyboard.cpp`, and that unit's
header records the same evidence from the other side ("the code just before 0x805632BC references the
same data pool as this range ... the retail translation unit continues to the left").  The range is
therefore worked as one unit; its extent settles as its bodies match.

Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
`python tools/symbols/dumpmap.py lookup` over the range's addresses and with
`grep -n "fn_8055C\|fn_8055D\|fn_8055E\|fn_80560\|fn_80561\|fn_80562" config/RMHE08/symbols.txt`:
every defined name of the range is a bare `.text` entry, and the runtime dump answers a bare
`zz_<addr>_` placeholder for every one probed - the shared dump names no function of the band).

Name evidence (docs/plan.md 12, evidence class 3).  No `__FILE__` string is emitted anywhere in the
range (no `.cpp`/`.c` source-name string is referenced by any function of it), so class 1 does not
apply.  The module is `homebutton`: the unit's whole `.data` vocabulary is the home-button software
keyboard's layout pool - `0x80651870 "T_title_text"`, `0x80651880 "fs_VK_ascii_keytop_a.brlyt"`,
`0x806518a0 "fs_VK_cellPhone_a.brlyt"`, `0x806518b8 "WiiBitmapFontType1.brfnt"`,
`0x806518d4 "fs_VK_textBox_a.brlyt"`, `0x806518ec "fs_VK_predictInput_a.brlyt"`,
`0x80651908 "fs_VK_toolbar_a.brlyt"`, `0x80651920 "fs_prdicSelWidw_a.brlyt"`,
`0x80651938 "fs_signWindow_a.brlyt"`, `0x80652300 "T_2l_TextBox"`, `0x806576b4 "fs_VK_textBox_a.brlyt"`,
`0x806576e8 "fs_VK_textBox_b.brlyt"`, `0x80657700 "fs_VK_bg_a.brlyt"`, `0x807946f0 "B_abc"`,
`0x807946f8 "T_sign"`, `0x80794700 "B_sign"` - the same pool the registered `homebutton/keyboard.cpp`
(immediately above at 0x805632BC) and `homebutton/gui.cpp` (0x80569DAC) are documented to own.  The
link neighbour is `homebutton/keyboard.cpp`, so the unit takes the `main` lib and cflags_main.  The
file is the lower slice of that keyboard translation unit: `keyboard.cpp` is the upper slice, this is
the part below it (the name is descriptive, class 3; the symbols keep the map's `fn_` stem with the
rule-7 deferral above).

Sections claimed: `.text` 0x8055C894-0x805632BC plus the `.ctors` words 0x8056F410-0x8056F424 (the
splitter assigns that run to this unit).  The range's `.data`/`.sdata`/`.sdata2` pools
(the keyboard layout strings, the float pools `lbl_8079D720`..`lbl_8079D73C`, the small-data globals
`lbl_807946D8`, `lbl_80795A78`/`lbl_80795A90`) live in other splits and are referenced here as the
map's `lbl_` symbols, never re-emitted (rule 10 / rule 2).

Exceptions: the retired target objects (`auto_03_80558E84_text.o`, `auto_03_8055EAB8_text.o`,
`auto_03_8056083C_text.o`) carry no extab/extabindex (and the discovery record says
`exceptions_on: false`), so the retail object was built with exceptions off while cflags_main turns
them on; the deviation is scoped to this file with a `#pragma exceptions off` (the same call as the
neighbouring `homebutton/keyboard.cpp` and `tiHKBManager.cpp`).

Reconstruction status (measured with `tools/units/recompile.py homebutton/keyboard_ui.cpp --measure <sym>`,
the official report metric, against MAIN's retired split objects `auto_03_80558E84_text.o`,
`auto_03_8055EAB8_text.o` and `auto_03_8056083C_text.o`): 70 of the range's 133 functions have a
body and 63 of them measure >= 80 %.  The unit is far short of the 80 % bar: the 70 bodies written
here are the mechanical half of the band (the no-ops, the constant returns, the accessors, the
tail-call thunks, the virtual forwarders, the guarded forwarders, the factory/deleting destructors,
the dispatch helpers and the string predicates) and the band's byte mass sits in the large bodies
that are not attempted yet (fn_8055EC3C 0x464, fn_8055D594 0x378, fn_8055CFC8 0x338, fn_8055D90C
0x26C, fn_805608F8 0x270, fn_80560BC4 0x280, fn_8055CB34 0x1EC, fn_8055CD60 0x1F4, fn_8055D478
0x11C, fn_8055D36C 0x10C, fn_8055E288 0x1A4, fn_805617B8 0x520, fn_80561CD8 0x420, ...).  The
70 bodies cover 2892 B of the range (the unit measures 9.0428 %); the 63 functions without a body
cover 24284 B.  The per-symbol table is in the outbox .pi/outbox/8055c894-fn-8055c894-0851.json.

RESIDUALS (what still differs and why):
  - **The virtual-call register is the unit's systematic residual.**  A virtual call's vtable pointer
    is materialised into a scratch GPR here (`lwz r4, 0(r3)`) where the retail object uses r12
    (`lwz r12, 0(r3)`), because the conformant spelling of a call through a slot outside our ranges
    is a struct of function pointers (rule 10) and MWCC only uses the r12 idiom for a *virtual* call
    on a declared class.  That is one instruction per call site: the ten 0x14-byte forwarders
    (fn_8055DB78/DB8C/DBB8/DBCC/DBE0, fn_80560FB4/FC8/FDC/FF0/1004) measure 98.00 % for that one
    reason, and every larger body carries the same penalty at each of its call sites.  The
    neighbour `homebutton/keyboard.cpp` records the identical residual.
  - fn_8055F4D8 / fn_8055F510 / fn_8055F548 71.07 % - the three small-data predicates.  Sizes exact
    (56 B).  The target materialises the `.sdata` base into r5 (`li r5, lbl_80795A78@sda21; lwz r3,
    0x4(r5)`) before moving self into r4; our spelling materialises it into r3 after the move.  A
    local-pointer form was tried (the current source); it does not move the base to r5.
  - fn_8056329C 73.75 %, sizes 32 vs 24 B - the nibble dispatcher.  The target is MWCC's `switch`
    range shape (`cmpwi; beq case1; bge default; b default`); our `switch` with a single `case 1`
    is narrowed to `cmpwi; bne default; b fn_805632BC; b fn_805634A4`, losing the two range
    branches.  A `switch` and an `if/else` were both tried.
  - fn_8056085C 70.53 %, sizes exact (144 B) - the list walk.  The loop body and the prologue save
    order differ; the call's vtable load lands in a fresh GPR (`lwz r6, 0(r31)`) where the target
    reuses r12, and MWCC schedules the `mr r28,r3` / `stw r28,0x10(r1)` pair differently.
  - fn_8055DF3C 72.63 %, sizes exact (76 B) - the small factory.  The logic is exact; MWCC schedules
    `li r4, 0x14` and `li r0, 0` on opposite sides of the prologue store / the `addi r4` here.
  - fn_80561018 63.78 %, sizes exact (92 B) - the state-1 forwarder.  The target hoists `cmpwi r4,1`
    above the prologue's register saves (the same scheduling residual `keyboard.cpp` records for its
    two stream destructors) and reuses r12 for the vtable load.
  - The seven this-adjusting thunks fn_8055EAB8/EAC0/EAC8/EAD0/EAD8/EAE0/EAE8 (`subi r3,r3,0xNN; b
    <victim>`, 0x4C/0x34/0x17F4/0x1A04/0x14/0x1C/0x24) are not written: they are MWCC's own output
    for a virtual inherited from a base sub-object, and the conformant way to get them (declare the
    class with `virtual`) would make MWCC emit the vtable into this object (rule 10), while writing
    them as a hand pointer adjustment is rule 6.  Same call as the neighbour `homebutton/gui.cpp`
    records for its three `addi r3,r3,-4` thunks.
  - The remaining 63 functions have no body yet (0 %, unpaired): see the outbox for the list.  The
    largest are fn_8055EC3C (0x464), fn_8055D594 (0x378), fn_8055CFC8 (0x338), fn_80560BC4 (0x280),
    fn_805608F8 (0x270), fn_8055D90C (0x26C), fn_8055CD60 (0x1F4), fn_8055CB34 (0x1EC), fn_8055E288
    (0x1A4), fn_8055D478 (0x11C), fn_8055D36C (0x10C), fn_805617B8 (0x520), fn_80561CD8 (0x420).

The `.ctors` words at 0x8056F410-0x8056F424 are claimed with this unit (the splitter assigns them
here); no static objects are reconstructed yet, so this object emits no `.ctors` section - a residual
for the next pass.
/

/* The retail object was built with exceptions off (no extab/extabindex in any of its sections).
```

## `homebutton/keyboard.cpp` (0x805632BC..0x80569DAC), now in homebutton/hbm_kb_event.cpp, hbm_kb_cursor.cpp

```text
homebutton/keyboard.cpp - the software-keyboard band at the head of the home-button GUI area.

`.text` 0x805632BC-0x80569DAC (142 functions, 27376 B).  This is one maximal unclaimed run
(docs/plan.md 12).

SEAM (unproven, and the brief says so): the left edge is a `--max-bytes` cap, not a translation-unit
boundary.  The evidence is that the code just before 0x805632BC references the *same* data pool as
this range: the tail of the previous retired split run (`auto_03_8056083C_text`, its functions
fn_805617B8/fn_805626DC) loads `lbl_80657698` ("WiiBitmapFontType2.brfnt", "fs_VK_textBox_a.brlyt")
and `lbl_80657F10`, exactly as this range's own functions do, so the retail translation unit
continues to the left and this registration is the upper slice of it.  The right edge *is* a real
boundary: 0x80569DAC is the first address of the registered `homebutton/gui.cpp`.
`tudiscover.py at 0x805632BC` (run in MAIN, whose asm dump the tool needs) reports only weak left
cuts (0x80561074/0x80561528/0x8056229C, "codegen fingerprint change", share 0.22) and no must-link
anchors - no cut it will stand behind.

Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
`python tools/symbols/dumpmap.py lookup <addr>` for every fn_8056 address of the range: every one
answers a bare `zz_<addr>_`/`FUN_<addr>` placeholder or an unrelated J3D/JSU name - the dump's map
resolves by nearest symbol and names no function of this band).

Name evidence (docs/plan.md 12): class 3 - what the code does plus the naming scheme of its
neighbours.  The unit's own `.data` pool is the keyboard layout vocabulary of the home-button menu
(`.data` 0x80657698 "WiiBitmapFontType2.brfnt" / "fs_VK_textBox_a.brlyt" / "fs_VK_bg_a.brlyt",
0x80657714 "T_hiragana" / "B_hiragana" / "T_katakana" / "P_dakuten" / "B_Gkey_handaku" /
"T_hankaku" / "T_zenkaku" / "T_Mode_roma_hira" / "B_Mode_roma_kata", 0x80657ba8 "N_Header" /
"N_Footer" / "T_Nigaoe" / "B_Nigaoe" / "T_Letter" / "T_TouchLetter", 0x80657d9c
"P_txtScrll_UP" / "P_txtScrll_DOWN" / "N_txt_scrl" / "N_TopBtn_00" / "B_txtScrll_UP" /
"B_txtScrll_DOWN", 0x80657c90 "T_2l_TextBox" / "B_2l_TextBox" / "N_MemoRoot" / "G_ArwRoop" /
"G_ArwL_HDAc") - a Japanese/roman software keyboard with a text box, a memo root, a portrait pane
and scroll arrows.  The module is `homebutton`: the immediately following registered unit is
`homebutton/gui.cpp` (0x80569DAC, documented as the homebutton::gui band on dump evidence and on
config/RMHE08/hbm_data/symbols.txt, the HomeButton RSO export table), and the band after it is
`tiHKBManager.cpp`, the home-button keyboard ("HKB") manager.  No `__FILE__` string is emitted
anywhere in the range (no `<name>.cpp`/`<name>.c` string is referenced by any function of it), so
class 1 does not apply and the name is descriptive; the file name `keyboard.cpp` is this unit's
share of the retail keyboard translation unit (see SEAM above).

Sections claimed: `.text` 0x805632BC-0x80569DAC only.  The unit's `.data` (the vtables at
0x80657798/0x806578c0/0x80657970) and its `.sdata`/`.sdata2` (the static objects `lbl_80795A90`,
the float pool `lbl_8079D738`..) live in other splits, so they are referenced here as the map's
`lbl_` symbols and never re-emitted (rule 10 / rule 2).

Exceptions: the retired target objects (`auto_03_8056083C_text.o`, `auto_fn_80566340_text.o`,
`auto_03_80566440_text.o`) carry no extab/extabindex, so the retail object was built with
exceptions off while cflags_main turns them on; the deviation is scoped to this file with a
`#pragma exceptions off` (the same call as the neighbouring `tiHKBManager.cpp`).

Reconstruction status (measured with `tools/units/recompile.py homebutton/keyboard --measure <sym>`,
the official report metric, against MAIN's retired split objects `auto_03_8056083C_text.o`,
`auto_fn_80566340_text.o` and `auto_03_80566440_text.o`): 71 of the range's 142 functions have a body,
38 of them byte-identical, and the unit measures 7.140 % (1954.6 of 27376 `.text` bytes).  The
per-symbol table is in the outbox .pi/outbox/805632bc-fn-805632bc-74c1.json.  The unit is far short of
the 80 % bar: the 71 bodies written this session are the mechanical half of the band (the no-ops, the
constant returns, the accessors, the tail-call thunks, the small virtual forwarders, the pane/text
lookups and the deleting destructors) and the band's byte mass sits in the large bodies that are not
attempted yet (fn_8056802C 0x9D4, fn_80563E50 0x6A8, fn_805655C4 0x604, fn_80565BC8 0x550,
fn_80564D54 0x534, fn_805638D4 0x518, fn_805632BC 0x1E8, ...) - the 71 bodies written here cover
2396 B of the range and the 71 functions without a body cover 24980 B.

RESIDUALS (what still differs and why):
  - **The virtual-call register is the unit's systematic residual.**  A virtual call's vtable pointer
    is materialised into a scratch GPR here (`lwz r4, 0(r3)` / `lwz r5, 0(r3)`) where the retail object
    uses r12 (`lwz r12, 0(r3)`), because the conformant spelling of a call through a slot outside our
    ranges is a struct of function pointers (rule 10) and MWCC only uses the r12 idiom for a *virtual*
    call on a declared class.  That is one instruction per call site: the nine 0x14-byte forwarders
    (fn_80563614 fn_805636A4 fn_80563DEC fn_80563E00 fn_80563E14 fn_80563E28 fn_80564A2C fn_80564A40
    fn_805655B0) measure 98.00 % for that one reason, and every larger body carries the same penalty at
    each of its call sites.  The neighbour `homebutton/gui.cpp` records the identical residual.
  - fn_80563740 58.59 %, fn_80563834 69.78 % - the two switch dispatchers, sizes exact.  Their bodies
    call `self->slot34()` *twice* in the retail object (the first call's result is discarded) and the
    stack tuple fn_80563740 hands to slot 0x224 is stored in the retail order; the source here spells
    that as two statements, so the difference left is which GPR each load lands in (the residual above)
    plus the order the argument tuple's stores are scheduled in.
  - fn_80568004 69.50 % - the `frsp`+`fneg` pair: the retail object converts before negating
    (`frsp f0,f1; fneg f0,f0`), our spelling (`out = -value;`) negates directly.  Four spellings tried
    (`-value`, `-(f32)value`, `0.0f - value`, a local); the size stays 40 B either way.
  - fn_80568DD0 69.86 % - the two-float return: sizes exact (56 B); the retail object materialises the
    constant pair on the stack and reloads it as two words, ours keeps the pair in the return
    registers - a source-shape residual in how the `Vec2f` temporary is initialised.
  - fn_8056931C 45.60 % - the pane lookup: the retail object computes the `lis/addi` of the pane name
    *before* the prologue's saves and keeps the intermediate in r3/r4, ours loads it into r4/r5 and
    schedules it later; the sequence of the four loads/calls is otherwise identical.
  - fn_8056945C/fn_80569470 58.00/45.11 % - the two `lbl_80794738` lookups: the `.sdata` string is
    reached with `li r4, lbl_80794738@sda21` in retail, which needs the symbol declared with a size
    inside the sdata threshold (`extern char lbl_80794738[8]` - measured: 55.0/37.7 % as an unsized
    `char[]`, 58.0/45.1 % with the size, i.e. the `@sda21` reloc is reached but the surrounding load
    order still differs).
  - fn_80563630 83.72 %, fn_805636BC 95.39 %, fn_80568C60 83.89 %, fn_80568DBC 88.00 %,
    fn_805694C0 83.85 %, fn_8056985C 87.17 %, fn_80569910 78.83 %, fn_80569970 93.00 %: sizes exact;
    the shortfall is the register residual above plus the source shape of the guard (retail compares
    the state word with `cmpwi`, i.e. the field is signed - modelled as `s32 field_0x1E4`).
  - the five deleting destructors (fn_805661EC fn_8056622C fn_80566280 fn_805662C0 fn_80566300) and the
    two stream destructors (fn_80569BC8 fn_80569C28) measure 87.50 / 81.74 %: sizes exact (64/92 B),
    and the residual is MWCC scheduling `cmpwi r3,0` above the prologue's stores in retail where our
    `if (self != 0 && flag > 0)` spelling schedules it after them.
  - the 14 this-adjusting thunks fn_80569D3C/D44/D4C/D54/D5C/D64/D6C/D74/D7C/D84/D8C/D94/D9C/DA4
    (`subi r3,r3,0x10|0x1A04; b <victim>`) are not written: they are MWCC's own output for a virtual
    inherited from a base sub-object, and the conformant way to get them (declare the class with
    `virtual`) would make MWCC emit the vtable into this object (rule 10), while writing them as a
    hand pointer adjustment is rule 6.  Same call as the neighbour `homebutton/gui.cpp` records for
    its three `addi r3,r3,-4` thunks.
  - the remaining 71 functions have no body yet (0 %, unpaired): see the outbox for the list.  The
    largest are fn_8056802C (0x9D4), fn_80563E50 (0x6A8), fn_805655C4 (0x604), fn_80565BC8 (0x550),
    fn_80564D54 (0x534), fn_805638D4 (0x518), fn_80566C94 (0x370), fn_80567A14 (0x328),
    fn_80565288 (0x328), fn_80564A54 (0x27C), fn_8056725C (0x268), fn_80568A00 (0x260),
    fn_805647FC (0x230), fn_80566A7C (0x218), fn_8056999C (0x1FC), fn_805632BC (0x1E8).
/

/* The retail object was built with exceptions off (no extab/extabindex in any of its sections).
```

## `homebutton/gui.cpp` (0x80569DAC..0x8056BBF0), now in homebutton/gui.cpp, tiHKBManager.cpp

```text
homebutton/gui.cpp - the homebutton::gui band at the tail of the DOL .text.

.text 0x80569DAC-0x8056BBF0 (52 functions, 7748 B).  This is one maximal unclaimed run
(docs/plan.md 12): the left seam is unproven, the right neighbour is `tiHKBManager.cpp`
(0x8056BBF0, `main` lib).  No `__FILE__` string is emitted anywhere in the range (checked with
tools/symbols/dumpmap.py: no `_<fnaddr>s_<file>_<addr>` local symbol has a `<fnaddr>` inside the
range), so evidence class 1 does not apply here.

Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
`grep -n "fn_80569\|fn_8056A\|fn_8056B" config/RMHE08/symbols.txt` - every defined name is a bare
`.text` entry, and tools/symbols/dumpmap.py lookup over the whole inventory answers only `FUN_`/
`zz_` placeholders or unrelated J3D/GX names for most of it).

Name evidence (docs/plan.md 12, evidence class 2): the shared runtime dump names six functions in
the range with the real spellings `homebutton::gui::drawLine_`, `homebutton::gui::Manager::~Manager`,
`homebutton::gui::Manager::getComponent`, `homebutton::gui::PaneManager::getPaneComponentByPane`
and `homebutton::gui::PaneComponent::contain`; two of them were verified against the code
(0x8056A478 emits two GX_LINES vertices with the line width and a colour word, and 0x8056A888 is a
deleting destructor over a list built by fn_80501C60/fn_80501BF4).  config/RMHE08/hbm_data/symbols.txt
(the HomeButton RSO export table) carries the same class hierarchy - homebutton::FrameController,
homebutton::GroupAnmController, homebutton::gui::{Component, Manager, Interface, PaneManager,
PaneComponent, EventHandler} - which fixes the module as `homebutton`.  The DOL owns no
`homebutton` lib, so the unit takes the `main` lib and cflags_main, the lib its link neighbour
`tiHKBManager.cpp` uses; the file is `homebutton/gui.cpp` (the namespace is `gui`).

Sections claimed: .text 0x80569DAC-0x8056BBF0 only.  The unit's own `.data` vtables live at
lbl_80658290/lbl_806582FC/lbl_80658358/lbl_806583A0, outside this proposal's range (a second
measured pass, playbook 23/29); they are referenced here through the object's vtable pointer
(rule 10), never re-emitted by a `virtual` declaration.

Reconstruction status: 34 of the 52 functions have a body; 27 measure >= 80 % (16 byte-identical).
Measured with `tools/units/recompile.py homebutton/gui.cpp --measure <sym>` against the split
object; the per-symbol numbers are in the outbox .pi/outbox/80569dac-fn-80569dac-9631.json.

Residuals (the functions with a body below the bar, and the reason):
  - fn_8056A85C / fn_8056A870 59.0 % - the counter clear; the target reuses the index register for
    the zero (`slwi r0,r4,1; li r4,0; add; sth r4`) where MWCC emits a fresh register (`li r5,0`)
    before the shift.  Four spellings tried (`= 0`, via a `u16*`, via a local); all 59.0.
  - fn_8056A888 75.3 % - the Manager destructor; the null check is hoisted above the register
    saves in the target and the vtable address materialises into r5 (r4 holds the list-walk 0).
  - fn_80569DAC 73.2 %, fn_8056A638 74.5 %, fn_8056ADB8 78.7 %, fn_8056B0AC 79.9 %,
    fn_8056AA6C/AA98 79.8 %, fn_8056B6D0 76.0 % - sizes exact; the shortfall is which GPR the
    vtable pointer is materialised into (`lwz r12,0(self)` vs a temp) and the prologue scheduling
    of the incoming argument saves, not the logic.
  - fn_8056AE78 has a body but is unmeasurable until the orchestrator re-splits: MAIN's fallback
    split object spells that address with a different symbol, so the report cannot pair it
    (`recompile.py --measure` reports "no pairing" rather than a score).

Not attempted yet (18): the three `addi r3,r3,-4; b <victim>` this-adjusting thunks
(fn_80569E4C/E54/E5C - a `-4` needs pointer arithmetic, rule 6), the two GX immediate-mode
quad/line drawers (fn_80569E64/fn_8056A1D8/fn_8056A478), the component dispatchers
(fn_8056AAC4/fn_8056AD7C), the layout-scene walkers (fn_8056B23C/fn_8056B320), the component
constructor/adder (fn_8056B440), the pane-component drawer (fn_8056B9DC), the RTTI walker
fn_8056B7D8 and PaneComponent::contain (fn_8056B8AC).

```
