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
then falls out of matching, and every unit that gets matched makes the next one easier. A boundary a
function too wide or too narrow is not a failure — an *unrelated* function in the set is, because that is
what wastes a matching session. So the output leads with the **MATCH SET** (functions certainly one TU),
then an *extended* range where real boundary evidence exists, then the data runs.

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
| the unit around an address or symbol | `python tools/splits/tudiscover.py at 0x804DA598` (or `at RSOLink`) |
| ... with the `splits.txt` block | add `--splits --unit src/RSO/runtime.c` |
| ... machine-readable (full symbol list) | add `--json` |
| widen the boundary search / list more candidates | `--window 80 --top 5` |
| cache, coverage, signal counts | `python tools/splits/tudiscover.py stats` |
| rebuild the graph cache | `python tools/splits/tudiscover.py cache --force` |
| scorecard for iterating on this tool | `python tools/splits/tudiscover.py bench [--seeds 400] [--compare <baseline.json>]` |

The graph (per-function data references, calls, codegen fingerprint, unwind entries) is built from
`build/<version>/asm/` once (~11 s / 13 836 files) and cached in `build/tmp/tudiscover/graph.json`; later
runs take under a second. Delete the cache after a re-split — `stats` reports the coverage difference.

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
| a `scope:local` symbol (795 data, 52 functions) and its referrers/callers | hard must-link |
| a `.sdata2`/`.sdata` constant whose value is copied elsewhere in the section and whose referrers span ≤ `--span-max` bytes | strong (per-TU pooling proven by the copy) |
| a `__FILE__` assert string naming a `.c`/`.cpp`: all its referrers are one TU, and two such strings bound a boundary | strong |
| adjacent private pool labels whose referrer runs are disjoint and ordered | strong boundary |
| a function called only from inside the range, or a caller/callee pair with no other callers | weak (soft vote) |
| a `_savegpr_*`/`stmw` flip or a record-form presence change between neighbours | weak (flag fingerprint, idea 21) |
| an alignment gap > 4 bytes | weak - `.text` here is one run whose gaps are only 4/8/12 bytes |
| dtk's `auto_*` units | **not evidence** - per-function build scaffolding (5 535 of 6 000 sampled hold one function) |
| a `lbl_` shared by several functions | **not evidence by itself** - `.sbss` 69.8 % and `.bss` 60 % of labels are ordinary cross-TU globals |
| a repeated constant value | only with the copy + span test above; dtk labels every pool word, so repeats alone are noise |
| `extab`/`extabindex`/`.init` labels | **excluded** - per-function unwind fragments, and `@eti_` names are aliases for arbitrary addresses in the section (real code loads `"@eti_8001FFF8"+0xA`) |

## Matching settles the boundary

1. **Take the MATCH SET and write it as one unit.** Do not wait for a perfect boundary — the set is what
   can be matched today, and the unit's flags come out of that diff (`mwcc-unit-matching`).
2. **Claim `.text` first, data afterwards, one range at a time.** A wrong data range costs the target's
   `R_PPC_NONE` pool relocations and *lowers* the score (playbook idea 23); measure each claim
   (`objdiff-verify`).
3. **Read a symbol that will not match as boundary evidence.** A function in the set that refuses to match
   under the unit's flags - or a `bl` to a function just outside the set that the same source file must
   contain - is the signal that the extent is wrong; the corrected extent then becomes a labelled range in
   tier 1 below.
4. **`--splits` output is a proposal and is never applied automatically.** `configure.py` (`mw_version`,
   `cflags_*`) and `splits.txt` stay deliberate edits.

## Iterating on this tool

Judge an idea by whether it makes the *set* better, never by boundary exactness alone.

1. **Score it.** One command, offline - no Ghidra and no Dolphin, or the numbers stop being reproducible:

   ```sh
   python tools/splits/tudiscover.py bench --seeds 400 --save build/tmp/tudiscover/baseline.json
   python tools/splits/tudiscover.py bench --compare build/tmp/tudiscover/baseline.json   # after a change
   ```

   Compare like with like: same `--seeds` and `--seed`.
