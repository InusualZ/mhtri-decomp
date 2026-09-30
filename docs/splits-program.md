# The splits program: every byte of the DOL gets a translation unit, with evidence

Owner-approved plan (2026-09-30). The DOL (not the RSO modules; `mh3.sel` is only its export list) is cut into
translation units (TUs) with evidence-backed edges **before** bodies are written. Units exist in `splits.txt` under
derived names in the `src/<module>/` scheme but are **not registered in `configure.py`** until a lane writes bodies
("split-proven, unregistered"); a generated `fn_<addr>` stem is a placeholder only. Uncertain edges stay **merged**
into the larger unit with the candidate cut recorded.

## Phases

* **0 - the checker and the grade format** (this file, `tools/splits/splitcheck.py`).
* **1 - text edges by band.** Proposal files only (`.pi/splits/phase1-<band>.json`); nothing goes into `splits.txt`.
* **2 - data attachment per cut**: `.ctors`/`.dtors`, pool, `.data`, `.bss`/`.sbss`, extab; resolve the
  ambiguous-owner / isolated-run / span-blocked classes of `datagap.py`.
* **3 - independent review** of the bands against what is already known (idea 94, `docs/data-order-seams.md`,
  `docs/pool-seams.md`, tudiscover / dataorder / poolseams evidence).
* **4 - apply phase by phase** to `splits.txt` through `dtk split` and the gate (a phase is applied as a whole).
* **5 - re-audit the landed units** with `splitcheck.py --baseline` (its defect list is the audit list).

## Grades

* **strong** - an exact invariant says the edge is there: a `__sinit` that is not its unit's last function (the next
  byte starts another TU), a `__FILE__` anchor, a pool literal read by two units (for merging), a strong `V->S` /
  zigzag seam that narrows to one symbol.
* **medium** - two independent soft observations agree, or one soft one with a narrow interval (a `pooldup` interval,
  a vtable owner jump, an extab owner jump together with a pool-run jump).
* **guess** - one soft observation, a size cap, or a wide seam gap. A `guess` cut is **never emitted** into
  `splits.txt`: the renderer merges the two sides into the larger unit and records the candidate cut.

## Proposal file format

`.pi/splits/phase<N>-<band>.json` (numbers may be ints or `"0x..."`):

```json
{"phase": 1, "band": "ef",
 "units": [
  {"derived_name": "fn_800B4AC8", "module": "ef",
   "ranges": {".text": [["0x800B4AC8", "0x800B99E8"]]},
   "cuts": [{"addr": "0x800B4AC8", "section": ".text", "grade": "strong",
             "evidence": [{"tool": "splitcheck", "command": "splitcheck.py --baseline --only ctors",
                           "finding": ".ctors word 0x8056F2E0 targets fn_800B4ABC, which ends at 0x800B4AC8"}],
             "reproduce": "python tools/splits/splitcheck.py --baseline --only ctors --unit fn_800AEE48"}],
   "open_questions": ["is 0x800B8000 a second cut? one pooldup vote only"]}]}
```

* A **cut** is the left edge of one of the unit's ranges (`addr` = that range's start in `section`); the first unit of a
  band needs none. Every other unit's `.text` start needs a cut. `grade` is `strong|medium|guess`; `strong` and
  `medium` need at least one `evidence` row (`tool`, `command`, `finding`); every cut needs `reproduce`.
* The unit's file in `splits.txt` is `<module>/<derived_name>.cpp`; `derived_name` is a plain identifier.
* `ranges` is the unit's **final** ranges per section (phase 1: `.text` only). The renderer derives the sections a
  text cut already determines: `extabindex`/`extab` by the function of each entry, `.ctors`/`.dtors` by the function each
  word points at; a section the unit lists itself is kept as written.
* **Rendering** (`splitcheck.py --proposal F`): units are taken in text order; a `guess` cut merges the unit into its
  left neighbour (the larger `.text` keeps its name; the absorbed unit and the candidate cut are printed); the proposal
  ranges are subtracted from the baseline units they overlap (a proposal that leaves a baseline unit with a hole, reuses a
  baseline name or overlaps another proposal unit is a `lint:` line and a non-zero exit).

