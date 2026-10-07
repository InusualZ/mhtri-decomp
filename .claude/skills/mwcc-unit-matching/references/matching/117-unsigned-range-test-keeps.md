---
id: 117
title: An unsigned range test keeps its two compares only with the upper bound written constant-first
status: works
problem: Retail tests a u32 against a range with two `cmplwi` + branches and our build emits one `addi`/`subis` + `cmplwi`, shortening the function and shifting every branch after it.
tags: [source-shape]
applies: [OS]
demo:
---

# 117. An unsigned range test keeps its two compares only with the upper bound written constant-first

**Problem.** `if (ch >= 0xD800 && ch <= 0xDFFF)` (or the `||` form) compiles to `addis`/`addi` plus one `cmplwi ch-lo, hi-lo`
and a single branch; retail has `cmplwi r,0xd800; blt` and `cmplwi r,0xdfff; ble`. The function is shorter and every branch
displacement after the test differs.

**Why it happens.** MWCC folds `x >= lo && x <= hi` into one unsigned compare when both sides read `variable OP constant`.
Writing the upper bound constant-first (`0xDFFF >= x`) is the same condition but not the shape the fold matches. The signed twin
is idea 100; this is the unsigned case, and for the `||` form the constant-first upper bound alone did not stop the fold in a
return-heavy function (see the example), so measure each test.

**How to work it.** When the target shows two `cmplwi` for one range test, write the upper bound as `HI >= x`.

**Result.** `OS/OSUtf`: `OSUTF32toANSI` 91.4 -> 100 % (`code < 0x80 || 0x9F < code`, `code >= 0x152 && 0x2122 >= code`);
`OSUTF8to32` kept its two compares and went to 100 % with `ch >= 0xD800 && 0xDFFF >= ch`; `OSUTF16to32` 61.6 -> 76.1 %.

**Example.**

```c
if (ch >= 0xD800 && ch <= 0xDFFF) { ... }   /* addis/addi + cmplwi 0x7ff */
if (ch >= 0xD800 && 0xDFFF >= ch) { ... }   /* cmplwi 0xd800; blt; cmplwi 0xdfff; ble - retail's shape */
```