2. **One idea at a time, as a row in the table below before it is tried**, behind a real CLI switch
   (`--span-max`, …) so the bench can A/B it in one run rather than by editing code between runs.
3. **Apply only what improves a tier and regresses nothing.** A change that shrinks the tail by making the
   tool louder (more singletons) is not an improvement; one new partial overlap, or a tier-1 start that
   moves off the claimed one, is a regression whatever else improved.
4. **Land it, re-measure on the committed tool, and quote those numbers** - never a scratch script's.
5. **Record it in the same session**: the row to `done`/`no` with a one-line reason, the new baseline in
   `build/tmp/tu-boundary-discovery.md`, and a line here if the *method* changed.

| tier | measures | v1 baseline | role |
| --- | --- | --- | --- |
| 1 labels | the units `splits.txt` claims (3 today): predicted `.text` start/end and data runs vs claimed | start **exact on all three**; end short (RSO/runtime 8 of 9 fns, Camellia 1 of 10, Runtime.PPCEABI.H 1 of 2) | guardrail. Tiny, and two of its units are the pathological ones - do not tune to it |
| 2 sweep | 400 random seeds: closure median/p75/p90/max, guard hits, singleton share, leak-free data runs, mean density | median 1, p75 26, p90 69, **max 132**, guard 0, singletons 67 %, leak-free 30 %, density 0.81 | the objective: `max`/`p90` catch over-merging, leak-free/density catch wrong boundaries |
| 3 consistency | closures of all sampled seeds: distinct intervals, *partially overlapping* pairs | 354 distinct, **0 partial overlaps** | regression canary: must stay 0 |

Tier 1 grows by itself — every unit that gets matched adds a labelled range — which is why the tool should
read `splits.txt` as an input as well as an answer key (idea 1).

| # | idea | problem it solves | status |
| --- | --- | --- | --- |
| 1 | claimed `splits.txt` ranges as hard constraints | solved ground gets re-proposed, and a closure can cross a boundary that is already decided | todo |
| 2 | classifier sweep (`--span-max`, copied-value rule) | the privacy thresholds are hand-picked guesses; the bench is the harness that can pick them | todo |
| 3 | one global partition instead of N independent closures | per-seed closures cannot use each other's evidence, and a seed with no anchors stays a singleton | todo |
| 4 | call-graph closure as a second, separately-labelled hypothesis | a helper called only from inside the set (the `RSONotify*` thunks) is invisible to layout evidence | todo - needs more tier-1 labels to judge |
| 5 | Dolphin dynamic tier (indirect callers, runtime data ownership) | static xrefs see no vtable calls, so a C++ unit's call closure is incomplete | todo |
| 6 | grow-while-`leak == 0` boundary policy | the closure finds the *start* reliably and under-covers the tail; growth may recover it | todo |

## What the tool cannot do yet

* **A unit with no pooled data leaves no layout evidence** - `Camellia`'s 10 functions return the seed
  alone: its S-box tables are named symbols with one referrer each, so nothing links them. The boundary then
  comes from the call graph, the Ghidra dump's names/order, and from matching.
* **The tail is under-covered**: `RSO/runtime` returns 8 of the 9 functions (`0x804D9EC4..0x804DAE40`;
  retail's range starts one function earlier at `0x804D9B4C`) because `LocateObject`'s only caller is
  outside the closure and it shares no private data. On all three claimed units the *start* is exact and
  the end is what is missed - which is why the set, not the range, is the deliverable.
* **Dynamic evidence is not wired in yet.** `dolphin_breakpoints` + `dolphin_stack` give *indirect* callers
  that no static xref has (this DOL is nw4r-heavy C++), and `dolphin_watch` gives data ownership at
  runtime.
* **RSO modules are out of reach**: they have no target objects at all (`docs/rso-modules.md`), so this tool
  covers the DOL only.
* Names, signatures and struct layouts come from the shared memory dump (`docs/memory-dump.md`) — this tool
  tells you *where* a unit is, never *what* it is.
