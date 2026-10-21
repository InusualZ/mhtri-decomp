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

Inputs -> outputs: object + splits.txt + the target object (`build/<v>/obj/<unit>.o`, read only for a weak `.text`
copy) -> object. Both inputs are the compile edge's implicit inputs in `tools/project.py`: the target object, which
`dol split` rewrites only when its bytes change, so a seam that moves the unit's range recompiles (and renames) the
candidate, while a split that changed nothing recompiles nothing; and `tools/lib/binary/extab.py`, the unwind rule.

## Invariants and rules

* `dtk dol split` names the exception tables it synthesises after the map address each entry occupies - ``@etb_800093A8`` for the ``extab`` entry at 0x800093A8, ``@eti_800222FC`` for the ``extabindex`` entry at 0x800222FC - and MWCC emits the same bytes under its own anonymous ordinals (``@905``) with a **local** binding. While a unit is ``NonMatching`` the target object ``dol split`` writes defines the map's name, so the link resolves; the moment the unit is ``Matching`` our object is the only definition, and if any *other* object in the link relocates that name ``mwldeppc`` fails with ``undefined: '@eti_800222FC'``. No source or compiler flag can reach it: ``@eti_800222FC`` is not an expressible identifier and ``mwcceppc.exe -help`` has no option that names or exports an extab symbol - the name and the binding are assembler-level output (``.pi/notes/resfile-flip.md``).
* This tool closes that gap as a post-compile step next to ``tools/elf/objalign.py``, chained into every MWCC rule by ``tools/project.py``. For each symbol the object defines under an ordinal name in its ``extab``/``extabindex`` section it computes the address the entry lands at - the section's claimed start from ``splits.txt`` plus ``st_value`` - renames the symbol to the map's spelling for that address (``@etb_%08X`` / ``@eti_%08X``, uppercase hex, dtk's own spelling) and sets the symbol's binding global. The rename is the half the linker needs for the *name*; the binding is the half it needs to accept the definition across objects - measured on `g3d/g3d_resfile`: rename alone, and rename plus a `.comment` `active_flags=0x08`, both still fail; rename **plus** a global binding links green and reproduces the DOL hash exactly.
* Only ``.symtab`` (``st_name``, ``st_info``) and ``.strtab`` (the appended names) are touched: no section's *contents* change, so the step is byte-neutral for the DOL by construction. It is a no-op for an object with no ordinal-named extab/extabindex symbol - the fast path decides that from the object alone, before reading ``splits.txt`` - and idempotent: a second run sees the map name rather than an ordinal and skips.
* One thing is deliberately left unrepaired: a promoted symbol still sits inside ``.symtab``'s *local* index range (``sh_info``), because restoring the local/global ordering would renumber the symbol table and with it every relocation's ``r_info`` symbol index - i.e. rewrite ``.rela*`` contents, the one thing this step must not touch. ``mwld`` resolves from ``st_info`` rather than from the ordering: measured over the whole build, 20 of the already-``Matching`` link inputs carry 94 renamed symbols and ``main.dol`` stays byte-identical.
* **A weak copy the link drops takes its entries with it (2026-10-21).** MWCC emits an inline function out of line,
  weak, in every TU that uses it, each copy with its own `extab` record and `extabindex` entry; mwld keeps the first
  definition and drops a later copy together with both. A weak `.text` function the **target object does not define**
  is such a copy (retail kept this unit's own copy otherwise): its entries have no map address, so they stay ordinal
  and **local**, and every later entry of the section is named at its offset minus the dropped entries' sizes
  (`lib.binary.extab.unwind_entries`). A global map name on a dropped copy's entry makes mwld die with internal error
  `ELF_linker.c` line 7133 (phase Optimizing) - bisected on ef/ef_drawlinestrategy by swapping one object on the
  build's own link line: the MWCC object with no post-step links, `objalign` alone links, `objextab` alone crashes,
  and making only the copies' four entry symbols local links with the DOL hash. Without a target object (or a weak
  function) the naming is the offset rule above. Measured over the whole build (800 objects): no section's bytes
  changed; the symbol tables of 11 objects changed, every changed row an `extab`/`extabindex` row; 9 of the 11 now
  define exactly the target object's `@etb_`/`@eti_` set (0 of 11 before); g3d/g3d_anmscn and ef/ef_drawstrategyimpl
  still differ because their own `extab` layout does (anmscn's `GetLightSetting` record is 8 bytes, the target's 24;
  strategyimpl's range spans three TUs). `ninja -k 0` 0 FAILED, `main.dol: OK`; with ef/ef_drawlinestrategy,
  ef_drawpointstrategy and ef_drawfreestrategy flipped to `Matching`, the link hashes
  bf4850739478caaedfe675949eb7c28595a7fde9 (before: the 7133 crash).
* One summary line per modified object; `-v` also prints every rename (and each dropped weak copy).

## Lib dependencies

binary.elf, binary.extab, project.splits.

## Test contract

Tier: fixture.
Today's selftest: in-file `selftest()` (`--selftest`) for the offset rule, and `tools/tests/elf/test_objextab.py` on
`lib.testing` (`FixtureTree`, `ElfBuilder`) for the weak-copy rule: the ef/ef_drawlinestrategy shape (ctor, two weak
dtor copies, `Draw`, dtor) keeps the copies' entries ordinal and local and names `Draw`'s `@etb_8000A344`; entries that
relocate `.text`+offset; a copy the target defines, no target object and a strong function the target lacks all keep
the offset rule; idempotence. The compile edge's inputs: `tools/tests/top/test_project.py`.

## Known gaps

a promoted symbol stays inside `.symtab`'s local index range (by design)
