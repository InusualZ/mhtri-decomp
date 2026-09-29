# The `.data` emission order as translation-unit seam evidence

Status: discovery measured 2026-09-29; the tool plan below is being executed (see "Progress").

## 1. The discovery

Finishing `Network/network_transport` showed that the retail `.data` run `0x805F94E0..0x805F9A40` **interleaves
each vtable with strings**, and that one translation unit (TU) cannot reproduce that order: after the lane
converted the peer types to real classes, every table was emitted at the right size, but the `.data` section
scored 10.4 % of 4108 B. The unit is several TUs, and the registered range hides it.

The reason is how MWCC lays out one TU's `.data`. Measured by compiling a scratch file with the project's own
flags (`Wii/1.3`, `-O3`, `-pool off`, `-str reuse`):

| position in the TU's `.data` | what | order inside the group |
| --- | --- | --- |
| 1 | initialised global data (over 8 B) | definition order |
| 2 | string literals (`@NNN`) | first-use order |
| 3 | **vtables** (`__vt__<class>`) | **reverse** of class order (`C, B, A` for `A, B, C`) |

Objects of at most 8 B go to `.sdata`, and const tables to `.sdata2`; neither takes part. The one built object
with several vtables in the repo agrees (`network_transport.o`: one `D`, eighteen `@` literals, seven `__vt__`).

Two consequences for **retail** `.data`, which is the concatenation of per-TU fragments in link order:

* **Rule V→S.** A vtable followed by a string or ordinary data symbol means a new TU starts there. Inside one TU
  nothing but another vtable can follow a vtable.
* **Rule zigzag.** Inside one TU adjacent vtables descend by owner address (their first code slot). An adjacent
  pair whose owners go *up* is a boundary between two TUs, even when no string separates them.

## 2. How much it finds

`tools/splits/dataorder.py scan` (Phase 0, landed) classifies every `.data` symbol of the retail DOL: **vtable** (leading `0,0` header, all other words code pointers or zero), **string**
(printable, NUL-terminated), otherwise **data**. Jump tables are not vtables: they have no `0,0` header (a first
pass that counted them gave a bogus 402/130 split).

| measure | result |
| --- | --- |
| `.data` symbols classified | 11,254: 231 vtables, 3,728 strings, 7,295 other |
| vtable → string transitions (V→S, strong) | 65: **4 inside registered units** (all in `network_transport`, the seams that cost a full pass), 61 in unclaimed `.data` |
| vtable → other data (V→D, weak) | 39: 37 in unclaimed `.data`, 2 at registered unit starts; a jump table is `.data` too and its place in the order is unmeasured |
| adjacent vtable pairs | 126: 62 "up" (seam) - 58 unclaimed, 3 inside `network_transport` (all real seams), 1 at a registered unit start; 52 "down" (same TU); 12 ties (equal owners: no evidence, not counted) |
| counterexamples found | none - but the ground truth is thin (see 3) |

## 3. Confidence and limits

* The compiler side is measured: one scratch TU plus one built object. The retail side is an inference from a
  linker that concatenates TU fragments (tudiscover already measures Spearman 1.000 for `.sdata2` against
  `.text`).
* Ground truth is scarce: registered units claim little vtable data, so only `network_transport` (which is now
  known to be multi-TU) exercises the rule. It has **not** been checked against tudiscover's must-link evidence
  (`__FILE__` anchors) - that needs the asm dump (`python tools/splits/dump_asm.py`, 200-400 s) and is Phase 1's
  acceptance test.
* Measured on `Wii/1.3` only. Other libraries use other compiler versions.
* Untested: RTTI-on classes (the game builds `-RTTI off`, so vtable headers are `0,0`), `extern "C"` data,
  function-local statics, and data emitted by `#pragma` sections.
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

* Playbook row 80 and this document: landed with the plan.
* Phase 0: landed (`tools/splits/dataorder.py`, 27-check selftest). Phases 1-4: in progress as parallel lanes.
