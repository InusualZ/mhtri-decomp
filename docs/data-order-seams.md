# The `.data` emission order as translation-unit seam evidence

Status: discovery measured 2026-09-29 and **corrected the same day** (section 3): the first version of the rule
was wrong because the measurement left out inline functions. The corrected rule and the tools built on it are
below; what is still unproven is listed in section 3.

## 1. The discovery

Finishing `Network/network_transport` showed that the retail `.data` run `0x805F94E0..0x805F9A40` interleaves
vtables with strings, and that one translation unit (TU) cannot reproduce that order: after the lane converted the
peer types to real classes, every table was emitted at the right size, but the `.data` section scored 10.4 % of
4108 B. The unit is several TUs, and the registered range hides it.

The reason is how MWCC lays out one TU's `.data`. Measured by compiling scratch files with the project's flags
(`Wii/1.3`, `-O3`, `-inline noauto`, `-str reuse`; `.pi/tmp/data-order-verify/` holds the scripts and the full
matrix):

| position in the TU's `.data` | what | order inside the group |
| --- | --- | --- |
| 1 | initialised global data (over 8 B) | definition order |
| 2 | string literals of **out-of-line** functions | first-use order; identical literals merge (`-str reuse`) |
| 3 | **vtables** (`__vt__<class>`) | **reverse** of class order (`C, B, A` for `A, B, C`) |
| 4 | string literals of **inline** functions (in-class bodies, virtual or not, free `inline` functions) | emission order; **one unmerged copy per inline instance** - the "inline tail" |

So a TU's `.data` is `D* S* V* s*`. Objects of at most 8 B go to `.sdata`, and const tables to `.sdata2`; neither
takes part. This order holds on all nine `Wii/*` compilers and on `GC/3.0a3`..`3.0a5.2`; `GC/1.0`..`2.7`
interleave each vtable with inline strings (`D S4 V1 s4 V2 S1`). The project builds nearly everything with
`Wii/1.3`, two libs with `Wii/1.0`, one with `GC/3.0a3` and one with `GC/1.2.5n`.

The linker concatenates TU fragments, so in **retail** `.data`:

* **V->S (strong).** Two vtable groups with strings (or other data) between them: the second group belongs to a
  later TU, because a TU's vtables are contiguous. The boundary lies somewhere in the gap - *after* any inline tail
  of the first TU. Its width can be one symbol (`network_transport`) or hundreds.
* **V->tail (weak).** A vtable group followed by strings and no later vtable: an inline tail of the same TU, or
  the next TU. **A vtable followed by strings is not, by itself, a seam.**
* **Zigzag (strong).** Inside one TU adjacent vtables descend by owner address (their first code slot); an adjacent
  pair whose owners go *up* is two TUs, even with no string between them. The reverse order held on every
  compiler tried, and no contradiction was found; it has not been tested against a retail TU with known classes.
* **V->D (weak).** A vtable followed by ordinary data: globals precede vtables in a TU, but a jump table is `.data`
  too and its place in the order is unmeasured.

## 2. How much it finds

`tools/splits/dataorder.py scan` classifies every `.data` symbol of the retail DOL: **vtable** (leading `0,0`
header, all other words code pointers or zero), **string** (printable, NUL-terminated), otherwise **data**. Jump
tables are not vtables: they have no `0,0` header (a first pass that counted them gave a bogus 402/130 split).

| measure | result |
| --- | --- |
| `.data` symbols classified | 11,254: 231 vtables, 3,728 strings, 7,295 other |
| V->S (strong: strings between two vtable groups) | 65: **4 inside registered units** (all in `network_transport`, gap widths 1, 16, 1, 44), 61 in unclaimed `.data` (23 with a gap of at most 8 symbols, 45 at most 30, widest 414) |
| V->D (weak) | 39: 37 in unclaimed `.data`, 2 at registered unit starts |
| adjacent vtable pairs | 126: 62 "up" (seam) - 58 unclaimed, 3 inside `network_transport`, 1 at a registered unit start; 52 "down" (same TU); 12 ties (equal owners: no evidence) |

