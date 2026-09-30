---
id: 19
title: Loop shape decides the loop idiom
status: works
problem: A loop can compile to a `mtctr`/`bdnz` countdown, to a compare-and-branch, or to a bottom-tested loop; the wrong idiom adds or removes instructions and moves every later register.
tags: [source-shape]
applies: []
demo:
---

# 19. Loop shape decides the loop idiom

**Problem.** A loop can compile to a `mtctr`/`bdnz` countdown, to a compare-and-branch, or to a bottom-tested
loop; the wrong idiom adds or removes instructions and moves every later register.

**Why try it.** MWCC picks the idiom from the source's shape, and it is visible in the first three
instructions after the loop's bound is computed, so it is a cheap thing to check before touching anything
else: `while (count--)` and `for (i = count; i > 0; i--)` become `mtctr` + `ble` + `bdnz` (with a
record-form shift as the entry test), `for (i = 0; i < n; i++)` becomes `cmpw` + `blt`, and an explicit
second induction variable (`for (i = 1, off = sizeof(RSOSection); ...)`) is what produces retail's separate
index and offset registers.

**Result.** `fn_804DA7E4`: the `mtctr`/`bdnz` idiom and the `srwi.` entry test only appeared with
`while (count--)` (a `for (i = 0; i < count; i++)` gave `cmplwi` + `ble` and one extra instruction). The
percentage did not move until idea 18 fixed the colours - the idiom and the colouring are two separate
residuals, so fix the shape first and only then chase registers. `RSOStaticLocateObject` reproduced
retail's `r15..r31` colouring (and `_savegpr_15` instead of `_savegpr_14`) only with the two-variable
`for`.
