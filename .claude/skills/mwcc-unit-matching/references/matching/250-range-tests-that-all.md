---
id: 250
title: Range tests that all branch forward to their bodies and end in b skip are a switch of case runs
status: works
problem: retail tests `cmplwi x,3; ble A; cmpwi x,4; blt skip; cmpwi x,7; ble B; b skip` and lays A then B after the tests; our `if (x <= 3) A else if (x >= 4 && x <= 7) B` branches past A with `bgt` and folds the second test into `subi`+`cmplwi`
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [108, 117]
---

# 250. Range tests that all branch forward to their bodies and end in b skip are a switch of case runs

**Problem.** A chain of range tests in retail branches *forward to every body* (`ble A`, `ble B`) and ends with an
unconditional `b skip`, the bodies laid out after it in source order. The same tests written as `if`/`else if` lay
the first body right after its test (`bgt` past it) and lower `x >= 4 && x <= 7` into a `subi`+`cmplwi` range test.

**Why it happens.** That layout is MWCC's switch lowering: every case run becomes a compare that branches to its
case label, the default is the trailing `b`, and the case bodies follow in source order. A run of contiguous cases
is tested as a binary-search range (`cmpwi lo; blt default; cmpwi hi; ble case`), not as `subi`+`cmplwi`.

**How to work it.** When every test of a chain jumps forward and the chain ends in `b skip`, write it as a `switch`
with one case run per body (`case 0: case 1: case 2: case 3:`). The compare signedness follows the switched
variable's type (`cmpwi` for `s32`). Measure: a single run of all cases (`case 0 ... 7` one body) is merged into one
range test, so this only applies where retail has two or more bodies.

**Result.** `enemy/fn_8011D448` `em_seq_exec` (the move-jitter mode, `s32 mode`): `if (mode >= 0 && mode <= 3) ...
else if (mode >= 4 && mode <= 7) ...` -> `switch (mode) { case 0..3: ...; break; case 4..7: ...; break; }`: +17.6
matched bytes, 98.47 -> 99.12. The same function's first test (0-2, 4-6, ==3, ==7 to one body) is *not* a switch
(the switch merges 0-7, -28 B); there `(mode >= 0 && mode <= 2) || (mode >= 4 && mode <= 6) || mode == 3 ||
mode == 7` on an `s32` gives retail's `cmplwi`/`subi`+`cmplwi`/`cmpwi`/`cmpwi` (+8.8 B over a `u32` spelling).

**Example.**

```
switch (mode) {                      /* cmplwi 3; ble A; cmpwi 4; blt skip; cmpwi 7; ble B; b skip; A: ...; B: ... */
case 0: case 1: case 2: case 3:
    a();
    break;
case 4: case 5: case 6: case 7:
    b();
    break;
}
```
