# `tudiscover` - Propose a TU boundary around an address from the asm dump's referrer runs, pool model, data order and `__FILE__` anchors; owns the graph cache and the asm stamp

<!-- generated from the module docstring of `tools/splits/tudiscover.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Propose the translation-unit (TU) boundary around an address, offline.

## Users

configure.py / the build (6); skills (17); CLAUDE.md (2); docs (27); imported by `attribute`, `callers`, `dataorder`, `dataseams`, `dump_asm`, `langcheck`, `poolseams`

## CLI

```
tudiscover.py at <address|symbol> [--window 40] [--json] [--splits]
[--unit src/<Lib>/<file>.c] [--max-funcs 400]
tudiscover.py stats                 # cache + coverage + observation counts
tudiscover.py cache [--force]       # (re)build build/tmp/tudiscover/graph.json
tudiscover.py dataorder             # the `.data` emission-order seams mapped to `.text` intervals
tudiscover.py --selftest            # fixtures only, no asm dump needed
tudiscover.py bench [--seeds 400] [--seed 7] [--seeds-per-unit 64] [--max-funcs 400]
[--json] [--save F] [--compare F]     # the tool's own scorecard
```
Subcommands: `at`, `stats`, `cache`, `bench`, `dataorder`, `prune`.
Flags: `--addr`, `--all`, `--allow-stale`, `--apply`, `--compare`, `--data-order`, `--force`, `--include-obj`, `--json`, `--limit`, `--max-funcs`, `--pool-model`, `--save`, `--seed`, `--seeds`, `--seeds-per-unit`, `--selftest`, `--source-span-max`, `--span-max`, `--splits`, `--top`, `--unit`, `--weak`, `--window`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: build/RMHE08/asm, map, DOL -> scored cuts, build/tmp/tudiscover/graph.json.

## Invariants and rules

* Why this works. The retail `main.dol` is a link of objects, and a linker concatenates each object's sections in link order. So every data section is a *concatenation of per-TU fragments in the same order as `.text`* - measured here as Spearman 1.000 between a `.sdata2` constant's address and the lowest address of the `.text` functions that reference it (0.990 for `.sdata`, 0.985 for `.rodata`). A **private** data symbol (a file-scope static, or a constant MWCC pooled per TU) therefore pins all of its referrers into one TU, and a discontinuity between two sections' referrer runs pins a boundary. This tool collects those observations from the already-split DOL's own disassembly (`build/<version>/asm/`, one `.s` per unit) and scores every candidate function boundary.
* Observations, in decreasing authority:
* **must-link** (a boundary here is impossible) - from a private label referenced from both sides, a `scope:local` function and its callers, or a `__FILE__` assert string naming a `.c`/`.cpp` file. The file name anchors are one per *distinct name* (one string can have several label copies), and a name is only anchored while its `.text` span holds no function referencing a *different* name and stays within `--source-span-max`; a rejected name still votes as a soft source-file change.
* **pool run jump** - two adjacent labels of one section whose referrer sets are disjoint and ordered; the boundary lies between the last referrer of the first run and the first of the second.
* **data order** (`dataorder`: strings between two vtable groups; `dataorder-zz` and `dataorder-weak` are softer; all soft by default, `--data-order strong` promotes the first) - MWCC emits one TU's `.data` as globals, out-of-line strings, vtables in reverse class order, then the strings of *inline* functions (the "inline tail", `docs/data-order-seams.md`). So in retail `.data` strings between two vtable groups (V->S) mean **a boundary somewhere in the gap** [first string, next vtable), after the leading inline-tail strings; a vtable followed by strings and no later vtable (V->tail) is no evidence and is dropped. Two adjacent vtables whose owners go *up* are two TUs (zigzag). `tools/splits/dataorder.py` classifies the symbols; here each seam becomes a `.text` interval - last referrer of the vtable run before it (and of the gap's tail strings) .. first referrer of the vtable run after the gap (a vtable's referrers are its constructors' stores; its owner function stands in when nothing references it). The strings past the tail are not used: they may belong to either TU. A vtable followed by other data (V->D) is off by default (a jump table is `.data` too and its place in the order is unmeasured): `--data-order weak` adds it as a weak vote, `--data-order off` disables the whole kind. Why soft: the `bench` tier 4 and `dataorder` subcommand measure it against `splits.txt` and the `__FILE__` anchors (docs/data-order-seams.md section 6).
* **pool model** (`--pool-model on`, default; docs/pool-seams.md) - MWCC emits ONE literal pool per TU, one entry per distinct value, and `mwld` does not merge pools. So an `.sdata2` float/double (or `.sdata` string) read by several functions is one TU's pool entry - a must-link even when its value is unique (the `--span-max` guard still keeps a far-apart reader pair out) - and **one `.sdata2` value at two pool addresses is two TUs**: the boundary lies between the last referrer of one copy and the first of the next (`pooldup`, soft; `--pool-model strong` makes it a strong pin). Independent check against the 95 `__FILE__`-anchored TUs: no `pooldup` interval and no shared literal spans two anchored files. `at` also names the registered units the range overlaps and their pool-sharing group (`tools/units/poolseams.py`): a fold candidate, not a tool limit.
* **codegen fingerprint** (soft) - a `_savegpr_*`/`stmw` change or a record-form presence change between two neighbouring functions is a per-TU flag change (playbook idea 21).
* **alignment gap** (soft) - a >4 byte gap; weak in this binary, where `.text` is one run with gaps of only 4/8/12 bytes.
* Every observation above is read from inside the `.fn <name>`..`.endfn <name>` span of the function it belongs to. Section *data* blocks that follow the last `.endfn` are not part of any function, and a label's own `.obj` block is not a reference to it - reading them as one is how a dangling data block became a fake two-referrer must-link anchor. A data object's `.rel <target symbol>, <label>` lines do name the symbol holding the relocated target address (`@1845`, a table of addresses inside `RSOStaticLocateObject`); that is used only as *ownership* of the object for its data run, never as a reference, so a table with several relocated functions cannot merge them.
* What is deliberately *not* used: dtk's `auto_*` units (they are per-function build scaffolding, not TU evidence), naive `lbl_` sharing (`.sbss` 69.8 % and `.bss` 60 % of labels are shared by several functions and are ordinary cross-TU globals), and a repeated `.sdata`/`.data` *string* (an initialised `char[]` is a distinct object, never pooled). A repeated `.sdata2` float is used only through the pool model above: the repeat itself is the boundary evidence, not a mere coincidence of values.
* Nothing is written outside `build/tmp/`: the `splits.txt` block is printed, never applied.

## Lib dependencies

refs, binary, project, cache, repo.

## Test contract

Tier: fixture; smoke: `bench` tiers against the live splits.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/splits/test_tudiscover.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

reads the asm dump (`dump_asm.py`), which is stale after a map edit; `cache`/`prune` manage the graph
