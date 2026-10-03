# `splitcheck` - read-only audit of `splits.txt`: 12 invariants per unit, and the retail-text reference scanner `Ctx`

## Purpose

Does every unit and every boundary of the current `config/RMHE08/splits.txt` satisfy what the retail DOL, `symbols.txt` and the
MWCC/mwld emission rules say? One verdict per unit per invariant. The proposal half (render, lint, `--emit-splits`, attach rows)
is retired with the splits program (`docs/tools/retired.md`); the program's record is `docs/splits-program.md`.

## Users

The land gate's splits rows and the lanes that register or recut a range (`--baseline --unit REGEX` quotes the numbers a finding
needs). `Ctx` (the scanner: `lis`+`addi`/`ori`/load pairs and r13/r2 small-data accesses decoded from the retail `.text`) is the
reference decoder `lib.ppc.scan_refs` was extracted from.

## CLI

```
python tools/splits/splitcheck.py --baseline [--json F] [--all] [--only INV[,INV]] [--unit REGEX] [--intervals]
python tools/splits/splitcheck.py --readers SECTION:START-END [--readers ...]
python tools/splits/splitcheck.py --selftest
```
Flags: `--all`, `--baseline`, `--dol`, `--intervals`, `--json`, `--limit`, `--only`, `--outbox`, `--readers`, `--selftest`, `--splits`,
`--symbols`, `--unit`.
Exit codes: 0 ok, 1 selftest failure, 2 no mode given (the `lib.findings` convention is the migration target; `migration.md`
records the current behaviour). `--baseline` itself exits 0 whatever it finds: it is an audit list, not a gate.
`--json F` writes the per-unit verdicts, the boundary checks, the seam requests and the pool groups.

## Inputs and outputs

* In: `splits.txt`, `symbols.txt`, the retail `main.dol` (and, for the suspected-seam rows, the lanes' `.pi/outbox/*.json`
  seam requests). Nothing in `docs/splits/` is read.
* Out: per invariant PASS / FAIL / UNKNOWN / `-` counts, the units with a FAIL (`--all` lists every unit), the ten top defects
  (ranked by invariant weight), one worst defect per failing invariant, the boundary summary (text cuts with a failing check),
  the suspected seams and the unowned-run summary.
* `--baseline --unit REGEX`: one `detail` line per `.ctors`/`.dtors` word of the matching units (function, its end, the closure
  end, the end with the unit's own vtable slots, the unit end); with `--intervals`, one `pooldup` line per pool value held at two
  addresses (both addresses, the last read of the first, the first read of the second, the interval a TU starts in).
* `--readers SEC:START-END`: each map symbol of the range, its owner unit and the units whose decoded text reads it.

## Invariants and rules

Verdicts: PASS, FAIL (names an address), UNKNOWN (the evidence does not decide), `-` (not applicable). Text references are
decoded, heuristic evidence (a register reused across a branch can fool the decoder) and each finding that depends on them says so.

* **order** - link order. FAIL: a section with several ranges and a gap between them (dtk takes one range per section), an empty or
  reversed range, two units' ranges overlapping, or a cycle in the graph of the units' section orders (`.bss`/`.sbss`/data
  sequences against the `.text` sequence are the same graph). UNKNOWN: abutting ranges of one section (one range in effect).
* **coverage** - every map symbol sits inside exactly one unit's range of its section: no symbol straddles a unit's range end,
  and a symbol under no range is counted in the `unowned` summary rather than failed.
* **text-cut** - a unit's `.text` starts and ends on a function symbol. FAIL: the range ends inside a function, or starts inside
  one (UNKNOWN when no function symbol decides it).
* **extab** - every `extabindex` entry is owned by the unit that owns its function and its `extab` record; FAIL names both units.
* **ctors / dtors** - the `.ctors` / `.dtors` word points at a function of the unit. A unit has one `.ctors` word (`__sinit`):
  more than one is FAIL (two or more TUs). The TU ends at the end of the **closure** of the sinit (R1): the sinit, its local
  callees, the address-taken functions and the slots of the vtables it stores - MWCC emits the deferred constructors, registered
  destructors and `b ctor` thunks after it. FAIL: the unit's text ends before or after that closure end (the finding names the
  boundary); UNKNOWN: a function at the closure end that is called from before the sinit, or a zero word. `__destroy_global_chain`
  and the other CRT chain entries are exempt for `.dtors`.
* **pool** - the `.sdata2` float/double and `.sdata` string pool of a unit is read (by a load) only by that unit, runs in
  first-use order per function (scheduling may reorder two loads of one function), holds each value once, and a claimed literal
  is read by the unit's own code. `.sdata` and `.sdata2` are two pools; order is judged inside each. FAIL: a literal read by two
  units, one value at two addresses read by one unit, a first use out of text order, a claimed literal no code of the unit reads.
  UNKNOWN: a pool no decoded code touches.
* **data-order** - no strong V->S or zigzag seam strictly inside the unit's `.data` (`docs/data-order-seams.md`): MWCC emits a
  TU's `.data` as globals, strings, vtables in reverse class order. A seam asserts a boundary in [first string, next vtable): a
  unit ending inside that gap passes. A zigzag pair is not a seam when the unit's own sinit closure stores the vtable, or when the
  member functions of the two classes alternate in the text.
* **vtable** - a vtable sits in the unit whose text holds one of its slots or stores its address. FAIL: its slots are in another
  unit; UNKNOWN: no slot or constructor store is owned by any unit.
* **jumptable** - a jump table sits in the unit that reads it (the `addi` forming its address followed by `lwzx`, `mtctr`, `bctr`;
  a field access through a `lis` is not the dispatch) and branches into only that unit's text. FAIL names the first foreign
  reader by name; UNKNOWN: no decoded reader and no code target.
* **bss** - a `scope:local` `.bss`/`.sbss` object is read by the unit that holds it. FAIL: only another unit reads it; UNKNOWN: no
  decoded code touches the unit's range.
* **local-static** - a `scope:local` data object (not a pool literal, not a jump table) is read by the text of one unit only:
  a static is private to its TU, so a local read from both sides of a boundary says the boundary is not a TU edge (FAIL on both
  units). A global is not a finding.

The boundary summary line counts each `.text` cut with a failing `fn-start` or `pool-shared` check.

## Lib dependencies

ppc, refs, binary, project, findings (target; today the tool carries its own copies, to be folded in the migration).

## Test contract

Tier: fixture (mini DOLs built in memory, `_make_dol` / `_mini`). Today's selftest: in-file `selftest()` (`--selftest`, 79
checks: decoder, each invariant's PASS/FAIL/UNKNOWN cases, ctors closure, pool order, V->S and zigzag seams, jump-table dispatch,
`--readers`, defect ranking). Target: `tools/tests/splits/test_splitcheck.py` on `lib.testing`; a check against the live tree
belongs under `TIER='smoke'`: `--baseline` must report 354 units with FAIL only in `pool` (76) and `data-order` (13).

## Known gaps

* `--baseline` does not exit non-zero on a FAIL (the two known invariants fail by design while the pool and V->S seams are
  recorded allowances in `docs/splits-program.md`); a gate row wanting a regression check compares counts.
