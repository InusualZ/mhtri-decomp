# `splitcheck` - read-only audit of `splits.txt`: 12 invariants per unit (the CLI over `tools/splits/invariants/`)

## Purpose

Does every unit and every boundary of the current `config/RMHE08/splits.txt` satisfy what the retail DOL, `symbols.txt` and the
MWCC/mwld emission rules say? One verdict per unit per invariant. The proposal half (render, lint, `--emit-splits`, attach rows)
is retired with the splits program (`docs/tools/retired.md`); the program's record is `docs/splits-program.md`.

## Users

The land gate's splits rows and the lanes that register or recut a range (`--baseline --unit REGEX` quotes the numbers a finding
needs). `invariants.context.Ctx` (the scanner: `lis`+`addi`/`ori`/load pairs and r13/r2 small-data accesses decoded from the retail `.text`) is the
reference decoder `lib.ppc.scan_refs` was extracted from.

## CLI

```
python tools/splits/splitcheck.py --baseline [--json F] [--all] [--only INV[,INV]] [--unit REGEX] [--intervals]
python tools/splits/splitcheck.py --readers SECTION:START-END [--readers ...]
```
Flags: `--all`, `--baseline`, `--dol`, `--intervals`, `--json`, `--limit`, `--only`, `--outbox`, `--readers`, `--splits`,
`--symbols`, `--unit`.
Exit codes: 0 ok, 1 selftest failure, 2 no mode given (the `lib.findings` convention is the migration target; `migration.md`
records the current behaviour). `--baseline` itself exits 0 whatever it finds: it is an audit list, not a gate.
`--json F` writes the per-unit verdicts, the boundary checks, the seam requests, the pool groups and the verdicts as
`lib.findings` rows (`rows`).

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

* The twelve invariants and their verdicts are `invariants.md` (one module each under `tools/splits/invariants/`).
* `--baseline` exits 0 whatever it finds: it is an audit list, not a gate; `--json F` writes the per-unit verdicts, the
  boundary checks, the seam requests, the pool groups and (WP3c) the same verdicts as `lib.findings` rows under `rows`.

## Lib dependencies

`lib.cli` (the entry point); the audit is `tools/splits/invariants/` (`audit` is its API), which stands on `lib.ppc`,
`lib.refs`, `lib.binary`, `lib.project`, `lib.repo`, `lib.findings` and the seam evidence.

## Test contract

Tier: fixture, `tools/tests/splits/test_splitcheck.py` (the 79 checks of the old in-file `selftest()` plus `Results.rows()`;
`invariants.md`). The live tree is smoke: `--baseline` reports 354 units with FAIL only in `pool` (76) and `data-order`
(13) on this tree.

## Known gaps

* `--baseline` does not exit non-zero on a FAIL (the two known invariants fail by design while the pool and V->S seams are
  recorded allowances in `docs/splits-program.md`); a gate row wanting a regression check compares counts.
