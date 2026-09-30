---
id: 80
title: A TU's `.data` is globals, strings, vtables in reverse, then inline-function strings - two vtable groups with strings between them are two TUs
status: works
problem: A unit whose classes all emitted vtables of the right size still scored 10.4 % on `.data`, because the retail run interleaves vtables with strings - an order no single TU produces - so it reads as a claim or class-model defect.
tags: [data, vtable, sections]
applies: [Network, Wii/*, GC/3.0a3]
demo: 080-data-order-vtables-strings.cpp
reviewed: 2026-09-29
related: [23, 52, 53, 54, 68, 70, 76]
---

# 80. A TU's `.data` is globals, strings, vtables in reverse, then inline-function strings - two vtable groups with strings between them are two TUs

**Problem.** `Network/network_transport` converted its peer types to real classes so the compiler would emit their
vtables, and every table came out at the right size - yet the unit's `.data` section scored 10.4 % of 4108 B. The
retail run `0x805F94E0..0x805FA4EC` interleaves vtables with strings, and no source spelling of one TU produces
that order. It reads as a data-claim or class-model problem and invites more class rewrites.

**How it looks.** Objdiff pairs every symbol of the `.data` section by name and every size matches, but the
*addresses* differ: retail has `vtable, vtable, string..., vtable` where our object has all the strings first and
all the vtables together. The tell is not a diff row but the order of `symbols.txt` rows in the claimed range:
strings between two vtables (`dataorder.py at <addr>` prints them with kinds).

**Why it happens.** MWCC lays out one TU's `.data` as: initialised globals over 8 B in definition order; the strings
of out-of-line functions in first-use order (identical literals merge under `-str reuse`); **vtables in the reverse
of class order**; then the strings of **inline** functions - in-class bodies and free `inline` functions, one
unmerged copy per instance (the "inline tail"). Small (at most 8 B) objects go to `.sdata` and const tables to
`.sdata2`. Measured on all nine `Wii/*` compilers and `GC/3.0a3`..`3.0a5.2`; `GC/1.0`..`2.7` interleave each vtable
with inline strings. The linker concatenates TU fragments, so in retail `.data`: two vtable groups with strings
between them are two TUs (the boundary is in the gap, after any inline tail), and two adjacent vtables whose
owners' first code slots go *up* in address are two TUs (inside one TU they descend). **A vtable followed by strings
is not by itself a seam** - it may be the inline tail (a first version of this row said it was, and eight g3d seams
and one `ef` seam, all inline asserts naming `*_ac.h` or `particle.h`, proved it wrong).

**How to work it.** Classify the run's symbols (vtable: leading `0,0` header and code pointers; string:
printable NUL-terminated; jump tables are not vtables) and look for strings between two vtable groups and "up"
vtable pairs before claiming or reconstructing it: `python tools/splits/dataorder.py at <addr>` (the symbols around
an address, with kinds and seams) and `dataorder.py scan` (every seam in the DOL); `python tools/splits/tudiscover.py
dataorder` turns the seams into `.text` intervals, and `dataseams.py` feeds `dataqueue`/`dataclaim` so a claim that
spans a seam is warned about. A bare header name (`x_ac.h`) or an unmerged repeated literal right after a vtable is an
inline tail (`dataorder.inline_tail` recognises it). Cut the claim at the seams: one claim per TU fragment. The
full grammar, the confidence and what is untested (RTTI-on classes, function-local statics, `-lang=c` TUs) are in
`docs/data-order-seams.md`.

**When NOT to apply.** Not evidence for a seam: a vtable followed by strings and no later vtable (weak - inline tail
or the next TU); a vtable followed by ordinary data (weak - a jump table is `.data` too and its place in the order is
unmeasured); a wide gap (it only says "a boundary is somewhere in here"); and code built with `GC/1.0`..`2.7`
compilers, which interleave. The rule is one TU vs many - it says nothing about *which* function belongs to which
side, which is what the other seam evidence (`__FILE__` strings, idea 54; referrers) is for.

**Result.** (Measured 2026-09-29, `dataorder.py scan`, re-run the same day.) The whole DOL has 231 vtables and 65
strings-between-vtable-groups (V->S) seams, plus 62 "up" (zigzag) adjacent pairs and 39 vtable->data (weak) rows; 156
candidate cuts sit in unclaimed `.data` and none is hidden inside a registered unit any more. Ground truth is
scarce (three built units have vtables), so the tool numbers are soft votes for proposals, not gate evidence.

**Example.** `network_transport`: gaps at 0x805F9570, 0x805F9610 and 0x805F9958 (a vtable follows each) split the
unit's `.data` into per-class TUs; 0x805F9A40 follows the last vtable and may be an inline tail. **The prediction held:**
the unit has since been split into per-class units (all four gaps are now unit boundaries, `dataorder.py at` 2026-09-29; 0x805F9A40 turned out to be the next TU, `NetworkSessionStable`, not an inline tail): `NetworkPeerBase`, `NetworkPeerBuffer`, `NetworkPeerUdp`,
`NetworkPeerMcs`, `network_socket_streams`, `NetworkResolverWii` and `NetworkSessionBase` (see the `Network/` blocks
in `splits.txt`), and `dataorder.py at 0x805F9570` still marks the `V->S` seam at the `NetworkPeerMcs`/socket-streams
edge.

**Demonstration.** `080-data-order-vtables-strings.cpp` (`ideas.py demo-check 80`) reproduces the order on Wii/1.3: the
two initialised globals, then the out-of-line strings, then `__vt__1B` before `__vt__1A` (reverse class order), then
the inline function's string; strings of 8 B or less go to `.sdata` and stay out of the run.

**Evidence.** The first version of the rule (a vtable followed by a string starts a new TU) was **wrong** because the
scratch test had no inline functions with strings; the correction (the "inline tail", row 4 of the layout) is
documented, with the g3d/`ef` seams that exposed it, in `docs/data-order-seams.md` section 3. The seam kinds in
that document: V->S (strong), zigzag (strong), V->tail and V->D (weak). Related: 52/68 (vtable emission), 53/70
(claiming `.data`), 54 (who owns a table).
