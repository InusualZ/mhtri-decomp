# `unitinfo` - List the units with source, or show what a unit spec resolves to and its exact compile command

## Purpose

Lists every unit that has source in a tree, or resolves one unit spec and prints its source, our object, the split
target and the compile command ninja would run, split into compiler, flags and tail. It is the `units`/`info` half
of the skill's `mt.py` (WP6: it replaces `tools/unitutil.py`'s `main`, whose output it keeps byte for byte).

## Users

`.claude/skills/mwcc-unit-matching/scripts/mt.py units|info`; `docs/matching/toolbox.md`; `ideas.py`'s demo hint.

## CLI

* `unitinfo.py` - `N unit(s) with source in this repo:` then one `  <report name>  <source>` line per unit.
* `unitinfo.py -u <unit>` (or the positional) - `unit`, `src`, `obj`, `target`, `compiler`, `flags`, `tail` lines.
* `--root DIR` reads another tree (default: the caller's tree, `lib.repo.repo_root`).
* Exit 0; an unresolvable spec or a unit ninja has no command for exits 1 with the reason (`SystemExit`).

## Inputs and outputs

Reads `src/`, `build/<version>/obj` (the version list) and `ninja -t commands`; writes stdout only.

## Invariants and rules

* A unit spec is any spelling `lib.units.Unit.resolve` accepts (`<Lib>/<file>`, `main/<Lib>/<file>`, `src/...`, a bare
  stem shared by no other unit, a built object path); an ambiguous stem is refused with the candidates.
* The compile command is the *exact* line the build would run (`lib.units.split_command`), including whatever
  `configure.py` put in that unit's `cflags`.
* A launch Windows refuses transiently (WinError 5) is retried (`lib.proc.install_spawn_retry`).

## Lib dependencies

`lib.units`, `lib.repo`, `lib.proc`.

## Test contract

Tier: fixture (`tools/tests/units/test_unitinfo.py`): the listing of a `FixtureTree` (nested and top-level units),
and one unit's resolution with a stub ninja runner (paths, the split, the target asked for, an unknown unit refused).

## Known gaps

* None.
