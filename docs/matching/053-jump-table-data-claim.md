---
id: 53
title: A sparse switch's jump table is readable once its `.data` range is claimed
status: works
problem: A unit whose switch has a compiler-emitted jump table measures just short of 100 % and its arms are unreachable from the `.text` alone: the table lives in `.data`, which the unit's `splits.txt` does not claim, so the splitter hands back zeros. Read as "the case-to-body mapping is unrecoverable" that looks like a hard ceiling, and the function gets written by hand or parked - one unit parked four functions on exactly that reading, and the ceiling was recorded in this project's own notes before a lane disproved it the same day.
tags: [data, sections, source-shape]
applies: []
demo:
---

# 53. A sparse switch's jump table is readable once its `.data` range is claimed

**Problem.** A unit whose switch has a compiler-emitted jump table measures just short of 100 % and its arms are
unreachable from the `.text` alone: the table lives in `.data`, which the unit's `splits.txt` does not claim, so
the splitter hands back zeros. Read as "the case-to-body mapping is unrecoverable" that looks like a hard
ceiling, and the function gets written by hand or parked - one unit parked four functions on exactly that
reading, and the ceiling was recorded in this project's own notes before a lane disproved it the same day.

**Why try it.** The table is *data the compiler emitted for this TU*. Claiming its `.data` range puts it in the
unit's own object, at which point the bytes and the relocations are both there - and the relocations are what
pair retail's `lis`/`addi`. Unclaimed, the same function measures **99.999**: close enough to look like a
codegen residual and send you hunting flags that are not the problem.

**Result.** Add the table's `.data` range to the unit's `splits.txt` block; the neighbouring units' records
bracket it exactly, the same way they bracket `.text`/`extab`. Then read the table from **`main.elf`**: each slot
is an absolute arm address, so grouping slots by target gives each arm's `case` set and the `default` epilogue.
Never hand-map DOL virtual addresses to file offsets - a first mapping that was silently wrong produced a
plausible-looking table whose arm addresses pointed into the *previous* function. Generate the arms mechanically
and **calibrate the generator against an already-landed sibling** before trusting it on your own range.

**Example.** (2026-09-26) `Pl/fn_802430E8`: claiming `.data 0x805C4134-0x805C4548` (261 entries) took its owner
from 99.999 to **100.0**, and diffing that table against the landed `Pl/fn_80241558`'s showed **52 of 59 arms
instruction-identical** - the two functions are siblings, so a 7,584 B body was recovered rather than guessed.
`Pl/fn_802373AC` then generated **145 arms** mechanically, having first required the same translator to
reproduce `src/Pl/fn_8023C2D0.cpp` line for line from the landed `0x805C34D4` table, and landed a **99.986 %**
unit whose `fn_802399C8` is byte-identical at 10,504 B.

**Correction (2026-09-26).** This section's earlier claim that `Pl/fn_8023C2D0`/`fn_80230FBC` were "blocked only
by a jump table misplaced inside `.data`" was **wrong**, and the way it was wrong is worth keeping. Both units
were byte-identical already, and their `.data` claims were in `splits.txt`; the misdiagnosis sent a lane hunting a
data-placement bug that did not exist. `flipcheck.py` judges the **object**, and the objects and their table claims
are both fine - the blocker is the **link**. What makes that reading trustworthy is the control set it ships with:
run against two units already flipped to `Matching` it returns READY, and run against `Pl/fn_802373AC` it returns
NOT READY at `.text +0x1671` (ours `1c`, target `1b`), which is exactly that unit's documented 12-instruction
register residual. A tool that returned READY for everything would have been the trap instead. So: when an object
is byte-identical and a flip is refused, suspect the **link** (`.ctors` ordering, link padding, the `active_flags`
export bit) before you suspect the data.

**Refinement (2026-09-26, evening): claim the table, not the band - and the row can be 0 %, not 99.999.**
`stage/fn_802B3270` measured **0 %**: a 23-entry `switch (st->mapno)` with two nested `areano` dispatches,
whose arms were unreadable for exactly this reason. Claiming **only the table's own range**
(`0x805CF728-0x805CF784`, 92 B = 4 x 23 - a size the compare chain proves *before* you claim anything) makes
our object **emit** it, so the unit's `.data` section pairs at **100 %**, and `main.elf` then hands over every
arm: the function went **0 -> 100.00 % byte-identical at 3688 B**, the unit 59.05 -> 88.75 %, and two more rows
reached byte-identical on the way (`fn_802B4824` 76.04 -> 100, `fn_802B45D4` 78.75 -> 100).
Do **not** claim the band around the table. The 19 hand-written labels in `0x805CF60C-0x805CFBE8` are target
bytes our source reproduces none of, and claiming them *lowers* the score (`tools/flags/dataclaim.py`'s
`lowers-score` rule, plan 8.4) - file them as `range` config_requests instead, with the reachability evidence:
the two records `data-queue.json` calls "unreferenced" are reached from this unit's own tables
(`0x805CF664/690 -> 0x805CF638`, `0x80792470 -> 0x805CF718`), so there "unreferenced" means "not yet
attributed", not "dead".
The shapes that finished the 3688 B body are the ones sections 18/19/34/37 describe, worth knowing together:
writes through the **struct field** (`w->show[show_i] = v; show_i++;`, worth 28 points), a store of
`*src; src++;` rather than `*src++` (7), the nested `fn_802FB8EC` dispatches as `switch`es while
`LbCheckKujiraEvent`/`fn_802FB9F8` stay `if`/`else if` (7), the callee returning **`u32`** rather than `s32`,
and declaring `hide_buf` **before** `show_buf` (the declaration order decides which stack slot MWCC picks -
reversed, it swaps them). A wrong prototype at a call site is its own measurable defect: `fn_802B4C5C` was
declared with **1** parameter where retail passes **3**, and fixing the arity moved it 95.69 -> 96.38.

**Refinement (2026-09-27, `fn_80429B94`): a unit that claims several runs of one section must own the bytes
between them.** Claiming a band's jump tables but not the unnamed blobs between them splits the range against
dtk's own `auto_<n>_<addr>_data` unit, that unit lands *inside* the claiming unit's address range, and
`dtk dol split` stops with `Cyclic dependency encountered while resolving link order: <unit> ->
auto_<n>_<addr>_data`. Either claim every run of that section, or drop a run entirely; the first is right when
the blobs are the band's own data. `fn_80429B94`'s `lbl_80603C98` (a colour table) and `lbl_80603CB8` are
referenced from `fn_80429B94` and sat between its sixth and seventh jump tables, so merging the claims
`0x80603C6C-0x80603C98` and `0x80603CE4-0x80603D10` into one `0x80603C6C-0x80603CE4` took the split from that
cycle to green with the DOL hash unchanged. A *leading* or *trailing* unclaimed run is harmless - a unit may
reference an auto unit's data, one direction, no cycle - only a run **between** two of the unit's own does this.

