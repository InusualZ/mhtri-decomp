---
id: 53
title: A sparse switch's jump table is readable once its `.data` range is claimed
status: works
problem: A unit whose switch has a compiler-emitted jump table measures just short of 100 % and its arms are unreachable from the `.text` alone - the table lives in unclaimed `.data`, so the splitter hands back zeros - and the function gets hand-written or parked.
tags: [data, sections, source-shape]
applies: []
demo: 053-jump-table-data-claim.cpp
reviewed: 2026-09-29
related: [23, 29, 54, 70, 78]
---

# 53. A sparse switch's jump table is readable once its `.data` range is claimed

**Problem.** A unit whose switch has a compiler-emitted jump table measures just short of 100 % and its arms are
unreachable from the `.text` alone: the table lives in `.data`, which the unit's `splits.txt` does not claim, so
the splitter hands back zeros. Read as "the case-to-body mapping is unrecoverable" that looks like a hard ceiling,
and the function gets written by hand or parked - one unit parked four functions on exactly that reading, and the
ceiling was recorded in this project's own notes before a lane disproved it the same day.

**How it looks.** A function with a `lis`/`addi` to a table, `mtctr`/`bctr`, and a series of arm bodies is at 99.99x %
(or 0 % when the whole switch is unwritten), the first divergence is the table's `@ha/@l` relocation (the
`lis`/`addi` pair that builds the address) naming a symbol we do not have, and the target's `.data` has an entry of
N words that our object lacks.

**Why it happens.** For a dense `switch` MWCC emits an **absolute-address jump table in `.data`** (`@N`, one 4-byte
slot per case value) and dispatches with `mtctr`/`bctr`. The table is *data the compiler emitted for this TU*.
Claiming its `.data` range puts it in the unit's own object, at which point the bytes and the relocations are both
there - and the relocations are what pair retail's `lis`/`addi`. Unclaimed, the same function measures 99.999:
close enough to look like a codegen residual and send you hunting flags that are not the problem.

**How to work it.**

1. Add the table's `.data` range to the unit's `splits.txt` block; the neighbouring units' records bracket it exactly,
   the same way they bracket `.text`/`extab`. The size is provable before you claim it: 4 x the case count the compare
   chain implies.
2. Read the table from **`main.elf`** (the linked image, not the DOL offsets): each slot is an absolute arm address,
   so grouping slots by target gives each arm's `case` set and the `default` epilogue. Never hand-map DOL virtual
   addresses to file offsets - a first mapping that was silently wrong produced a plausible table whose arm
   addresses pointed into the *previous* function.
3. Generate the arms mechanically and **calibrate the generator against an already-landed sibling** before trusting it
   on your own range.
