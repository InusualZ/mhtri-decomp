---
id: 239
title: An addi+cmplwi test of -1 and 0 is written as one unsigned compare of c + 1
status: works
problem: Retail tests a value against -1 and 0 with `addi r0,c,1; cmplwi r0,1; ble skip`; ours with `c != -1 && c != 0` emits two compares and branches.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
---

# 239. An addi+cmplwi test of -1 and 0 is written as one unsigned compare of c + 1

**Problem.** A guard that skips the two "no value" encodings -1 and 0 reads `addi r0,r11,0x1; cmplwi r0,0x1; ble`
in retail. `if (c != -1 && c != 0)` compiles to `cmpwi c,-1; beq; cmpwi c,0; beq` - one instruction longer, and
every later instruction is shifted.

**Why it happens.** MWCC does not merge two equality tests into a range test; the unsigned range form only comes
from source that is already one compare.

**How to work it.** Write the test as `(u32)(c + 1) > 1` (true for every value except -1 and 0). Try it before any
switch rewrite: a `switch` with `case -1: case 0:` measured no better here.

**Result.** `enemy/enemy_control` (Wii/1.3, -O4 for the function): `em_level_set` 95.73 -> 97.53 % (the guard of
its sub-buffer loop, unrolled three times).

**Example.**

```
s32 c = main->size_class;
if ((u32)(c + 1) > 1) {            /* c != -1 && c != 0 */
    switch (c) { ... }
}
```
