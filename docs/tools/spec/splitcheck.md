# `splitcheck` - Read-only checker of a `splits.txt`: 11 invariants per unit (`--baseline` is the phase-5 audit); renders and lints proposal files; owns the retail-text reference scanner `Ctx`

<!-- generated from the module docstring of `tools/splits/splitcheck.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

splitcheck.py - a read-only checker for a `splits.txt`: does every unit and every boundary satisfy what we know?

## Users

docs (34); imported by `applysplits`, `dataattach`, `matchinggain`

## CLI

```
python tools/splits/splitcheck.py --baseline [--json F] [--all] [--only INV[,INV]] [--unit REGEX] [--intervals]
python tools/splits/splitcheck.py --proposal F [--proposal G ...] [--emit-splits OUT] [--json F]
python tools/splits/splitcheck.py --selftest
```
Flags: `--all`, `--baseline`, `--data-by-reader`, `--dol`, `--emit-splits`, `--intervals`, `--json`, `--limit`, `--only`, `--outbox`, `--proposal`, `--readers`, `--selftest`, `--splits`, `--symbols`, `--unit`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: splits.txt, symbols.txt, DOL -> PASS/FAIL/UNKNOWN table, JSON.

## Invariants and rules

* `--baseline` checks the repository's current `config/RMHE08/splits.txt` and prints the audit list (phase 5 of the splits program, `docs/splits-program.md`): per invariant PASS/FAIL/UNKNOWN counts, the top defects, the suspected seams (the seam requests in `.pi/outbox/*.json`, the pool groups of `docs/pool-seams.md`). `--proposal` renders a proposal file (`.pi/splits/phase<N>-<band>.json`, format in `docs/splits-program.md`) into a candidate `splits.txt` (strong + medium cuts applied, `guess` cuts merged, never emitted), lints the proposal, checks the candidate and prints the delta against the baseline. Nothing is written except `--json` / `--emit-splits`.
* A phase 2 proposal (`docs/splits-program.md`, "Phase 2 files") may have no `units` and carries `attach` rows (data ranges given to a unit of the candidate; the legacy names `data_attach` and `attachments` load as aliases with a warning), `unowned_data` rows (the deferrals), `moves`, and units without `.text` placed by `after`. `--readers SECTION:START-END` prints each map symbol of a data range with its owner in the candidate and the units whose decoded text reads it.
* `--baseline --unit REGEX` also prints, for the matching units, one `detail` line per `.ctors`/`.dtors` word (the function, its end, the closure end, the end with the unit's own vtable slots, the unit end) and, with `--intervals`, one `pooldup` line per pool value held at two addresses (both addresses, the last read of the first, the first read of the second, the interval a TU starts in): the numbers a proposal's `reproduce` row quotes. `--proposal` also reports every proposed cut, a `guess` included, that a `scope:local` data object is read across (a static is one TU's) and exits 1 on one.
* Invariants (one verdict per unit per invariant; PASS / FAIL / UNKNOWN, `-` = not applicable; evidence = an address):
* order link order: no overlapping ranges, one range per section, no cycle between the units' section orders (`.bss`/`.sbss`/data sequences against the text sequence are the same graph) coverage every map symbol sits inside exactly one unit's range of its section (no gap under a symbol, no straddle) text-cut a unit's `.text` starts and ends on a function symbol extab every `extabindex` entry is owned by the unit that owns its function and its `extab` record ctors/dtors the `.ctors`/`.dtors` word points at a function of the unit; a unit has one `.ctors` word (`__sinit`), and the TU ends at the end of the closure of the sinit's local callees and address-taken functions (MWCC emits the deferred constructors, registered destructors and `b ctor` thunks after it), not at the sinit's own end pool idea 94: the `.sdata2` float/double and `.sdata` string pool of a unit is read only (by a load) by that unit, runs in first-use order (per function), and holds each value once data-order docs/data-order-seams.md: no strong V->S / zigzag seam strictly inside the unit's `.data` vtable a vtable sits in the unit whose text holds one of its slots or stores it jumptable a jump table sits in the unit that reads it and branches into bss a local `.bss`/`.sbss` object is read by the unit that holds it local-static a `scope:local` data object (not a pool literal) is read by the text of one unit only: a static is private to its TU, so a local read from both sides of a boundary says the boundary is not a TU edge
* The text references (pool first-use, jump table and bss readers) are decoded from the retail `.text`: a `lis` + `addi`/ `ori`/load pair, or an r13/r2 small-data access. That is heuristic evidence (a register reused across a branch can fool it) and is stated as such in every finding that depends on it.

## Lib dependencies

ppc, refs, binary, project, findings.

## Test contract

Tier: fixture (mini DOLs).
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/splits/test_splitcheck.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

the proposal half is retired (retired.md); `--baseline` stays
