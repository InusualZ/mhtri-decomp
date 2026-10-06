# `objsame` - Whether every compiled object of two build trees is the same modulo `@N`/`$N` numbering and named renames

## Purpose

Compares `build/RMHE08/src/**.o` of two build trees (MAIN and a slot by default, or a commit's own objects built in a
scratch directory) section by section, relocation by relocation and defined symbol by defined symbol, with every
compiler counter (`@<digits>`, `$<digits>`) read as `@N`/`$N` and an optional rename map applied to the base side, and
prints the units that differ. It is the byte-neutrality proof of a header-only or rename-only change.

## Users

Integrators and lanes proving a shared-header, declaration-only or rename-only change moved no object (the network pilot
wrote `objsnap.py` by hand for this, about 15 minutes per integration).

## CLI

```
python tools/objdiff/objsame.py                      # MAIN vs the invocation's tree
python tools/objdiff/objsame.py BASE TREE            # two named trees
python tools/objdiff/objsame.py --base-tree main     # the tree vs the objects of commit `main` (built once, cached)
python tools/objdiff/objsame.py --base-tree $(git merge-base main HEAD) --renames-from-git   # "only renames differ"
python tools/objdiff/objsame.py --rename-map renames.txt          # `old new` / `old=new` lines (symedit rename-batch input)
python tools/objdiff/objsame.py --rename-map fn_80001000=em_frame_check,fn_80002000=em_after
python tools/objdiff/objsame.py --unit "Network/*"   # only units whose stem matches (repeatable)
python tools/objdiff/objsame.py --all-sections       # .comment and the string tables too
python tools/objdiff/objsame.py --all-objects        # also objects of unregistered units (orphans; skipped by default)
python tools/objdiff/objsame.py --json               # lib.findings schema + base, tree, compared, same, renamed, skipped, counts
```
Exit codes: 0 every compared object is the same or differs only by the named renames, and none is one-sided; 1 a unit
differs, is unreadable or exists in one tree only; 2 a tree has no `build/RMHE08/src/`, BASE and `--base-tree` are both
given, a rename map is malformed, or the base tree could not be exported or built.
`--json`: `{tool: "objsame", rows, ok, summary, base, tree, compared, same, renamed: [{unit, renames}], skipped, counts,
renames}`; one FAIL row per differing, unreadable or one-sided unit, its `detail` the reasons. The last text line is
`objsame: N identical, M differ only by renames, K differ, J one-sided (of C compared; S unregistered object(s) skipped)`,
and every renamed-only unit is listed `RENAMED <unit>  old -> new, ...` with the renames it used.

## Inputs and outputs

Reads the two trees' compiled objects and `configure.py` (the registered units). Writes stdout only - except
`--base-tree REF`, which exports REF into `build/tmp/objsame/base-<sha12>/` of the compared tree (gitignored) and runs
`configure.py` and `ninja` there. Without `--base-tree` it builds nothing: build both trees first.

## Invariants and rules

