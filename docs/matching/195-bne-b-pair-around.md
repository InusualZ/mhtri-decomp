---
id: 195
title: A bne + b pair around a call that may replace a value is a ternary that keeps it: x = c ? x : f(x)
status: works
problem: Retail has `cmplwi r3,1 ; bne L ; b skip ; L: ... bl f ; mr rX,r3 ; skip:` where ours has one `beq skip`
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [119]
---

# 195. A bne + b pair around a call that may replace a value is a ternary that keeps it: x = c ? x : f(x)

**Problem.** After a test, retail branches *into* the replacing call with `bne` and jumps over it with an
unconditional `b`; ours tests the opposite condition and skips with a single `beq`. The function is 4 bytes
short and every later branch offset moves.

**Why it happens.** `if (c != 1) x = f(x);` and `if (c == 1) {} else { x = f(x); }` both give the single inverted
branch. A conditional *expression* whose true arm is the variable itself keeps both arms as blocks: the
`x` arm becomes the `b skip` (with peephole off nothing removes it).

**How to work it.** Rewrite the conditional replacement as `x = cond ? x : f(x);`, keeping the condition in the
same sense retail tests it (`== 1` when retail branches with `bne` into the call).

**Result.** `sound/snd_bank_loader`: `snd_stage_bank_load` 98.69 -> 100, `quest_bgm_load` 98.78 -> 100,
`snd_area_env_bank_load` 97.78 -> 99.64 (with the same shape). The empty-`if` and `!=` spellings both measured
98.69 on the first.

**Example.**

```
map = move_work_state_ck() == 1 ? map : quest_pair_table_get(map, rank);
```
