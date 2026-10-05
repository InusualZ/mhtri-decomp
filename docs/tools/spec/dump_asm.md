# `dump_asm` - Regenerate dtk's per-unit asm dump on demand (write_asm is off), replacing the old one, and stamp it

<!-- generated from the module docstring of `tools/splits/dump_asm.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Regenerate the split disassembly dump that `tudiscover` and `callers` read. The new dump **replaces** the old directory,
so a `.s` the split no longer writes never survives a refresh.

## Users

skills (2); CLAUDE.md (1); docs (11)

## CLI

```
python tools/splits/dump_asm.py              # dump the invocation's tree, replace its dump, then stamp it
python tools/splits/dump_asm.py --root DIR   # the same for DIR (its config, its dtk, its build/)
python tools/splits/dump_asm.py --check      # report the dump's age only, no split (exit 1 if not fresh)
python tools/splits/dump_asm.py --dry-run    # print the command it would run
```
Flags: `--root`, `--check`, `--dry-run`.
Exit codes: 0 ok; 1 `--check` not fresh, or the split wrote no `.s`; dtk's own exit code when the split fails; 2 no
toolchain, unreadable inputs, or a swap that could not finish. No `--json`.

## Inputs and outputs

Inputs -> outputs: `<root>/config/RMHE08/config.yml` (+ the map, splits and DOL it names) -> `<root>/build/RMHE08/asm/`
and its `.stamp.json`. Scratch, all under `<root>/build/RMHE08/`: `dump_asm.yml` (the temp config, deleted after the run),
`.dump_asm-stage/` (the split's `out_dir`, kept between runs; its `asm/` is moved out on success) and
`.dump_asm-retired.<pid>/` (the old dump during the swap).

## Invariants and rules

* `--no-update` keeps the hand-edited `symbols.txt`/`splits.txt` out of the run (dtk's own "for build systems" mode), and the dump is stamped with the hashes of the three files it is a function of, so `tudiscover stats` can say when it has outlived a symbol edit. That matters because stale asm is silent: `asm_files()`'s docstring records a stale copy printing `bl fn_80456DD4` where the canonical one prints `bl _savegpr_14`, which zeroes a codegen fingerprint.
* Nothing here is written outside `<root>/build/<game>/` (`dump_asm.yml`, the stage, the dump itself, the stamp).
* **One tree.** The config, the toolchain and the output all come from `--root` (default: the invocation's tree, never the
  tree this file lives in), so `MAIN/tools/splits/dump_asm.py` run in a worktree dumps the worktree. `lib.artifacts` runs
  each tree's own copy with the tree as cwd, which resolves to the same tree.
* **The dump is replaced, never merged.** dtk's split writes into the stage; only after a zero exit that wrote at least
  one `.s` is the old `asm/` renamed to the retired name (its stamp goes with it), the staged `asm/` renamed in, and the
  retired directory deleted. A failed or empty split, or a swap that cannot finish (a file held open on Windows), leaves
  the old dump **and its stamp** in place - they still describe each other - and the failed second rename puts the old
  directory back.
* **The stamp is written last** and carries the input hashes read **before** dtk ran: a map edit made during the run
  reads `stale`, never `fresh`.
* **It deletes only what it owns**: `build/<game>/asm`, the stage, the stage's `asm/` and a retired dump, each a real
  directory (not a link or junction) under the tree's real `build/<game>/` (`removal_refusal`); any other path is a
  `SystemExit` that deletes nothing. A leftover from an interrupted run is removed at the start of the next.

## Lib dependencies

repo, refs (`DumpStamp`, `dump_files`). No tool import: the `tudiscover` edge is gone.

## Test contract

Tier: fixture. `tools/tests/splits/test_dump_asm.py` (replaces `dump_asm_selftest.py`) on a temp tree with a fake dtk
runner: the temp-config rewrite (CRLF kept), the tree is `--root`'s, a refresh removes a stale unit and stamps the new
count, a failed or empty split changes neither the dump nor the stamp, a failed swap restores the old dump, a mid-run map
edit stamps `stale`, leftovers are cleared, the removal guard refuses every path it does not own (and deletes nothing),
the CLI exits; plus `tudiscover`'s view of the stamp state machine. Each of five mutations (merge instead of swap, no
guard, post-run input hashes, swap after a failed split, no restore) fails at least one check.

## Known gaps

* It is the registry's `asm-dump` refresh (`lib.artifacts`): `tudiscover` runs it itself when the dump is stale, `callers`
  with `--refresh`, `fresh.py refresh asm-dump` by name. Measured 2026-10-05 at 415 files: 4.2-6.0 s wall (dtk 3.4 s, the
  retired dump's delete 1.3 s); the merge-in-place version took 3.9 s and left every stale file behind (MAIN's dump: 2 489
  files, of which a fresh split writes 415 - 2 074 stale).
* dtk rewrites every `.s` on each run (no skip-unchanged); `callers`' index and `tudiscover`'s graph are keyed on the
  dump's content (`lib.refs.dump_signature`), so a refresh that changes no byte keeps both caches.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* `build/RMHE08/asm/` is dtk's disassembly of every split unit - one `.s` per unit, ~92 MB across 13 500 files. Nothing in the build reads it (`config.asm_dir = None`, and no ninja edge names it); `tools/splits/tudiscover.py` does, so `config/RMHE08/config.yml` sets `write_asm: false` and this tool produces the dump when a session needs it. That turns a cost the split paid on *every* run - `symbols.txt` is one of its dirty-check inputs, so a rename re-dumps all of it, ~200 s of a ~380 s `ninja` - into one full split per attribution session.
