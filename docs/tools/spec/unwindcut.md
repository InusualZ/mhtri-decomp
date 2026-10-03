# `unwindcut` - Re-cut one unit at a function boundary: partition `.text`/extab/extabindex from the retail image, the `.ctors`/`.dtors` words to drop, paste-ready lines; refuses a non-boundary cut

<!-- generated from the module docstring of `tools/units/unwindcut.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Re-cut one unit at a function boundary: print the exact `splits.txt` lines for both halves.

## Users

docs (2)

## CLI

```
python tools/units/unwindcut.py <unit> <cut-addr>
python tools/units/unwindcut.py menu/menu_message 0x802AA764
python tools/units/unwindcut.py --selftest
```
Flags: `--dol`, `--json`, `--object`, `--selftest`, `--splits`, `--symbols`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: splits, DOL, map, object -> lines.

## Invariants and rules

* **The job this replaces.** A seam re-cut (`menu-seam-recut-4622`, `.pi/notes/menu-seam-recut-4622.md`) took a lane ~15 mechanical minutes: read the unit's claimed ranges out of `splits.txt`, objdump `extabindex`, find the record whose `fn_addr` is the cut, do the three section partitions by hand, and discover - only when `dtk dol split` refused - that un-claiming a `.text` range also obliges dropping the `.ctors` word pointing into it. All of that is arithmetic over two files, so it is one command.
* **What it reads.**
* the unit's claimed ranges from `config/RMHE08/splits.txt` (the claimed half is the *prefix* of the full range - the tool never writes the file);
* the unwind records from the **retail image** (`orig/RMHE08/sys/main.dol`), as 12-byte `{fn_addr, fn_size, etab_addr}` entries in `extabindex`, starting at the unit's claimed `extabindex` start. The DOL is the source because that section is the *whole* original run: the split object (`build/RMHE08/obj/<unit>.o`) only covers what `splits.txt` already claims, so it cannot name the released tail a re-cut has to describe. When the split object is present its section sizes are used as a cross-check on the claimed half (a mismatch is a warning, not a guess).
* **The three safety properties.**
* **A cut must be a function boundary.** The cut has to be some record's `fn_addr` *and*, when `symbols.txt` carries `.text` function rows, a function has to start there with the previous function ending exactly there. A cut in the middle of a function - or one that matches no record - is refused, and the refusal names the function it would have cut through. An un-claiming that splits a function corrupts the split; the refusal is the tool's whole point.
* **The record invariant must hold.** Verified for every record in the unit's full range: `etab_addr == extab_start + 8*i` (1:1 with `extab` at 8 B each) and `fn_addr` strictly increasing (function-address order). The lane proved this for all 103 `menu/menu_message` records; when it does *not* hold the tool refuses and says which record broke it, rather than emitting a guess.
* **The halves must sum to the original.** `.text`, `extab` and `extabindex` bytes, the record count and the `.text` function-row count are all checked: `kept + tail == original`, and the claimed prefix's `extab`/`extabindex` ends must be exactly `start + 8*k` / `start + 12*k`. An asymmetry is reported.
* **The `.ctors`/`.dtors` rule (undocumented before this tool).** Un-claiming a `.text` range obliges dropping any `.ctors`/`.dtors` word **whose target function leaves with the cut**: dtk derives the `.ctors` claim from the unit that owns the constructor's target, so a surviving word would make it refuse `Mismatched splits for .ctors 4:0x8056F374 (menu/menu_message.cpp) and function 3:0x802ABC4C (auto_fn_802ABC4C_text)` (the real first attempt). The tool scans the `.ctors`/`.dtors` claims for words targeting the unit's range and names the ones to drop; dropping the line lets dtk re-derive it. The rule is recorded in `docs/pipeline.md` §9.4.1.
* **The full range.** A unit's claim may already be the *kept* half of an earlier cut: `splits.txt` then ends at `0x802AA764` while the released tail is the unclaimed gap up to the next claim (`stage/stg_w.cpp` at `0x802AD9C0`). The tool re-absorbs that immediately-following gap in `.text`, `extab` and `extabindex` - and only when all three agree on the same record count, which is what makes the re-cut reproducible rather than a guess. Nothing is written.
* **Read-only.** Like `dataclaim.py`, it prints and never edits `splits.txt`.

## Lib dependencies

binary.dol, binary.elf, project.

## Test contract

Tier: fixture (synthetic DOL).
Today's selftest (`tools/units/unwindcut_selftest.py`): No build, no `ninja`, no repository state: every fixture is a synthetic DOL (built by this file), a `splits.txt`, and a `symbols.txt` in the system temp, so the contract is pinned: * the re-cut's arithmetic - the full range re-absorbed from the unclaimed gap, the record split at the cut, the three section halves and the sum check; * the two refusals that make the tool safe - a cut mid-function (no record and no symbol starts there), and a record run whose `etab_addr != extab base + 8*i`; * the `.ctors`/`.dtors` rule - a word whose target leaves with the cut is named for dropping, a word whose target stays keeps its claim; * the paste-ready text itself, so a formatting regression is a failure and not a surprise on a real cut.
Target: `tools/tests/units/test_unwindcut.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
