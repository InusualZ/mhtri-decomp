---
name: tu-boundary-discovery
description: Identify which symbols belong to one translation unit (and which data ranges it owns) from a single symbol address, offline, using the split DOL's own layout - the per-TU ordering of the data sections, private pooled constants, scope:local anchors and __FILE__ assert strings - through tools/splits/tudiscover.py, so a candidate set of functions can be written and matched together and the unit's exact extent settles as they match. Use when picking up an unsplit fn_XXXXXXXX, when deciding which symbols to match together, or when a splits.txt range must be proposed or sanity-checked.
license: MIT
compatibility: decomp-toolkit project layout with a completed split (build/<version>/asm/ per-unit disassembly, config/<game>/symbols.txt, splits.txt). Pure Python 3, no network, nothing written outside build/tmp/.
metadata:
  author: mhtri-dtk
  tool: tools/splits/tudiscover.py
---

# Which symbols belong to one translation unit

**The deliverable is a candidate *set of symbols to match together*, not a compliant boundary.** The tool's
job is to get that set right enough that the unit's source can be written and measured; the exact extent
then falls out of matching. A boundary a function too wide or too narrow is not a failure — an *unrelated*
function in the set is, because that is what wastes a matching session.

There is also a hard reason not to chase the boundary: **the linked DOL is indifferent to it.** Splitting
the same bytes into different objects still links byte-identically, so `build.sha1` cannot falsify a
boundary. A boundary is only *observable* where the two sides were built differently — a different compiler
or `cflags` (`mw_version`, `-O` level, peephole/scheduling, `-use_lmw_stmw`, `-pool`). If two neighbouring
chunks match under one flag set, the split point between them is unfalsifiable and therefore harmless; if
they do not, the flag mismatch is what tells you the boundary is wrong. So the output leads with the
**MATCH SET** (functions certainly one TU), then an *extended* range where real evidence exists, then runs.

The method rests on one property of a linker-built image: every section is a concatenation of the objects'
fragments in link order, so **the data sections are in the same order as `.text`**. Measured in this DOL:
Spearman(offset in section, lowest referring `.text` address) = **1.000** for `.sdata2` (7,224 labels),
0.990 for `.sdata`, 0.985 for `.rodata`. Two consequences carry the method:

* a **private** data symbol - a file-scope static, or a constant MWCC pooled per TU - has all of its
  referrers inside one TU, so it pins them together;
* a discontinuity between two sections' referrer runs pins a boundary between them.

## Commands

| goal | command |
| --- | --- |
| the set around an address or symbol | `python tools/splits/tudiscover.py at 0x804DA598` (or `at RSOLink`) |
| ... with the `splits.txt` block | add `--splits --unit src/RSO/runtime.c` |
| ... machine-readable (full symbol list) | add `--json` |
| widen the boundary search / list more candidates | `--window 80 --top 5` |
| cache, coverage, signal counts, source anchors, parse self-check | `python tools/splits/tudiscover.py stats` |
| remove the stale split-tree duplicates | `python tools/splits/tudiscover.py prune [--apply] [--include-obj]` |
| rebuild the graph cache | `python tools/splits/tudiscover.py cache --force` |
| scorecard for iterating on this tool | `python tools/splits/tudiscover.py bench [--seeds 400] [--compare <baseline.json>]` |

