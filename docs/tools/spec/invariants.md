# `invariants` - The `splits.txt` audit as one module per invariant, over one context, rendered as `lib.findings` rows

`tools/splits/invariants/` (WP3c, from the 2 059-line `splitcheck.py`). `splitcheck.py --baseline` is the CLI; this package is
the audit.

## Purpose

Says, for every unit of `config/RMHE08/splits.txt`, whether each of twelve invariants holds against the retail DOL and the map:
one module per invariant, one context they all read, one result store that renders as `lib.findings` rows.

## Users

`tools/splits/splitcheck.py` (through `audit`, the package's API: `analyse`, `run_checks`, `report_json`, the printers,
`readers_report`, `ctors_detail`, `pool_intervals`, `pool_groups`, `load_ctx`). Nothing else imports it.

## Modules

| module | holds |
| --- | --- |
| `context` | the verdicts, `INVARIANTS`/`WEIGHT`, the splits/map/DOL views (`Unit`, `Splits`, `parse_splits`, `parse_symbols`, `Dol` = `bytes_at`), `Ctx` (the indices, the decoded text references through `lib.refs.text_refs`, the sinit and vtable-slot walks), `Results` (+ `rows()`), `load_ctx` |
| `order`, `coverage`, `textcut`, `extab`, `ctors`, `pool`, `data_order`, `vtable`, `jumptable`, `bss`, `localstatic` | one invariant each: `check_<name>(ctx, res)` (+ its helpers: `file_order_offenders`, `unit_sinit_closure`, `ctors_detail`, `pool_intervals`, `pool_groups`, `seams_inside`, `jumptable_reader_sites`) |
| `audit` | `run_checks` (the order the invariants run in), `boundaries`, `top_defects`, `seam_requests`, `report_json`, the table printers, `analyse`, `readers_report` |

To add an invariant: a module with `check_<name>(ctx, res)` that calls `res.add(unit, "<name>", status, addr, finding)`, its
name in `context.INVARIANTS` and `WEIGHT`, one line in `audit.run_checks`, its cases in `tests/splits/test_splitcheck.py`.

## Rows

`Results.rows()` is one `lib.findings.Row` per (unit, invariant): name `<unit>: <invariant>`, status PASS / FAIL / UNKNOWN /
SKIP (`-`, not applicable), `detail` the finding that set the worst verdict (prefixed with its address), `evidence` the
pass/fail counts. `splitcheck.py --baseline --json F` writes them under `rows`, beside the keys it always wrote.

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

`lib.ppc` (the scanner and opcode tables), `lib.refs` (`text_refs`: the `Ctx` scan loop), `lib.binary.dol`, `lib.project`,
`lib.repo` (the tree, MAIN, the DOL fallback), `lib.findings` (`Row`); the seam evidence (`tools/splits/seams/evidence.py`:
the literal and value-witness rules, the `.data` seams, `components`).

## Test contract

Tier: fixture (`tools/tests/splits/test_splitcheck.py`, 80 checks, moved from the in-file `selftest()`): mini DOLs built in
memory (`_make_dol` / `_mini`) - the decoder, each invariant's PASS/FAIL/UNKNOWN cases, the ctors closure, pool order, V->S
and zigzag seams, jump-table dispatch, `--readers`, the defect ranking, `Results.rows()`. A live check would be smoke:
`--baseline` reports 354 units with FAIL only in `pool` (76) and `data-order` (13) on this tree (measured, not pinned).

## Known gaps

* An invariant still accumulates into `Results` (`res.add`) rather than returning its own rows; the rows are rendered from
  the store, so the twelve modules kept their tested signatures. Returning rows per invariant is a later step if the gate
  ever composes them.
* `Ctx` keeps its symbol-index mapping (`refs`/`load_refs`/... by data-symbol index) and the linker-operand filter over
  `lib.refs.text_refs`' raw output (`lib-refs.md`).