A wide gap says only "a TU boundary is somewhere in here": unclaimed `.data` holds many TUs with no vtables, so the
useful seams are the narrow ones.

## 3. Confidence, and the correction

* **The first version of the rule was wrong.** It said "a vtable followed by a string starts a new TU". The
  scratch test behind it had no inline functions with strings. Phase 1 (`tudiscover`) then found the rule
  contradicted by strong `__FILE__` anchors at eight g3d seams (0x8058BE40, 0x8058C928, 0x8058D5F8, 0x8058F070,
  0x8058F388, 0x8058F6B0, the zigzag 0x8058F670, 0x8058FB00) and at 0x80594A40 in `ef/ef_drawstrategyimpl`. A
  read-only verification lane (2026-09-29) tested four explanations and found **inline-function strings** (the
  fourth row of the table): at every one of those seams the first strings after the vtable are inline-function
  asserts (`g3d_resnode_ac.h`, `g3d_calcworld.h`, `particle.h`, repeated unmerged), and their first referrer is a
  function of the *same* registered unit as the TU's `__FILE__`. The vtable classification is right (leading `0,0`,
  code slots) and g3d is `Wii/1.3` like the Capcom code, so it is not a compiler difference. The rule is wrong for
  **all** `Wii/1.3` code as first written, Capcom included; the grammar above is the correction.
* **What survives.** Strings between two vtable groups (V->S), and the zigzag. Under the corrected grammar the
  `network_transport` gaps at 0x805F9570, 0x805F9610 and 0x805F9958 are still seams: a vtable follows each gap.
  The last one, 0x805F9A40, follows the final vtable of the run, so it may be an inline tail.
* **Ground truth is scarce.** Only three built units have vtables (293 built); `network_transport` is the only
  multi-vtable one, and no `Matching` g3d unit claims `.data`. The compiler side is measured; the retail side is an
  inference from a linker that concatenates TU fragments (tudiscover measures Spearman 1.000 for `.sdata2` against
  `.text`).
* **Tool numbers were computed with the old rule.** Phase 1's bench (strong pins 3 hit / 30 miss) and Phase 2/3's
  seam counts treat the first string after a vtable as the boundary, which the inline tail moves later. They are
  warnings and soft votes only; Phase 5 realigns them (section 6: the strong bench pins went from 3 hit / 30
  miss to 0 hit / 10 miss, and a `V->S` seam is now a gap, not a cut at the first string).
* Untested: RTTI-on classes (the game builds `-RTTI off`, so vtable headers are `0,0`), `extern "C"` data,
  function-local statics, data emitted by `#pragma` sections, `-lang=c` translation units, and `GC/1.2.5n` code
  (which would interleave).
* A vtable owner is approximated by its first code slot; the key function would be exact. Twelve adjacent pairs
  share an owner (identical first slot) and are treated as no evidence.

## 4. Which tools it touches

| tool | today | what the rule adds |
| --- | --- | --- |
| `tools/splits/tudiscover.py` | observations: must-link, pool-run jump, codegen fingerprint, alignment gap; never the layout *inside* a TU's data | two new observations - V→S (hard, like a pool-run jump) and the zigzag (soft). Vtables have no code referrers except constructors, so pool-run jumps never see them |
| `tools/units/attribute.py`, `brief.py` (TU probe) | cuts big unclaimed proposals by `__FILE__` names or a size cap; many briefs warn "cut by `--max-bytes`, no evidence" | the 61 data seams in unclaimed `.data` become cut points; the brief can list data-seam evidence |
| `tools/units/dataclaim.py`, `dataqueue.py` | proposes `.data` claims and files requests | a claim that contains a V→S transition or an "up" pair spans several TUs: warn/refuse, and cut a proposal at fragment ends |
| `tools/units/flipcheck.py`, `datagap.py` | reports a `.data` mismatch as bytes/size | detect "same symbols and sizes, different order" and say **order-only: multi-TU seam** |
| `tools/units/vtableaudit.py` | rule 10: an owned vtable must be emitted | also check emission order in the built object (vtables last, reverse) |
| `docs/matching.md` + the `decompiler` profile | seam check text mentions `__FILE__` and pins | playbook row 80; the seam-check step names the data-order check |

