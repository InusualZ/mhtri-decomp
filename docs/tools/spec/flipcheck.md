# `flipcheck` - Can our object fill every section the unit's `splits.txt` claims (size, bytes, permutation class, undefined references, `.comment` trim risks)? The gate's READY row

<!-- generated from the module docstring of `tools/units/flipcheck.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Check whether a unit can be flipped to Object(Matching, ...): can our object fill the region?

## Users

the landing gate (4); profiles (`.claude/agents`) (8); skills (31); docs (51)

## CLI

```
python tools/units/flipcheck.py                 # every registered unit
python tools/units/flipcheck.py <unit> [...]    # named units
python tools/units/flipcheck.py --verbose <unit> # row 36: one line per force-active symbol after the summary
python tools/units/flipcheck.py --json [--root TREE] <unit> [...]   # the verdict as data (another tree's files)
python tools/units/flipcheck.py --selftest      # the link/byte/claim checks, against fixtures only
```
Flags: `--selftest`, `--verbose`, `--json`, `--root TREE` (`set_root`: every path re-derived from TREE).
Exit codes: Exit status is non-zero when any unit is not flip-ready.
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}`, one row per unit (FAIL detail = the problems joined
by `; `, a PASS row's evidence = the notes), plus `units: {unit: {ready, problems, notes}}` and `missing` (a unit
with no `splits.txt` entry, also a FAIL row); exit 1 when any unit is not ready or missing. `sweepcomments --unit`
reads it.

## Inputs and outputs

Inputs -> outputs: src/obj objects, splits, map, link inputs -> reasons, exit.

## Invariants and rules

