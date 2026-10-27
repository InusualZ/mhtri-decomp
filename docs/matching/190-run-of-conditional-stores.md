---
id: 190
title: A run of conditional stores from a stepping address is a short loop: written out, the constants fold
status: works
problem: retail keeps an address in a saved register and steps it with addis/subi between stores; ours folds every store to its own lis/addi constant and saves one more register
tags: [source-shape]
applies: []
demo: 
---

# 190. A run of conditional stores from a stepping address is a short loop: written out, the constants fold

**Problem.** Retail materialises a base address once (`lis r3,0x90cd; subi r30,r3,0x7000`) and steps it between
`if (p[k] == NULL) p[k] = addr;` stores with `addis r30,r30,1; subi r30,r30,0x7f40`; the written-out source compiles to
per-store `lis`/`addi` constants, an extra saved register and a function 20 bytes longer.

**Why it happens.** MWCC propagates the constant through straight-line code, so every `addr += N` folds. A short
`for` loop is unrolled only after propagation has run, so the unrolled copies keep the running register.

**How to work it.** Write the run as a 2- or 3-trip loop over the slot index (`for (i = 0; i < 2; i++) { if
(w.tex[i] == NULL) w.tex[i] = addr; addr += 0x80C0; }`), leaving the stores that differ (an unconditional one) outside.

**Result.** `fn_80047398` / `face_work_init` (0x170): 90.92 % (388 B) -> 100 % (368 B).

**Example.**

```
u8* addr = (u8*)0x90CC9000;
memset(&face_work, 0, sizeof(FaceWork));
for (i = 0; i < 2; i++) {
    if (face_work.tex[i] == NULL) {
        face_work.tex[i] = (TPLPalette*)addr;
    }
    addr += 0x80C0;
}
face_work.tex[2] = (TPLPalette*)addr;
```
