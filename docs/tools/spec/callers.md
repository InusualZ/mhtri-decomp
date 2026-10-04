# `callers` - Who calls / who reads an address: the whole-DOL reference index built from the asm dump on addresses (never labels), cached; object-relocation fallback; `--range` referrer runs

<!-- generated from the module docstring of `tools/units/callers.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Who calls this function / who reads this data: the whole-DOL caller index, keyed on addresses.

## Users

the landing gate (1); profiles (`.claude/agents`) (6); skills (13); CLAUDE.md (2); docs (53); no tool imports it (WP3c:
`accessextent`, `dataclaim`, `dataclosure` read `lib.refs.RefIndex`, the same index and query)

## CLI

```
python tools/units/callers.py <address|name> [--json] [--code|--data] [--kind K[,...]]
[--pointers] [--limit N] [--rebuild]
python tools/units/callers.py --range 0x80799F98 0x80799FDC [--step 4] [--each] [--json]
# the per-address referrer runs over a range (the .sdata2 seam)
python tools/units/callers.py --stats          # what the index is, and how old the answer is
python tools/units/callers.py --selftest       # this file + callers_selftest.py (fixtures only)
```
Flags: `--code`, `--data`, `--each`, `--json`, `--kind`, `--limit`, `--pointers`, `--range`, `--rebuild`, `--selftest`, `--stats`, `--step`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: build/RMHE08/asm or obj/ -> build/tmp/callers/graph.json -> report.

## Invariants and rules

* **The trap this tool exists to survive: the dump is stale, and its labels are printed stale.** `build/RMHE08/asm/` is dtk's disassembly of the symbol map *as of the split* (see `tools/splits/dump_asm.py`); a `bl` whose callee was renamed afterwards still prints the OLD label, so grepping the dump for the new name finds nothing. The dump in this tree carries the proof: at 0x803A146C the canonical `menu/multi_result.s` prints `bl game_mode_sub_state_set1`, while the stale top-level copy `auto_fn_803A13B4_text.s` still prints `bl fn_803A1680` - one instruction, two labels, and only one of them is in `symbols.txt`. Therefore:
* **the graph is built on addresses.** A `bl`/`b` target is decoded from the instruction's own displacement (`NIA = CIA + EXTS(LI||0b00)`), so it is exact and needs no map at all; a data reference (`X@ha`/`@l`/`@sda21`/...) resolves its name through *the dump's own label table* (the `# <section>:0xOFF | 0xADDR | size:` headers), which is stale in exactly the same way the reference is, so an old name still lands on the right address;
* **names are resolved per run** through the current map, via `symedit`-style streaming access (never printing `symbols.txt` - non-negotiable 7), so a caller renamed since the dump is named correctly and a query on a *new* name is answered by address;
* **the index is cached** (`build/tmp/callers/graph.json`) and rebuilt only when the dump changes - a signature over every `.s` file's path, size and mtime. A `symbols.txt` edit does **not** rebuild it: the graph holds no name from the map. In this tree the build is 245 258 references over 54 256 target addresses and takes 8-10 s; a cached query answers in ~1.3 s.
* **What it reports, and what each column is evidence for.**
* `call` - a `bl`/`bla` whose target address the encoding decodes. This is the "who calls this" answer.
* `branch` - `b`/`ba` to a *symbol* (a tail call, or a jump into another function). The `.L_ADDR` branches inside one function are not callers and are not indexed.
* `addr` - the symbol's address is materialised (`lis r3, X@ha` + `addi r3, r3, X@l`). For a function this is the "who installs this as a task, who puts it in a table" answer, which a call-only grep misses: the quest field task `fn_8028BF1C` has no direct caller at all - it is only ever *taken* and handed to `Tsk_Change`.
* `read` / `write` - a load from / store to the symbol's address (the `.sdata` accesses that carry most game state). Classified from the mnemonic (`l*` loads, `st*` stores); `li`/`lis` and every other form that names the symbol is `addr`.
* `pointer` - a `.4byte X` entry inside a data object (run `--pointers` to list): a function-pointer table, a vtable, an `@eti_` exception-table index. The *site* is the containing object's address, not the exact word: the dump prints no address on a bare `.4byte`, and guessing one from the object's directive stream would be a second parser for a number nobody needs.
* `arg` - what the caller materialises in `r3` before a `bl`, inferred from the instructions immediately before the call (through `lib.ppc.decode_rw`, the tree's one register read/write decode): `0x0`, `&some_label`, `0 (r3 live-in)` when the preceding call's return flows in, `0?` when a branch or a truncated window separates them, and `? (opcode)` when it cannot be judged. It is an inference, and it says which way it is unsure instead of guessing.
* **Limits, stated so no count is over-read.** An indirect call (`bctrl`, or through a table) has no static target and is invisible *as a call*: it shows up as an `addr`/`pointer` reference to the callee when the address is materialised, and as nothing when it is loaded from memory. A data reference is coalesced at its `lis`+`addi` pair into one site. The dump's state is always printed; when it is stale the *texts* of the instructions are too (never the addresses), and `python tools/splits/dump_asm.py` refreshes it.
* **No dump is not "0 callers"** (the defect `tudiscover` was fixed for): a missing dump is its own message with the remedy, `exit 2`, and `"error": "no asm dump"` under `--json`.
* **It is a reader.** No `src/` edits, no renames, no writes outside `build/tmp/callers/`.

## Lib dependencies

`lib.refs` (the index, the cache, `RefMap`, `query`, `readers_of`, `range_report`, `DumpStamp`), `lib.cache`
(`stat_digest`), `lib.report` (`rel_path`), `lib.repo` (`VERSION`); `lib.repo.resolve_input` for the input locations.

## Test contract

Tier: fixture (fake asm dump, map, splits, cache).
Today's selftest (`tools/units/callers_selftest.py`): Fixtures only: a three-file fake asm dump (`build/<game>/asm`, including a stale top-level copy of one function), a fake `symbols.txt`/`splits.txt` and a temp cache - no build tree, no DOL, and no dump on disk, so the check count is identical in MAIN and in a fresh worktree. `callers.selftest()` holds the checks, so this entry point and `callers.py --selftest` cannot drift.
Target: `tools/tests/units/test_callers.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

two censuses (dump, objects) of unequal detail; `accessextent` re-decodes to repair. The parser, both index builds, the
cache (WP2a), the map, the query, the readers and the runs (WP3c) are `lib.refs`; this tool keeps the input locations, the
dump's verdict line and the rendering. Its selftest is still the in-file `selftest()` (150 checks; the object fallback's
fixture is an `ElfBuilder` now, not `callees._fixture_elf`).

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* **The question nothing else answered.** `tools/units/callees.py` answers "what does this unit call"; the inverse - "who calls this function, who reads this data" - had no tool at all, so it was answered with a hand-written grep over the dump (2800 `.s` files / 89 MB in this tree, one per split unit), and the quest recon lane wrote the 30-line version of *this*, calling it the most useful thing it built. This is that tool, supported, cached and self-tested.
