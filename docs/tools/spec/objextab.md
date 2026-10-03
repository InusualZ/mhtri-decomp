# `objextab` - Post-compile ninja step: rename MWCC's ordinal `extab`/`extabindex` symbols to the map's `@etb_/@eti_` spelling and make them global

<!-- generated from the module docstring of `tools/elf/objextab.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Give an MWCC object the map's names for its ``extab``/``extabindex`` entries.

## Users

skills (2); docs (6)

## CLI

```
python tools/elf/objextab.py <object> [--splits config/RMHE08/splits.txt]
[--unit <splits key>] [-v] [--dry-run]
python tools/elf/objextab.py --selftest
```
Flags: `--dry-run`, `--selftest`, `--splits`, `--unit`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: object + splits.txt -> object.

## Invariants and rules

* `dtk dol split` names the exception tables it synthesises after the map address each entry occupies - ``@etb_800093A8`` for the ``extab`` entry at 0x800093A8, ``@eti_800222FC`` for the ``extabindex`` entry at 0x800222FC - and MWCC emits the same bytes under its own anonymous ordinals (``@905``) with a **local** binding. While a unit is ``NonMatching`` the target object ``dol split`` writes defines the map's name, so the link resolves; the moment the unit is ``Matching`` our object is the only definition, and if any *other* object in the link relocates that name ``mwldeppc`` fails with ``undefined: '@eti_800222FC'``. No source or compiler flag can reach it: ``@eti_800222FC`` is not an expressible identifier and ``mwcceppc.exe -help`` has no option that names or exports an extab symbol - the name and the binding are assembler-level output (``.pi/notes/resfile-flip.md``).
* This tool closes that gap as a post-compile step next to ``tools/elf/objalign.py``, chained into every MWCC rule by ``tools/project.py``. For each symbol the object defines under an ordinal name in its ``extab``/``extabindex`` section it computes the address the entry lands at - the section's claimed start from ``splits.txt`` plus ``st_value`` - renames the symbol to the map's spelling for that address (``@etb_%08X`` / ``@eti_%08X``, uppercase hex, dtk's own spelling) and sets the symbol's binding global. The rename is the half the linker needs for the *name*; the binding is the half it needs to accept the definition across objects - measured on `g3d/g3d_resfile`: rename alone, and rename plus a `.comment` `active_flags=0x08`, both still fail; rename **plus** a global binding links green and reproduces the DOL hash exactly.
* Only ``.symtab`` (``st_name``, ``st_info``) and ``.strtab`` (the appended names) are touched: no section's *contents* change, so the step is byte-neutral for the DOL by construction. It is a no-op for an object with no ordinal-named extab/extabindex symbol - the fast path decides that from the object alone, before reading ``splits.txt`` - and idempotent: a second run sees the map name rather than an ordinal and skips.
* One thing is deliberately left unrepaired: a promoted symbol still sits inside ``.symtab``'s *local* index range (``sh_info``), because restoring the local/global ordering would renumber the symbol table and with it every relocation's ``r_info`` symbol index - i.e. rewrite ``.rela*`` contents, the one thing this step must not touch. ``mwld`` resolves from ``st_info`` rather than from the ordering: measured over the whole build, 20 of the already-``Matching`` link inputs carry 94 renamed symbols and ``main.dol`` stays byte-identical.
* One summary line per modified object; `-v` also prints every rename.

## Lib dependencies

binary.elf, project.splits.

## Test contract

Tier: fixture.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/elf/test_objextab.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

a promoted symbol stays inside `.symtab`'s local index range (by design)
