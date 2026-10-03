# `dump_asm` - Regenerate dtk's per-unit asm dump on demand (write_asm is off) and stamp it with the input hashes

<!-- generated from the module docstring of `tools/splits/dump_asm.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Regenerate the split disassembly dump that `tudiscover` reads.

## Users

skills (2); CLAUDE.md (1); docs (11)

## CLI

```
python tools/splits/dump_asm.py              # dump, then stamp it
python tools/splits/dump_asm.py --check      # report the dump's age only, no split (exit 1 if not fresh)
python tools/splits/dump_asm.py --dry-run    # print the command it would run
```
Flags: `--check`, `--dry-run`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: DOL, map -> build/RMHE08/asm/.

## Invariants and rules

* `--no-update` keeps the hand-edited `symbols.txt`/`splits.txt` out of the run (dtk's own "for build systems" mode), and the dump is stamped with the hashes of the three files it is a function of, so `tudiscover stats` can say when it has outlived a symbol edit. That matters because stale asm is silent: `asm_files()`'s docstring records a stale copy printing `bl fn_80456DD4` where the canonical one prints `bl _savegpr_14`, which zeroes a codegen fingerprint.
* Nothing here is written outside `build/<game>/` (`dump_asm.yml`, the dump itself, the stamp).

## Lib dependencies

repo, proc, cache.

## Test contract

Tier: fixture (temp config).
Today's selftest (`tools/splits/dump_asm_selftest.py`): The risk `write_asm: false` introduces is a dump that silently outlives the map it was generated from (stale asm zeroes codegen fingerprints - see `asm_files()`'s docstring), so the two things worth pinning down are the temp-config rewrite that keeps the repo's own `config.yml` untouched, and the state machine in `tudiscover.asm_stamp_status`: missing / unstamped / fresh / stale / truncated. Both read their inputs from module globals, so this points those at a temp tree: no dtk, no real dump, no writes outside the temp directory.
Target: `tools/tests/splits/test_dump_asm.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* `build/RMHE08/asm/` is dtk's disassembly of every split unit - one `.s` per unit, ~92 MB across 13 500 files. Nothing in the build reads it (`config.asm_dir = None`, and no ninja edge names it); `tools/splits/tudiscover.py` does, so `config/RMHE08/config.yml` sets `write_asm: false` and this tool produces the dump when a session needs it. That turns a cost the split paid on *every* run - `symbols.txt` is one of its dirty-check inputs, so a rename re-dumps all of it, ~200 s of a ~380 s `ninja` - into one full split per attribution session.
