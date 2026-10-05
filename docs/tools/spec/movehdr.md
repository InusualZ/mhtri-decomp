# `movehdr` - Move every tracked header from `include/` beside the sources under `src/`, proving no `#include` changes target

## Purpose

Carries out the owner's 2026-10-05 ruling: the top-level `include/` folder goes, every header lives under `src/`
(`include/P` -> `src/P`, with an exception table), and the compile's include root becomes `src/`. Before it writes
anything it simulates every quoted `#include` in both layouts and refuses if one would reach a different file.

## Users

The orchestrator's header-program batch (step 3), once. Afterwards `--dry-run` is the check that the tree is in the
finished state ("nothing to do").

## CLI

```
python tools/units/movehdr.py --dry-run          # the plan: counts, include rewrites, configure.py flags, refusals
python tools/units/movehdr.py                    # carry it out (git-stages the renames, the rewrites and configure.py)
python tools/units/movehdr.py --exception include/a/b.h=src/c/b.h   # one more exception to the mirror rule
python tools/units/movehdr.py --json             # the plan as JSON (moves, done, rewrites, collisions, changed, ...)
python tools/units/movehdr.py --selftest
```
Exit codes: 0 planned/done, 1 refused (nothing written), 2 could not run.

## Inputs and outputs

Reads `git ls-files`, every tracked `.c/.cp/.cpp/.h/.hpp/.inc/.s` under `src/` and `include/`, the untracked
`build/RMHE08/include/` (the second `-i` root) and `configure.py`. Writes only: the moved headers (filesystem rename,
then one `git add -A --pathspec-from-file` over old and new paths), the include lines of the exception table, the two
`configure.py` flag lines (`"-i include",` -> `"-i src",`, assembler `"-I include",` -> `"-I src",`), and removes the
emptied `include/` directories.

## Invariants and rules

* **The mapping**: `include/P` -> `src/P`; `EXCEPTIONS` overrides it. Today one entry:
  `include/nw4r/g3d/g3d_resmat.h` -> `src/g3d/g3d_resmat.h` (it declares `src/g3d/g3d_resmat.cpp`'s symbols, so it moves
  beside that unit; its three includers are rewritten to `"g3d/g3d_resmat.h"`).
* **Refused, nothing written**: a destination that exists (case-insensitive: Windows), two sources mapping to one
  destination, an include whose target would change, or a `configure.py` that does not carry exactly one of each flag
  spelling.
* **The include simulation** resolves every quoted include under **both** search orders (the including file's
  directory first, and last), case-insensitively, against the roots `include` + `build/RMHE08/include` before and `src` +
  `build/RMHE08/include` after; a line whose old target is an exception source is rewritten to the new spelling and
  re-checked. Angle-bracket includes are counted (zero on this tree).
* **Stem clashes are reported, not refused**: a header landing as `src/D/stem.h` while a unit `src/D2/stem.c(pp)`
  exists in another directory. `Ownership._owns` matches the module-qualified stem (`D/stem`), so these do not change
  ownership; they are listed so a reviewer can confirm it.
* **Idempotent**: a tree with nothing tracked under `include/` is the finished state; the plan is empty, the simulation
  resolves the tree against itself and the exit is 0.

## Lib dependencies

`lib.cli` (the entry point). Git and the filesystem directly.

## Test contract

Tier: fixture. `tools/tests/units/test_movehdr.py` builds a temp git repo (a header tree, an exception, a local-relative
`types.h` include, `configure.py`) and pins: the dry-run plan and that it writes nothing; the apply (renames staged, the
exception's includer rewritten, the flags switched, `include/` gone); idempotence; and the three refusals (an existing
destination, a case-insensitive clash, an include that would change target - the mutation check: a `src/types.h` that
shadows the moved one is refused).

## Known gaps

* It does not edit comments that mention `include/` paths (292 source files carry 752 such mentions at 0e592b75d); a
  comment sweep is separate, measured work.