The graph (per-function data references, calls, codegen fingerprint, unwind entries, `.rel` ownership) is
built from `build/<version>/asm/` once (~12 s / 13 751 files) and cached in
`build/tmp/tudiscover/graph.json`; later runs take under a second. Delete the cache after a re-split.
`stats` prints three trust checks: the **parse self-check** (every `.fn` span's first instruction address
must equal the symbol map's address), the **asm range collisions** it dropped, and the **source-anchor
table** with its accept/reject verdict per name.

## Reading the output

* **`MATCH SET`** - the closure of the anchors: these functions are *certainly one TU*. Start here. It is a
  lower bound on the TU, not the full extent: neighbours with no anchors may still belong.
* **`extended`** - the closure grown outwards, but **only by strong evidence** (`pool`, `source`); it equals
  the set when there is none.
* **boundary candidates** - per side, ranked, with the *pins* that name them and `share` = the fraction of
  local evidence landing there. `strong xN` means N real boundary statements; `weak` means only the
  fingerprint/gap signals below, which fire all over the binary and cannot move a boundary on their own.
* **`dens`** - share of labels inside the printed run that the range actually references. `dens 1.00` plus
  `leak 0` is a claimable run; a low `dens` or any `leak` means the run spans other TUs' data. The
  `--splits` block refuses to print those, and prints why instead.

## What is evidence, and what is not

| signal | authority |
| --- | --- |
| a `scope:local` symbol (795 data, 52 functions) and its referrers/callers | hard must-link - but see the caveat below: on a *DOL* this is an annotation dtk echoes, not proof |
| a `.sdata2`/`.sdata` constant whose value is copied elsewhere in the section and whose referrers span ≤ `--span-max` bytes | strong (per-TU pooling proven by the copy) |
| a `__FILE__` assert string naming a `.c`/`.cpp`, anchored **per name** (one name can have several label copies): accepted while no function inside its span cites a *different* name and the span is ≤ `--source-span-max` (0x8000) | strong - 92 of 93 names accepted here, 1 rejected (`menu_infomation.cpp`, 65 KB span) |
| adjacent private pool labels whose referrer runs are disjoint and ordered | strong boundary |
| a `@`-pool object referenced from inside one range only - resolve the asm's `"@NAME_ADDRESS"` spelling to the map's `@NAME` by **address** (`@1841` is the RSO unit's string pool) | strong, when the referrer set is exclusive |
| `.rel <function>, ...` in a data object's `.obj` block | ownership only (which function owns a jump table/vtable); never a must-link between functions |
| a function called only from inside the range, or a caller/callee pair with no other callers | weak (soft vote) |
| a `_savegpr_*`/`stmw` flip or a record-form presence change between neighbours | weak (flag fingerprint, idea 21) |
| an alignment gap > 4 bytes | weak - `.text` here is one run whose gaps are only 4/8/12 bytes |
| dtk's `auto_*` units | **not evidence** - per-function build scaffolding |
| a `lbl_` shared by several functions | **not evidence by itself** - `.sbss` 69.8 % and `.bss` 60 % of labels are ordinary cross-TU globals |
| a repeated constant value | only with the copy + span test above; dtk labels every pool word, so repeats alone are noise |
| `extab`/`extabindex`/`.init` labels | **excluded** - per-function unwind fragments, and `@eti_` names are aliases for arbitrary addresses in the section (real code loads `"@eti_8001FFF8"+0xA`) |

Two known weaknesses of the reference graph itself:

* References are collected from **token presence in the disassembly, not register liveness**. An independent
  scan that tracked register writes collapsed one label's apparent 12 286 references to its true 27. Treat a
  *huge* referrer count as suspicious until a liveness-aware scan confirms it.
* The reference source is the **split tree's asm**, which accumulates stale duplicates across
  re-splits - 188 files here: 83 `auto_fn_*` that shadow the current `auto_dtor_*` range, 9 sub-range
  copies (the pre-2025 header spelling), and the 2 top-level stem leftovers (one of which, a stale
  `camellia.s`, silently zeroed the `camellia_setup128/256` fingerprint). The tool detects all three
  classes, never parses a shadowed file, and `prune --apply` removes them; **`--splits` refuses to hand
  off a block while any remain**, so a proposal can never come from a stale parse.

## Cutting a C++ unit: decode the `extabindex` records

A `.text` seam is invisible from addresses alone - functions are packed on 4-byte boundaries and nothing in
the DOL says where one retail object ended. The exception tables are the exception: for C++ code they are a
fingerprint of the *function set*, and dtk enforces the correspondence, refusing a claim that crosses one:

```
Mismatched splits for extabindex 2:0x8001E540 (sys_mem.cpp) and function 3:0x80040360 (main.cpp)
```

Each `extabindex` record is 12 bytes - `{function_address, function_size, extab_address}` - so the seam can be
read out of the original DOL instead of guessed (`tools/units/m2cinput.py` has the reader):

```python
import struct
from m2cinput import Dol                 # tools/units on sys.path
d = Dol("orig/RMHE08/sys/main.dol")
for i in range(10):
    fn, size, etab = struct.unpack(">III", d.read(0x8001E540 + 12 * i, 12))
```

Batch 4 is the worked example: the records at 0x8001E540/0x8001E54C name `main.cpp`'s functions at 0x80040360
and 0x80040420 (so `main.cpp` owns `.text` through 0x80040478, `extab` through 0x80006770, `extabindex`
through 0x8001E558), and the next four name `sys_mem.cpp`'s (0x80040478..0x8004054C, its own `extab`
0x80006770-0x80006810). The records also give the order inside the unit, which is what the unit header states.

Its limit: it pins only functions that *have* an unwind record. A unit's plain-C neighbours are still cut with
`tudiscover`'s closure plus the call graph - A's `fn_800403AC` in the same batch has no `extab`, and it was
placed by `Screen_w` ownership and the closure edge.

## Matching settles the boundary

1. **Take the MATCH SET and write it as one unit.** Do not wait for a perfect boundary — the set is what
   can be matched today, and the unit's flags come out of that diff (`mwcc-unit-matching`).
2. **Claim `.text` first, data afterwards, one range at a time.** A wrong data range costs the target's
   `R_PPC_NONE` pool relocations and *lowers* the score (playbook idea 23); measure each claim
   (`objdiff-verify`).
3. **Use a flag mismatch as the boundary signal.** A function in the set that refuses to match under the
   unit's flags is either source you have not worked out yet or a function from a differently-built
   neighbour; the flag diff (`-opt` fingerprint, `mw_version` family, `-use_lmw_stmw`) tells the two apart,
   and the corrected extent becomes a labelled range in tier 1.
4. **`--splits` output is a proposal and is never applied automatically.** `configure.py` (`mw_version`,
   `cflags_*`) and `splits.txt` stay deliberate edits.

## Worked example: what using it on `RSOLink` found

The first answer was **wrong**, and the scorecard did not notice - both facts shaped this skill:

* `at RSOLink` returned an 8-function MATCH SET held together by **one** anchor, `@1845`
  (`.data:0x80629C08`, the unit's jump table). It was an artifact: the parse split each file on `"\n.fn "`,
  so the `.obj "@1845"` *definition* block that follows the last `.endfn` counted as a reference from the
  previous function. Function blocks are now `.fn`..`.endfn` spans, the artifact went away, and the set
  correctly collapsed to **1 function, 0 anchors** - the unit appeared to have no layout evidence at all.
* Asking *which symbols really belong to that unit* then found the evidence that had been invisible: the
  unit's `.data` **string pool** (`@1841`, `0x80629B90`) is referenced by `RSOStaticLocateObject` and
  `RSORelocateSmallDataSection` and by nothing else - but the asm spells it `"@1841_80629B90"` while the
  map says `@1841`, and the reference resolver treated a leading `@` as a relocation modifier
  (`tok.split("@")[0]` is empty), so both references were dropped. Resolved by address, that one label
  pins **8 of the 9 claimed functions** (`0x804D9EC4..0x804DAE40`) and leaves `LocateObject` as the single
  function nothing links - which is what the independent review of that unit had concluded too (its only
  caller, `fn_804DAEB0`, lives in the *next* block). Tier 1 reports exactly that, as `DISAGREES: 9 seeds,
  2 intervals (1 fn / 8 fn)`.
* The lesson generalises: the same 8-function set was produced twice, once by a parse artifact and once by
  a real anchor. **An answer that does not name its evidence is worthless** - and no scorecard caught the
  artifact, because the artifact and the truth happened to agree.
* **Tier 1 had been seeding only the unit's first function**, so it reported "8 of 9 functions, start exact"
  for a set no mid-unit seed agreed with. Tier 1 now seeds *every* function of a claimed range and reports
  agreement (`consistent` / `disagrees` / `no evidence`).
* Fixing a `(?<!\w)data:` regex (it matched inside `.data:` too) added the **largest anchor class in the
  binary** - source-file assert strings - and grew the tail (max 132 → 146). The widest, `g3d_state.cpp`
  at 146 functions / 17 352 B, was stress-tested and is a **real TU, not a false merge** (18 referrers, one
  run once filename-less functions are ignored, no other name cited inside, pool runs bounded at the
  anchor) - with the caveat that its span is a *lower bound*, since 60 % of it is uncited.
* The one tier-1 success (`Runtime.PPCEABI.H/__init_cpp_exceptions.cpp`, exact) is **accidentally
  supported**: its recipe is already written in `docs/getting_started.md`, and the privacy of a DOL-local
  symbol cannot be proven from the binary. It is the weakest available test of the tool; do not read it as
  the tool having validated itself.
* The RSO unit also shows the data side working independently of `.text`: its jump table (`@1845`,
  `.rel`-owned by `RSOStaticLocateObject`), its string pool (`@1841`) and `lbl_80629C40` (used by
  `RSORelocate`) are referenced **only** from inside the claimed range, and the real pool is
  `0x80629B90..0x80629CA8`. Note the tension: `@1841` is the best *anchor* evidence in that unit and the
  claim idea 23 measured as harmful (1.35 % lost) - which is exactly why anchors and run claims are
  separate things in the tool.

## Handing off the block

`--splits` is the handoff, and it **refuses to print** while the split tree holds files that shadow
current ones - a proposal must not be built from a stale parse. The fix is one idempotent command:

```sh
python tools/splits/tudiscover.py prune --include-obj --apply
```

It removes exactly what the tool has proven stale (a shadowed same-range copy, a sub-range copy naming no
function the map knows, or a stale stem beside a configured-unit copy) and nothing else: `build/` is
gitignored build output, dtk never rewrites those files, and the removal cannot change the DOL or the
linked build (verified - the scorecard is bit-identical after pruning 188 files). `--allow-stale` exists
only for reading a proposal without acting on it.

## Iterating on this tool

Judge an idea by whether it makes the *set* better, never by boundary exactness alone.

1. **Score it.** One command, offline - no Ghidra and no Dolphin, or the numbers stop being reproducible:

   ```sh
   python tools/splits/tudiscover.py bench --seeds 400 --save build/tmp/tudiscover/baseline.json
   python tools/splits/tudiscover.py bench --compare build/tmp/tudiscover/baseline.json   # after a change
   ```

   Compare like with like: same `--seeds` and `--seed`. Use a larger `--seeds` to see tail effects that 400
   samples miss (the 0x8000 source-span guard only became visible at 4 000).
2. **One idea at a time, as a row in the table below before it is tried**, behind a real CLI switch
   (`--span-max`, `--source-span-max`, …) so the bench can A/B it in one run rather than by editing code
   between runs.
3. **Apply only what improves a tier and regresses nothing.** A change that shrinks the tail by making the
   tool louder (more singletons) is not an improvement; one new partial overlap, or a tier-1 `CONSISTENT`
   unit lost, is a regression whatever else improved.
4. **Land it, re-measure on the committed tool, and quote those numbers** - never a scratch script's.
   Verify a *new* success adversarially before believing it (the `Runtime.PPCEABI.H` case above looked like
   a win until it was checked against the docs).
5. **Record it in the same session**: the row to `done`/`no` with a one-line reason, the new baseline in
   `build/tmp/tu-boundary-discovery.md`, and a line here if the *method* changed.

| tier | measures | baseline (`--seeds 400`) | role |
| --- | --- | --- | --- |
| 1 labels | the units `splits.txt` claims (3 today): seeds **every** function in the range and reports agreement, plus start/end deltas and predicted runs vs claimed | `consistent 1  disagrees 1  no evidence 1`; 2 of 3 units have evidence - `RSO/runtime` disagrees in the informative way (8 of its 9 functions agree, `LocateObject` is the outlier and nothing links it) | guardrail. Tiny, and its claims are the pathological ones - do not tune to it |
| 2 sweep | 400 random seeds: closure median/p75/p90/max, guard hits, singleton share, leak-free data runs, mean density | median 1, p75 33, p90 72, **max 146**, guard 0, singletons 63 %, leak-free 35 %, density 0.82 | the objective: `max`/`p90` catch over-merging, leak-free/density catch wrong boundaries |
| 3 consistency | closures of all sampled seeds: distinct intervals, *partially overlapping* pairs | 347 distinct, **0 partial overlaps** | regression canary: must stay 0 |

Tier 1 grows by itself — every unit that gets matched adds a labelled range — which is why the tool should
read `splits.txt` as an input as well as an answer key (idea 1).

| # | idea | problem it solves | status |
| --- | --- | --- | --- |
| 1 | claimed `splits.txt` ranges as hard constraints | solved ground gets re-proposed, and a closure can cross a boundary that is already decided | todo |
| 2 | classifier sweep (`--span-max`, `--source-span-max`, copied-value rule) | the thresholds are hand-picked (0x4000 / 0x8000); the bench is the harness that can pick them properly | todo - the source-span guard was chosen from one measurement |
| 3 | one global partition instead of N independent closures | per-seed closures cannot use each other's evidence, and a seed with no anchors stays a singleton | todo |
| 4 | gap-chained referrers instead of one interval per label | a 146-function anchor built from 18 scattered referrers asserts far more than it measured: chaining referrers by gap distance would bound the claim to what is actually cited | todo - recommended by the widest-anchor stress test |
| 5 | liveness-aware references | token presence over-counts (12 286 → 27 on one label) and under-counts (a stale register looks live) | todo |
| 6 | sub-range containment in the asm dedupe (done) | 9 stale files shadowed current ones under a different header spelling; the tool now detects any file whose range is contained in another's **and** names no function the map knows, and `prune --apply` removes all three classes (188 files, 92 of them asm). Bench is bit-identical afterwards, `outside_map` self-check noise 1 466 → 1 457 | done |
| 7 | call-graph closure as a second, separately-labelled hypothesis | a helper called only from inside the set (the `RSONotify*` thunks in `RSO/runtime`) is invisible to layout evidence | todo - the thunks case is a known-open question, not a win |
| 8 | Dolphin dynamic tier (indirect callers, runtime data ownership) | static xrefs see no vtable calls, so a C++ unit's call closure is incomplete | todo |
| 9 | grow-while-`leak == 0` boundary policy | the closure finds the *start* reliably and under-covers the tail; growth may recover it | todo |
| 10 | the veto off-by-one (done) | a cut index is `boundary before f[c]`, so an anchor `(a,b)` forbids `a < c <= b`, not `c == a`. Using `<=` on the left vetoed the closure's own start and pushed the suggested boundary one function out. Fixed: `at fn_8026CC7C` now extends to `0x8026BA1C..0x80270018` (the seam the pool fragment argues for) instead of stopping at `0x8026FFBC`, and the `Pl` claims are reproduced exactly by every seed | done |

## What the tool cannot do yet

* **A unit whose functions share no data at all leaves no layout evidence.** `Camellia` is that case here
  (1 of 10 functions, no anchors: its S-box tables are named symbols with one referrer each). The extent
  then comes from the call graph, the Ghidra dump's names/order, and from matching. `RSO/runtime` is *not*
  that case: its string pool pins 8 of its 9 claimed functions, leaving `LocateObject` unlinked
  (only caller in the next block, no data references of its own).
* **`scope:local` is not a proof on a DOL.** 0 of 21 655 auto `lbl_*` objects carry it, and 22 451 of
  23 216 `scope:local` objects are `@`-pool symbols; the split object is dtk-synthesized and echoes the
  annotation as `STB_LOCAL`. A `static` `.sdata` object and a one-TU global are byte-identical in a DOL.
* **The three claimed units are a weak test set**: two have no evidence at all and the third is the recipe
  documented in `docs/getting_started.md`. Tier 2/3 are what an idea is actually judged on.
* **Stale split-tree artifacts**: `build/<version>/asm/` and `obj/` accumulate duplicates across re-splits
  (95 files each here; dtk never prunes), and there is a second generated tree at `build/tmp/exp/out/asm/`.
  Prefer the path from `splits.txt`'s group name; do not consume a bare top-level `.s`.
* **RSO modules are out of reach**: they have no target objects at all (`docs/rso-modules.md`), so this tool
  covers the DOL only.
* Names, signatures and struct layouts come from the shared memory dump (`docs/memory-dump.md`) — this tool
  tells you *where* a unit is, never *what* it is.
