---
id: 160
title: A void function's two-condition early exit is one || return, not two returns
status: works
problem: A void function's entry checks branch beq / bne / b in retail and bne / beq (4 bytes shorter) in ours, whatever order or nesting the two checks are written in
tags: [source-shape]
applies: [DWCi]
demo: 160-void-functions-two-condition.cpp
---

# 160. A void function's two-condition early exit is one || return, not two returns

**Problem.** A void function that leaves early on either of two conditions has, in retail, a `beq` to the exit, a
`bne` into the body and a `b` to the exit; ours has two conditional branches straight to the exit and is 4 bytes
shorter, so everything after the entry shifts. Two `if (...) return;`, a nested `if (a == 0) { ... }`, an
`if (a && b) { body }` and an empty-statement return all give our shape.

**Why it happens.** `if (a == 0 || b == 0) return;` is one condition with one exit target; MWCC lowers the `||` as
"first test true -> exit, second test false -> body, else exit", and the jump over the body survives. Two separate
returns, or the inverted `&&` around the body, are two independent tests that each fold into a conditional branch.

**How to work it.** Spell the two exit tests as one `||` early return. Idea 71 is the value-returning sibling
(`if (ok != 0) return ok;` vs one trailing return).

**Result.** `DWCi/gt2Socket` `DWCi_removeRequest` 97.84 -> 100 (`if (req->freeAtAcceptReject != 0 ||
req->callbackLevel != 0) return;`), `DWCi/gt2Buffer` `DWCi_bufferFrame` 96.86 -> 100 (`if (data == NULL || len ==
0) return;`); both units then read every row 100 and flipcheck READY.

**Example.**

```
void DWCi_bufferFrame(GTI2Buffer* buffer, const u8* data, s32 len) {
    if (data == NULL || len == 0) {   /* beq exit; bne body; b exit */
        return;
    }
    ...
}
```

**Demonstration.** `160-void-functions-two-condition.cpp`: the `||` return compiles to `beq`, `bne`, `b` and is
0x64 bytes; the two-return spelling has no unconditional branch and is 0x60 bytes.
