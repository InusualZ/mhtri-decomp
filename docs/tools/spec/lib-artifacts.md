# `lib/artifacts` - The derived-artifact registry: inputs, rebuild command, cost, and `ensure()` under a policy

## Purpose

Says, for each derived build artifact of a tree, whether it still describes its inputs, why not, and how to rebuild it - and
makes it current when a tool needs it (`ensure`), under one policy rule (`auto` / `warn` / `refuse`).

## Users

`tudiscover` (`asm-dump`, default `auto`), `callers` (`asm-dump`, default: fall back to the split objects), `unitscore` and
`symdiff` (`report` / `objects`, default `refuse`), `recompile --measure` (`split`, default `refuse`), `landing/base.py`
(record-base runs the `report` command unconditionally), `integrate --no-build` (`report`, a warning), `lib.lanes.seed`
(`split_reasons` / `manifest_reasons` are `build_is_current`), `fresh.py` (the CLI).

## Public API

* `REGISTRY: {name: Artifact}` - `Artifact(name, what, path, inputs, deps, cost, cost_note, check, command, tree_local,
  touches_tracked)`; `get(name)`, `order(names)` (dependencies first, each once; a cycle raises `ValueError`).
* `Context(root, src=None, obj=None, report=None, runner=None, main=None)` - a `src`/`obj` pair scopes `objects` and
  `report` to one unit; `runner(argv, cwd)` replaces the subprocess (tests, probes).
* `status(name, ctx, check=None) -> Status`, `statuses(ctx, names=None)`; `Status(name, state, reason, command, cost, path,
  refreshed, seconds, note)`, `.fresh` (fresh or n/a), `.remedy()`, `.as_dict()`.
* `ensure(names, ctx, policy=None, default="refuse", out=stderr, lock_timeout=None, checks=None) -> [Status]`;
  `refresh(name, ctx, ...)` (one artifact, under its lock); `resolve_policy(policy, default)`,
  `effective_policy(policy, default, runner, out)`.
* `StaleArtifact` (refuse; a `SystemExit`), `RefreshFailed` (the command failed, or left it stale; a `SystemExit`).
* The shared rules: `split_reasons(build_root, input_root)`, `manifest_reasons(...)`, `newer_inputs(output, inputs)`,
  `ninja_plan(ctx, target)` (`ninja -n`, dry run), `SPLIT_INPUTS`, `MANIFEST_INPUTS`, `Lock`.

## The registry (measured 2026-10-04, this machine)

| artifact | inputs | refresh command | cost | deps |
| --- | --- | --- | --- | --- |
| `manifest` | `configure.py`, `tools/project.py`, `tools/ninja_syntax.py` (mtime vs `build.ninja`) | `python configure.py` | 0.5-2 s | - |
| `split` | `config.yml`, `symbols.txt`, `splits.txt`, the DOL, `mh3.sel` (mtime vs `config.json`) | `ninja build/RMHE08/config.json` | 1.8-1.9 s (`.ninja_log`, 415 objects) | manifest |
| `asm-dump` | `symbols.txt`, `splits.txt`, the DOL (sha1, `lib.refs.DumpStamp`) | `python tools/splits/dump_asm.py` | 4.2-6.0 s (415 files, replacing the old dump) | - |
| `tudiscover-graph` | the dump's count/bytes, `symbols.txt` sha1, tudiscover's SCHEMA and parse code | `python tools/splits/tudiscover.py cache` | 17 s cold, 1.9 s cached | asm-dump |
| `callers-index` | every dump file's name and content (`lib.refs.dump_signature`), `lib.refs.SCHEMA` | `python tools/units/callers.py --stats` | 11.8 s cold, 1.6 s cached | asm-dump |
| `objects` | a unit's include closure (`lib.report.unit_reasons`); tree: `ninja -n all_source` | `ninja <obj>` / `ninja all_source` | 0.2-24 s per unit | manifest |
| `report` | the objects, the split target objects, `objdiff.json`; unit: `lib.report.report_reasons`; tree: `ninja -n report.json` + newest `obj/*.o` | `ninja build/RMHE08/report.json` | 1.1-1.2 s + stale compiles | split, objects |
| `slot-build` | MAIN's split, report bytes and compile outputs (`lib.lanes.pool.verify`) | `python tools/units/slots.py refresh N` (from MAIN) | 0.16 s incremental, 0.71 s full (892 files, 42 MB) | - |

