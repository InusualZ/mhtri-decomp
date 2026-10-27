---
id: 197
title: Inline calls in an or-chain expand last-term first; a conditional macro keeps the source order
status: works
problem: an or-chain of small inline predicates evaluates its terms in reverse (the last field is loaded first) while the target loads them in order
tags: [source-shape, allocator]
applies: [WPAD/wpad]
demo: 
---

# 197. Inline calls in an or-chain expand last-term first; a conditional macro keeps the source order

**Problem.** `f(a0) | f(a1) | ... | f(a7)` with `f` a `static inline` predicate (`|x - y| > limit`) loads the fields of
`a7`, `a6`, ... first, while the target loads `a1`, `a0`, `a2`, ..., `a7` and keeps every result live until one balanced
`or` tree at the end.

**Why it happens.** MWCC expands inline calls inside an expression before it evaluates the tree, walking the operands
last-term first. A macro has no call to expand; its `?:` conditionals are evaluated in the tree's own order (the
heavier operand first, the right one on a tie), which is the target's order.

**How to work it.** Spell the predicate as a macro whose result is itself a conditional, so the comparison is made
right after each absolute value instead of all at the end:
`#define MOVED_BY(a, b, limit) (((((a) - (b)) < 0 ? ((b) - (a)) : ((a) - (b))) > (limit)) ? 1 : 0)`.
A bare `(... > limit)` without the outer `? 1 : 0` keeps the order but moves every comparison to the end. Fields the
target reads with `lhz` need an unsigned field (a `(u16)` cast in a macro argument stays `lha` + `clrlwi`).

**Result.** `WPAD/wpad` `wpadSampleChanged` 89.75 -> 93.57 (the remaining gap is register colouring of the
accumulator); the inline, the statement-per-term and the bare-macro spellings measured 89.75, 85.81 and 67.95.

**Example.**

```
changed |= debounce(cb, MOVED_BY(s->accX, l->accX, 12) | MOVED_BY(s->accY, l->accY, 12)
                            | MOVED_BY(s->accZ, l->accZ, 12), 0);   /* loads accY, accX, accZ - as retail */
```
