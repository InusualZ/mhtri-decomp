---
id: 125
title: A pointer test as li rX,0 + cmplw is a void-pointer NULL
status: works
problem: retail tests a pointer parameter with `li r31,0` + `cmplw r3,r31` (the 0 then reused as a local's initial value) where ours emits `cmpwi r3,0`
tags: [source-shape, allocator]
applies: []
demo: 
---

# 125. A pointer test as li rX,0 + cmplw is a void-pointer NULL

**Problem.** Retail opens a function with `li r31,0; cmplw r3,r31` (an unsigned register compare against a
register that also becomes a local's `= 0` initial value); ours compares the same pointer with `cmpwi r3,0`
and materialises the local's 0 later, so the first few instructions differ and the allocator shifts.

**Why it happens.** The SDK's C headers define `NULL` as `((void*)0)`. A pointer compared against a
void-pointer constant is a pointer-typed value, not the integer immediate a literal `0` gives, so MWCC
can share it with a 0 already live in a register (the `s32 result = 0;` the function starts with) and
compares the two registers unsigned. With `#define NULL 0` (the project's `types.h`) the compare folds to
`cmpwi rX,0`. It only shows where a 0 is already in a register: a pointer tested against `(void*)0` with no
such 0 around still compiles to `cmpwi` (measured in `NWC24/nwc24_msg`, where retail's `li r0,0; cmplw`
stays unexplained).

**How to work it.** In a C unit of an SDK library whose diff shows this pair, redefine `NULL` after the
includes (`#undef NULL` / `#define NULL ((void*)0)`) and spell the pointer tests `p == NULL`. Keep integer
compares of call results (`OSGetCurrentThread() == 0`) as integer 0 where retail has `cmpwi r3,0`.

**Result.** `NCD/ncdsystem` (GC/3.0a5.2, `cflags_nwc24`): `NCDGetCurrentIfConfig` 95.16 -> 100,
`NCDGetCurrentIpConfig` 96.51 -> 100; the `OSGetCurrentThread() == NULL` tests had to go back to `== 0`
(`NCDGetLinkStatus` 98.26 -> 95.80 -> 98.26).

**Example.**

```c
#undef NULL
#define NULL ((void*)0)

s32 GetConfig(u8* out) {
    s32 result = 0;          /* li r31,0 */

    if (out == NULL) {       /* cmplw r3,r31 - with `0` this is cmpwi r3,0 */
        return -3;
    }
    ...
}
```
