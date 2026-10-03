# `linkorder` - Reconstruct the DOL image from `main.elf` and compare it to the original slot by slot (order, bytes, header), attributing the first divergence to a unit/object

<!-- generated from the module docstring of `tools/units/linkorder.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Link-order audit for flips (roadmap 7.19): does the linked ELF still reproduce the original DOL?

## Users

skills (3); docs (4)

## CLI

```
python tools/units/linkorder.py                  # whole link: order, addresses, bytes, first divergence
python tools/units/linkorder.py --unit <unit>    # one unit's claimed region (works before a flip too)
python tools/units/linkorder.py --json           # the same report as data, for a batch gate
```
Flags: `--dol`, `--elf`, `--json`, `--no-stale-check`, `--unit`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: main.elf, main.dol, splits, map -> report.

## Invariants and rules

* What it checks -------------- order the ELF's code/data sections in address order against the DOL's text0..text6 / data0..data10 slots: same count, same addresses, same sizes. A DOL slot is the section's size rounded up to 0x20, the original's file-offset alignment (measured on the green link: 10/10 slots). bytes every slot, from the ELF's file bytes, against the DOL's - the whole `dtk dol diff`, bucketed per section, with the first differing byte and the count per section. header the reconstructed DOL header against the original's, field by field. For the first divergence it prints both words, the original symbol (from `symbols.txt`) and the linked symbol (from the ELF symtab), then the unit from `splits.txt` and the object `configure.py` links for it.
* `--unit` scopes the same audit to the ranges one unit's `splits.txt` entry claims. That is the pre-flip check: before a flip the region holds dtk's target object and matches, afterwards it holds ours. It also reports whether a claimed range is a *fragment* of a shared `.ctors`/`.dtors` slot - the link-order case roadmap 7.19 is about, which `flipcheck.py` cannot see - and it names the object the link actually used (`src/` for a `Matching` unit, the target `obj/` for a `NonMatching` one). `extab`/`extabindex` fragments are not flagged: `g3d/g3d_resanmamblight.c` is green with one. The content half of the proof (object vs target object) stays `flipcheck.py`'s job; this tool owns the link shape.
* `land.py` does not call this yet (it was built under a no-edit-other-tools constraint); the natural hook is a `linkorder.py --json` step after `ok`, reading `ok` and the per-unit verdicts.
* Read-only by construction: it opens the ELF, the DOL, `symbols.txt`, `splits.txt` and `configure.py`.

## Lib dependencies

binary, project.

## Test contract

Tier: fixture (hand-built ELF + DOL).
Today's selftest (`tools/units/linkorder_selftest.py`): No build, no `ninja` and no repository state: the fixture is a hand-built ELF32 big-endian image and the DOL it should reproduce, both written by this file, so the contract is pinned - which sections become DOL slots, how the slot size is derived, where the first divergence is reported, and who it is attributed to - instead of being re-derived from whatever `build/RMHE08/main.elf` happens to contain today. The real-data counterpart is the tool itself run against the green link, which reports MATCH.
Target: `tools/tests/units/test_linkorder.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

not a gate row yet

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* Why this exists --------------- A flip substitutes our object for dtk's split object, so the only proof that the *link* is still the original is `ninja build/RMHE08/ok`. When it goes red, `dtk dol diff` names a symbol and a count of differing bytes - not the unit or the object responsible - and `build/RMHE08/main.MAP` cannot help: it is a stale 2024 artefact, because dtk's link does not write one. So this tool reconstructs the DOL image from the linked `build/RMHE08/main.elf` alone - section headers, PT_LOAD contents, entry point and the NOBITS span - and compares it to `orig/RMHE08/sys/main.dol`. No relink, no `ninja`, nothing written.
