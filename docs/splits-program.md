# The splits program: every byte of the DOL gets a translation unit, with evidence

Owner-approved plan (2026-09-30). The DOL (not the RSO modules; `mh3.sel` is only its export list) is cut into
translation units (TUs) with evidence-backed edges **before** bodies are written. Units exist in `splits.txt` under
derived names in the `src/<module>/` scheme but are **not registered in `configure.py`** until a lane writes bodies
("split-proven, unregistered"); a generated `fn_<addr>` stem is a placeholder only. Uncertain edges stay **merged**
into the larger unit with the candidate cut recorded.

## Phases

* **0 - the checker and the grade format** (this file, `tools/splits/splitcheck.py`).
* **1 - text edges by band.** Proposal files only (`docs/splits/proposals/phase1-<band>.json`, seven bands `a`..`g` plus
  `phase1-reconcile.json`, which resolves the cross-band conflicts); nothing goes into `splits.txt`.
* **2 - data attachment per cut**: `.ctors`/`.dtors`, pool, `.data`, `.bss`/`.sbss`, extab; resolve the
  ambiguous-owner / isolated-run / span-blocked classes of `datagap.py`.  One engine (`tools/splits/dataattach.py`) writes
  `docs/splits/proposals/phase2-reconcile.json`; the text folds the data forced are `phase2-folds.json`; the decisions are in
  `phase2-reconcile.md` (section "Phase 2 files" and "The data attachment engine" below).
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

`docs/splits/proposals/phase<N>-<band>.json` (numbers may be ints or `"0x..."`):

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
  * unit: `derived_name`, `module`, `ranges`, `cuts`, `open_questions`, `after` (below) and the bookkeeping `removes_cuts` (the
    registered cuts the unit makes disappear: full cut objects, each should be a registered range start - a warning when
    not), `absorbs` (baseline unit names it swallows whole; a trailing comment after the name is fine) and
    `replaces_tail_of` (a baseline unit name, or a list of them, it cuts a tail from);
  * cut: `addr`, `section`, `grade`, `evidence`, `reproduce`, `kind` (a label such as `sinit-closure`,
    `registered-edge`, `pooldup-forced`, `callers`; free text) and `keep_registered_edge`.
* **`module: "main"`** is the module of a root-level unit (`src/<name>.cpp`, no directory); the candidate names it
  `main/<derived_name>.cpp` and phase 4 drops the directory.
* **`keep_registered_edge: true`** on a cut says "this only restates a registered edge whose position is proven to lie
  inside a wide interval": the cut is grade-neutral (`grade` and `evidence` become optional), is **not counted as a
  proven cut**, and is never merged away; the cut must sit on a registered range start **or end** (the end of a registered
  unit is the start of the unowned run after it; an error otherwise). The
  renderer prints the cut counts apart: `strong N, medium N, guess N, keep_registered_edge N`.
