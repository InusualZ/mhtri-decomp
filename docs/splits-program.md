# The splits program (2026-09-30 .. 2026-10-03): history, and the knowledge that outlives its tools

> Every tool this program ran is retired; what each was, its replacement and its history are `docs/tools/retired.md`.

The program cut the whole DOL (not the RSO modules; `mh3.sel` is only their export list) into translation units (TUs) with
evidence-backed edges before bodies were written. It is **finished and its tools are retired** (`docs/tools/retired.md`:
`applysplits`, `dataattach`, `matchinggain`, the proposal half of `splitcheck`; the proposal files and window manifests were
deleted, git history keeps them). What stays: `python tools/splits/splitcheck.py --baseline` audits `splits.txt` against the
invariants below (spec: `docs/tools/spec/splitcheck.md`), and `tudiscover.py`, `dataorder.py`, `poolseams.py`, `callers.py`,
`dataclaim.py --unit` answer the one-address questions a re-cut needs.

## Phases and outcome

* **0 - checker and grade format.** `splitcheck --baseline`: 11 invariants (a 12th, `local-static`, came with phase 3).
* **1 - text edges by band** (seven bands `a`..`g` plus a reconcile of cross-band conflicts). Proposal files only.
* **2 - data attachment per cut** (`.ctors`/`.dtors`, pool, `.data`, `.bss`/`.sbss`, extab) by one engine, replacing five lane
  engines. 881 data ranges attached (strong 751, medium 130), 16,464 symbols.
* **3 - independent review** against idea 94 (pool), `docs/data-order-seams.md`, `docs/pool-seams.md` and the discovery tools; it
  fixed the decoder, the `__sinit` definers, per-section pool order, the file-order check and the jump-table dispatch rule.
* **4 - apply** to `splits.txt` in six address windows (a, b, c, d, e, fg), one lane and one landing each; the source files of every
  folded or recut registered unit were merged too, re-registered `NonMatching` (a folded `Matching` unit was demoted: 7 demotions,
  11 `Matching` units gained data). Each window was accepted by plain `dtk dol split`, alone and cumulatively. Landings that
  needed one carried a recorded `--allow-orphan` / `--allow-rule10` allowance (`docs/pipeline.md` sections 4 and 11).
* **5 - re-audit** with `splitcheck --baseline`.

