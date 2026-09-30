---
id: 73
title: Never append `, ...` to a definition to dodge an argument-count mismatch
status: works
problem: A retired object calls a function through a declaration with more arguments than the source signature carries, so the compiler refuses it. The tempting fix is to make the definition variadic (`void fn(int a, ...)`), which makes the complaint go away.
tags: [source-shape, process]
applies: []
demo:
---

# 73. Never append `, ...` to a definition to dodge an argument-count mismatch

**Problem.** A retired object calls a function through a declaration with more arguments than the source
signature carries, so the compiler refuses it. The tempting fix is to make the definition variadic
(`void fn(int a, ...)`), which makes the complaint go away.

**Why try it.** It compiles, it links, and the call sites are unchanged - the mismatch looks cosmetic.

**Result.** MWCC emits a **full varargs prologue for every function declared that way**. A 12-byte thunk
became 108 bytes and its unit scored 27 %. The correct workaround is a **fixed unused parameter** with the
width the caller passes (`void fn(int a, void* unused)`), which changes nothing in the prologue. Measured in
`g3d/fn_80075DCC.cpp` (2026-09-25): with the varargs spelling the unit collapsed; with a fixed unused `void*`
it reached 139/216 symbols at or above the bar.

**Example.** `void fn_8007B48C(void* self, u32 a, ...)` -> 108 bytes and 27 % unit;
`void fn_8007B48C(void* self, u32 a, void* unused)` -> the 12-byte thunk retail has.
