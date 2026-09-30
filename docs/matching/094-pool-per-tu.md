---
id: 94
title: One literal pool per TU: a pool literal two units read means the units are one original TU
status: works
problem: A unit's literal pool is a partial pool - two neighbouring units' `.sdata2` interleave or share entries, the first `.data` object is 4 B late, or a claim "cannot be reproduced" - because the original was ONE TU cut into several registered units.
tags: [data, sections, measurement]
applies: [Wii/1.3]
demo: 094-pool-per-tu.cpp
reviewed: 2026-09-30
related: [23, 29, 43, 53, 58, 80]
---

# 94. One literal pool per TU: a pool literal two units read means the units are one original TU

**Problem.** A unit's `.sdata2`/`.sdata` will not reproduce: our object's pool is shorter or longer than the
target's, an entry the target holds is read by a neighbouring registered unit, the unit's first `.data` object
sits 4 B late against an 8-aligned start that belongs to the neighbour, or a claim reads "cannot be reproduced"
(idea 58's shared `0x8079A008`). Nothing is wrong with the source or the flags: the original was **one
translation unit** that the registry cut into several units, and no piece can own the pool alone.

**Why it happens.** MWCC emits **one literal pool per TU**: one entry per distinct value, in creation order, function
by function in source order, floats and doubles separate, a double 8-aligned; the int->float magic
`0x4330000080000000` is one of those entries, not a linker-made one. `mwldeppc` **does not merge pools across
objects**. Measured with the real compiler and linker: `f` and `g` in one TU (`1.5f+2.5f`, `1.5f+3.5f`) pool `2.5f 1.5f
3.5f` = **12 B**; the same two functions in two TUs link to **16 B with two copies of `1.5f`**. So a pooled literal
address that two registered units read is one TU's entry (a **fold**), and one value at two addresses is two TUs
(a **cut**). `-sdata2 N` only moves the entries between `.sdata2` and `.rodata`, it never merges or reorders them.

**How to work it.**

1. Read the group before writing bodies: `python tools/units/poolseams.py --unit <unit>` (or `datagap.py --pool-seams`).
   It lists the registered units that read a pooled literal with yours, whether their `.text` is adjacent (a TU's
   `.text` is contiguous: a unit *inside* the span is part of the fold too), whether the pool runs in first-use order,
   whether the group's `.data` interleaves foreign data (then the pool model and the data model disagree), and a confidence.
2. `python tools/splits/tudiscover.py at <addr>` adds the `pool model` block (the fold candidate) and `pool dedupe`
   (where a value repeats: a new TU's first code lies in the interval).
3. A claim, flip or pool difference that a group explains is **blocked by a seam**, not by the source: `datagap.py`,
   `flipcheck.py` and `sectiongap.py` say "candidate fold: ..." instead of a bare deferral. Register the units as one
   (or claim the whole TU's `.text` and pool together, idea 29) rather than hunting a flag.
4. Do **not** treat every multi-unit address as a literal: a named global scalar/table, every `.data`/`.bss`/`.rodata`
   object and an `.sdata` `char[]` are read by several TUs legitimately. Only `.sdata2` 4/8 B and `.sdata` strings are
   edges; only `float`/`double`-typed `.sdata2` rows are dedupe witnesses (an untyped word can be half of an 8-byte object).

**Result.** Whole tree, 2026-09-30 (`docs/pool-seams.md`): **20 groups over 100 of 302 registered units**, 745 shared
literals, 16 text-adjacent and 4 interleaved, confidence high 4 / medium 15 / low 1. The cockpit case reproduces:
`menu/fn_802E4978 + hud/cockpit_quest + hud/fn_802EBED8 + ef/eft035` (`0x802E4978-0x802F5138`, 18 shared literals,
adjacent, first-use order holds; `eft035` is itself several TUs). Premise check on the real DOL: 567 of 1,862 distinct
float/double values sit at several addresses (5,219 extra copies); across 95 `__FILE__`-anchored TUs (285 pooled
literals, 270 of whose values recur in other pools) **no value is held twice inside one TU and no shared literal spans two
files**; 75 of 232 units hold one value at two addresses, i.e. are already several TUs. `tudiscover` bench: `pooldup`
precision 0.240 (29 hit / 92 miss, misses sit in those several-TU units), claimed unit starts pinned 157 -> 162 under
`--pool-model strong`.

**Example.**

```
/* one TU: f and g share ONE 1.5f */
float f(float x) { return x * 1.5f + 2.5f; }   // .sdata2: 2.5f 1.5f
float g(float x) { return x * 1.5f + 3.5f; }   //          3.5f          -> 12 B
/* the same two functions as two registered units: 8 B + 8 B, two 1.5f, link to 16 B */
```

**When NOT to apply.** A literal that is a real named global (`extern const float k`) is shared across TUs and proves
nothing; the group's confidence (adjacent text, first-use order, contiguous `.data`) is the guard. A TU that is still
several TUs inside one unit (`coarse`) chains its neighbours into one component: fold it in pieces.

**Demonstration.** `094-pool-per-tu.cpp` (`ideas.py demo-check 94`, 2026-09-30): `f` and `g` in one TU give
`.sdata2` 12 B, bytes `402000003fc0000040600000` (`2.5f 1.5f 3.5f`: creation order, never a second `1.5f`), and both
functions relocate against the same anonymous `@7` entry. The two-TU contrast (8 + 8 B, linking to 16 B) is measured with
`mwldeppc`, not in the demo, because a demo is one TU.
