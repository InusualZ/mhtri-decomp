---
id: 147
title: Pool and string offsets shifted by a missing function: write the functions the linker strips
status: works
problem: Every string or literal row of a unit relocates against the right section but the wrong offset, or the literal pool is in another order, while the functions that use them are correct.
tags: [data, sections]
applies: [Wii/1.3, GC/3.0a5.2]
demo: 
related: [36, 94, 113, 116]
---

# 147. Pool and string offsets shifted by a missing function: write the functions the linker strips

**Problem.** A unit's string rows (`.data` string pool) or literal pool (`.sdata2`) start at the right place but
the offsets drift, or two literals swap, and the rows that read them sit at 9x % with only relocation offsets
wrong. The section is shorter than the target's claim.

**Why it happens.** MWCC pools a TU's strings and literals as it generates each function, in definition order,
*before* the linker removes unreferenced functions. Retail's TU contained functions nobody in the game calls; the
linker stripped their code, but their strings and literals stay in the pool between the live ones. Without those
functions ours has a shorter pool with the live entries at other offsets.

**How to work it.** Find the gaps in the target's pool (strings no live function references, a literal pooled
before the first used one), identify the functions that used them (the SDK's public API of that module is the
usual source), and write them in their definition-order place. They cost nothing in `.text` (stripped again) and
need no map rows. An unused `static inline` does not help: only an emitted function pools anything.

**Result.** `WUD/wud` (Wii/1.3): writing the four stripped functions restored the `.data` string-pool offsets
(four rows moved, two to 100). `AX/AXFXReverbHiExp` (Wii/1.3): the SDK's `GetMemSize` ahead of the init puts
`32000.0f` first in `.sdata2`. `NCD/ncdsystem` (GC/3.0a5.2): a stripped static ahead of the API also fixes `.bss`
order (idea 113). Open: `NHTTP/d_nhttp` still lacks four stripped public calls' messages (905 of 1120 B).

**Example.**

```c
/* retail strips it, but its literals sit first in the pool */
u32 AXFXReverbHiExpGetMemSize(AXFX_REVERBHI_EXP* reverb) {
    return ... * 32000.0f ...;
}
```