* A flip replaces the original bytes with our compiled object, so the object has to provide every section the unit's `splits.txt` entry claims - same size, same alignment. Comparing against the *target object* is not enough: dtk's split object is itself incomplete (a unit can claim extab/extabindex/data ranges that no single object in the build emits), which is how a byte-identical object still scrambles main.dol.
* **What a refusal says, and what the three classes are.** The ` - ` lines are the reasons a flip would break the DOL, and they are *add-only*: lanes and the profiles parse them, so no wording here is ever rewritten. Three of them carry a class a lane has to tell apart:
* `no compiled object (build/RMHE08/src/<unit>.o) - compile it first` means the object is absent. When the object **is** there but emits none of the sections this check compares (a bodyless unit whose object is `.comment` and nothing else - `NHTTP/NHTTP_os_RVL`), the line says exactly that and names what `splits.txt` claims instead, because the old wording sent lanes into a rebuild loop (`ninja -n` answers "no work to do").
* A section byte difference prints the **first** differing byte on its own line (unchanged), then the **differing-byte count**, then - when the two sections are the same size and every symbol's bytes match at its *own* address - names the section a **permutation**: the object's layout is the source's definition order, not the address order. That class (`Network/NetworkPat`: 577 of 720 `.text` bytes mislaid, every per-symbol score at 100 %) is invisible to the first-byte line, which reads the same as a three-instruction residual. The same defect got measured with a moved symbol carrying a word of its own too (`NetworkPat`'s three `delete*` functions score 99.7 %, not 100 %), which no lane can act on either, so a fourth line names the weaker case as `the section's layout is a permutation` when the sizes agree, every shared symbol has the same size on both sides, at least one sits at a different address and the mislaid layout accounts for more of the differing bytes than the symbols' own content does.
* Referenced symbol(s) our object relocates that a flip would leave **undefined**: not defined by our object, no row in `symbols.txt`, no link input other than the target object providing them, and the target object not defining them either (`Network/NetworkWiiMediator`: four constructor names, `undefined: '<name>'` on a flip). This is the general relocation half of a flip check, not just the `@etb_`/`@eti_` fragment class.
* **Row 36 is one line per unit**: the target-exported symbols our `.comment` leaves un-exported and nothing in the link references are folded into `row 36: N function(s) force-active in retail .comment, not in ours: a, b, c, d, e, f, ... (first 6) - ...; mark each __declspec(export) (--verbose lists all)` (`row36_lines`; `ROW36_FIRST` names). `--verbose` adds the old per-symbol line (`ROW36_SYMBOL_LINE`, the wording `comment_trim_risks` still returns) after it. Measured on `Network/NetworkWiiMediator` at `6a1c9583a`: 22 row-36 lines -> 1 (53 -> 32 output lines for it and `Network/net_session_close`), every other line identical; the gate's `flipcheck_problems` reads the ` - ` lines, so a refusal still names the row.
* **Symbol offsets in the positioned data sections (2026-10-07).** `.bss`/`.sbss`/`.sbss2`/`.sdata`/`.sdata2` are compared by
  symbol, not only by size and bytes (a NOBITS section has none): a named symbol at another offset than the target's, or a
  named target symbol our object does not define (the dead-stripped spare global), is a problem line pointing at
  `python tools/mwlink_debugger.py trace <unit>` (`lib.objcompare.data_symbol_gaps`). Skipped as carrying no identity: `@`
  names, `gap_` labels, `lbl_`/`zz_` names (the target's spelling of pooled constants) and `name$NNN` function statics; a
  symbol's size is not compared (`MSL_C/errno` is 4 B ours, 8 B in the target and links). Measured on MAIN's 502 objects: the
  92 units READY before stay READY except `TRK/gdev_cc` (`.bss` buffers swapped: a real order defect).
* **A map name over an anonymous pool word is not missing (2026-10-21).** dtk's map names words of a literal pool
  (`ef/ef_postfield`'s 14 `.sdata2` rows `ef_pf_*`) that MWCC emits as local `@NNN` literals. A target name our object does
  not define is satisfied when (a) the bytes at its offset are equal in both sections, (b) every symbol of ours over that
  range is local or there is none (`lib.objcompare.unnamed_spellings`), and (c) no link input other than the target object
  references the name (the reverse-reference count, docs/pipeline.md 15.3); the excused names are a note (`<section>: N
  target name(s) our object emits unnamed ...`). Still refused: a NOBITS section (no bytes to prove), a differing word, a
  global of ours under another name over it, a referenced name, a named symbol at another offset, and any check run
  without the link context. Where our code loads those words stays `relocation_place_problems`' check (a load of the other
  equal literal is refused there). Pinned by `tools/tests/units/test_flipcheck_poolnames.py` (10 checks; 6 fail against the
  previous flipcheck; dropping the byte, binding or reference guard each fails 1). Measured on 651 objects at `ac489be6`:
  READY 296 -> 298 (`ef/ef_postfield`, `NHTTP/NHTTP_os_RVL`'s `.sdata` string `NHTTPi_haltMessage`; each trial flip links
  the exact DOL), the 301 Matching units' verdicts unchanged, 11 more units name fewer layout gaps and stay NOT READY.
* **A short section before a flipped successor is link padding (2026-10-07).** A unit whose section ends short of its
  claim (`OS/OSLink` .text 0x18 of 0x20; the `.sdata`/`.sbss` 4-byte shortfalls) links correctly when the next unit in
  link order for that section is a linked object, because the link pads it up to the successor's alignment
  (`lib.objcompare.linked_trailing_pad`, `flipcheck.linked_pad_note`): the section is not `extab`/`extabindex`/`.init`; the
  target's tail is zero with no symbol but `gap_` labels; and the successor - `Object(Matching)` in this tree's
  configure.py, or named on the same command line (the gate passes the batch) - built by us at an alignment (at most 16)
  that starts exactly at the claim's end. The note reads `... the 0xK-byte tail is link padding ...`. An unflipped
  successor (its target object aligned to 4) is refused, and when the successor flipped at 16 would excuse the shortfall
  the size line says so: `... only once its successor <unit> is flipped (16-aligned); flip from the tail of each run`.
* **The padding must hold down the whole run (2026-10-07).** A flipped successor that is itself short hands the next
  section to the linker again, so the shortfall is accepted only when the chain `flipcheck.padding_chain_break` follows
  (unit -> successor -> ...) ends on a unit that fills its claim, every link of it being flipped. Measured: OS/OSMemory ->
  OSMutex (-0xC) -> OSReboot (-0xC) -> OSReset unflipped moved every symbol after OSReboot by 0xC, 64228 `.text` bytes of
  the DOL (call sites), and READY was printed for OSMemory and OSMutex. It now reads `... and <unit> (short by 0xN) is
  followed by unflipped <next>: flip from the tail of the run`.
* **An unflipped successor aligned by splits.txt pads the shortfall (2026-10-07).** A `.text` split line may carry
  `align:16` (dtk accepts it, the DOL is unchanged, the stand-in object under `build/RMHE08/obj` then reports align 16).
  A short unit whose unflipped successor's stand-in is 16-aligned links at its claimed addresses without flipping the
  successor (the stand-in fills its own claim, so there is no chain): `flipcheck.linked_pad_note` reads the stand-in's
  alignment and `padding_chain_break` ends the chain there. Measured on 201 `.text` splits that start 16-aligned: 9 units
  became READY (IPC/ipcMain, MTX/mtxvec, MTX/vec, OS/OSIpc, OS/OSMemory, OS/OSMutex, OS/OSNandbootInfo, OS/OSReboot,
  OS/OSSync), none lost it, and flipping all nine together keeps the DOL hash.
* **A function the link dead-strips is not compared (2026-10-14).** A unit may carry a GUESS body of a function the
  retail linker stripped (written so its strings land in the pool): our `.text` is longer than the claim, and a trial flip
  links byte-identical because mwld strips it again. `flipcheck.deadstrip_model` takes such a function out before the size
  and byte comparison when all of these hold: `lib.objcompare.dead_symbols` finds it dead with the roots `deadstrip_roots`
  gives (every name another link input references - the unit's own objects never count: the target object before the
  flip, ours after it - what the batch's objects reference, FORCEACTIVE, the entry; plus our `.comment` force-active flags and every symbol a live
  range relocates); no target symbol and no `symbols.txt` row has its name; and `lib.objcompare.strip_section` can lay out
  the rest (each function at its `.comment` alignment after the previous one, zero gaps, as the object itself already is).
  The note reads `.text: 0xN of the object's 0xM bytes are dead-stripped function(s) ...; the 0xK bytes the link keeps are
  compared`, and a size line that still fails ends `... - after the link dead-strips 0xN B of function(s) from the object's
  0xM`. **A data symbol the same strip takes with it is a refusal**: `<unit>: the link dead-strips N data symbol(s) with the
  stripped function(s) ... - <name> 0xS B of <section>, ... - which our object counts and the link drops, ...`
  (BTE/btm_link_key: `@867`/`@871`, the 0x40 B its trial flip lost from `.data`; a unit whose stripped functions read only
  strings a kept function also reaches keeps them). Measured on all 613 registered units at `72145208a`: READY 229 -> 234
  (BTE/bta_hh_utils, btm_sco, btm_sec, gap_utils, gki_buffer), no unit lost READY, 24 refused units changed refusal text
  only; the reverse-reference row below then refuses gki_buffer. A trial flip of the other four together links
  `main.dol: OK`.
* **A dead weak data copy is stripped too (2026-10-14).** MWCC emits a base class's implicit vtable as a weak copy;
  when only a weak dtor reads it, the link dead-strips both. A dead data symbol that is weak, defined by no target
  symbol and named by no `symbols.txt` row leaves the layout through the same `strip_section` (note `<section>: 0xN
  of the object's 0xM bytes are dead-stripped weak data <name> (...); the 0xK bytes the link keeps are compared`), and
  the rest is compared in order with the trailing-fill rules as before. Every other dead data symbol - local (the
  btm_link_key strings), global, a name the target defines or a map row names - keeps the refusal. A weak copy the
  strip removes no longer also carries the "the link keeps our copy" note. Measured on all 629 units at `29194d15`:
  READY 270 -> 271 (homebutton/hkb_pane: `__vt__Q44nw4r3lyt6detail8PaneBase` 0xC B, `.data` 0x80 -> 0x74 of 0x78,
  the 4-byte tail fill), homebutton/hkb_animation's `.data` refusal goes (`__vt__Q34nw4r3lyt13AnimTransform`; it
  stays refused on 12 `.text` bytes), no unit lost READY, 15 contradictory kept-copy notes gone. A trial flip of
  hkb_pane links `main.dol: OK` and reads READY flipped as well.