`docs/build-performance.md`'s ~18 s split and 200-400 s dump were measured at 13.5 k objects; at 415 objects both are seconds.

## Invariants and rules

* **One rule per artifact.** The checks call the existing rules - `DumpStamp.status`, `lib.report.unit_reasons` /
  `report_reasons`, `lib.lanes.pool.verify`, tudiscover's own `cache --check` (its stamp hashes its parse code) - never a copy;
  `seed.build_is_current` *is* `split_reasons` + `manifest_reasons`.
* **Policies.** `auto` rebuilds what is stale in dependency order, each once, under `build/tmp/fresh/<name>.lock` (exclusive
  create; a second process waits, re-checks and skips the rebuild another process just did; a lock whose holder pid is dead is
  taken over); `warn` prints one line per stale artifact and goes on; `refuse` raises `StaleArtifact` naming the artifact, its
  reason and its refresh command. Precedence: a tool's flag, then `$FRESH`, then the tool's default.
* **A fresh artifact prints nothing**, so a tool's output is byte-identical when everything is current.
* **A rebuild must leave the artifact fresh**: it is re-checked after the command, and a command that exits 0 but leaves it
  stale is `RefreshFailed`, never a silent pass. A rebuild over `EXPENSIVE_S` (60 s) announces its cost on one line first.
* **Never in the fixture test tier**: `auto` with no stub `runner` degrades to the tool's default (`warn` when that default is
  `auto`) and says so; `tools/selftest.py` also drops `FRESH` from every child's environment.
* **Tracked files**: only `split` may write one (dtk writes `symbols.txt`/`splits.txt` back when its analysis adds to them -
  measured: on the current map it rewrites neither, mtimes unchanged). Its refresh is wrapped in `git status` before/after and
  a change is reported (`Status.note` and a WARNING line); every `ninja` run does the same split already.
* A rebuild command runs with `FRESH=warn`, so it never cascades: its dependencies were refreshed first.
* Gitignored output only: every artifact lives under `build/` or is `build.ninja`/`objdiff.json`; nothing here edits a tracked
  file itself.

## Lib dependencies

`lib.proc`, `lib.testing`; lazily `lib.refs`, `lib.report`, `lib.repo`, `lib.lanes.pool`.

## Test contract

Tier: fixture (`tools/tests/lib/test_artifacts.py`, stub rebuild commands, temp trees). Pinned: `order` (deps first, once, a
cycle refused); `split_reasons`/`manifest_reasons` and `seed.build_is_current` agree on missing / fresh / stale; refuse names
the reason and command; warn prints one line and runs nothing; fresh prints nothing; auto rebuilds once and releases the lock;
a failing rebuild and a rebuild that leaves it stale both raise; three concurrent callers rebuild once; a dead holder's lock is
taken over and a live one past the timeout is refused; `FRESH` precedence; the fixture-tier degrade; the expensive-rebuild
line; the unit-scoped `objects`/`report` checks. Mutations measured to fail it: no lock (3 failures), no post-rebuild check
(6), `split_reasons` always empty (3), no fixture guard (a real rebuild escapes), a silent warn (4).

## Known gaps

* `objects`/`report` at tree scope read stale in MAIN until the next `ninja` after this lands: `Camellia/camellia.o` was
  rewritten outside ninja by `measure_selftest.integration_rows` (it compiled into the live tree, and the landing gate runs
  every selftest), so ninja saw "stored deps info out of date" and rebuilt it on every run. Reproduced in a worktree (one run
  of the old selftest makes `ninja -n all_source` list Camellia; the fixed one compiles in a scratch tree and leaves it at
  "no work to do"). `mwcc_matrix` still compiles into the real object on purpose (`expect=unit.obj_ours`).
* Every `dump_asm.py` run rewrites every `.s` (dtk has no skip-unchanged); the `callers-index` and `tudiscover-graph` are keyed
  on content (`lib.refs.dump_signature`), so a refresh with identical bytes keeps both: `callers --stats` after a no-change
  refresh 11.2 s (rebuild) before, 2.7 s (hit) after (2026-10-05). Both checks word a moved byte the same way
  (`lib.refs.DUMP_CHANGED`).
