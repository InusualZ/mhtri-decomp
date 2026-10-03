# `symedit` - Query and surgically edit `symbols.txt` without loading it: find/show/at/range/refs/check, rename(-batch), merge-batch; the map parser other tools import

<!-- generated from the module docstring of `tools/symbols/symedit.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Query and surgically edit a dtk symbol map without loading it into context.

## Users

configure.py / the build (1); profiles (`.claude/agents`) (13); skills (22); CLAUDE.md (5); docs (26); imported by `callees`, `datagap`, `dumpmap`, `m2cinput`, `phantom`, `promote`, `symbolpreflight`, `unwindcut`

## CLI

```
symedit.py find   <regex> [--limit N] [--section .text] [--type function]
symedit.py show   <name> [<name> ...]
symedit.py at     <address> [--count N]          # symbols around an address, in order
symedit.py range  <start> <end> [--section S]    # members of a split range
symedit.py refs   <name> [--roots src include docs] [--code-only]
symedit.py check                                 # duplicate names / addresses, bad lines
symedit.py rename <old> <new> [--dry-run] [--force] [--no-refs]
symedit.py rename-batch <file> [--dry-run]       # lines: "old new" (# comments allowed)
symedit.py merge-batch <file> [--dry-run] [--no-refs]   # lines: "merge <phantom> <previous> <size>"
symedit.py --selftest                            # the checks, against temp fixtures only
```
Subcommands: `find`, `show`, `at`, `range`, `refs`, `check`, `rename`, `rename-batch`, `merge-batch`.
Flags: `--code-only`, `--count`, `--dry-run`, `--file`, `--force`, `--json`, `--limit`, `--no-refs`, `--roots`, `--section`, `--selftest`, `--type`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: symbols.txt -> rows; rename edits.

## Invariants and rules

* `config/RMHE08/symbols.txt` is ~65 700 lines / 4.5 MB and the per-module RSO maps add 4 462 more lines. Never paste those files into a prompt or read them whole - go through this script, which prints only the lines you asked for and changes only the name token you asked it to change.
* `rename` writes through `tools/units/sharedfiles.py` (docs/plan.md 7.12): the edit is a temp file + `os.replace` transaction that restores the previous bytes exactly if it fails, and the map's own line ending is preserved. Before a byte is written it asserts the shape a rename depends on - the old name is defined exactly once, on a line that parses as a map line, and the rewrite yields a line that parses back as the new name. Re-applying a rename that is already in the file is a no-op. It refuses when the new name is already taken (unless `--force`), when `--force` would still leave one name at two addresses, and when the new name is not a valid symbol name; it warns about in-repo references to the old name (a rename is always two edits: this file *and* the source - see CLAUDE.md -> Conventions -> "Commenting and naming").
* `merge-batch` is the other half of `tools/symbols/phantom.py` (docs/plan.md 7.9): a phantom is an unnamed `fn_*` that is really the previous function's dead epilogue, so a merge grows the previous symbol's `size:` and deletes the phantom's line. Per row it refuses - before any write - unless both symbols are defined exactly once, in the same section, the previous ends exactly at the phantom's address, no other name sits at that address, the stated size is exactly the two sizes added, the two scopes agree, and the phantom has no in-repo reference. An already-merged row is a no-op. When the plan's previous name is stale (a rename landed after `phantom.py` ran), the refusal names the symbol that actually ends at the phantom's address instead of guessing.

## Lib dependencies

project.symbols, text, git, repo.

## Test contract

Tier: fixture (temp maps).
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/symbols/test_symedit.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

the proxy cannot resize or delete a symbol except through `merge-batch`