* **The verdict is the same after the flip (2026-10-14).** The model runs whether the unit's link input is the target
  object (NonMatching) or our object (`Object(Matching)`, the state the gate checks), and our object's own references
  root nothing in either state. Before this, a flipped unit skipped the model and the gate refused the flip batch of
  BTE/bta_hh_utils, btm_sco, btm_sec and gap_utils that a NonMatching tree called READY. Measured at `cafb12d1b`: the
  four READY with configure.py flipped (`main.dol: OK`); on the unflipped tree READY 237 -> 244, the seven gains all
  already `Object(Matching)` with the DOL hash green (BTE/l2c_api, BTE/port_rfc, DSP/dsp_task, DVD/dvd_broadway,
  OS/OSCache, OS/OSNet, OS/OSReset), no other verdict or refusal changed.
* **A weak copy an earlier input defines is dropped (2026-10-14).** MWCC emits an inline function or an implicit vtable
  as a weak copy in every TU that uses it; mwld keeps the first definition in link order and drops the later copies'
  bytes. `flipcheck.weak_duplicate_symbols` removes such a copy from our object before the size and byte comparison
  (through the same `strip_section` layout as the dead-strip model, in any strippable section) when all of these hold:
  it is weak and of size > 0; the target object does not define the name; and a link input placed **before** ours
  (`link_ctx["inputs"]`, the order of the link line) defines it, global or weak - a unit named in the same batch
  counted as its built object at its target's link position. A copy that only a later input defines, or that no other
  input defines, is kept and counted (a note says which). The note reads `<section>: 0xN of the object's 0xM bytes are
  ... weak copies <names> that an earlier link input also defines (the link keeps the first definition); the 0xK bytes
  the link keeps are compared`; a size line that still fails outside `.text` ends `... - after the link drops 0xN B of
  symbol(s) from the object's 0xM`. Measured on all 629 registered units at `dcaef625`: nw4r/ut_ResFontBase (weak
  `ut::Font` dtor and vtable; ut_Font, input 605, precedes it at 606) and nw4r/ut_CharWriter (weak `ut::Color` dtor;
  font/flfnt, input 24) go NOT READY -> READY; trial flips of each alone and of both together hash `main.dol: OK`.
  homebutton/hkb_window stays refused with `.text` +0xC (was +0x4C): its weak `ut::Color` dtor is dropped for
  flfnt's, its `lyt::Window::Content` dtor is dead-stripped, and its weak `ut::Color` ctor has no other definer, so the
  link keeps it - its trial flip put the ctor at 0x8052E350 inside the unit and grew `main.dol` by 0x20 (the 0xC
  rounded to the DOL section alignment).
