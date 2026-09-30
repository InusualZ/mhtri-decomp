---
id: 11
title: Do not read a large size gap as "different source"
status: works
problem: The target object is often hundreds of bytes bigger than ours, and several functions sit at 0 %, which reads like the original source containing code ours does not have (different unrolling, extra rounds, extra statements).
tags: [measurement]
applies: []
demo:
reviewed: 2026-09-29
related: [2, 3, 5, 13]
---

# 11. Do not read a large size gap as "different source"

**Problem.** The target object is often hundreds of bytes bigger than ours, and several functions sit at 0 %,
which reads like the original source containing code ours does not have (different unrolling, extra rounds,
extra statements).

**How it looks.** `.text` 21600 B against the target's 24420 B, six of ten functions at 0 %, and the diff shows
target instructions with no counterpart in ours.

**Why it happens.** Aggressive optimizer flags *remove* instructions (fusing, pooling table bases, `stmw` in
place of a call sequence), so "the target is bigger" can mean nothing more than "our flags are too aggressive".
Size gaps are the most misleading signal in a decomp.

**How to work it.** Check the flag hypothesis before rewriting code: read the target's disassembly for the fused
or pooled shapes (idea 3), then sweep one flag at a time (idea 5) and watch the size gap close. If a flag set
closes the whole gap and the sizes match function by function, the source was never the problem. The per-function
size deltas summed to the total gap is the arithmetic check (`CLAUDE.md`, compiler-flag drift).

**When NOT to apply.** If no flag closes the gap and the extra target instructions are genuinely absent from
every variant, the source may differ after all (idea 13); a residual there is recorded, not chased with flags.

**Result.** In the worked example the entire 2820-byte gap was flag-caused (peephole fusion, pooled table bases,
`stmw`), and with the final flags every function size matched exactly. A source rewrite at that point would have
been wasted work on correct code.

**Example**

```
baseline  -O4,p -inline auto -use_lmw_stmw on    -> .text 21600 B, 6/10 functions at 0%
final     -O3 -inline noauto -opt nopeephole ... -> .text 24420 B, 9/10 byte-identical
```

**Evidence.** Camellia unit, measured early in the campaign (flag list abbreviated as recorded then).
