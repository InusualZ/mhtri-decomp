# `dataseams` - Thin consumer layer over dataorder's strong seams: cut points, warnings and order-only notes for the claim tools

<!-- generated from the module docstring of `tools/units/dataseams.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

dataseams.py - the `.data` emission-order seams (tools/splits/dataorder.py) as a guard for data-claim tools.

## Users

skills (1); docs (3); imported by `dataclaim`, `datagap`, `dataqueue`, `flipcheck`

## CLI

```
python tools/units/dataseams.py [START END]     # the strong seams (optionally inside one range)
python tools/units/dataseams.py --selftest
```
Flags: `--selftest`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: dataorder -> seams.

## Invariants and rules

* A `V->S` row is a **gap** `[addr, latest)`, not a cut at its first string: a TU's inline-function strings follow its vtables, so the boundary lies somewhere in the gap, after the leading `tail` strings. This layer therefore gives each row a `cut` (the address of the first symbol after the tail) and only *cuts* a gap that is narrow (`width <= NARROW`, where the position is known to a few symbols); a wide gap only *warns* that "a TU boundary lies in [addr, latest)". A `zigzag` seam has no gap and cuts at its own address.
* This module is the thin consumer layer the claim tools share - `dataqueue.py` (cut a proposed run), `dataclaim.py` (warn on a run or a rule-12 claim), `flipcheck.py` and `datagap.py` (name an order-only mismatch). It classifies nothing itself: every kind comes from `dataorder`.

## Lib dependencies

The seam evidence (`seams.md`: `strong_seams`, `NARROW`, `section_ranges`,
`retail_symbols`), `lib.binary.elf`, `lib.repo`.

## Test contract

Tier: fixture.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_dataseams.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* `dataorder.py` classifies every retail `.data` symbol and finds the TU seams (docs/data-order-seams.md): strings between two vtable groups (`V->S`) or two adjacent vtables whose owners go up (`zigzag`). Those two are **strong**; `V->tail` (a vtable, strings, no later vtable: possibly an inline tail) and `V->D` are weak and are never used to cut, warn or refuse.