* **A dropped weak `.text` copy takes its unwind entries with it (2026-10-21).** mwld drops a function's `extab`
  record and `extabindex` entry together with the function, so for every `.text` copy `weak_duplicate_symbols` drops,
  `dropped_unwind_symbols` removes the `extab`/`extabindex` symbols covering it (`lib.binary.extab.unwind_entries`,
  the rule `tools/elf/objextab.py` names the kept entries by) through the same `strip_moves` layout, and the
  relocation-place check maps the kept entries through those moves. The note reads `extab: 0xN of the object's 0xM
  bytes are the unwind entries <names> of the removed function(s) (the link drops them with the function); the 0xK
  bytes the link keeps are compared`. A copy the link keeps (no earlier definer) keeps its entries counted, and an
  entry the old `objextab` named at its own offset (two slots late) is refused on the relocations. Measured on all
  651 registered units at `8b3cb635` with the patched `objextab`: READY 298 -> 301, the gains
  ef/ef_drawlinestrategy, ef_drawpointstrategy and ef_drawfreestrategy (`extab` 0x38 vs claimed 0x28, `extabindex`
  0x54 vs 0x3C before); no unit lost READY; their trial flip together hashes `main.dol: OK`. ef_drawfreestrategy's five
  named `.sdata2` pool rows (`ef_free_hundred`, `_epsilon`, `_int_to_double`, `_one`, `_zero`) are excused by the
  pool-name rule (`lib.objcompare.unnamed_spellings`) and read as a note.
