---
id: 326
title: A count-after-append equality kept as a materialised 0/1 (cntlzw) is a local assigned the comparison and then tested
status: works
problem: Retail computes `a == b` with `subf; cntlzw; srwi.` and branches on the result; `if (a != b)` compiles to a bare `cmpw`/`beq`.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [148]
---

# 326. A count-after-append equality kept as a materialised 0/1 (cntlzw) is a local assigned the comparison and then tested

**Problem.** The diff shows `subf r0,r0,r3; cntlzw r0,r0; srwi. r0,r0,5; bne` in retail where ours has
`cmpw r0,r3; beq`: the comparison is evaluated as a value first and then tested.

**Why it happens.** The source stored the equality in a variable (`ok = (x == y); if (ok == 0)`), which makes MWCC
build the 0/1 value with `cntlzw`/`srwi.` and branch on its record form. `if (x != y)` never builds the value.

**How to work it.** Assign the comparison to a local, then test the local against 0 (`appended = (count + 1 ==
ArrayLength(list)); if (appended == 0) { ... }`). Reusing an existing `s32` local or a new one measures the same.

**Result.** `DWCi/gt2Message` `DWCi_requestBlockState` (Wii/1.3): 98.84 % -> 100.00 % (916 B, same size).

**Example.**

```c
count = ArrayLength(list);
ArrayAppend(list, &message);
appended = (count + 1 == ArrayLength(list));
if (appended == 0) {
    /* overflow path */
}
```
