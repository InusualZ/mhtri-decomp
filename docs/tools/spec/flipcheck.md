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
python tools/units/flipcheck.py --selftest      # the link/byte/claim checks, against fixtures only
```
Flags: `--selftest`.
Exit codes: Exit status is non-zero when any unit is not flip-ready.
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: src/obj objects, splits, map, link inputs -> reasons, exit.

## Invariants and rules

* A flip replaces the original bytes with our compiled object, so the object has to provide every section the unit's `splits.txt` entry claims - same size, same alignment. Comparing against the *target object* is not enough: dtk's split object is itself incomplete (a unit can claim extab/extabindex/data ranges that no single object in the build emits), which is how a byte-identical object still scrambles main.dol.
* **What a refusal says, and what the three classes are.** The ` - ` lines are the reasons a flip would break the DOL, and they are *add-only*: lanes and the profiles parse them, so no wording here is ever rewritten. Three of them carry a class a lane has to tell apart:
* `no compiled object (build/RMHE08/src/<unit>.o) - compile it first` means the object is absent. When the object **is** there but emits none of the sections this check compares (a bodyless unit whose object is `.comment` and nothing else - `NHTTP/NHTTP_os_RVL`), the line says exactly that and names what `splits.txt` claims instead, because the old wording sent lanes into a rebuild loop (`ninja -n` answers "no work to do").
* A section byte difference prints the **first** differing byte on its own line (unchanged), then the **differing-byte count**, then - when the two sections are the same size and every symbol's bytes match at its *own* address - names the section a **permutation**: the object's layout is the source's definition order, not the address order. That class (`Network/NetworkPat`: 577 of 720 `.text` bytes mislaid, every per-symbol score at 100 %) is invisible to the first-byte line, which reads the same as a three-instruction residual. The same defect got measured with a moved symbol carrying a word of its own too (`NetworkPat`'s three `delete*` functions score 99.7 %, not 100 %), which no lane can act on either, so a fourth line names the weaker case as `the section's layout is a permutation` when the sizes agree, every shared symbol has the same size on both sides, at least one sits at a different address and the mislaid layout accounts for more of the differing bytes than the symbols' own content does.
* Referenced symbol(s) our object relocates that a flip would leave **undefined**: not defined by our object, no row in `symbols.txt`, no link input other than the target object providing them, and the target object not defining them either (`Network/NetworkWiiMediator`: four constructor names, `undefined: '<name>'` on a flip). This is the general relocation half of a flip check, not just the `@etb_`/`@eti_` fragment class.
* Exit status is non-zero when any unit is not flip-ready.

## Lib dependencies

objcompare, binary, project, findings, lanes.naming.

## Test contract

Tier: fixture.
Today's selftest (`tools/units/flipcheck_selftest.py`): No build, no `ninja` and no repository state: every object is a fixture ELF32 big-endian image written by this file, so the contract is pinned - how a `.comment` entry maps to an ELF symbol, that entries are paired by name (the target and our object order their symbol tables differently), that only an *unreferenced* symbol is a trim risk, and that metadata sections, 0-size labels and the reverse flag direction are ignored. The map-symbol check is pinned on the same means: a `@etb_`/`@eti_` symbol only the target defines that *another* linked object references (and no input, ours included, provides) is reported - now as an informational note, because `tools/elf/objextab.py` names those symbols in the build (a current object provides them, and then the check is silent). The three classes a refusal has to tell apart are pinned here too, on fixtures: an object that exists but emits none of the compared sections is *not* "no compiled object" (the wording is asserted verbatim, as is the first-difference line the lanes parse); a same-size section whose symbols all carry their bytes at their own address is named a permutation, and a size/layout/pad/single-symbol difference is not; and a referenced name nothing a flip can use defines is reported while the pinned exemptions (defined here, a map row, another provider, the target's own unresolved reference, the linker script's own symbols) stay silent.
Target: `tools/tests/units/test_flipcheck.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

READY is necessary, not sufficient (pipeline 8); the relocation half does not run for an already-Matching unit