* **Rendering** (`splitcheck.py --proposal F`): units are taken in text order; a `guess` cut merges the unit into the unit
  that TOUCHES its left edge - a proposal unit with a range ending at the cut, else a registered unit that does (the larger
  `.text` keeps its name; the absorbed unit and the candidate cut are printed, `into registered X` for the latter, whose
  ranges then grow by the absorbed unit's); a guess cut with no touching unit is a `lint:` error and the unit stays; the
  proposal ranges are subtracted from the baseline units they overlap (a proposal that leaves a baseline unit with a hole,
  reuses a baseline name or overlaps another proposal unit is a `lint:` line and a non-zero exit).
* **Chains**: a unit folded into a registered unit extends it, so the next `guess` cut that touches the folded unit's end folds
  into the same registered unit (a run of guess units after a registered one is one unit, not a lint error).
* **`supersedes`** (top-level, normally in a reconciliation file `phase<N>-reconcile.json`): a list of `{"band", "unit",
  "reason"}`; the named unit (`derived_name`) of the proposal whose `band` matches is dropped before rendering, so a unit
  two bands both propose (or one band's unit another extends) is replaced by the superseding file's unit. A name that matches
  nothing is a `warn:` line; the renderer prints `superseded (dropped before rendering): band B unit U`.
* **`removes_cuts`** may name a registered range **end** as well as a start (the edge of a registered unit against an unowned run).
* **Data of a recut registered unit** (`--data-by-reader`, default on; `--no-data-by-reader` turns it off): for each data
  run of a baseline unit whose text the proposal splits (or absorbs), the symbols are assigned to the pieces in text order by
  a monotone DP (cost: readers in another piece, decoded from the retail text; ties keep a symbol with the earlier piece).
  A section a proposal unit already lists over that run is left as written. It is a **PROVISIONAL phase-1 default** - not
  evidence; phase 2 replaces it - and the renderer prints every (unit, section) it assigned.

## Phase 2 files (one format)

A phase 2 file is an ordinary proposal file whose `units` may be empty (or hold a fold / a data-only unit) and which carries these lists
(`splitcheck.render` is the only implementation; `lint_proposal_full` validates every field):

* top level: `phase` 2, `lane`, `window` (`[lo, hi]`, the `.text` window a generated file is for), `attach`, `unowned_data`, `moves`, `open_questions`.
  **`attach` is the canonical name.** `data_attach` and `attachments` (the names the first six lanes used) load as aliases with a lint **warning
  naming the file**; both names in one file is an error.
* **`attach` row** - one range of data given to a unit of the candidate:
  * the unit: `unit` (a candidate unit name, a proposal unit's `derived_name`, or a name a `guess` merge or an `absorbs` replaced - it resolves to the
    surviving unit) and/or `text_addr` (the `.text` address the unit holds; it wins over `unit`, a mismatch is a warning; it survives the renames a merge
    causes; a data-only unit has none);
  * the range: `section` + `range: [start, end]` (canonical), or `ranges` (a list of pairs with `section`, or `{section: [[start, end], ...]}`), or
    `section` + `start` + `end`; any data section, `extab`, `extabindex` (not `.text`/`.init`);
  * `grade` (`strong|medium|guess`), `signal`/`signals`, `evidence` (`tool`, `command`, `finding`; required for strong/medium), `reproduce` (required),
    `kind`, `symbols`, `bytes`, `note`, and the two fold fields: `takes_from` (the unit whose range the row may take) and `provisional`.
  * **What a range may take**: unowned bytes; bytes its own unit holds (a restatement); bytes of a **provisional by-reader piece** (the evidence replaces
    the default); the bytes of the unit its row names in `takes_from` - that unit, left with no range, leaves the candidate (a data-only unit that is a
    fragment of its neighbour's TU). A byte any other unit keeps - a registered unit nothing recut, a proposal unit's own listed range - is a `lint:` line and
    the range is not applied; so is a unit the candidate lacks and two rows over one byte. A `guess` row is counted and never applied. Abutting plain
    ranges of one unit join.
* **`unowned_data` row** - a run nobody could be given: `section`, `range` (or `start`/`end`), `candidates` (names, or objects with a `unit`; empty only
  with a `reason`/`note`: a linker-generated table), a stated cause (`reason`, `why`, `evidence` or `note`), `kind`, `symbols`, `bytes`, `reproduce`.
  A deferred range the candidate gives to a unit is a `lint:` line unless the row says `provisional: true` or the holder is a by-reader piece.
* **`after: <unit>`** on a proposal unit that has no `.text` (a **data-only unit**, like the registered `Pl/pl_frame_data`): the unit sits right behind
  the named unit in file (= link) order; units behind one anchor keep the file order; without `after` the unit is last and a warning says so; an anchor the
  candidate lacks is a `lint:` line.
* **`moves: [{"unit", "after", "reason"}]`** places a candidate unit (registered or proposed) behind another: the candidate order is the link order, and
  `dtk` reads file order.
* Accounting: the units an `attach` gives data to are listed with the proposal units by `splitcheck --proposal`, so their FAILs are the proposal's, not
  "new failures outside the proposal units"; `dataattach.py`'s settle loop is what keeps them from failing an invariant the phase 1 candidate does not.
* `splitcheck --proposal ... --readers SECTION:START-END` prints, per map symbol of a data range, its owner in the candidate and the units whose decoded text
  reads it (the evidence an `attach` row quotes); `dataattach.py --explain` is the engine's per-symbol verdict.

## The data attachment engine (`tools/splits/dataattach.py`)

```
python tools/splits/dataattach.py [--window LO HI] [--lane NAME] [--out F] [--stats F] [--proposal F ...] [--linker] [--no-settle]
python tools/splits/dataattach.py --explain UNIT_REGEX|ADDR [--section S]     # a unit's segments, or the run holding an address, per symbol
python tools/splits/dataattach.py --holdout                                    # the solver on the registered data
python tools/splits/dataattach.py --vtable-order                               # per unit: up/down/tie vtable pairs (the zigzag premise)
```

* **Input**: the candidate `splitcheck.render` makes of phase 1 a..g + `phase1-reconcile.json` + `phase2-folds.json` (when it exists); the provisional by-reader
  data of recut units is stripped (those symbols are decided here). **Runs**: the symbols no range covers between two owned ranges; the **chain** of a run is the
  text units in link order from the owner before it to the owner after it.
* **Evidence** per symbol: strong (weight 1000) = the units whose decoded text reads it (a literal only by a load), a jump table's branch-target unit;
  derived (weight 1) = the units a table's code pointers enter, the units of the data symbols it points to or is pointed from (two rounds; owned and decided
  symbols count by their unit).
* **Solver**: a monotone dynamic program over the chain; a symbol is *decided* when every optimal assignment gives it one unit (the readers force it, or it lies
  between two symbols of one unit), else *ambiguous* (an interval of units). A strong row the optimum violates is a **contradiction**; static ones
  (`jumptable_`, `scope:local`) and clusters of three between adjacent units are an **interleave** (the data of two units alternates: not attached, deferred).
  Inside one owner's pool block only the longest first-use-ordered stretch is decided (**pool-order**). A data-only registered unit whose neighbours' data is
  one unit's on both sides is that unit's fragment (**`fold-data-only`**, `takes_from`; every range of it follows, and the data after it is decided on the folded
  candidate). The `extab`/`extabindex` entries past the last range go to the unit holding their function; `--linker` lists the tables mwldeppc emits.
* **Settle loop**: the decided rows are rendered and every invariant run; a row that makes a unit FAIL an invariant the phase 1 candidate does not fail it on
  is blocked - an `unowned_data` row `multi-tu` and a `guess` row - until nothing new fails. The finding is evidence: the unit holds more than one TU.
* **Output** is deterministic (the same bytes under any `PYTHONHASHSEED`); rows are written for the units whose `.text` starts in `--window`.
* **Hold-out** (`--holdout`): every third owned data range of a section is hidden per batch (180 ranges in three batches), decided again as an unowned run
  between its neighbours and compared with the registered owner. Over the whole DOL: 3,127 symbols, **3,045 decided, 3,018 right, 27 wrong, 61 undecided**
  (99.11 %; `.sdata2` 287/287, `.sdata` 72/72, `.sbss` 99/99, `.bss` 86/94, `.rodata` 72/73, `.data` 2,402/2,420). The 27 wrong are boundaries the readers
  contradict (`enemy/em_pop`|`em_model` 17, `em010`|`em011`/`em008` 6, `DWCi_NatNeg` 4). The five lane engines it replaced, on the same hold-out:
  p2a 98.31 % (3,074 decided, 52 wrong), p2b 99.11 % (identical), p2c 98.29 % (3,034, 52), p2d 98.26 % (2,987, 52), p2fg 98.32 % (3,088, 52); all the 52 are
  one set of boundaries, p2b alone defers 25 of them (the interleave zone) at the cost of 18 right symbols.
* **A fold is accepted** when the engine re-run on `phase 1 + the fold` renders with no FAIL **finding** the reference does not have (a count can stay the same
  while a finding is new: the pool check of a merged unit names a value held at two addresses); `phase2-reconcile.md` section 4 has the nine scenarios.

## The checker

```
python tools/splits/splitcheck.py --baseline [--json F] [--all] [--only INV,INV] [--unit REGEX] [--intervals]
python tools/splits/splitcheck.py --proposal F [--proposal G] [--emit-splits OUT] [--json F] [--readers SECTION:START-END]
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
`vtable`, `jumptable`, `bss`, `local-static` (definitions in the tool's docstring). Also emitted: one record per text boundary
(`fn-start`, `pool-shared`), `coverage_gaps` (unowned symbol runs per section), the ranked `top_defects`, and for
`--baseline` the suspected seams (the `kind: seam` requests in the primary checkout's `.pi/outbox/*.json`, and the pool
groups recomputed from the decode, cf. `docs/pool-seams.md`).

* `--baseline --unit REGEX` (and `--proposal ... --unit REGEX`, for the candidate) also prints a `detail` line per `.ctors` word (the function, its end, the closure end, the end with the
  unit's own vtable slots, the unit end) and, with `--intervals`, one `pooldup` line per pool value held at two addresses (both addresses, the last read of the first, the first read of the second,
  the interval a TU starts in): the numbers a `reproduce` row quotes. `--proposal` also reports every proposed cut, a `guess` included, that a `scope:local` data object is read across
  (a static is one TU's; a guess cut is merged away and never reaches the candidate's own `local-static` check) and exits 1 on one.
* The `ctors` closure follows the unit's own vtable slots past the unit end too, so a cut placed over them FAILs with `cut_at` = the closure end; past the end only an inline-sized
  first slot (<= 0x40 B) opens the run, a large function is another TU's member.
* The decoded data extent is the **map's** (owned ranges and unowned symbols alike): a literal of an unowned `.sdata2` pool
  past the last owned range is read by the text that loads it, as one inside an owned range is.
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
  The crt chain entries (`__destroy_global_chain`, ...) are exempt. The closure also takes the functions that follow `L`
  when each is a **slot of a vtable that a function of the unit (before the run) stores** - the inline virtual functions a TU
  emits after its `__sinit` (found by scanning the map's whole `.data` for words equal to the function start) - together with
  what they call after themselves.
* `pool` (idea 94): a literal read by two units means one TU; one value at two addresses read by one unit means two TUs;
  a claimed pool must run in first-use order and be read by its unit. A numeric `.sdata2` literal is **read** only by a
  load (`lfs/lfd/lwz/lhz/lbz/psq_l` through r13/r2, a `lis`, or a register an `addi`/`ori` formed the address into; calls
  and jumps end the register's life): a `lis/addi` that only passes or stores the address is not a read. Strings (`.sdata`)
  are used by their address, so any reference counts. First-use order is judged **per function**: literals first used in
  the same function carry no order (the scheduler reorders loads), a literal first used in an earlier function than the
  one before it fails.  **Per section**: the `.sdata` strings and the `.sdata2` literals of a unit are two pools (the strings sit at lower addresses, so one
  address-sorted list called every late string followed by an early float an inversion), each judged against its own first-use order; the inversion at
  the lowest address is reported.
* `data-order`: a strong `V->S` row says a boundary lies in `[addr, latest)` - after the first string, before the next vtable group.
  * **V->S fails a unit only when the unit also holds the later vtable group** (`latest < unit end`): a unit that ends inside the gap, or where the next group
    starts, has its boundary at its own end.
  * a `V->S` or `zigzag` seam is not a seam when the vtable on either side of it (the later one for a zigzag) is stored by a function in the closure of the
    same unit's `__sinit` (a deferred constructor instantiated in this TU for a class defined elsewhere);
  * a `zigzag` pair whose two classes' own member functions **alternate in the text** is one TU (a TU is one contiguous text range, so no cut separates them).
  * Not adopted: p2fg's "count a zigzag pair only when its two owners lie in different units' text" (the first slot does not order nw4r/homebutton vtables:
    pairs up 23, down 26, tie 9 in the reconciled candidate); on the reconciled candidate it changes nothing (`data-order` is 0 with the exemptions above).
* Effect of each rule on the reconciled candidate (phase 1 + `phase2-reconcile.json` + `phase2-folds.json`; FAIL counts when the rule is reverted):
  per-section pool order `pool` 63 -> 92; V->S rule `data-order` 0 -> 18 (baseline 0 -> 3); zigzag interleave exemption `data-order` 0 -> 10; zigzag
  instantiated exemption `data-order` 0 -> 5. `jumptable` names its foreign reader first by name (a set of units has no order): no count changes.
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

## Phase 2 result (2026-10-01, phase 1 + `phase2-reconcile.json` + `phase2-folds.json`, 355 units)

| invariant | FAIL phase 1 candidate (360 units) | FAIL + phase 2 |
| --- | ---: | ---: |
| order, coverage, text-cut, extab, ctors, dtors, vtable, bss, local-static | 0 | 0 |
| pool | 63 | 63 |
| data-order | 0 | 0 |
| jumptable | 1 | 1 |

* 859 data ranges attached (strong 732, medium 127), 15,722 symbols; lint 0, warnings 0, 0 new failures; four text folds, one data-only unit and one move
  applied, three folds held with the failing check (`phase2-reconcile.md` section 4).
* Symbols in no unit, whole map, before -> after: `.data` 8,454 -> 2,310, `.sdata2` 6,802 -> 86, `.sdata` 2,052 -> 213, `.bss` 439 -> 29, `.sbss` 904 -> 37,
  `.rodata` 277 -> 38; the rest is deferred by cause (`multi-tu` 1,327 symbols, `interleave` 815, `unread` 332, `pool-order` 151, `ambiguous` 93) for phase 3.