4. Claim **the table, not the band around it** (below): the hand-written labels next to a table are target bytes our
   source does not reproduce, and claiming them *lowers* the score (`tools/units/dataclaim.py`'s `lowers-score`
   verdict; use `python tools/units/datagap.py --unit <unit>` to see the gap). A claim that must own several runs of
   one section must also own the bytes between them, or dtk's `auto_*_data` unit lands inside the range and `dol
   split` dies with a link-order cycle.

**Result.** (2026-09-26) `Pl/fn_802430E8`: claiming `.data 0x805C4134-0x805C4548` (261 entries) took its owner from
99.999 to **100.0**, and diffing that table against the landed `Pl/fn_80241558`'s showed **52 of 59 arms
instruction-identical** - the two functions are siblings, so a 7,584 B body was recovered rather than guessed.
`Pl/fn_802373AC` then generated **145 arms** mechanically, having first required the same translator to reproduce
`src/Pl/fn_8023C2D0.cpp` line for line from the landed `0x805C34D4` table, and landed a **99.986 %** unit whose
`fn_802399C8` is byte-identical at 10,504 B.

**When NOT to apply.** The table is only the *object's* half of a flip: when an object is byte-identical and a flip
is refused, suspect the **link** (`.ctors` ordering - idea 46; link padding; the `active_flags` export bit - idea
36; an odd-start claim - idea 55) before you suspect the data. See the correction below.

**Demonstration.** `053-jump-table-data-claim.cpp` (`ideas.py demo-check 53`): an 8-case dense switch compiles to
`mtctr`/`bctr` with a 32-byte `.data` table (8 x 4 B) reached through an anonymous `@N` symbol by a `@ha/@l` pair.
The object itself shows the table exists and is data; whether *retail's* claim matches still needs the target
object and `splits.txt`.

**Evidence (dated refinements and one correction).**

*Correction (2026-09-26).* An earlier claim here - that `Pl/fn_8023C2D0`/`fn_80230FBC` were "blocked only by a jump
table misplaced inside `.data`" - was **wrong**. Both units were byte-identical already, and their `.data` claims
were in `splits.txt`; the misdiagnosis sent a lane hunting a data-placement bug that did not exist. `flipcheck.py`
judges the **object**, and the objects and their table claims were fine - the blocker was the **link** (idea 55's
alignment). The control set that makes `flipcheck.py`'s reading trustworthy: against two units already flipped it
returns READY; against `Pl/fn_802373AC` it returns NOT READY at `.text +0x1671` (ours `1c`, target `1b`), that
unit's documented register residual. Re-run 2026-09-29: `flipcheck.py Pl/fn_8023C2D0` READY, and the unit is
already `Object(Matching)`.

*Claim the table, not the band (2026-09-26, evening).* `stage/fn_802B3270` measured **0 %**: a 23-entry
`switch (st->mapno)` with two nested `areano` dispatches whose arms were unreadable for exactly this reason.
Claiming **only the table's own range** (`0x805CF728-0x805CF784`, 92 B = 4 x 23 - a size the compare chain proves
*before* you claim anything) makes our object **emit** it, so the unit's `.data` pairs at 100 %, and `main.elf`
hands over every arm: the function went **0 -> 100.00 % byte-identical at 3688 B**, the unit 59.05 -> 88.75 %, and
two more rows reached byte-identical on the way. The 19 hand-written labels in `0x805CF60C-0x805CFBE8` are target
bytes our source reproduces none of; they went to the campaign's data-claim queue with reachability evidence (two
records once called "unreferenced" are reached from this unit's own tables: "unreferenced" meant "not yet
attributed"). The shapes that finished the 3688 B body are the ones ideas 18/19/34/37 describe: writes through the
**struct field** (`w->show[show_i] = v; show_i++;`, worth 28 points), `*src; src++;` rather than `*src++` (7),
nested dispatches as `switch`es while two callees stay `if`/`else if` (7), a callee returning **`u32`** rather than
`s32`, and declaring `hide_buf` **before** `show_buf` (declaration order decides the stack slot). A wrong call-site
prototype is its own defect: `fn_802B4C5C` declared with **1** parameter where retail passes **3** moved 95.69 ->
96.38 when fixed.

*A claim of several runs must own what is between them (2026-09-27, `fn_80429B94`).* Claiming a band's jump tables
but not the unnamed blobs between them splits the range against dtk's `auto_<n>_<addr>_data` unit, which lands
*inside* the claiming unit's range, and `dtk dol split` stops with `Cyclic dependency encountered while resolving
link order: <unit> -> auto_<n>_<addr>_data`. Either claim every run of that section or drop a run; the first is
right when the blobs are the band's own data. Merging `0x80603C6C-0x80603C98` and `0x80603CE4-0x80603D10` into
`0x80603C6C-0x80603CE4` (which covers the referenced `lbl_80603C98` colour table and `lbl_80603CB8`) took the split
from that cycle to green with the DOL hash unchanged. A *leading* or *trailing* unclaimed run is harmless (one
direction, no cycle); only a run **between** two of the unit's own does this.
