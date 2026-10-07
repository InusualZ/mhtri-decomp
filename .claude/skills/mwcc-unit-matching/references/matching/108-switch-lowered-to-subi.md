---
id: 108
title: A switch lowered to subi + cmplwi range tests in a -O3 unit is #pragma optimization_level 4
status: works
problem: Retail's multi-case switch tests ranges with `subi r0,rN,K ; cmplwi r0,1 ; ble` (ranges first, then the single values) where ours emits a `cmpwi rN,K ; blt ; cmpwi rN,K+1 ; ble` pair per range, so the chain is longer and every branch after it shifts.
tags: [pragma, source-shape]
applies: [Wii/1.3]
demo: 
reviewed: 2026-10-07
related: [39, 100]
---

# 108. A switch lowered to subi + cmplwi range tests in a -O3 unit is #pragma optimization_level 4

**Problem.** A `switch` with adjacent same-target cases (`case 28: case 29:`, `case 26: case 27:`) compiles under the
unit's `-O3` to one signed range test per group (`cmpwi x,28 ; blt ; cmpwi x,29 ; ble`), in source order. Retail's
chain tests each group with a single unsigned compare (`subi r0,x,28 ; cmplwi r0,1 ; ble`), puts the groups first and
the single values after them. Source spellings do not reach it: an explicit `(u32)(t - 28) <= 1` gives the retail
compare but an `if` chain cannot share a block between two ranges without a `goto`, and the order differs.

**Why it happens.** The range-first, unsigned-compare lowering is the switch lowering of optimisation level 4. A
scoped `#pragma optimization_level 4` turns it on for one function while the rest of the unit stays at the library's
`-O3` (`-O4,p` as a command-line flag gives the same chain).

**How to work it.** When the first divergence of a switch dispatch is `cmpwi/blt/cmpwi/ble` against `subi/cmplwi/ble`,
bracket the function with `#pragma optimization_level 4` ... `#pragma optimization_level reset` (a codegen pragma
lives in the TU that needs it, rule 14) and re-measure; keep the switch in retail's block order. Probe first on a
scratch file with the real cflags (`-O3` versus `#pragma optimization_level 4` versus `-O4,p`).

**Result.** `ef/eft013_fx` (2026-10-07): `fn_80107640` 87.65 -> 99.01 %, `fn_801093F4` (written) 94.3 -> 97.7 % with
the same switch source before and after the pragma (99.4 % with the later tweaks), and the written `fn_80108A74`, `eft013_setup_effect` and
`eft013_setup_uv_model` all carry the same range chain.

**Example.**

```
switch (type) { case 9: case 28: case 29: A(); break; case 13: case 26: case 27: B(); break; default: D(); }
/* -O3:  cmpwi r0,28 ; blt ; cmpwi r0,29 ; ble ; cmpwi r0,26 ; blt ; cmpwi r0,27 ; ble ; cmpwi r0,9 ; ...
 * lvl4: subi r0,r3,28 ; cmplwi r0,1 ; ble ; subi r0,r3,26 ; cmplwi r0,1 ; ble ; cmpwi r3,9 ; beq ; cmpwi r3,13 ; beq */
```