## 5. Plan

Each phase is one branch, one landing, with a selftest; nothing in it changes codegen, and a tool that moves a
gate row is verified on the whole tree first.

* **Phase 0 - the core** (`tools/splits/dataorder.py`): classify `.data` symbols from the DOL and the map;
  fragment a run; report V→S transitions and zigzag pairs with addresses and owners; CLI `dataorder.py at
  <addr>`, `dataorder.py scan --json`. Selftest on fixtures, plus the real DOL: the four `network_transport`
  seams must be found. Acceptance: reproduces 231 / 65 / 4 / 61 and the 62 up / 52 down / 12 tie split.
* **Phase 1 - tudiscover** (needs the asm dump): the two observations, scored by `tudiscover.py bench`.
  Acceptance: bench precision/recall does not drop, and the `network_transport` seams (0x805F9570, 0x805F9610,
  0x805F9958, 0x805F9A40) are proposed as boundaries; report any V→S transition a strong must-link contradicts.
* **Phase 2 - data-claim guards** (`dataclaim.py`, `dataqueue.py`): warn when a claim spans a seam; cut a
  proposal at fragment ends. Then `flipcheck.py`/`datagap.py` order-only diagnosis.
* **Phase 3 - proposals** (`attribute.py`, `brief.py`): data seams as cut evidence and in the TU probe text.
* **Phase 4 - audit and docs**: `vtableaudit.py` emission-order check; playbook row 80 (done with this
  document) and the `decompiler` profile's seam-check step.

Order: 0 first; then 1, 2, 3 and 4 are independent and can run as parallel lanes.

## 6. Progress

All five phases are landed (2026-09-29), each behind its own selftest and the landing gate.

| phase | landed | what it is | measured |
| --- | --- | --- | --- |
| 0 | `6adaff25e` | `tools/splits/dataorder.py`: classify, seams, fragments; `scan`, `at` | 27 checks; 231 / 65 / 4 / 61 and 62 up / 52 down / 12 tie reproduced |
| 4 | `00a0a38a1` | `vtableaudit.py --order`: vtables after every other `.data` symbol, reverse class order; profile and reviewer text | 3 of 293 built units have vtables, 0 findings; `network_transport`'s 7 vtables are in exact reverse class order |
| 2 | `4d5768b95` | `dataseams.py` over `dataorder`; `dataqueue` cuts runs at strong seams and warns on a request; `dataclaim` seam warnings; `flipcheck`/`datagap` "order-only"/multi-TU line | 76 of 3,050 queue runs contain a strong seam (127 seams); both tools name the four `network_transport` seams (its object is 0x560 of 0x100C B, so it reads multi-TU, not order-only - that path is fixture-tested only) |
| 3 | `035653805` | `attribute.py` `data_seams` on proposals, `attribute.py dataseams`, `brief.py` TU-probe paragraph with a lower bound on the TU count and candidate cuts (never applied) | 11 of 67 ready pool proposals carry an interior seam (at least 2-5 TUs), 14 carry any; a run only counts when it is dense (`density >= 0.5`), a rule the lane added |
| 1 | `29a0d8ddf` | `tudiscover.py` observations `dataorder` / `dataorder-zz` / `dataorder-weak`, `--data-order off\|on\|strong\|weak` (default `on` = soft votes), the `dataorder` subcommand, bench tier 4 | `strong` costs 0.001 precision and gains no recall, so it is opt-in; the four `network_transport` seams are proposed |

**What Phase 1 changed about the claim.** The rule is not universal: eight seams are contradicted by `__FILE__`
anchors, all in the g3d (NW4R) library, and 28 pinned intervals lie wholly inside one registered unit. Treat the
rule as established for the Capcom game code compiled with `Wii/1.3` (`network_transport`, and the built objects
above) and as **unproven or wrong elsewhere** until the library's compiler version is checked. The tools therefore
default to soft evidence and warnings, and none of them refuses.

