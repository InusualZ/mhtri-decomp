# `phantom` - Find phantom `fn_*` rows (dead epilogues) with reachability evidence from the linked DOL; prints merge plans for symedit merge-batch

<!-- generated from the module docstring of `tools/symbols/phantom.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Find phantom `fn_XXXXXXXX` symbols - map entries that are not function entries at all.

## Users

skills (1); docs (5)

## CLI

```
python tools/symbols/phantom.py                        # counts + the merge/unclear list
python tools/symbols/phantom.py --all --limit 40       # also list the `keep` records
python tools/symbols/phantom.py --json                 # every record, machine-readable
python tools/symbols/phantom.py explain fn_8005236C    # all four evidence items for one symbol
python tools/symbols/phantom.py --selftest
```
Flags: `--all`, `--dol`, `--dump`, `--file`, `--json`, `--limit`, `--max-size`, `--section`, `--selftest`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: DOL, map, dump -> records.

## Invariants and rules

* Every candidate is reported with its evidence, never a bare verdict:
* its address and size, and the symbol immediately before it in the same section (name, size, end address) - `inside` when the candidate starts inside that symbol's extent, `adjacent` when it starts exactly at its end, `gap` otherwise;
* whether anything reaches it. The linked DOL is scanned for `bl` callers, `b`/`bc` branch targets, `lis`+`addi`/`ori` address materialisations in code, and 32-bit address words in the data sections (a vtable slot or a jump-table entry). Nothing reaches it -> merge candidate; something calls it -> real, reported `keep`. This is the relocation evidence the plan asks for, read out of the linked image: every relocation the split objects carried is already applied there, so it subsumes both `tools/symbols/symedit.py refs` and the `.rela` records, and it sees intra-unit calls too.
* whether the bytes at its address look like a function prologue at all (`stwu r1,-N(r1)`, `mflr`, `stmw`) - the plan's phantom set fails this;
* one verdict: `merge-into <previous>`, `keep`, or `unclear` with the reason. A merge needs a dead epilogue (the symbol's bytes are exactly one `blr`) that nothing reaches and that the 2021 runtime symbol dump does not name (`DumpSymbols.zip` -> `Dump_Loading85.raw.map`, `docs/memory-dump.md`). The dump carries `zz_<address>_` placeholders for genuinely unnamed functions and *no line* at a non-function, which is how the five phantoms were confirmed; its silence is only used where it covers the address (a populated neighbourhood).
* **This tool only reports - it never edits the symbol map.** A merge is a symbol-map edit (grow the previous symbol's `size:` comment, delete the phantom's line) and that edit is a human's, through the symbol-map proxy `tools/symbols/symedit.py` (`rename`/`rename-batch`); the proxy cannot resize or delete a symbol today, so the merge lines are printed as a two-line edit plan and applied by hand. Nothing in this file writes `symbols.txt`, `splits.txt` or any other file, and `symbols.txt` is only ever read through `symedit`'s line parser.
```
--max-size 8        unnamed `fn_*` at most this many bytes are candidates (default 8)
--section .text     restrict the scan to one section
--file PATH         symbol map (default config/RMHE08/symbols.txt)
--dol PATH          original DOL, read-only (default orig/RMHE08/sys/main.dol)
--dump PATH|auto    runtime symbol map, a `.zip` member or a plain `.map`; `auto` finds the 2021 dump
```

## Lib dependencies

binary (`Dol`, and `DolBuilder` for the fixture image), ppc, project (`SymbolMap`), repo (the default `--file`/`--dol`
are the invocation's tree). No tool import: `symedit refs` is run as a process for `explain`.

## Test contract

Tier: fixture (a `DolBuilder` image).
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/symbols/test_phantom.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

`DumpMap` is a second reader of the dump format `dumpmap.parse_map_text` reads: it takes a line's first token and its
second-to-last, so each of the 109 glued line pairs dumpmap splits yields one entry - the second address under the first
name. It has no lib home yet (a `phantom -> dumpmap` import would be a new tool edge); unifying it changes the dump half
of phantom's verdicts, so it needs its own measured batch.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* A phantom is an unnamed `fn_XXXXXXXX` in `config/RMHE08/symbols.txt` that the map treats as a function while the bytes at its address are really part of the *previous* function's body: the dead epilogue MWCC emits after a `mtctr`/`bctr` tail-call dispatcher, a jump-table island, or padding. `docs/plan.md` 7.9 records the incident: five 4-byte `fn_*` in `auto/80040598_fn_80040598` were not functions (dead epilogues) and cost four dispatchers their last 11 points (`docs/matching.md` 25).
