# `dataorder` - Classify retail `.data` symbols and list the TU seams (V->S, zigzag) MWCC's emission order implies; the reusable core five tools import

<!-- generated from the module docstring of `tools/splits/dataorder.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

dataorder.py - translation-unit seams read from the *order* of retail `.data`.

## Users

profiles (`.claude/agents`) (2); skills (6); CLAUDE.md (1); docs (13); imported by `dataseams`, `splitcheck`, `tudiscover`

## CLI

Subcommands: `scan`, `at`.
Flags: `--json`, `--selftest`, `--window`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: DOL + symbols.txt -> seams.

## Invariants and rules

* MWCC lays out one TU's `.data` in a fixed order (measured with the project's flags, see `docs/data-order-seams.md`): initialised globals over 8 B in definition order, the strings of out-of-line functions in first-use order, **vtables in the reverse of class order**, then the strings of **inline** functions (the "inline tail": in-class bodies and free `inline` functions, one unmerged copy per instance). So a TU is `D* S* V* s*`, and the linker concatenates TU fragments. In retail:
* **V->S** (strong) - two vtable groups with strings between them: the second group is another TU, and the boundary lies somewhere in the gap (after any inline tail). A vtable followed by strings and *no* later vtable is `V->tail` (weak): it may be an inline tail of the same TU, which is exactly what the g3d "contradictions" were. **A vtable followed by strings is not by itself a seam.**
* **V->D** (weak) - a vtable followed by ordinary data (a jump table is `.data` too, its place is unmeasured);
* **zigzag** - two *adjacent* vtables whose owners' first code slots go **up** in address are two TUs (inside one TU they descend).
* This module is the reusable core: classify the DOL's `.data` symbols, list the seams, cut a run into TU fragments. It reads only the DOL and `config/RMHE08/symbols.txt`. Nothing is written.
* dataorder.py scan [--json] # every seam in the DOL, with the counts `docs/data-order-seams.md` quotes dataorder.py at <address|symbol> [--window N] # the symbols around one address, their kinds and the seams between dataorder.py --selftest

## Lib dependencies

binary.dol, project.symbols, project.splits.

## Test contract

Tier: fixture (_FakeDol); smoke: the whole-DOL scan reports counts, never pins.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/splits/test_dataorder.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

its selftest pins `> 30` seams in the real DOL
