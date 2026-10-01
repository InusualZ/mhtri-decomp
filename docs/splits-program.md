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
* **4 - apply phase by phase** to `splits.txt` through `dtk split` and the gate (a phase is applied as a whole). Owner decision (2026-10-01): the source files of every folded or recut
  registered unit are merged too, re-registered `NonMatching` (a folded `Matching` unit is demoted). Six address windows, one lane and one landing each:
  `tools/splits/applysplits.py` (`plan`, `apply`, `manifest`, `verify`), the manifests and the lane brief are in `docs/splits/phase4/`.
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
  The engine's kinds: `ambiguous`, `unread`, `interleave`, `multi-tu` / `invariant` (blocked by the settle loop), `pool-order`, `reader-vs-pointer`, `orchestrator` (an
  override), `linker-generated`. `attach` signals: `reader`, `sinit`, `jumptable`, `pointer-graph`, `code-pointers`, `forced`, `orchestrator`, ... (`dataattach`'s labels).
  The engine also writes `open_questions` objects (`unit`, `section`, `question`, `candidate_interval`, `reproduce`): a pool whose first use goes down inside a unit
  that reads all of it, a data-only unit folded away, a range an owner holds that another unit's own `__sinit` constructs.
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
python tools/splits/dataattach.py [--window LO HI] [--lane NAME] [--out F] [--stats F] [--proposal F ...] [--linker] [--no-settle] [--overrides F]
python tools/splits/dataattach.py --explain UNIT_REGEX|ADDR [--section S]     # a unit's segments (a data-only unit: its ranges and readers), or the run holding an address, per symbol
python tools/splits/dataattach.py --holdout                                    # the solver on the registered data
python tools/splits/dataattach.py --vtable-order                               # per unit: up/down/tie vtable pairs (the zigzag premise)
```

`--explain` and the generator load the analysis from a pickle (`build/tmp/dataattach/analysis-<key>.pkl`, gitignored, the newest three kept) keyed by the
stat of the map, `splits.txt`, the DOL, the proposal files and the three tool sources: the first call decodes the whole text (about 12 s), the next ones take
0.2-0.6 s; `--no-cache` rebuilds. (`callers.py` caches its own index in `build/tmp/callers/graph.json` the same way: the first call of a fresh tree builds it,
about 8 s, then 1.2 s.)

* **Input**: the candidate `splitcheck.render` makes of phase 1 a..g + `phase1-reconcile.json` + `phase2-folds.json` (when it exists); the provisional by-reader
  data of recut units is stripped (those symbols are decided here). **Runs**: the symbols no range covers between two owned ranges; the **chain** of a run is the
  text units in link order from the owner before it to the owner after it.
* **Evidence** per symbol: strong (weight 1000) = the unit whose own `__sinit` constructs it (label `sinit`: a write into the object, the `this` of a constructor
  call, `__register_global_object`'s object and node, a tail-called constructor's `this` - the `.ctors` word's function inside the unit's text; the address merely
  being stored as a value, a vtable of another unit, is not a definition; a string or pool constant passed to a call is an argument) - which beats the foreign
  reads of an extern - else the units whose decoded text reads it (a literal only by a load), a jump table's branch-target unit; derived (weight 1) = the units
  a table's code pointers enter, the units of the data symbols it points to or is pointed from (a closure run to a fixpoint, at most 16 rounds; owned and decided
  symbols count by their unit).  Not a pointer: a word at an address that is not 4-aligned, and any word of a holder that is mostly non-pointer data (at least 64
  non-zero words of which under 1 % point into the map: an SJIS table has 12 "pointers" in 4,000 words).  Labels: `reader`, `readers`, `sinit`, `jumptable`,
  `vtable-store`, `local-static`, `pool-order`, `file-string` (only a real `__FILE__` / file-name string), `string-reader` (any other string literal one unit
  reads); a row's signal counts follow the same priority (label, then `pointer-graph`/`code-pointers`, then `forced`).
* **Solver**: a monotone dynamic program over the chain; a symbol is *decided* when every optimal assignment gives it one unit (the readers force it, or it lies
  between two symbols of one unit), else *ambiguous* (an interval of units). A strong row the optimum violates is a **contradiction**; static ones
  (`jumptable_`, `scope:local`) and clusters of three between adjacent units are an **interleave** (the data of two units alternates: not attached, deferred).
  **`reader-vs-pointer`**: a symbol one unit reads or constructs while the pointer tables around it (owned or strongly decided neighbours, at most two units)
  name another unit, and an **array** - at least four contiguous equal-size elements one pointer table names - whose readers split it between units, are
  deferred with both sides listed (an array is one object: both halves are deferred, none is attached). A disagreement the link order encloses (the symbol lies
  between two symbols of the unit that reads it) is not carved out - taking it out would leave the unit two ranges in one section - it stays in the row, which is
  graded medium and carries the disagreement in its note.
  **Pool order** (idea 94): inside one owner's block of `.sdata` strings only the longest first-use-ordered stretch is decided, a string a pointer initialiser of the
  owner's own table names counts as used first (MWCC emits those strings before the functions' own), the rest is given back to the units that read it (a symbol
  only a foreign unit reads between two symbols the owner reads is a foreign read the link order forces, not a second owner); a block nobody else reads is kept
  whole. Numeric `.sdata2` literals are TU-local - the reader owns them - so an inversion among them cuts nothing back: the row is attached and the inversion goes
  to the doc's `open_questions` (the unit holds two TUs' pool order), the settle loop accepts the `pool` FAIL it makes (`accepted` in `--stats`).
  A data-only registered unit whose neighbours' data is one unit's on both sides, or whose symbols one unit's own `__sinit` constructs, is that unit's fragment
  (**`fold-data-only`**, `takes_from`; every range of it follows, and the data after it is decided on the folded candidate). Data an owner's range already holds
  that another unit's own `__sinit` constructs (em010's `vec_pair_*`) is not moved by the solver: each such group is an `open_questions` row naming both units and
  the interval (a recut: an `attach` override with `takes_from`). The `extab`/`extabindex` entries past the last range go to the unit holding their function;
  `--linker` lists the tables mwldeppc emits.
  **Grades**: strong needs every symbol decided by readers (or by derived evidence as good as one: a table of code pointers whose every non-zero word enters the
  unit's text, a pointer-graph symbol whose every holder is a `.data` symbol the unit's registered ranges already hold), no contradiction, no disagreement left
  in the row, and at least `max(3, N/50)` of its N symbols (at most N) read by the unit (`ANCHOR_MIN`, `ANCHOR_SHARE`); anything else is medium, and a medium
  row whose first symbol another unit also reads says so in its note. A decided `.data` block is **cut at the strong V->S seams inside it** - including a seam
  whose later vtable the unit's own `__sinit` instantiates, which is no data-order edge but still where one vtable group's strings end - and every piece is a row of
  its own, graded by its own anchors.
* **Settle loop**: the decided rows are rendered and every invariant run; a row that makes a unit FAIL an invariant the phase 1 candidate does not fail it on
  is blocked - an `unowned_data` row `multi-tu` and a `guess` row - until nothing new fails. The finding is evidence: the unit holds more than one TU.
  Blocking a piece of a block cut at a seam blocks the pieces after it too (a hole would leave the unit two ranges: an `order` FAIL), the pieces before stay.
  A data-only unit the attachments leave out of link order (`splitcheck`'s `order`: its file position is all that orders it) is moved by the loop itself - the
  doc's `moves` - behind the in-order unit before it by address.
* **Overrides** (`--overrides FILE`, `docs/splits/proposals/phase2-overrides.json` is the empty skeleton): `{"overrides": [row, ...]}`, a row forces a decision
  the engine cannot derive: `{"unit" | "text_addr", "section", "start", "end" (or "range"), "action": "attach" | "defer" | "exclude", "grade": "strong" | "medium",
  "signal": "orchestrator", "evidence": [{"tool", "command", "finding"}], "note", "reproduce", "candidates", "takes_from"}`. `attach` gives the range to the unit
  (evidence required, never `guess`; the row is written with signal `orchestrator` and the note; with `takes_from` it is one row over bytes another unit owns -
  a recut - and that unit gives them up), `defer` lists it in `unowned_data` (kind `orchestrator`, `candidates` or the unit), `exclude` writes it nowhere. An unknown
  unit, an unusable action, a missing evidence or two overlapping rows is refused before anything runs. The overrides are inputs of the solver (an attach is a
  decision for the data after it) and still pass the settle loop: one that makes an invariant fail is **blocked** (an `unowned_data` row with the note
  `orchestrator override N blocked`, a `guess` row, and `overrides` in `--stats` says `blocked` with the failing check). The output with the same overrides is
  byte-identical. A `takes_from` row is one direct record (it holds owned bytes, so the solver sees no symbol in it): its outcome is `applied`, and it removes the
  `sinit_owner_questions` row for the group it decides.
* **Output** is deterministic (the same bytes under any `PYTHONHASHSEED`); rows are written for the units whose `.text` starts in `--window`.
* **Hold-out** (`--holdout`): every third owned data range of a section is hidden per batch (180 ranges in three batches), decided again as an unowned run
  between its neighbours and compared with the registered owner. Over the whole DOL: 3,127 symbols, **3,040 decided, 3,013 right, 27 wrong, 64 undecided**
  (99.11 %; `.sdata2` 287/287, `.sdata` 72/72, `.sbss` 99/99, `.bss` 86/94, `.rodata` 72/73, `.data` 2,397/2,415). Before the phase 3 fixes: 3,045 decided,
  3,018 right, 27 wrong, 59 undecided (99.11 %); the five symbols that became undecided are `reader-vs-pointer` deferrals, and no symbol became wrong.
  The 27 wrong are boundaries the readers contradict (`enemy/em_pop`|`em_model` 17, `DWCi_NatNeg` 4, and 6 symbols held by `enemy/em010_prog`'s range: `em011_prog_tbl`, `lbl_8056FCD0` and four `vec_pair_*` - there the engine decides `em011`/`em008`
  and the range is wrong: `em011_prog_tbl` is `em011`'s, the pairs are constructed by `em008`'s and `em011`'s own `__sinit`s). The five lane engines it replaced, on the same hold-out:
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
  the asm dump). A `lis` value stays live for 200 instructions in a volatile register, and in a callee-saved one (r14..r31, they survive calls) until
  the register is written - by a load, an `addi`/`ori`, or any opcode that writes a GPR (`written_reg`); `mr rA, rB` copies what rB held. A function that forms three
  or more distinct section starts with `addi` (the module loader's `_f_sbss2`, `_f_sdata2`, ... `RSOStaticLocateObject`) takes linker symbols, not the first symbol of each
  section: those references are dropped. The decoder also records the stores through an address and the calls (`bl`, or a tail `b` to a function start) made
  while r3..r10 hold one (`Ctx.store_refs`, `Ctx.pass_refs`), which `Ctx.sinit_definers()` turns into the globals a unit's own `__sinit` constructs.
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
  the lowest address is reported. **Initialisers**: a string a pointer initialiser names (an aligned `.data`/`.rodata`/`.sdata` word of the unit's own ranges pointing at
  it, `const char* names[] = {...}`) is emitted before the strings the functions use, so its first use sorts first and carries no order among such strings
  (`pool_first_use`); a table in another unit's data does not count. Measured on the baseline: no count changes (the landed pools have no such table).
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
* `order`: `rename:` ranges (`.ctors$10`, `.dtors$15`) are ordered by the linker script and left out of the order graph. Besides overlaps, a unit's gap and the
  cycles of the address graph, the ranges of every section, taken by address, must have **non-decreasing file positions** (`dtk` reads `splits.txt` top to
  bottom, and a unit with no text has only its file position to order it): a longest-non-decreasing-subsequence over each section's ranges picks the ones in
  order, every range outside it is an `order` FAIL naming the units it lies between by address and their file positions (`file_order_offenders`, the fewest ranges
  that must move). On the reconciled candidate it finds exactly `Pl/pl_frame_data` and `Pl/pl_act_data` (`.sdata2` 0x80799E00..0x80799FDC, between
  `lobby/fn_80220038` and `Pl/pl_act_step` by address); `dataattach` places them (its `moves`).
* `jumptable`: a **reader** of a jump table is the dispatch - the `addi`/`ori` that forms the table's address followed, within 16 instructions, by an indexed load
  (`lwzx`/`lwzux`) through that register, then `mtctr` and `bctr` (`jumptable_reader_sites`) - not any reference whose address falls inside the symbol: an `lhz` struct-field
  load through a base formed with `lis` is not one. The finding names the first foreign reader by name. Baseline: 63 PASS / 1 FAIL -> 64 PASS / 0 FAIL; the
  reconciled candidate 1 -> 0 (`jumptable_8059FE90`'s four reader units were stale `lis` values the decoder kept live, the single true reader is the `addi` at 0x801139C4 of `fn_8011392C` in `ef/eft019`; `jumptable_805C390C`'s false readers were `lhz` field loads).

* `python tools/splits/matchinggain.py` lists the `Object(Matching, ...)` units of `configure.py` whose data sections the candidate changes (the gain, per section, registered -> candidate)
  and the Matching units the candidate no longer has under their name (with the unit that swallowed them): the phase 4 list of data to define or units to demote.

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

Phase 3 checker fixes (2026-10-01), baseline on the same tree before -> after: identical except `jumptable` 63 PASS / 1 FAIL -> 64 / 0 (the table above is the
2026-09-30 state: the tree now gives order 305/0/1, ctors 20/39/2, pool 121/129/0, data-order 21/0/0, bss 44/0/1).

* 149 of 306 units have a FAIL; 294 text cuts, 55 of them cross a shared pool literal; 19 pool groups (20 in
  `docs/pool-seams.md`, which counts from the objects' relocations); 22 seam requests in the outbox (18 land in a unit).
* Unowned (no unit range): `.text` 5,407 functions in 26 runs, `.data` 8,562 symbols in 62 runs, `.sdata2` 6,802 in 9 -
  the backlog the program covers; `coverage` is PASS because every *claimed* edge is clean.

## Phase 2 result (2026-10-01, regenerated after the phase 3 fixes: phase 1 + `phase2-reconcile.json` + `phase2-folds.json`, 354 units; lane B's numbers: `docs/splits/proposals/phase3-notes.md` - pool 75, `.data` 1,020 unowned)

| invariant | FAIL phase 1 candidate (360 units) | FAIL + phase 2, before the fixes (355 units) | FAIL + regenerated phase 2 (354 units) |
| --- | ---: | ---: | ---: |
| coverage, text-cut, extab, ctors, dtors, vtable, bss, local-static | 0 | 0 | 0 |
| order | 0 | 0 (2 under the file-order check) | 0 |
| pool | 63 | 63 (66 under the new decoder) | 81 |
| data-order | 0 | 0 | 0 |
| jumptable | 1 | 1 (0 under the dispatch rule) | 0 |

* 881 data ranges attached (strong 751, medium 130), 16,464 symbols; lint 0, warnings 0, 0 new failures; four text folds, two data-only folds (`Pl/bss_pool` by sinit, `cockpit_icon_data`),
  one data-only unit, three moves (one hand-written, two derived by the engine) applied, three folds held with the failing check (`phase2-reconcile.md` section 4). The pool count
  rises by the 18 numeric/string blocks whose first use goes down inside a unit that reads all of them: attached, recorded as open questions (`phase2-reconcile.md` section 7).
* Symbols in no unit, whole map, before -> after (previous regeneration in brackets): `.data` 8,454 -> 1,843 (2,310), `.sdata2` 6,802 -> 8 (86), `.sdata` 2,052 -> 55 (213),
  `.bss` 439 -> 34 (29), `.sbss` 904 -> 35 (37), `.rodata` 277 -> 12 (38); the rest is deferred by cause (`multi-tu` 891 symbols, `interleave` 815, `unread` 207,
  `ambiguous` 50, `reader-vs-pointer` 25) for phase 3.
* Hold-out: 3,040 decided, 3,013 right, 27 wrong, 64 undecided (99.11 %); before 3,045 / 3,018 / 27 / 59 (99.11 %): the five symbols that became undecided are rule 5 deferrals.
