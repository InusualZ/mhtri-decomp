---
id: 95
title: Under peephole off, a narrow store wants a compound assignment and a byte is a mask, not a cast
status: works
problem: Retail stores a narrow value with no `clrlwi` (or fuses a shift and a mask into one `rlwinm`) while ours adds a `clrlwi` - and turning the peephole on to lose it breaks the unfused compares the same function needs.
tags: [source-shape, pragma]
applies: [Wii/1.3]
demo: 095-narrow-store-spelling.cpp
reviewed: 2026-09-30
related: [32, 39]
---

# 95. Under peephole off, a narrow store wants a compound assignment and a byte is a mask, not a cast

**Problem.** A band whose target keeps the *unfused* compare forms (`clrlwi r0,r3,24 ; cmpwi r0,0`, never
`clrlwi.`) is compiled `#pragma peephole off`. In that mode `callbackStep = (u8)(callbackStep + 1)` gains a
`clrlwi r0,r0,24` before the `stb` that retail does not have, and `idByte = (u8)(value >> 16)` is `srwi` + `clrlwi`
where retail has a single `rlwinm r0,r30,16,24,31`. Switching the peephole back on removes those instructions but
fuses every compare the band needs unfused (`updateCallbackStep`: 96.22 % on, 95.68 % off, with either spelling of
the stores the score is capped by the other half).

**Why it happens.** With the peephole off MWCC keeps the explicit and the implicit narrowing conversion as an
instruction. Two spellings never create one: a **compound assignment** to a narrow lvalue (`x += 1`, the `addi`
result is stored by `stb` directly) and an **explicit mask** (`(v >> 16) & 0xFF`, which the instruction selector
folds into one `rlwinm`; a `(u8)` cast of the same shift is two instructions).

**How to work it.** In a function that must stay `peephole off`, rewrite `field = (u8)(field + 1)` and
`field = field + 1` as `field += 1`, and each `(u8)(v >> n)` as `(v >> n) & 0xFF` (`v >> 24` needs no mask,
`(u8)v` is `v & 0xFF`). Keep the field's real type: a `u8` field also drops the `extsb` that an `s8` spelling adds
(`idByte_11C`). Measure the row before and after; the gain is the removed instruction, nothing else moves.

**Result.** `Network/fn_8041A87C` (2026-09-30), whole file `peephole off`: `updateCallbackStep` 96.22 -> 100 with
six `x += 1` spellings (the last, `callbackStep = step + 1` from a `u8` local, only matched as `+= 1`), and
`startMatch` 93.63 -> 100 with the four id bytes as masks. The same two functions had been bracketed
`#pragma peephole on` before; the brackets are gone.

**Example.**

```
r->step = (unsigned char)(r->step + 1);   // addi ; clrlwi ; stb
r->step += 1;                             // addi ; stb
r->b0 = (unsigned char)(v >> 16);         // srwi ; clrlwi ; stb
r->b0 = (v >> 16) & 0xFF;                 // rlwinm ; stb
```