**Open**
* Fit the observation weights (1.0 / 0.5 / 0.25 are unfitted); tune referrer selection with more ground truth.
* Check whether g3d and the `ef` strategy units use a different MWCC version or emission order.
* The order-only diagnosis is tested on fixture objects only, and `attribute.py queue` has not been re-run, so the
  queue and the pool briefs carry no `data_seams` yet (the next regeneration adds them).
* Split `Network/network_transport` at the four seams into per-class units, so its `.data` can match.

**Correction (same day).** The verification lane's answer to "is the g3d contradiction true?": yes for the rule as
first written, and the cause is the inline tail, not a different compiler (section 3). `dataorder.py` now returns
`V->S` only when a later vtable follows the strings (with `latest`, `width` and `tail` fields), `V->tail` and
`V->D` as weak. Phase 5 below carries the consumers.

* **Phase 5 - realign the consumers with the inline tail.** `vtableaudit.py --order` must not flag `@NNN` strings
  after a vtable (an inline tail); `tudiscover` must treat a `V->S` seam as "a boundary in the gap", after the
  leading inline-tail strings, and re-run bench tier 4; `dataseams`/`dataqueue`/`dataclaim` must cut a gap by its
  `tail`, not at its first string; `attribute`/`brief` wording says "a boundary in [addr, latest)", not a cut at
  a string.

**Phase 5 - measured (branch `worker/data-order-p5-ca02`).**

| consumer | change | before -> after |
| --- | --- | --- |
| `vtableaudit.py --order` | `@NNN` / `@STRING@<inline function>` strings after a vtable are the inline tail, not a `vtable-before-data` finding; only an initialised global after a vtable is | scratch TU (global, strings, two classes, an in-class inline body, a free inline function; `Wii/1.3 -O3`): 2 false findings -> 0; a hand-moved global: 1 finding |
| `tudiscover.py` (`dataorder*`, bench tier 4) | a `V->S` seam is a gap `[addr, latest)`: interval = last referrer of the vtable run and of the tail strings .. first referrer of the next vtable run; no whole-fragment tightening; `V->tail` dropped from pins | `--data-order strong`: 36 cuts, 3 hit / 30 miss (precision 0.091) -> 10 cuts, 0 hit / 10 miss (0.000); default `on`/`off`: precision 0.110, recall 0.555, unchanged (the seams are soft) |
| `tudiscover.py dataorder` | contradictions by a must-link | 10 (8 by a `__FILE__` anchor) -> 2 (1 by an anchor, the g3d_scnroot zigzag); the 8 g3d and the ef V->S seams are no longer contradicted: 8 g3d + ef now pin an interval holding a registered unit start; pinned intervals at a unit start 20 -> 38; V->S pinned/overlap 42/22 -> 49/16 |
| `dataseams.py`, `dataqueue.py`, `dataclaim.py` | each `V->S` row gets a `cut` (first symbol after the tail); a gap of at most 8 symbols cuts there, a wider one only warns "a boundary in [addr, latest)"; zigzag cuts at its address; warnings say "at least N TUs" | 76 of 3,050 queue runs contain a strong seam (unchanged); 49 hold a cuttable seam (narrow V->S gap or zigzag), 27 only wide gaps (warn, no cut); of the 65 V->S rows 25 are narrow (width <= 8), 40 wide; no narrow gap has a detected tail |
| `attribute.py`, `brief.py` | `V->S` item carries `latest`/`width`/`tail`; its candidate cut is between the two vtable owners (not after one); `V->tail` records dropped; TU-probe text says "a boundary in [addr, latest)" | 11 of 67 ready pool proposals carry an interior seam (14 any): unchanged, the real DOL has no `V->tail` row |
| `flipcheck.py`, `datagap.py` | order-only / multi-TU lines come from `dataseams` (corrected strong seams) and say "a boundary in [a, b)" for a gap | fixture-tested (the real order-only case is still untested) |

Findings for the rule text (not edited here): `dataorder.seams()`'s `tail` counts only leading strings that are bare
header names, but the real inline tails start with an assert message (`NW4R:Failed assertion ...`, then
`g3d_resnode_ac.h`, message, header, ...), so `tail` is 0 for every real seam and no narrow gap is cut after a tail
yet; the 0x805F94E0 seam family of `network_transport` and the g3d gaps are the evidence.