* **What a removed symbol alone reads goes with it (2026-10-27).** Three gaps closed together, measured on
  Runtime.PPCEABI.H/Gecko_ExceptionPPC (an unreferenced `static` placed first for the scheduler, a weak `~exception`
  nothing calls, and a weak `__RTTI__Q23std9exception` copy another input defines first): (1) a dead-stripped
  function's `extab` record and `extabindex` entry leave with it, as a dropped weak copy's do
  (`dropped_unwind_symbols` over both sets; the note now reads `... are the unwind entries <names> of the removed
  function(s) ...` for both); (2) a dropped weak copy keeps nothing alive - `dead_symbols(..., removed=)` neither keeps
  it nor follows its relocations - so the type-name string only that `__RTTI__` copy read is dead, and a dead
  **local** data symbol with no target name and no map row leaves the layout like a dead weak one (note `...
  dead-stripped local data <names> ...`) - but only when nothing the model still counts reads it: liveness is judged a
  second time with every dead symbol the model keeps counted (a dead function the target names) held live (`kept=`),
  and a local whose section cannot be laid out joins the data refusal; (3) `strip_moves` accepts a section that ends
  in zero fill up to its alignment (MWCC pads `extab` after an 18-byte record) and repeats that fill after the kept
  layout; (4) `dead_symbols` holds bytes of a strippable section that no symbol covers live, with what they relocate.
  Globals and target-named data keep the refusal. Measured on all 655 units at `50425ad2` (before: main's
  flipcheck, after: this one, same build): READY 347 -> 348, the gain Gecko_ExceptionPPC (`.rodata` 0x71 vs 0x68,
  `extab` +8, `extabindex` +12 before); the 352 `Object(Matching)` units keep their verdicts and problem lines (347
  READY; NAND/NANDCore, CPlusLibPPC, TRK_interrupt_vector_stubs, TRK_interrupt_vectors and `__start` refused as
  before), five of them change only the unwind note's wording. Refused units that change: KPAD/kpad loses its
  `.sdata2` refusal (`@329`, read only by the stripped `KPADSetBtnRepeat`, goes and the shifted pool is the target's
  byte for byte), g3d/g3d_state and menu/menu_sysmsg lose their `extab`/`extabindex` size lines. Gecko_ExceptionPPC
  flipped alone links `main.dol: OK` and reads READY flipped; on the flipped tree the same comparison gives the same
  set (353 Matching: READY 347 -> 348, the gain Gecko_ExceptionPPC, no other Matching verdict or problem line changed).
* **A leading alignment fill is not a size gap (2026-10-14).** dtk can start a claim at the previous object's end,
  before the unit's own aligned start (`DVD/dvd`'s `.sbss` claim 0x80795090 holds 0x10 bytes of fill, labelled
  `pad_10_80795090_sbss`, before its 32-aligned data). `lib.objcompare.leading_fill` accepts the lead when all of
  these hold: not `extab`/`extabindex`/`.init`; the claim's start rounded up to our section's alignment leaves a
  non-zero lead and our size plus the lead fits the claim; the target's lead bytes are zero (NOBITS: none); no target
  symbol starts inside the lead but dtk's fill labels (`gap_...`, `pad_<size>_<address>_<section>`:
  `objcompare.is_fill_label`); and `flipcheck.predecessor_fills` - the registered range ending at the claim's start is an
  unflipped stand-in, or a linked object whose built section fills its claim. Our size then counts the lead, the bytes
  and the positioned-symbol offsets are compared after it (and a remaining tail goes through the trailing-pad rules).
  The note reads `<section>: the 0xN-byte lead is alignment fill (...) - our 0xK bytes are compared at +0xN`. dtk's
  `pad_` labels are fill labels in `data_symbol_gaps` too. Measured at `dcaef625`: OS/OSStateTM (already
  `Object(Matching)`, DOL green; `.bss` lead 0x10) goes NOT READY -> READY; DVD/dvd's `.sbss` refusal goes (its trial
  flip differs from retail in exactly the 6 `.text` bytes of `__DVDPrepareReset`'s register swap, so the lead links
  as modelled; the unit stays refused on those bytes); AX/AXVPB, NAND/NANDCore and OS/OSExec lose their lead lines and
  keep other refusals. No unit lost READY (266 -> 269).
