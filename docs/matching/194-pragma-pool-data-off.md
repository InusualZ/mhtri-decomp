---
id: 194
title: #pragma pool_data off is the source-level -pool off: each string gets its own lis/addi
status: works
problem: Retail addresses each string literal with its own lis/addi pair, ours shares one `...data.0` base register - and configure.py is not this lane's to change
tags: [pragma, data]
applies: [Wii/1.3]
demo: 
related: [43]
---

# 194. #pragma pool_data off is the source-level -pool off: each string gets its own lis/addi

**Problem.** A function that uses three or more string literals of one section compiles to one
`lis rN,...data.0@ha` base plus `addi rX,rN,delta` per string, while retail has `lis`/`addi` against each
string's own symbol (idea 43). Idea 43 fixes it with the per-library `-pool off` flag, which is a
`configure.py` change.

**Why it happens.** `-pool` (on by default) pools the TU's data objects of one section behind one base register.
MWCC also accepts the setting as a pragma, `#pragma pool_data on|off`, which applies to the rest of the TU like
any codegen pragma (idea 32).

**How to work it.** Put `#pragma pool_data off` in the unit's `.cpp` (next to its other codegen pragmas, rule 14),
before the first function that needs it, and re-measure every row of the unit. It changes nothing for `extern`
data and for SDA (`.sdata`) objects, exactly like the flag.

**Result.** `sound/snd_bank_loader` (Wii/1.3, `cflags_main`): `snd_quest_bgm_load` 89.40 -> 100 once its four
string externs became literals (pooled: `lis r30,...data.0@ha` + `addi r4,r30,0x1c`; with the pragma: one
`lis`/`addi` per string); no other row of the unit moved.

**Example.**

```
#pragma pool_data off
void f(char* b) { sprintf(b, "16/whd/%s.whd", "tutorial"); sprintf(b, "16/tsb/%s.tsb", "tutorial"); }
```
