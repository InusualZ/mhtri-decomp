# `undefrefs` - The gate's add-only row: a relocation our object carries that no link input can define, with the target's spelling hint; base snapshot cached per source hash; `--census`

<!-- generated from the module docstring of `tools/units/undefrefs.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

The flip blocker a score cannot see: a relocation whose name no link input defines.

## Users

the landing gate (2); imported by `backlog`, `dataclaim`, `datagap`, `land`

## CLI

```
python tools/units/undefrefs.py <unit> [...]   # the refusal, spelled out
python tools/units/undefrefs.py <unit> --base <rev>  # judge against the base revision's own objects
python tools/units/undefrefs.py --census [PATH]  # the pre-existing-debt register
python tools/units/undefrefs.py --selftest
```
Flags: `--base`, `--base-snapshot`, `--census`, `--census-out`, `--json`, `--main`, `--rebuild-index`, `--selftest`, `--snapshot-base`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: src/obj objects, map, link inputs -> hits.

## Invariants and rules

* **What this row asks.** Not a relocation diff - one question, per unit in the batch: *does our object relocate a name that no link input can define?* For each relocation our compiled object (`build/RMHE08/src/<unit>.o`) carries, the name it references is fine when
* our object defines it itself (a local/partial definition),
* `config/RMHE08/symbols.txt` carries a row for it (the map is the project's name oracle),
* another link input on `main.elf`'s link line defines it (global/weak) - the target object is excluded, because a flip *replaces* it,
* the linker script assigns it (`_stack_addr`, ...) or it is the EABI base / entry symbol,
* the target object references it too but does not define it (the reference is already in the link, unresolved - the flip adds nothing), or
* no input defines it and some input already references it (the link is already broken the same way).
* Everything else is a name that must come from nowhere: a flip would leave it `undefined: '<name>'`. The refusal names the name, and where the target records a **different spelling at the same relocation offset** (or, when the layouts differ, the single same-stem spelling in the target) it names both - that spelling is the fix.
* **Add-only (the row is a *delta*, not a verdict).** The tree already carries pre-existing debt: a census of the landed `NonMatching` units found 61 of 285 with a wrong-linkage/undefined reference already in the base object (`enemy/enemy_control` calls `ckResourceName` where the map's row is `ckResourceName__FPc`). Refusing those would refuse every batch that touches such a unit for debt it did not create - the same mistake `stylelint.py --diff` avoids ("an existing finding never blocks a landing, an *added* one refuses"). So the row refuses only a name that is **not in the batch base's own unresolved set**: at `record-base` time the base's objects for the batch's units are compiled once and their unresolved names cached (`snapshot_base`, keyed by the base source hash); the gate subtracts that set, refuses the new names only, and **reports** the pre-existing ones as debt (a note naming the unit, the count and the two spellings for the first). A unit whose base source did not exist is new, so every reference is its own. The census is a separate register (`--census`), never the gate's output.
* **Why it is cheap.** The batch is a handful of units; the only non-trivial part is "which names does the link provide". `link_symbol_index` caches the link inputs' defined globals and references under `build/tmp/undefrefs/link-symbols.json` keyed by each input's size+mtime (the pattern `callers.py` uses for its ELF fallback), and on a miss re-reads only the inputs that changed - so a gate run pays for its own batch's objects, not the whole 2200-object link. `dossier.parse_elf` is the one ELF reader.
* **Not a nuisance.** A unit whose rows are wrong in *other* ways still passes: `Network/NetworkPat`'s 62 wrong vtable slots point at real functions that *are* `symbols.txt` rows, so this row is silent (the selftest pins it as a negative fixture). And a unit whose *only* wrong reference is pre-existing passes too (the selftest pins that as the regression test for add-only).

## Lib dependencies

objcompare.undefined, binary, project, cache, findings, units.

## Test contract

Tier: fixture; the cached link index on a FixtureTree.
Today's selftest (`tools/units/undefrefs_selftest.py`): No build and no repository state: every object is a fixture ELF32 big-endian image written by this file, so the contract is pinned on its own means. The two catches of the incident are fixtures here - the wrong mangled struct tag (same relocation *offset* in the target) and the C-linkage spelling (the target's mangled name found by the *stem* when the layouts differ) - and both must name the two spellings. The negative fixture is the Pat unit's shape: wrong slots that point at real functions which *are* `symbols.txt` rows must stay silent. A clean unit (defined here, mapped, another provider, the target's own unresolved reference, the linker's own symbol) is silent too. The batch path is pinned end to end: a candidate decides the cached index is built, a clean batch does not, and the index survives in `build/tmp/undefrefs/link-symbols.json`.
Target: `tools/tests/units/test_undefrefs.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

absorbs `relocaudit` as `--census` (retired.md)

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* **The incident this closes (2026-09-28, three times in one day).** A unit's rows can score 96-100 % while the object calls a symbol that no link input defines, because a `bl` under a *different relocation name* scores exactly the same as the right one. Measured: `quest/arenatask`'s `arena_eqdata_from_userdata` scored 100.00 % while our object referenced `dl_acdata_to_ar_eqdata__FP11ArenaEqDataUc` and the target's was `dl_acdata_to_ar_eqdata__FP14_arena_eq_dataUc` (an 11-char struct tag against a 14-char one); `hud/cockpit_quest`'s two rows scored 96.38 / 96.50 % while referencing `get_move_work_adrs` under C linkage against the target's mangled `get_move_work_adrs__FUc`; and the Pat vtable's bytes were 114/114 identical while only 50 of 112 slots relocated correctly. Each was a flip blocker, and no gate row that reads a score could see any of them.
