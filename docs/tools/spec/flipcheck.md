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
over 8 are never fill; our own bytes are still compared). Mutations: no pad rule fails 6, any tail symbol accepted 1,
the next section's alignment ignored 2.
Target: `tools/tests/units/test_flipcheck.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

* READY is necessary, not sufficient (pipeline 8); the relocation half does not run for an already-Matching unit.
* WP3a (behaviour change, 1 unit on this tree): the link's reference set now reads every relocation section of an input, as undefrefs and the linker do; the old reader kept only the last of dtk's two `.rela.data` in `Network/NetworkCommunityPat`, so `Network/NetworkLayerPatStep`'s `stepRequest__15NetworkLayerPatFP14NetworkRequest` read as unreferenced (a false row-36 trim risk) and is READY now.
* Its spelling note (a target reference starting with the name) is not `objcompare.spelling_hint`; unifying changes the refusal text lanes parse.
