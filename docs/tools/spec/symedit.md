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
symedit.py merge-batch <file> [--dry-run] [--no-refs]   # lines: "merge <phantom> <previous> <size>",
                                                         # "fold <label> <object> <size>", "size <object> <size>"
symedit.py split <row> <offset> <new-name> [--scope S] [--dry-run]   # the inverse of a merge: shrink <row> to <offset> bytes,
                                                         # add <new-name> at address+offset with the remainder
symedit.py --selftest                            # the checks, against temp fixtures only
```
Subcommands: `find`, `show`, `at`, `range`, `refs`, `check`, `rename`, `rename-batch`, `merge-batch`, `split`.
Flags: `--code-only`, `--count`, `--dry-run`, `--file`, `--force`, `--json`, `--limit`, `--no-refs`, `--roots`, `--section`, `--selftest`, `--type`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.
* `rename` / `rename-batch --rewrite [--comments]`: after the map write, rewrite every code reference to the old name
  under `src/` (`lib.cscan.rewrite_identifiers`); `--comments` also rewrites comment mentions. Never
  rewritten: a string or char literal, an `#include` line, a token beside `/` or `\` or followed by a source suffix (a
  path), a token after `@`, `$`, `.` or before `@`, `$` (a string-table, section or member spelling); a mangled
  `name__Fv` never matches `name`. Byte-exact, line endings kept; a re-run is a no-op.
* `rewrite-batch FILE [--comments] [--dry-run]`: the same sweep without a map edit - a lane's stale spelling of a name
  the map already changed.

## Inputs and outputs

Inputs -> outputs: symbols.txt -> rows; rename edits.

## Invariants and rules

* `config/RMHE08/symbols.txt` is ~65 700 lines / 4.5 MB and the per-module RSO maps add 4 462 more lines. Never paste those files into a prompt or read them whole - go through this script, which prints only the lines you asked for and changes only the name token you asked it to change.
* `rename` writes through `lib.project.symbols.write_text` (a `lib.text.Transaction`, docs/plan.md 7.12): the edit is a temp file + `os.replace` transaction that restores the previous bytes exactly if it fails, and the map's own line ending is preserved. Before a byte is written it asserts the shape a rename depends on - the old name is defined exactly once, on a line that parses as a map line, and the rewrite yields a line that parses back as the new name. Re-applying a rename that is already in the file is a no-op. It refuses when the new name is already taken (unless `--force`), when `--force` would still leave one name at two addresses, and when the new name is not a valid symbol name; it warns about in-repo references to the old name (a rename is always two edits: this file *and* the source - see CLAUDE.md -> Conventions -> "Commenting and naming").
* **A valid name is what dtk and objdiff carry** (`lib.project.symbols.VALID_NAME_RE`, 2026-10-06): an identifier
  start or `@`/`$`, then word characters and `.$@<>,-\`, so template manglings
  (`__ct__Q34nw4r2ut19TagProcessorBase<c>Fv`, `ofs_to_obj<...>__FPCvl`), MWCC's `@LOCAL@`/`@GUARD@` labels and
  `__sinit_\PatConnection_cpp` rename like any other name; whitespace, `=`, `;`, `:` and `/` are refused (the line
  could not parse back). Measured: three probe rows renamed to `probe_tmpl__Q24nw4r9Probe<i,c>Fv`,
  `@GUARD@probe_guard__Fv@x` and `__sinit_\probe_unit_cpp` re-split, linked `main.dol: OK`, and appear by those names
  in the split object and on objdiff's target side. The reference scan (`refs`, the rename warning, the merge
  refusal) matches a name with `lib.project.symbols.name_pattern` - not inside a longer identifier - because `\b`
  finds no boundary before `@` or after `>`.
* `merge-batch` is the other half of `tools/symbols/phantom.py` (docs/plan.md 7.9): a phantom is an unnamed `fn_*` that is really the previous function's dead epilogue, so a merge grows the previous symbol's `size:` and deletes the phantom's line. Per row it refuses - before any write - unless both symbols are defined exactly once, in the same section, the previous ends exactly at the phantom's address, no other name sits at that address, the stated size is exactly the two sizes added, the two scopes agree, and the phantom has no in-repo reference. An already-merged row is a no-op. When the plan's previous name is stale (a rename landed after `phantom.py` ran), the refusal names the symbol that actually ends at the phantom's address instead of guessing.
* **Data objects (2026-10-06).** `merge`/`fold <label> <object> <size>` on two `type:object` rows folds a stray label
  into one data object: the label may sit at the object's end or inside it, the size is the union's (`max(ends) -
  object`; a label inside leaves it as it is), no other symbol may start inside the result, the scopes may differ (a dtk
  label carries none) and the label must have no in-repo reference. `size <object> <size>` sets a data object's size
  (growing or shrinking; never a function, never over another symbol's start). One row per object per batch. Replay:
  NET-A's hand-written fold of `lbl_80600042` into `PacketTable_BaseOffset_ID1` (0x1732 + 0x4BE) on the map before
  899663afe gives the landed row byte for byte, apart from its separate rename to `patPacketTable`.

## Lib dependencies

project.symbols, text, cscan (`--rewrite`), repo (the invocation's tree, resolved on first use - never at import).

## Test contract

Tier: fixture (temp maps).
Today's selftest: in-file `selftest()` (`--selftest`). The name rule: `tools/tests/lib/test_project.py`
`test_template_and_local_names` (seven real names valid and renamed and parsed back, six malformed ones refused,
`name_pattern` on `@`/template names; the old `[A-Za-z_][\w.$]*` rule fails it, a `\b` pattern fails 1). The data
merges: `test_data_merges` (patPacketTable's end fold, an inner label, a resize, the no-op, the refusals, the batch-file
grammar); a resize that plans nothing fails 5. The split: `test_split_plans` (the shrunk row and the remainder row in both line endings, scope inherited/replaced, the re-apply, eight refusals).
Target: `tools/tests/symbols/test_symedit.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

the proxy cannot resize or delete a symbol except through `merge-batch` (and `split` shrinks the row it cuts)