* **The reverse-reference row (2026-10-14).** A global the target object defines that another link input relocates, and
  that neither our object (global or weak) nor any other input defines, makes the flip's link fail: `<unit>: N symbol(s)
  the target object defines and another link input references are not defined by our object - <names> (referenced from
  <objects>) - so a flip fails with `undefined: '<name>'`; ...` (`reverse_reference_problems`, docs/pipeline.md 15.3; the
  `@etb_`/`@eti_` fragments stay `external_map_symbol_notes`' note). Measured: BTE/gki_buffer (its `.bss` label
  `lbl_8070DF3E`, read by `mh3_pad.o`; its trial flip failed with exactly that `undefined:`) goes READY -> NOT READY; the
  143 other units the row names were NOT READY already; READY 234 -> 233.
* **`ninja diff` is not a flip signal.** `dtk dol diff` fails on main itself with `Expected to find symbol @eti_8001E558`:
  420 `@etb_`/`@eti_` symbols of the already-Matching units (objextab renames them global) are absent from the linked ELF's
  symbol table although their bytes are present and the DOL hash holds. No dead-strip is involved (`__nwa__FUl`,
  `fn_8004054C` and the four records are in the flipped ELF); the hash is the check.
* **A trailing alignment pad is not a size gap (2026-10-06).** dtk gives a unit the zero fill up to the next
  object's aligned start, MWCC's object ends before it, and the link puts the fill back. A section shorter than its
  claim is accepted - a `.` note `<section>: 0xN of 0xM bytes - the 0xK-byte tail is alignment fill (...)` instead of
  the size line, and the byte comparison then covers our own bytes only - when all of these hold
  (`lib.objcompare.trailing_pad`): the section is not `.text`/`.init`/`extab`/`extabindex`; the alignment (the larger of
  the two objects', at most 8) rounds our size up to exactly the claim; the target's tail bytes are all zero (a NOBITS
  section has none); no target symbol sits in the tail except dtk's `gap_` fill labels; and the registered range that
  starts at the claim's end exists and its section, at every alignment it can be linked with (its target object's and,
  when built, ours), starts exactly there (`pad_context`). An unknown next section is not accepted. Measured on MAIN's
  objects at 2caaab9ca: `Network/PatConnection` (.data 0x3B24 of 0x3B28, .sdata 0x7 of 0x8) and `g3d/g3d_gpu` (.data
  0x42 of 0x48) go NOT READY -> READY, and a real link with both as `Object(Matching)` in a scratch worktree hashes
  `main.dol: OK`; `Network/NetworkPool`'s .sdata 0x5, .sdata2 0x4 and .sbss 0x4 of 0x8 are accepted (its `.text`
  still differs in MAIN's build; the NET-B lane reports its own link OK).
* **dtk's auto object is a successor too (2026-10-15).** When no registered range starts at a short claim's end,
  `pad_context` takes the next section's alignment from dtk's auto split object that starts there
  (`flipcheck.auto_successor`: `auto_<nn>_<ADDR>_<section>` as this tree's `build/RMHE08/config.json` lists it - never a
  stale file left under `obj/`). Such an object is never flipped, so the link places it at its own alignment and the
  trailing-pad rule above applies unchanged. Measured: `g3d/g3d_cpu` (`.sdata2` 0x4 of the 0x8 claim at 0x80795F50,
  followed by `auto_11_80795F58_sdata2`, 2**3) goes NOT READY -> READY; its trial flip hashes `main.dol: OK`.
* **No byte line for bytes only one object has (2026-10-15).** When the size line already refuses a section and the
  bytes both objects carry are equal, the two byte lines are not printed (they named a first difference past our
  section's end, `at +0x4 (ours 00, target 00)`); a difference inside the shared bytes is still printed as before, and
  the `.data` seam diagnosis still runs. Measured on 651 units at `06aed493`: 34 units lose such lines, all still refused
  by their size line, no other line changes.
* **Relocations are compared by the place they resolve to (2026-10-15).** Equal bytes do not make a match when a
  relocation at the same offset names another copy of identical data: `g3d/g3d_resvtx`'s accessors loading the other
  of four `"ref"` copies, `g3d/g3d_scn`'s inline accessors each naming the other's string - READY printed, the trial flip
  moved the DOL hash. `relocation_place_problems` places both objects at the claim (`lib.objcompare.reloc_places`: a
  local symbol at its section's `splits.txt` start plus its value and the addend; a named global or weak, and any
  undefined name, at its `symbols.txt` address - two spellings of one address agree; an unmapped name compares by name;
  our sections shifted by a leading fill and mapped through the dead-strip model's `strip_moves` layout, a relocation
  inside a stripped function not compared) and refuses one line per section:
  `<section>: N relocation(s) resolve to a different place or type than the target's at the same offset, so the link
  writes other addresses even where the bytes are equal - +0xOFF ours 0xADDR, target 0xADDR; ...` (offsets in the
  target's layout, the first `RELOC_PLACE_FIRST` spelled, each relocation type named when the two differ). An offset
  only one object relocates is not this line. Measured on 651 units at `06aed493`: 22163 relocated offsets of the 295
  `Object(Matching)` units compared, all equal (22153 resolved to addresses), READY unchanged for every unit; the line
  appears on 255 of the 360 NOT READY units and is the only refusal of none. The real `g3d/g3d_resvtx` object with the
  `.text` relocations at +0x4C0/+0x550 swapped (bytes unchanged) reads READY on main and `.text: 2 relocation(s) ...
  +0x4BC ours 0x80791258, target 0x8079125C; ...` here.
* **Advisory: renamed relocation targets whose values differ** (`relocation_value_notes`, `lib.objcompare.value_diffs`,
  the same pass as `relocdiff --values`): a relocation at the same offset, of the same type, in the same instruction,
  whose target is `@N` in ours and `lbl_X` in the target (or another unit's `lbl_`, read from the retail DOL at its map
  address) but whose bytes differ - a pool word or string that objdiff, `rawsame` and every byte check here score equal.
  One note `advisory: N relocation(s) name a different datum than the target's under another name, and the bytes differ
  (relocdiff --values) - <owner>+0xOFF ours NAME = VALUE, target NAME = VALUE; ...` (the first `VALUE_FIRST` spelled).
  It is a note, never a problem, and an `advisory:` note prints under NOT READY too. A refusing row is a later decision
  once the census (`relocdiff --values`, 102 mismatches in 22 units at `58e3e191`) is worked down.
* Exit status is non-zero when any unit is not flip-ready.

## Lib dependencies

objcompare (sizes, bytes, layout classes, `reloc_facts`, `link_index`, `undefined`), project; tool APIs: `claims.norm_unit`, `dataseams`, `poolseams`. No binutils: sizes and bytes are read from the objects (`objdump -h`/`objcopy` before WP3a).

## Test contract

Tier: fixture.
Today's selftest (`tools/units/flipcheck_selftest.py`): No build, no `ninja` and no repository state: every object is a fixture ELF32 big-endian image written by this file, so the contract is pinned - how a `.comment` entry maps to an ELF symbol, that entries are paired by name (the target and our object order their symbol tables differently), that only an *unreferenced* symbol is a trim risk, and that metadata sections, 0-size labels and the reverse flag direction are ignored. The map-symbol check is pinned on the same means: a `@etb_`/`@eti_` symbol only the target defines that *another* linked object references (and no input, ours included, provides) is reported - now as an informational note, because `tools/elf/objextab.py` names those symbols in the build (a current object provides them, and then the check is silent). The three classes a refusal has to tell apart are pinned here too, on fixtures: an object that exists but emits none of the compared sections is *not* "no compiled object" (the wording is asserted verbatim, as is the first-difference line the lanes parse); a same-size section whose symbols all carry their bytes at their own address is named a permutation, and a size/layout/pad/single-symbol difference is not; and a referenced name nothing a flip can use defines is reported while the pinned exemptions (defined here, a map row, another provider, the target's own unresolved reference, the linker script's own symbols) stay silent.
The row-36 summary is pinned there too (`row36_lines`, and `check` folding eight risks into one line, nine with `verbose`). `--json` and `--root` run end to end in `tools/tests/units/test_sweepcomments.py` (`--unit` on a `FixtureTree`: NOT READY without an object, READY with a byte-identical one).
The pad rule: `tools/tests/units/test_flipcheck_pad.py` (15 checks; objects shaped like the five real sections and
NetworkPool's NOBITS `.sbss`: each is fill; a non-zero tail, a non-`gap_` symbol in the tail, a next section aligned to
4, no registered next section and a shortfall past the alignment each stay a problem; `.text`/`extab` and an alignment
over 8 are never fill; our own bytes are still compared). `test_positioned_symbols` pins the symbol check (swapped order, spare global, `.sdata2`, the skipped names, size not compared; check off fails 1).
Mutations: no pad rule fails 6, any tail symbol accepted 1,
the next section's alignment ignored 2.
`test_linked_successor_pad` (9 more checks, 24 in all): OSLink/OSMessage-shaped objects - a Matching 16-aligned successor and a
same-batch successor accept, an unflipped one (target aligned to 4, with the hint), a successor our object aligns to 4 and a
non-zero tail refuse; `.text` is excused, `extab` never, `.sdata` 4 B before an 8-aligned successor is. Mutation: the
linked-successor test off fails 2. `test_padding_chain` (2 more checks): a short OSMessage before an unflipped OSMutex refuses OSLink, a run ending on a full unit accepts; chain check off fails 1.
The dead-strip model and the reverse-reference row: `tools/tests/units/test_flipcheck_deadstrip.py` (32 checks, objects
shaped like gap_utils, btm_link_key and gki_buffer): accepted - a stripped function between two kept ones, the 16-aligned
zero-gap layout, a function only another stripped one calls; refused - another input references it, a map row names it, a
kept function calls it, FORCEACTIVE names it, the kept bytes differ, a non-zero gap, functions packed tighter than their
`.comment` alignment, a string only the stripped function reads that the target carries (refused on the kept size); the liveness rules (`.comment` 0x08, `extabindex`,
`.ctors`, a section-symbol reference, an unreadable object); hkb_pane's weak dead vtable accepted (read only by a
stripped function, or by nothing), refused when a kept function reads it, a map row names it, the target defines it
(the btm_link_key near miss, weak and global) or it is a global absent from the target; the reverse-reference row
refused, silent when ours or another input defines the name; the same READY before and after the flip (our object as
the link input, its own call from one stripped function to another rooting nothing), and a called function still
refused after it. Mutations: model off fails 5, data-strip refusal off 1, map-row guard off 1, reverse-reference off 1,
weak-data strip off 3, its weak-bind guard off 2, its target-name guard off 1, its map-row guard off 1, `strip_section`'s zero-gap check off 1, its alignment check off 1, the model skipped for a linked unit 1, the unit's own object counted as another input 1.
The weak-copy and leading-fill rules: `tools/tests/units/test_flipcheck_weak_lead.py` (28 checks, objects shaped like
ut_ResFontBase, hkb_window and DVD/dvd): accepted - weak `.text` and `.data` copies an earlier input defines, a dead
one, an earlier batch unit's built object; refused - no other definer, only a later one, a global copy, no link order,
differing kept bytes, a batch object without the name, a target that defines the name (not stripped); the lead
accepted NOBITS, PROGBITS and after a linked full predecessor; refused on non-zero lead bytes, a real symbol in the lead,
no predecessor, a 16-aligned section, a short linked predecessor and a reordered layout. Mutations (each exits 1): weak
model off, link order ignored, binding ignored, target-name guard off, batch object unchecked, leading fill off, lead
bytes unchecked, lead symbols unchecked, predecessor not judged, linked predecessor size ignored, symbols not shifted.
`test_auto_successor_pad` (4 more checks in `test_flipcheck_pad.py`, 37 in all): g3d/g3d_cpu-shaped - ready before an
8-aligned auto object the split lists; refused by the size line alone before a 4-aligned one and before one only a stale
file under `obj/` names; a short section's own bytes still compared. Mutation: main's flipcheck fails 3.
The removed-symbol closure: `tools/tests/units/test_flipcheck_strip_unwind.py` (15 checks, Gecko_ExceptionPPC-shaped):
accepted - an unreferenced static with its `extab` record and `extabindex` entry, a dropped weak `__RTTI__` copy's
type-name string, `extab`'s trailing fill kept; refused - a static a kept function calls (`.text`/`extab`/`extabindex`
stay over the claim), a weak copy no other input defines (its string stays), a global another input references, a
string a dead target-named function reads; `dead_symbols`' `removed`, `kept` and uncovered bytes rooting their loads (the
poolnames fixture's symbol-less `.text`); `strip_moves`' trailing fill (zero, up
to the alignment). Mutations: main's flipcheck fails 7 (of 12 there: `dead_symbols` takes no `removed`); `kept` ignored fails 1.
The relocation places: `tools/tests/units/test_flipcheck_reloc.py` (11 checks, g3d_resvtx-shaped): the same copies under
other names ready; swapped copies refused with the verbatim line; the same place under another type refused; map
aliases agree, another map address refused, an unmapped name compared by name; `strip_moves`/`apply_moves`, offsets
mapped through the strip, the one-sided offset and the unresolved place not compared. Mutation: main's flipcheck fails 3.
Target: `tools/tests/units/test_flipcheck.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

* READY is necessary, not sufficient (pipeline 8); the relocation half does not run for an already-Matching unit.
* WP3a (behaviour change, 1 unit on this tree): the link's reference set now reads every relocation section of an input, as undefrefs and the linker do; the old reader kept only the last of dtk's two `.rela.data` in `Network/NetworkCommunityPat`, so `Network/NetworkLayerPatStep`'s `stepRequest__15NetworkLayerPatFP14NetworkRequest` read as unreferenced (a false row-36 trim risk) and is READY now.
* Its spelling note (a target reference starting with the name) is not `objcompare.spelling_hint`; unifying changes the refusal text lanes parse.
