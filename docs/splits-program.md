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
* **Fields** (validated by the linter: an unknown key is a `warn:` line, a wrong type a `lint:` error):
  * file: `phase`, `band`, `text_range` (or `range`), `units`, `open_questions` (a list of strings or objects: the
    candidate cuts kept out of the candidate, e.g. `{"unit", "question", "candidate_interval", "positions", "evidence",
    "reproduce"}`);
  * unit: `derived_name`, `module`, `ranges`, `cuts`, `open_questions`, and the bookkeeping `removes_cuts` (the
    registered cuts the unit makes disappear: full cut objects, each should be a registered range start - a warning when
    not), `absorbs` (baseline unit names it swallows whole; a trailing comment after the name is fine) and
    `replaces_tail_of` (a baseline unit name, or a list of them, it cuts a tail from);
  * cut: `addr`, `section`, `grade`, `evidence`, `reproduce`, `kind` (a label such as `sinit-closure`,
    `registered-edge`, `pooldup-forced`, `callers`; free text) and `keep_registered_edge`.
* **`module: "main"`** is the module of a root-level unit (`src/<name>.cpp`, no directory); the candidate names it
  `main/<derived_name>.cpp` and phase 4 drops the directory.
* **`keep_registered_edge: true`** on a cut says "this only restates a registered edge whose position is proven to lie
  inside a wide interval": the cut is grade-neutral (`grade` and `evidence` become optional), is **not counted as a
  proven cut**, and is never merged away; the cut must sit on a registered range start (an error otherwise). The
  renderer prints the cut counts apart: `strong N, medium N, guess N, keep_registered_edge N`.
* **Rendering** (`splitcheck.py --proposal F`): units are taken in text order; a `guess` cut merges the unit into the unit
  that TOUCHES its left edge - a proposal unit with a range ending at the cut, else a registered unit that does (the larger
  `.text` keeps its name; the absorbed unit and the candidate cut are printed, `into registered X` for the latter, whose
  ranges then grow by the absorbed unit's); a guess cut with no touching unit is a `lint:` error and the unit stays; the
  proposal ranges are subtracted from the baseline units they overlap (a proposal that leaves a baseline unit with a hole,
  reuses a baseline name or overlaps another proposal unit is a `lint:` line and a non-zero exit).
* **Data of a recut registered unit** (`--data-by-reader`, default on; `--no-data-by-reader` turns it off): for each data
  run of a baseline unit whose text the proposal splits (or absorbs), the symbols are assigned to the pieces in text order by
  a monotone DP (cost: readers in another piece, decoded from the retail text; ties keep a symbol with the earlier piece).
  A section a proposal unit already lists over that run is left as written. It is a **PROVISIONAL phase-1 default** - not
  evidence; phase 2 replaces it - and the renderer prints every (unit, section) it assigned.

## The checker

```
python tools/splits/splitcheck.py --baseline [--json F] [--all] [--only INV,INV] [--unit REGEX]
python tools/splits/splitcheck.py --proposal F [--proposal G] [--emit-splits OUT] [--json F]
python tools/splits/splitcheck.py --selftest
```

It is read-only. The DOL is found in the tree, else in the primary checkout (`orig/` is not in a worktree); it never
needs a build directory. **The same fallback serves the other readers** (`unitutil.resolve_input`, one implementation):
`tudiscover` (DOL and the asm dump), `callers` (the asm dump, else the split objects' relocations) and `dataorder` read
the tree's own `orig/` / `build/` when it has them, else MAIN's by path, read-only. MAIN is `$MHTRI_MAIN` (for the tree
the tools serve) or the parent of the git common dir. `dump_asm.py` always writes the tree's own dump; `tudiscover`'s
status line says `[MAIN's dump ..., read-only]` when it reads MAIN's. A dump whose stamp does not match the tree's
`symbols.txt`/`splits.txt` is reported `stale`, as before. Per unit and per invariant the verdict is PASS / FAIL / UNKNOWN (`-` = does not apply) with
the evidence address. Invariants: `order`, `coverage`, `text-cut`, `extab`, `ctors`, `dtors`, `pool`, `data-order`,
`vtable`, `jumptable`, `bss` (definitions in the tool's docstring). Also emitted: one record per text boundary
(`fn-start`, `pool-shared`), `coverage_gaps` (unowned symbol runs per section), the ranked `top_defects`, and for
`--baseline` the suspected seams (the `kind: seam` requests in the primary checkout's `.pi/outbox/*.json`, and the pool
groups recomputed from the decode, cf. `docs/pool-seams.md`).

* Text references (who reads a pool literal, a jump table, a bss object) are **decoded from the retail `.text`**
  (`lis` + `addi`/`ori`/load, r13/r2 small-data accesses): heuristic, so a pool finding names the literal and the
  reader and a reviewer confirms it with `python tools/units/callers.py <address>` (the same kind of index, built from
  the asm dump).
* `ctors`: a unit has one `.ctors` word and it points at the unit's `__sinit`, but the `__sinit` is **not** the TU's last
  function: MWCC emits the local constructors, the destructors registered by address and the `lis/addi/b ctor` thunks
  after it. The TU ends at `L`, the end of the closure of the sinit's callees and address-taken functions that lie after it
  (inside the unit; a `bl`/`b` or a `lis`+`addi` of a function start). PASS when `L` is the unit's end (within 0xC of
  padding); FAIL with `cut_at = L` when the unit goes on, **unless** the function at `L` is called or address-taken from
  before `L` inside the unit - then it is UNKNOWN (the boundary is not confirmed). More words than one means several TUs.
  The crt chain entries (`__destroy_global_chain`, ...) are exempt.
* `pool` (idea 94): a literal read by two units means one TU; one value at two addresses read by one unit means two TUs;
  a claimed pool must run in first-use order and be read by its unit. A numeric `.sdata2` literal is **read** only by a
  load (`lfs/lfd/lwz/lhz/lbz/psq_l` through r13/r2, a `lis`, or a register an `addi`/`ori` formed the address into; calls
  and jumps end the register's life): a `lis/addi` that only passes or stores the address is not a read. Strings (`.sdata`)
  are used by their address, so any reference counts. First-use order is judged **per function**: literals first used in
  the same function carry no order (the scheduler reorders loads), a literal first used in an earlier function than the
  one before it fails.
* `data-order`: a strong `V->S` seam is not a seam when the vtable on either side of it is stored by a function in the
  closure of the same unit's `__sinit` (a deferred constructor instantiated in this TU for a class defined elsewhere).
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