## The checker

```
python tools/splits/splitcheck.py --baseline [--json F] [--all] [--only INV,INV] [--unit REGEX]
python tools/splits/splitcheck.py --proposal F [--proposal G] [--emit-splits OUT] [--json F]
python tools/splits/splitcheck.py --selftest
```

It is read-only. The DOL is found in the tree, else in the primary checkout (`orig/` is not in a worktree); it never
needs a build directory. Per unit and per invariant the verdict is PASS / FAIL / UNKNOWN (`-` = does not apply) with
the evidence address. Invariants: `order`, `coverage`, `text-cut`, `extab`, `ctors`, `dtors`, `pool`, `data-order`,
`vtable`, `jumptable`, `bss` (definitions in the tool's docstring). Also emitted: one record per text boundary
(`fn-start`, `pool-shared`), `coverage_gaps` (unowned symbol runs per section), the ranked `top_defects`, and for
`--baseline` the suspected seams (the `kind: seam` requests in the primary checkout's `.pi/outbox/*.json`, and the pool
groups recomputed from the decode, cf. `docs/pool-seams.md`).

* Text references (who reads a pool literal, a jump table, a bss object) are **decoded from the retail `.text`**
  (`lis` + `addi`/`ori`/load, r13/r2 small-data accesses): heuristic, so a pool finding names the literal and the
  reader and a reviewer confirms it with `python tools/units/callers.py <address>` (the same kind of index, built from
  the asm dump).
* `ctors`: a unit has one `.ctors` word and it points at the unit's **last** function (`__sinit`); more words, or a
  word pointing earlier, means the unit spans several TUs and names the cut (`cut_at`). The crt chain entries
  (`__destroy_global_chain`, ...) are exempt.
* `pool` (idea 94): a literal read by two units means one TU; one value at two addresses read by one unit means two TUs;
  a claimed pool must run in first-use order and be read by its unit.
* `order`: `rename:` ranges (`.ctors$10`, `.dtors$15`) are ordered by the linker script and left out of the order graph.

## How a reviewer re-derives a cut

1. Run the cut's `reproduce` command; its output must show the `finding` text.
2. `python tools/splits/tudiscover.py at <addr>` for the soft observations, `poolseams.py --unit` /
   `dataorder.py at <addr>` for the pool and `.data` evidence; `python tools/units/callers.py <addr>` for readers.
3. `python tools/splits/splitcheck.py --proposal <file>`: the candidate's FAIL count per invariant must not exceed the
   baseline's, and "new failures outside the proposal units" must be 0.
4. A `guess` cut is judged by the question it leaves in `open_questions`, not by its verdict.

## Baseline (2026-09-30, tree at `eafb93634`, 306 units, about 2 s)

| invariant | PASS | FAIL | UNKNOWN |
| --- | --- | --- | --- |
| order | 305 | 0 | 1 (`Network/network_pat_control`: 6 abutting `.data` ranges) |
| coverage | 306 | 0 | 0 |
| text-cut | 301 | 0 | 0 |
| extab | 267 | 0 | 0 |
| ctors | 16 | 45 | 0 |
| dtors | 1 | 0 | 0 |
| pool | 119 | 130 | 0 |
| data-order | 18 | 3 | 0 |
| vtable | 21 | 0 | 0 |
| jumptable | 62 | 1 | 0 |
| bss | 43 | 0 | 1 |

* 149 of 306 units have a FAIL; 294 text cuts, 55 of them cross a shared pool literal; 19 pool groups (20 in
  `docs/pool-seams.md`, which counts from the objects' relocations); 22 seam requests in the outbox (18 land in a unit).
* Unowned (no unit range): `.text` 5,407 functions in 26 runs, `.data` 8,562 symbols in 62 runs, `.sdata2` 6,802 in 9 -
  the backlog the program covers; `coverage` is PASS because every *claimed* edge is clean.