Numbers: **306 -> 354 units** in `splits.txt`; unclaimed `.text` **5,407 functions (26 runs) -> 0**; whole-map symbols in no unit,
before -> after phase 2: `.data` 8,454 -> 1,843, `.sdata2` 6,802 -> 8, `.sdata` 2,052 -> 55, `.bss` 439 -> 34, `.sbss` 904 -> 35,
`.rodata` 277 -> 12. Baseline audit at 306 units: 149 units with a FAIL (ctors 45, pool 130); at the end (354 units) **82 units
FAIL, only `pool` (76) and `data-order` (13)**, every other invariant 0 - those are recorded allowances (a unit that holds two
TUs' pool order or a V->S seam inside a TU the evidence cannot cut), not open edges. The data engine's hold-out (hide every third
owned run, decide it again from its neighbours): 3,127 symbols, 3,040 decided, 3,013 right, 27 wrong, 64 undecided (99.11 %); the
27 wrong were boundaries the readers contradict (`enemy/em_pop`|`em_model` 17, `DWCi_NatNeg` 4, six symbols in `em010_prog`).

## Rules the program established

* **Link order is a monotone assignment.** Units link in text order in every section, so data ownership along an unowned run is
  non-decreasing: a run's chain is the units from the owner before it to the owner after it, and a symbol is decided when every
  optimal monotone assignment gives it one unit. A strong read the optimum violates is a contradiction that names a multi-TU unit
  or a foreign read (an extern).
* **R1, the `.ctors` closure.** A unit has one `.ctors` word and it points at the unit's `__sinit`, but the `__sinit` is not the
  TU's last function: MWCC emits the local constructors, the destructors registered by address and the `lis/addi/b ctor` thunks
  after it. The TU ends at `L`, the end of the closure of the sinit's callees and address-taken functions after it, plus the
  functions that are slots of a vtable a function of the unit stores (inline virtuals emitted after the sinit; an inline-sized
  first slot, <= 0x40 B, opens the run, a large function is another TU's member). PASS when `L` is the unit end (0xC of padding);
  FAIL with `cut_at = L` when the unit goes on, unless the function at `L` is called or address-taken from before `L` (then
  UNKNOWN). Several words mean several TUs. The CRT chain entries (`__destroy_global_chain`, ...) are exempt.
* **The local-static rule.** A `scope:local` data object (not a pool literal, not a jump table) is read by the text of one TU: a
  local read from both sides of a boundary says the boundary is not a TU edge.
* **Pool per TU (idea 94).** MWCC emits one literal pool per TU and `mwld` does not merge pools: a literal read by two units means
  one TU; one value at two addresses read by one unit means two TUs. A numeric `.sdata2` literal is read only by a load (a
  `lis/addi` that passes or stores the address is not a read); `.sdata` strings count by address. First-use order is judged per
  function (the scheduler reorders loads) and per section (`.sdata` strings and `.sdata2` literals are two pools). A string a
  pointer initialiser of the unit's own table names is emitted before the strings the functions use. Numeric literals are
  TU-local, so an inversion among them is attached and recorded, not cut back.
* **The `.data` emission-order seam grammar** (`docs/data-order-seams.md`): MWCC emits a TU's `.data` as globals, strings, then
  vtables in reverse class order. A strong **V->S** row (vtable then string) asserts a boundary in `[first string, next vtable
  group)`; it fails a unit only when the unit also holds the later vtable group (a unit ending inside the gap has its boundary at
  its own end). A **zigzag** pair (adjacent vtables whose first slots go up) is no seam when the unit's own `__sinit` closure stores
  the vtable (a deferred constructor instantiated for a class defined elsewhere) or when the two classes' member functions
  alternate in the text (a TU is one contiguous text range). A decided `.data` block is cut at its strong V->S seams.
* **Data a unit constructs is its data**: the unit whose own `__sinit` constructs a symbol (a write into the object, the `this` of
  a constructor call, `__register_global_object`'s object and node) beats the foreign reads of an extern; a vtable of another unit
  merely stored as a value is not a definition. An array (>= 4 contiguous equal-size elements one pointer table names) is one
  object: if its readers split it, the whole array is deferred.
* **Jump tables**: the reader is the dispatch (`addi` forming the address, `lwzx` within 16 instructions, `mtctr`, `bctr`), not
  any reference inside the symbol.
* **File order is link order**: `dtk` reads `splits.txt` top to bottom, so each section's ranges taken by address must have
  non-decreasing file positions; a data-only unit has only its file position to order it. `rename:` ranges (`.ctors$10`,
  `.dtors$15`) are ordered by the linker script and left out of the order graph.
* **What dtk refuses** (measured): a split off a 4-byte boundary, and the 2-byte `.sdata` / 4-byte `.sdata2` gaps between two
  units. A section start is aligned to the TU's largest object, so an off-word symbol is packed behind the previous TU's last
  object (the previous unit takes it and its padding), except where the following unit holds word-sized objects.
* **Language of a merged unit**: one C++ mangled function in a range proves a `.cpp` TU (the unit is one compile; unmangled
  neighbours are placeholders or `extern "C"`); only unmangled names with C linkage and `.c` sources make it `.c`.

## Evidence classes and grades

Soft observations (`tudiscover at`): `__FILE__` strings and file-name strings, RVL_SDK banners and version strings, the
`em*_prog` / `ef*` per-module tables, `__sinit` definers, vtable owner jumps, extab owner jumps, pool-run jumps, `pooldup`
intervals, link-order neighbours. Exact invariants (the checker's) are strong. Grades used by the proposals:

* **strong** - an exact invariant says the edge is there (a `__sinit` that is not its unit's last function, a `__FILE__` anchor, a
  literal read by two units, a V->S/zigzag seam narrowed to one symbol).
* **medium** - two independent soft observations agree, or one with a narrow interval.
* **guess** - one soft observation, a size cap or a wide gap; a guess cut was merged into the larger unit, never emitted.

A data row was strong only when readers (or equally good derived evidence: a code-pointer table whose words all enter the unit's
text) decided every symbol, with at least `max(3, N/50)` symbols read by the unit; anything else was medium.

## Lessons

* Evidence from the retail text beats naming: a name or file layout is a hypothesis, a read from the unit's own function is not.
* A heuristic decoder needs its own hold-out: hiding registered data and re-deciding it found 27 wrong boundaries and the five
  lane engines' 52; a count can stay equal while a finding is new, so compare findings, not counts.
* A candidate must always be rendered over the file it was written against; rendering over a half-applied tree names a unit twice.
* A window checked alone can fail `order` where a neighbour has not landed; the cumulative state is the test.
* Auto-cut byte caps (`--max-bytes`) produced partial TUs and unions; a unit header's "seam unproven" is the record of those.
* A pool FAIL where a unit reads one value at two addresses is evidence of two TUs the data cannot yet separate; record it as an
  allowance instead of cutting without proof.

The per-unit notes the program carried (homebutton) are in `docs/splits/phase4/homebutton-carried-notes.md`: the headers of
`src/homebutton/` and `include/homebutton/` point at it.
