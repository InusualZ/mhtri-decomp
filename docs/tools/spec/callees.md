# `callees` - Name a unit's generated callees from the target object's relocations: owner/state via the lint's Ownership, call shape from `dtk elf disasm`, cross-file references

<!-- generated from the module docstring of `tools/units/callees.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Name the callees of a unit's range: every generated symbol its bodies reference, who owns it, the call shape, and whether a repo-wide rename is needed.

## Users

docs (5); no tool imports it (WP3c: the owner vocabulary is
`lib.project.ownership.owner_label`, the register decode `lib.ppc.decode_rw`)

## CLI

```
python tools/units/callees.py <unit> [--json] [--limit N] [--no-shape] [--no-scan]
python tools/units/callees.py --selftest      # this file + callees_selftest.py
```
Flags: `--json`, `--limit`, `--no-scan`, `--no-shape`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: target .o, map, splits, src -> table.

## Invariants and rules

* **What it reads, and why from the *target* object.** The split target object (`build/<ver>/obj/<Lib>/<file>.o`) is retail's compiled unit: its `.text` relocations name, in the map's own spelling, every symbol the original bodies called. That is the authority for "what the body will reference", and it exists before a line of source is written - which is exactly when the gate applies. Our object is read too when it exists, and a reference that is in *ours* but not in the target is the rule-7 defect the batch would otherwise be adding, so it is marked `ours` and called out.
* **What each column is evidence for.**
* `owner` / `state` - `symbols.txt` (address + section) and `splits.txt` (ranges), through the *same* `Ownership` index the lint uses (`tools/units/stylelint.py`), so this tool and the lint can never disagree about who owns an address. `state=reconstructed` means the owner unit has source in `src/` (rename its source too); `state=registered` means a split range but no source yet (the map row is the only half); `state=unsplit` means no registered owner at all (a band header under `src/unsplit/`).
* `shape` - the instructions at the reference site, read out of the target's own disassembly (`dtk elf disasm`, the same build tool directory as objdiff-cli): which argument registers the caller materialises (`r3..rN`) and whether it reads the return. This is a *heuristic inference* - the object carries no prototype - but it is the evidence a name is derived from, and it is exact for the common shapes. The tool says which way it is unsure instead of guessing: `0 (r3 live-in)` (the preceding call's return flows into r3), `0?` (nothing materialised but a branch separates r3's def from the call), and `?`/`N undecoded` when an instruction in the window is outside the decoder's subset.
* `files` / `XF` - every in-repo `code` mention of the name. The scan is `symedit.find_refs`'s classification (`src/`, so the `path` bucket that protects `#include`s applies here too) done in one pass instead of one pass per name. `XF` (cross-file) is set when the code references live in **more than one file**: those need the repo-wide `symedit.py rename`, not a local edit.
* **Layout.** This lives in `tools/units/` beside `dossier.py` and `symbolpreflight.py` rather than in `tools/objdiff/`: `tools/objdiff/` is the objdiff-cli wrapper directory (`symdiff.py`, `slotmap.py`), while this tool never calls objdiff - it reads the object, the ownership map and the source tree, which is the `tools/units/` layer. It reads the object through `lib.binary.elf` (the project's one ELF reader) rather than growing a second one.
* **It is a reader.** No `src/` edits, no renames, no writes to any shared file.
* **When a unit has nothing to rename it says so plainly** - that is the good answer, and this tool exists to make it a one-command answer rather than a reassurance:
```
== Network/network_state: no generated references - every symbol its bodies reference has a real name
```

## Lib dependencies

`lib.binary.elf` (the target object's relocations - WP3c, was
`dossier.parse_elf`), `lib.binary.objdump` (`dtk elf disasm`), `lib.ppc` (the call-shape decode), `lib.project`
(`Ownership`, `Splits`, `owner_label`, `source_exists`), `lib.names`; the reference scan is `symedit`'s public
`REF_SUFFIXES`/`group_hits`.

## Test contract

Tier: fixture (hand-built ELF, fixture disassembly).
Today's selftest (`tools/units/callees_selftest.py`): Fixtures only: a hand-built ELF32-BE object, a map/splits/source tree in a temp directory and fixture disassembly text - no build tree, no compiler, so the check count is identical in MAIN and a fresh worktree. `callees.selftest()` holds the checks so this entry point and `callees.py --selftest` cannot drift.
Target: `tools/tests/units/test_callees.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* **Why this exists.** Conventions rule 7 (a batch may not *add* a reference to a generated symbol) is a gate before any body is written: the body's callees must be renamed first, map row and source together. A lane spent ~20 minutes hand-surveying 21 such symbols for one unit - which unit owns each, the call shape, and whether a cross-reference lived in another unit's source or header (one did, and only a repo-wide scan found it). That survey is mechanical, so it is mechanised here.
