---
id: 73
title: Never append `, ...` to a definition to dodge an argument-count mismatch
status: works
problem: A caller passes more arguments than the source signature has and the compiler refuses; making the definition variadic (`void fn(int a, ...)`) silences it but emits a full varargs prologue.
tags: [source-shape, process]
applies: []
demo: 073-no-variadic-dodge.cpp
reviewed: 2026-09-29
related: [57, 72]
---

# 73. Never append `, ...` to a definition to dodge an argument-count mismatch

**Problem.** A retired object calls a function through a declaration with more arguments than the source
signature carries, so the compiler refuses it. The tempting fix is to make the definition variadic
(`void fn(int a, ...)`), which makes the complaint go away.

**How it looks.** A function that should be a 12-byte thunk is ~100 bytes and full of `stw rN,disp(r1)` /
`stfd fN` register saves (a varargs prologue), and its unit collapses in score.

**Why it happens.** It compiles, it links, and the call sites are unchanged - the mismatch looks cosmetic.
But MWCC emits a **full varargs prologue for every function declared that way**: the whole integer and FP
argument-register save area.

**How to work it.** Use a **fixed unused parameter** with the width the caller passes
(`void fn(int a, void* unused)`), which changes nothing in the prologue - or, better, find the real
signature (idea 72's arity, idea 57's widths).

**When NOT to apply.** If the original really is variadic (a `printf`-shaped function), the prologue *is* the
target; check the target's prologue before "fixing" it.

**Result.** Measured at the time (2026-09-25, `g3d/fn_80075DCC.cpp`): `void fn_8007B48C(void* self, u32 a, ...)`
was 108 bytes and the unit scored 27 %; with a fixed unused `void*` it is the 12-byte thunk retail has and
the unit reached 139/216 symbols at or above the bar.

**Example.**

```c
void fn(void* self, u32 a, ...)          /* 108 B: register save area, FP spills */
void fn(void* self, u32 a, void* unused) /* 12 B: the retail thunk               */
```

**Demonstration.** `073-no-variadic-dodge.cpp` (`ideas.py demo-check 73`): `fixed(s32, void*)` is 4 bytes
(`blr`), `variadic(s32, ...)` is 0x50 bytes with the `stfd` FP spills.
