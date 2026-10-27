---
id: 248
title: Two early-exit checks that share one tail: a comma-sequenced || condition
status: works
problem: retail emits one copy of an exit tail for two checks separated by statements; goto-free source duplicates the tail
tags: [source-shape]
applies: [quest/quest_entry]
demo: 
---

# 248. Two early-exit checks that share one tail: a comma-sequenced || condition

**Problem.** Two `if (...) { tail; break; }` checks with statements between them (`if (A) { x = 0; tail; } calls();
if (B) { tail; }`) appear in retail with one copy of `tail`: the second check branches back into the first's tail. A
goto-free source writes the tail twice (an inline helper), which adds its size and shifts everything after it.

**Why it happens.** Retail's source jumped to the tail (a goto). Without one, the only way to name the tail once is a
single condition that covers both checks.

**How to work it.** Fold the in-between statements into the condition with the comma operator:
`if ((A && (x = 0, 1)) || (calls(), B)) { tail; break; }`. MWCC lays the second operand out before the tail (a forward
branch instead of retail's backward one), so the order of the blocks still differs, but the tail is emitted once.

**Result.** quest/quest_entry: quest_main_step 83.72 % -> 84.98 % (3104 -> 3068 B against retail's 2832). The same
merge for the clear tail (whose second site needs a re-test of a value) scored lower (84.98 -> 82.30) and was dropped.

**Example.**

```
if ((sys_a == 0 && rec != NULL && w->count <= 0 && (w->count = 0, 1)) ||
    (step_a(item), step_b(item), failed_ck(item) == 1)) {
    fail_tail(item);
    break;
}
```