* "The same" is: every non-metadata section present on both sides with equal size and bytes; every relocation section's
  sorted `(offset, type, symbol)` rows equal with symbol names normalised; the defined symbols' `(name normalised,
  section, value, size, bind)` equal (`STT_FILE` rows ignored). The first difference of each kind is named.
* **Counters**: MWCC numbers its labels (`@123`, `@7@func@var`) and its local statics (`__arraydtor$6556`) in emission
  order, so one literal or local added in a header renumbers every later one of every includer; that is not a codegen
  change and is never reported. Measured 2026-10-04 at 042956de2, MAIN vs three slots: 21/14/0 units differ, against
  92/88/0 when names are compared raw; `$N` added 2026-10-06 (one `__arraydtor$N` unit of 367 at 9db8fbe62).
* `--all-sections` compares `.strtab` string by string, normalised (so `@99` -> `@1000` is not a byte shift), and
  `.symtab` with every `st_name` offset zeroed (the names are compared through the symbol rows). Before, the raw tables
  made 113 of 367 units differ at 9db8fbe62 where the default view had 34.
* **Renames** map a base-side name to the tree's (applied before the counter normalisation). An object that differs
  without them and is the same with them is "differs only by renames" (`RENAMED`, not a failure); the renames it used
  are listed. `--renames-from-git [REF]` derives them from the tree's `symbols.txt`: a row renamed at an unchanged
  address against REF (`lib.project.symbols.rename_pairs`; default REF: `--base-tree`'s, else `main`), plus every
  generated stem the map already names otherwise (`stem_renames`: a base source that still spelled `fn_<ADDR>` for a row
  the map had renamed). A stem whose address is wrong (`fn_80501EE0` for `vec3_scale` at `0x80051EE0`) is not a rename.
* **Registered units only**: an object whose stem no `Object(...)` of that tree's `configure.py` names is skipped and
  counted (`lib.artifacts.registered_stems`, the `orphan-objects` artifact's rule); `--all-objects` compares them.
* **`--base-tree REF`** exports the commit with `git archive` (never a worktree: nothing is registered with git), copies
  `orig/RMHE08`'s split inputs from the tree or MAIN (never a link), runs `configure.py` pointed at the existing toolchain
  (`--compilers/--sjiswrap/--dtk/--binutils/--objdiff` of the tree, else MAIN) and `ninja all_source` (or only the
  objects of the `--unit` globs). The first build runs the commit's own split. A marker file says the export finished;
  without it the directory is wiped and redone, and a failed `ninja` removes the marker. **The touch-the-sources trap**:
  a lane that copied a commit's sources over a built tree with `shutil.copy2` kept their old mtimes, so ninja saw every
  object newer than its source and rebuilt nothing - the comparison was against the wrong objects. The export never
  copies an object in, so every requested object is compiled from that commit's sources; reuse is only of the export's
  own build.
* A unit present in one tree only is reported (`BASE`/`TREE`), never silently skipped.

## Measured

* 2026-10-06, the tree at 65b341058 against `--base-tree 9db8fbe62` (two landings apart): 367 compared, 118 unregistered
  objects skipped; without renames 334 identical / 33 differ; with `--renames-from-git` 334 identical / 30 differ only
  by renames / 3 differ (`ef/ef_emitter` moved a data label, `ef/ef_particlemanager` and `lobby/lb_companion_ui` call a
  stem whose address is not the map row's). The export, split and 367 compiles took 22 s; a cached re-run 1.8 s.

## Lib dependencies

`lib.objcompare` (`object_sections`, `load`, `first_difference`, `differing_bytes`), `lib.artifacts`
(`registered_stems`, `SPLIT_INPUTS`), `lib.project.symbols` (`rename_pairs`, `stem_renames`, `read`), `lib.git`,
`lib.proc`, `lib.findings`, `lib.repo`, `lib.binary.elf`.

## Test contract

Tier: fixture (`tools/tests/objdiff/test_objsame.py`, 31 checks, objects built with `ElfBuilder` in a `FixtureTree`, a
`GitFixture` for the git-derived renames and the base tree): renumbered `@N` and `$N` labels are the same; with
`--all-sections` a label of another length is the same and a real name change differs; a `.text` byte, a relocation
name, a `.sdata2` constant and a new symbol each differ and are named; one-sided objects are reported; `--unit` narrows;
a rename map turns a renamed object into `renamed` with the renames used, a non-renamed difference still differs, the
file/inline/malformed forms; `rename_pairs` and `renames_from_git`; unregistered objects skipped and `--all-objects`;
`--base-tree` with a stub runner (exported below `build/tmp/objsame/base-<sha>`, orig copied not linked, configure then
ninja of the matching objects, a finished export reused, an unfinished one wiped); the CLI's exit codes and summary.
Mutations measured: `@`-only counters fail 3, an un-normalised `.strtab` 1, an un-zeroed `.symtab` 1, ignoring the
rename map 3, no registered filter 2.

## Known gaps

* It does not compare the target split objects (`build/RMHE08/obj/`): those come from the DOL split, and
  `verifyunit`/`land`'s drift row already fingerprint them.
* A `.comment` difference is ignored unless `--all-sections` (a compiler-version change would show in the code anyway).
* Base trees accumulate under `build/tmp/objsame/` (about the size of one `build/RMHE08` each); nothing prunes them.
